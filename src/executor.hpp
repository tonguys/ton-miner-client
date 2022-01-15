#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "boost/fiber/condition_variable.hpp"
#include "boost/fiber/mutex.hpp"
#include "boost/process/group.hpp"
#include "boost/filesystem.hpp"
#include "boost/core/ignore_unused.hpp"
#include "fmt/core.h"
#include "models.hpp"

#include "fmt/format.h"

#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

namespace crypto {

class Waiter {
private:
  boost::fibers::condition_variable cond;
  boost::fibers::mutex mutex;
  bool ready = false;
  long n = 0;

public:
  typedef std::shared_ptr<Waiter> Ptr;

  bool Ready() {
    std::unique_lock<boost::fibers::mutex> lock(mutex);
    return ready;
  }

  void Add() {
    std::unique_lock<boost::fibers::mutex> lock(mutex);
    n += 1;
  }

  void Done() {
    bool allDone = false;
    {
      std::unique_lock<boost::fibers::mutex> lock(mutex);
      n -= 1;
      if (n <= 0 && !ready) {
        ready = true;
        allDone = true;
      }
    }
    if (allDone) {
      cond.notify_one();
    }
  }

  void Wait() {
    std::unique_lock<boost::fibers::mutex> lock(mutex);
    ready = false;
    cond.wait(lock, [this]() { return ready; });
  }

  void Notify() {
    {
      std::unique_lock<boost::fibers::mutex> lock(mutex);
      if (ready) {
        return;
      }
      ready = true;
    }
    cond.notify_one();
  }
};

namespace exec_res {
struct Timeout {};

struct Crash {
  Crash() = default;

  template <typename... Args>
  explicit Crash(fmt::format_string<Args...> fmt, Args &&...args)
      : msg(fmt::format(fmt, std::forward<Args>(args)...)) {}

  std::string msg;
  int code = 0;
};

struct Ok {
  model::Answer answer;
};

using ExecRes = std::variant<Timeout, Crash, Ok>;

inline std::string Dump(const Timeout &time) {
  boost::ignore_unused(time);
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
  const boost::filesystem::path path;
  boost::filesystem::path result_path;
  inline static const char *const resName = "mined.boc";

  std::shared_ptr<boost::process::group> pg;
  std::shared_ptr<Waiter> waiter;
  std::atomic_bool running = false;

public:
  explicit Executor(const model::Config &cfg)
      : factor(cfg.boostFactor), path(cfg.miner) {
    result_path = boost::filesystem::current_path() / resName;
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
  exec_res::ExecRes exec(const model::MinerTask &task, int gpu);
  exec_res::ExecRes execSafe(const model::MinerTask &task, int gpu);

public:
  std::optional<exec_res::Ok> Run(const model::MinerTask &task);
  void Stop();
};

} // namespace crypto

#endif