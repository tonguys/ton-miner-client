#include "boost/filesystem.hpp"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
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

// overload pattern:
// https://www.cppstories.com/2019/02/2lines3featuresoverload.html/
template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
template <class... Ts> overload(Ts...) -> overload<Ts...>;

namespace time = std::chrono;

class Timestamp {
private:
  long unixTime;

public:
  Timestamp() = default;
  explicit Timestamp(long unix_timestamp) : unixTime(unix_timestamp) {}
  explicit Timestamp(time::time_point<time::steady_clock> tp) {
    unixTime =
        time::duration_cast<time::seconds>(tp.time_since_epoch()).count();
  }

  Timestamp &operator=(const Timestamp &) = default;
  Timestamp &operator=(Timestamp &&) = default;
  Timestamp(const Timestamp &) = default;
  Timestamp(Timestamp &&) = default;
  ~Timestamp() = default;

  long GetUnix() const { return unixTime; }
  auto GetChrono() const {
    return time::time_point<time::steady_clock>{time::seconds{unixTime}};
  }
};

} // namespace util

struct Statistic {
  int count;
  long long rate;
};

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
  std::optional<Statistic> statistic;
};

struct MinerTask : Task {
  long long iterations;
  std::vector<int> gpu;

  MinerTask &operator=(const Task &task) {
    Task::operator=(task);
    return *this;
  }

  MinerTask(long long _iterations, const Task &task, std::vector<int> _gpu)
      : iterations(_iterations), gpu(std::move(_gpu)) {
    *this = task;
  }
};

struct Config {
  std::string token;
  std::string url;
  spdlog::level::level_enum logLevel;
  boost::filesystem::path logPath;
  boost::filesystem::path miner;
  long boostFactor;
  long long iterations;
  std::vector<int> gpu;

  // NOTE: DONT FORGET TO INCRIMENT IN CASE OF ADDING OPTIONS
  static constexpr int numberOfField = 8;

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

class LogPathOption {
  boost::filesystem::path miner;

public:
  void Set(Config &cfg) { cfg.logPath = std::move(miner); }

  LogPathOption &operator=(boost::filesystem::path path) {
    miner = std::move(path);
    return *this;
  }
  LogPathOption &operator=(std::string_view path) {
    return *this = boost::filesystem::path(path.data());
  }
};

class MinerPathOption {
  boost::filesystem::path miner;

public:
  void Set(Config &cfg) { cfg.miner = std::move(miner); }

  MinerPathOption &operator=(boost::filesystem::path path) {
    miner = std::move(path);
    return *this;
  }
  MinerPathOption &operator=(std::string_view path) {
    return *this = boost::filesystem::path(path.data());
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
  std::vector<int> data;

public:
  void Set(Config &cfg) { cfg.gpu = data; }

  GPUOptions &operator=(std::vector<int> gpu) {
    data = std::move(gpu);
    return *this;
  }
};

inline TokenOption Token;
inline UrlOption Url;
inline LogLevelOption LogLevel;
inline LogPathOption LogPath;
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

void to_json(json &j, const Statistic &st);
void from_json(const json &j, Statistic &st);

template <class T>
inline typename std::enable_if<
    std::is_same<decltype(Dump(T{})), std::string>::value, std::ostream &>::type
operator<<(std::ostream &os, const T &x) {
  os << Dump(x);
  return os;
}

} // namespace crypto::model

#endif