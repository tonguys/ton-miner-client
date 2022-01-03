#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "fmt/format.h"
#include "fmt/ostream.h"
#include "nlohmann/json.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#ifndef MODELS_HPP
#define MODELS_HPP

#define _REGISTER_LOG_LEVEL(level) levels[#level] = level

namespace crypto::model {

struct Err {
  // we will not map server error responses into any type,
  // couze we only need to log it, json string will do the trick
  std::optional<std::string> body;
  int code;
  std::string msg;
};

struct Ok {
  nlohmann::json body;
};

// get/register
struct UserInfo {
  std::string pool_address;
  std::string user_address;
  long shares;
};
using RegisterResponse = std::variant<Err, UserInfo>;

namespace util {
using namespace std::chrono;

class Timestamp {
private:
  long unix;

public:
  Timestamp() = default;
  explicit Timestamp(long unix_timestamp) : unix(unix_timestamp) {}
  explicit Timestamp(time_point<steady_clock> tp) {
    unix = duration_cast<seconds>(tp.time_since_epoch()).count();
  }

  Timestamp &operator=(const Timestamp &) = default;
  Timestamp &operator=(Timestamp &&) = default;
  Timestamp(const Timestamp &) = default;
  Timestamp(Timestamp &&) = default;
  ~Timestamp() = default;

  long GetUnix() const { return unix; }
  auto GetChrono() const { return time_point<steady_clock>{seconds{unix}}; }
};

} // namespace util

// get/task
struct Task {
  std::string seed;
  std::string complexity;
  std::string giver_address;
  std::string pool_address;
  util::Timestamp expires;
};
using TaskResponse = std::variant<Err, Task>;

// post/send_answer
struct AnswerStatus {
  bool accepted;
};
using SendAnswerResponse = std::variant<Err, AnswerStatus>;
struct Answer {
  using Byte = uint8_t;
  std::vector<Byte> boc;
  std::string giver_address;
};

struct MinerTask : Task {
  long long iterations;
  int gpu;

  MinerTask &operator=(const Task &task) {
    Task::operator=(task);
    return *this;
  }

  MinerTask(long long _iterations, const Task &task, int _gpu)
      : iterations(_iterations), gpu(_gpu) {
    *this = task;
  }
};

struct Config {
  std::string token;
  std::string url;
  spdlog::level::level_enum logLevel;
  std::filesystem::path miner;
  long boostFactor;
  long long iterations;
  int gpu;

  // NOTE: DONT FORGET TO INCRIMENT IN CASE OF ADDING OPTIONS
  static constexpr int numberOfField = 7;

  template <class... Args> explicit constexpr Config(Args... args) {
    static_assert(sizeof...(args) == numberOfField,
                  "Not correct number of arguments in Config");
    (args.Set(*this), ...);
  }
};

class TokenOption {
  std::string data;

public:
  void Set(Config &cfg) { cfg.token = std::move(data); }

  TokenOption &operator=(std::string token) {
    data = std::move(token);
    return *this;
  }
};

class UrlOption {
  std::string data;

public:
  void Set(Config &cfg) { cfg.url = std::move(data); }

  UrlOption &operator=(std::string url) {
    data = std::move(url);
    return *this;
  }
};

// TODO: This may be done in compile time: make it constexpr, write static map,
// use static_assert
inline std::optional<spdlog::level::level_enum>
ToLevel(const std::string &level) {
  using namespace spdlog::level;
  std::map<std::string, level_enum> levels;
  _REGISTER_LOG_LEVEL(trace);
  _REGISTER_LOG_LEVEL(debug);
  _REGISTER_LOG_LEVEL(info);
  _REGISTER_LOG_LEVEL(warn);
  _REGISTER_LOG_LEVEL(err);
  _REGISTER_LOG_LEVEL(critical);
  _REGISTER_LOG_LEVEL(off);

  assert(levels.size() == n_levels);

  auto it = levels.find(level);
  if (it == levels.end()) {
    return std::nullopt;
  }
  return it->second;
}

class LogLevelOption {
  spdlog::level::level_enum data;

public:
  void Set(Config &cfg) { cfg.logLevel = data; }

  LogLevelOption &operator=(spdlog::level::level_enum level) {
    data = level;
    return *this;
  }

  LogLevelOption &operator=(const std::string &level) {
    return *this = ToLevel(level).value_or(spdlog::level::debug);
  }
};

class MinerPathOption {
  std::filesystem::path miner;

public:
  void Set(Config &cfg) { cfg.miner = std::move(miner); }

  MinerPathOption &operator=(std::filesystem::path path) {
    miner = std::move(path);
    return *this;
  }
  MinerPathOption &operator=(std::string_view path) {
    return *this = std::filesystem::path(path);
  }
};

class BoostFactorOption {
  long data;

public:
  void Set(Config &cfg) { cfg.boostFactor = data; }

  BoostFactorOption &operator=(long factor) {
    data = factor;
    return *this;
  }
};

class IterationsOption {
  long long data;

public:
  void Set(Config &cfg) { cfg.iterations = data; }

  IterationsOption &operator=(long long iterations) {
    data = iterations;
    return *this;
  }
};

class GPUOptions {
  int data;

public:
  void Set(Config &cfg) { cfg.gpu = data; }

  GPUOptions &operator=(int gpu) {
    data = gpu;
    return *this;
  }
};

inline TokenOption Token;
inline UrlOption Url;
inline LogLevelOption LogLevel;
inline MinerPathOption MinerPath;
inline BoostFactorOption BoostFactor;
inline IterationsOption Iterations;
inline GPUOptions GPU;

std::string Dump(const Err &);
std::string Dump(const Ok &);
std::string Dump(const UserInfo &);
std::string Dump(const Task &);
std::string Dump(const MinerTask &);
std::string Dump(const AnswerStatus &);
std::string Dump(const Answer &);
std::string Dump(const Config &);

using json = nlohmann::json;

void to_json(json &j, const UserInfo &info);
void from_json(const json &j, UserInfo &info);

void to_json(json &j, const Task &task);
void from_json(const json &j, Task &task);

void to_json(json &j, const AnswerStatus &status);
void from_json(const json &j, AnswerStatus &status);

void to_json(json &j, const Answer &answer);
void from_json(const json &j, Answer &answer);

template <class T>
inline typename std::enable_if<
    std::is_same<decltype(Dump(T{})), std::string>::value, std::ostream &>::type
operator<<(std::ostream &os, const T &x) {
  os << Dump(x);
  return os;
}

} // namespace crypto::model

#endif