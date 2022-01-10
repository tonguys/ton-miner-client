#include "executor.hpp"

#include "boost/process/group.hpp"
#include "boost/range/adaptor/tokenized.hpp"
#include "boost/regex/v5/match_flags.hpp"
#include "models.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <istream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/exception/exception.hpp"
#include "boost/fiber/buffered_channel.hpp"
#include "boost/fiber/condition_variable.hpp"
#include "boost/fiber/fiber.hpp"
#include "boost/fiber/mutex.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include "boost/process.hpp"
#include "boost/process/args.hpp"
#include "boost/process/detail/child_decl.hpp"
#include "boost/process/exception.hpp"
#include "boost/process/io.hpp"
#include "boost/process/pipe.hpp"
#include "boost/range/adaptors.hpp"
#include "boost/range/algorithm_ext/push_back.hpp"
#include "boost/regex.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace crypto {

namespace bp = boost::process;

class Waiter {
private:
  boost::fibers::condition_variable cond;
  boost::fibers::mutex mutex;
  bool ready = false;
  long n = 0;

public:
  typedef std::shared_ptr<Waiter> Ptr;

  void Add() {
    std::unique_lock<boost::fibers::mutex> lock(mutex);
    n += 1;
  }

  void Free() {
    bool done = false;
    {
      std::unique_lock<boost::fibers::mutex> lock(mutex);
      n -= 1;
      if (n <= 0) {
        ready = true;
        done = true;
      }
    }
    if (done) {
      cond.notify_one();
    }
  }

  void Wait() {
    std::unique_lock<boost::fibers::mutex> lock(mutex);
    cond.wait(lock, [this]() { return ready; });
  }

  void Notify() {
    {
      std::unique_lock<boost::fibers::mutex> lock(mutex);
      ready = true;
    }
    cond.notify_one();
  }
};

std::string Executor::taskToArgs(const model::MinerTask &t, int gpu) {
  namespace bm = boost::multiprecision;

  bm::cpp_int complexityBigInt;
  bm::cpp_int seedBigInt;

  std::stringstream hexFormatSS;
  hexFormatSS << std::hex << t.complexity;
  hexFormatSS >> complexityBigInt;
  hexFormatSS.clear();

  hexFormatSS << std::hex << t.seed;
  hexFormatSS >> seedBigInt;

  return fmt::format("-vv -g {} -F {} -e {} {} {} {} {} {} {}", gpu, factor,
                     t.expires.GetUnix(), t.pool_address,
                     bm::to_string(seedBigInt), bm::to_string(complexityBigInt),
                     t.iterations, t.giver_address, resName);
}

std::vector<std::string> parsed(std::string args) {
  // TODO: regex is a shit, think about better solution
  std::vector<std::string> res;
  auto from = boost::adaptors::tokenize(args, boost::regex("\\s+"), -1,
                                        boost::regex_constants::match_default);
  boost::push_back(res, from);
  return res;
}

exec_res::ExecRes Executor::execSafe(const model::MinerTask &task, int gpu,
                                     boost::process::group &g) {
  using namespace exec_res;
  ExecRes res;
  try {
    res = exec(task, gpu, g);
  } catch (boost::process::process_error &e) {
    res = Crash("Exec got boost exception: {}; code: {}", e.what(),
                e.code().message());
  } catch (std::exception &e) {
    res = Crash("Exec got exception: {}", e.what());
  } catch (...) {
    res = Crash("Exec got unknown exception");
  }
  return res;
}

exec_res::ExecRes Executor::exec(const model::MinerTask &task, int gpu,
                                 boost::process::group &g) {
  boost::asio::io_service ios;
  std::future<std::string> outData;
  boost::asio::streambuf errData;

  auto args = taskToArgs(task, gpu);
  spdlog::info("Miner args: {}", args);
  bp::child ch(path.string(), g, bp::args(parsed(args)), bp::std_in.close(),
               bp::std_err > errData, bp::std_out > outData, ios);

  ios.run();
  auto status = outData.wait_until(task.expires.GetChrono());
  if (status == std::future_status::timeout) {
    ch.terminate();
    ch.wait();
    return exec_res::Timeout{};
  }

  auto out = outData.get();
  std::string err((std::istreambuf_iterator<char>(&errData)),
                  std::istreambuf_iterator<char>(nullptr));
  spdlog::info("Miner stdout:\n{}", out);
  spdlog::info("Miner stderr:\n{}", err);

  ch.wait();
  auto code = ch.exit_code();
  if (code != 0) {
    return exec_res::Crash{"non-nil exit code", code};
  }

  if (!answerExists()) {
    return exec_res::Crash{"can`t locate boc file", -1};
  }

  model::Answer answer;
  answer.giver_address = task.giver_address;
  answer.boc = getAnswer();
  spdlog::debug(answer);
  return exec_res::Ok{answer};
}

exec_res::ExecRes Executor::Run(const model::MinerTask &task) {
  if (running) {
    throw std::runtime_error("method Run called for already running Executor");
  }

  // Here we pass some data by ref. We know, that it should work, as we
  // terminate processes before living the scope. But anyway, it may be
  // dangerous, so
  //
  // TODO: use shared_poiner or some safe sollution

  auto waiter = std::make_shared<Waiter>();
  using buff_t = boost::fibers::buffered_channel<exec_res::Ok>;
  std::shared_ptr<buff_t> buff;

  boost::process::group g;
  for (auto gpu : task.gpu) {
    spdlog::info("Starting miner for GPU #{}", gpu);
    boost::fibers::fiber([buff, waiter, task, gpu, &g, this]() {
      waiter->Add();
      auto outcome = execSafe(task, gpu, g);
      spdlog::debug("Exec #{} done", gpu);

      using namespace exec_res;
      std::visit(model::util::overload{
                     [waiter, gpu](const Timeout &) {
                       spdlog::info("Exec #{} timed out", gpu);
                       waiter->Free();
                     },
                     [waiter, gpu](const Crash &c) {
                       spdlog::warn("Exec #{} crashed: {}", gpu, Dump(c));
                       waiter->Free();
                     },
                     [waiter, buff, gpu](const Ok &ok) {
                       spdlog::info("Exec #{} found an answer", gpu);
                       buff->push(ok);
                       waiter->Notify();
                     }},
                 outcome);
    }).detach();
  }
  waiter->Wait();
  auto res = buff->value_pop();
  buff->close();

  g.terminate();
  // waiting for no more then one second is not a rocket scince trick, but we
  // realy prefer low possibility of zombie processes to dead locks
  g.wait_for(std::chrono::seconds(1));
  return res;
}

void Executor::Stop() {
  if (!running) {
    return;
  }
}

bool Executor::answerExists() {
  spdlog::trace("Checking {} path answer", result_path.string());
  return std::filesystem::exists(result_path);
}

std::vector<model::Answer::Byte> Executor::getAnswer() {
  std::ifstream file(result_path, std::ios::binary | std::ios::in);
  return std::vector<model::Answer::Byte>(std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>());
}

} // namespace crypto