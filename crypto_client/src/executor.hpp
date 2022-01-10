#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "boost/process/group.hpp"
#include "fmt/core.h"
#include "models.hpp"

#include "fmt/format.h"

#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

namespace crypto {

namespace exec_res {
struct Timeout {};

struct Crash {
  Crash() = default;
  template <typename... T> explicit Crash(std::string_view msg, T &&...args) {
    msg = fmt::format(msg, std::forward(args)...);
  }
  std::string msg;
  int code = 0;
};

struct Ok {
  model::Answer answer;
};

using ExecRes = std::variant<Timeout, Crash, Ok>;

inline std::string Dump([[maybe_unused]] const Timeout &time) {
  return fmt::format("Timeout{{}}");
}

inline std::string Dump(const Crash &crash) {
  return fmt::format("Crash{{code:{}, msg:{}}}", crash.code, crash.msg);
}

inline std::string Dump(const Ok &ok) {
  return fmt::format("Ok{{answer:{}}}", ok.answer);
}

} // namespace exec_res

class Executor {
private:
  const long factor;
  const std::filesystem::path path;
  std::filesystem::path result_path;
  inline static const char *const resName = "mined.boc";

  bool running = false;

public:
  explicit Executor(const model::Config &cfg)
      : factor(cfg.boostFactor), path(cfg.miner) {
    result_path = std::filesystem::current_path() / resName;
  };

  ~Executor() = default;

  Executor(Executor &) = delete;
  Executor(Executor &&) = delete;

  Executor &operator=(Executor &) = delete;
  Executor &operator=(Executor &&) = delete;

private:
  std::string taskToArgs(const model::MinerTask &t, int gpu);
  bool answerExists();
  std::vector<model::Answer::Byte> getAnswer();
  // TODO: Hide boost::process from user
  exec_res::ExecRes exec(const model::MinerTask &task, int gpu,
                         boost::process::group &g);
  exec_res::ExecRes execSafe(const model::MinerTask &task, int gpu,
                             boost::process::group &g);

public:
  exec_res::ExecRes Run(const model::MinerTask &task);
  void Stop();
};

} // namespace crypto

#endif