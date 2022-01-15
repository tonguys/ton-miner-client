#include "executor.hpp"

#include "boost/fiber/algo/algorithm.hpp"
#include "boost/fiber/channel_op_status.hpp"
#include "boost/fiber/operations.hpp"
#include "boost/process/group.hpp"
#include "boost/range/adaptor/tokenized.hpp"
#include "boost/regex/v5/match_flags.hpp"
#include "models.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include "boost/filesystem.hpp"
#include <fstream>
#include <future>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
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
#include "boost/fiber/algo/shared_work.hpp"
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

exec_res::ExecRes Executor::execSafe(const model::MinerTask &task, int gpu) {
  using namespace exec_res;
  ExecRes res;
  try {
    res = exec(task, gpu);
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

exec_res::ExecRes Executor::exec(const model::MinerTask &task, int gpu) {
  boost::asio::io_service ios;
  std::future<std::string> outData;
  boost::asio::streambuf errData;

  auto args = taskToArgs(task, gpu);
  spdlog::info("Miner args: {}", args);

  auto _pgCopy = this->pg;
  bp::child ch(path.string(), *_pgCopy, bp::args(parsed(args)),
               bp::std_in.close(), bp::std_err > errData, bp::std_out > outData,
               ios);

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

std::optional<exec_res::Ok> Executor::Run(const model::MinerTask &task) {
  if (running.load()) {
    throw std::runtime_error("method Run called for already running Executor");
  }
  running.store(true);

  // Here we pass Task by ref. We know, that it should work, as we
  // terminate processes before living the scope. But anyway, it may be
  // dangerous, so
  //
  // TODO: use shared_poiner for Task or some other safe sollution

  this->waiter = std::make_shared<Waiter>();
  this->pg = std::make_shared<boost::process::group>();

  using buff_t = boost::fibers::buffered_channel<exec_res::Ok>;
  auto buff = std::make_shared<buff_t>(2);

  std::atomic_bool found = false;
  for (auto gpu : task.gpu) {
    spdlog::info("Starting miner for GPU #{}", gpu);
    std::thread([&found, buff, task, gpu, this]() {
      spdlog::debug("Starting task for #{}", gpu);
      waiter->Add();
      auto outcome = execSafe(task, gpu);
      spdlog::debug("Exec #{} done", gpu);

      using namespace exec_res;
      std::visit(model::util::overload{
                     [this, gpu](const Timeout &) {
                       spdlog::info("Exec #{} timed out", gpu);
                       waiter->Done();
                     },
                     [this, gpu](const Crash &c) {
                       spdlog::warn("Exec #{} crashed: {}", gpu, Dump(c));
                       waiter->Done();
                     },
                     [&found, this, buff, gpu](const Ok &ok) {
                       if (found.load()) {
                         return;
                       }
                       found.store(true);
                       spdlog::info("Exec #{} found an answer", gpu);
                       buff->try_push(ok);
                       waiter->Notify();
                     }},
                 outcome);
    }).detach();
  }
  waiter->Wait();
  spdlog::debug("All miner tasks complited");

  try {
    if (pg->valid()) {
      pg->terminate();
      // waiting for no more then one second is not a rocket scince trick, but
      // we realy prefer low possibility of zombie processes to dead locks
      // if (!pg->wait_for(std::chrono::seconds(1))) {
      //   spdlog::warn("Not all miners exited");
      // }
      pg->detach();
    }
  } catch (const boost::process::process_error &e) {
    spdlog::debug("Excteption on miner termination: {}, {}", e.code(),
                  e.what());
  }

  exec_res::Ok res;
  auto status = buff->try_pop(res);
  if (status != boost::fibers::channel_op_status::success) {
    spdlog::debug("Buffer status: {}", status);
    return std::nullopt;
  }
  buff->close();

  return res;
}

void Executor::Stop() {
  if (!running.load()) {
    return;
  }
  running.store(false);

  spdlog::debug("Stopping exec");
  waiter->Notify();
  try {
    if (pg->valid()) {
      pg->terminate();
    }
  } catch (...) {
  }

  waiter.reset();
  pg.reset();
}

bool Executor::answerExists() {
  spdlog::trace("Checking {} path answer", result_path.string());
  return boost::filesystem::exists(result_path);
}

std::vector<model::Answer::Byte> Executor::getAnswer() {
  std::ifstream file(result_path, std::ios::binary | std::ios::in);
  return std::vector<model::Answer::Byte>(std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>());
}

} // namespace crypto