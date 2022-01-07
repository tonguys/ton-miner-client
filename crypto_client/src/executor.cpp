#include "executor.hpp"

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
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/exception/exception.hpp"
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

std::string Executor::taskToArgs(const model::MinerTask &t) {
  namespace bm = boost::multiprecision;

  bm::cpp_int complexityBigInt;
  bm::cpp_int seedBigInt;

  std::stringstream hexFormatSS;
  hexFormatSS << std::hex << t.complexity;
  hexFormatSS >> complexityBigInt;
  hexFormatSS.clear();

  hexFormatSS << std::hex << t.seed;
  hexFormatSS >> seedBigInt;

  return fmt::format("-vv -g {} -F {} -e {} {} {} {} {} {} {}", t.gpu, factor,
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

exec_res::ExecRes Executor::ExecImpl(const model::MinerTask &task) {
  boost::asio::io_service ios;
  std::future<std::string> outData;
  boost::asio::streambuf errData;

  auto args = taskToArgs(task);
  spdlog::info("Miner args: {}", args);
  bp::child ch(path.string(), bp::args(parsed(args)), bp::std_in.close(),
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

  if (!AnswerExists()) {
    return exec_res::Crash{"can`t locate boc file", -1};
  }

  model::Answer answer;
  answer.giver_address = task.giver_address;
  answer.boc = GetAnswer();
  spdlog::debug(answer);
  return exec_res::Ok{answer};
}

exec_res::ExecRes Executor::Exec(const model::MinerTask &task) {
  try {
    spdlog::info("Exec miner");
    return ExecImpl(task);
  } catch (boost::process::process_error &e) {
    spdlog::error("Exec got boost exception: {}; code: {}", e.what(),
                  e.code().message());
  } catch (std::exception &e) {
    spdlog::error("Exec got exception: {}", e.what());
  } catch (...) {
    spdlog::error("Exec got unknown exception");
  }
  return exec_res::Crash{};
}

bool Executor::AnswerExists() {
  spdlog::trace("Checking {} path answer", result_path.string());
  return std::filesystem::exists(result_path);
}

std::vector<model::Answer::Byte> Executor::GetAnswer() {
  std::ifstream file(result_path, std::ios::binary | std::ios::in);
  return std::vector<model::Answer::Byte>(std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>());
}

} // namespace crypto