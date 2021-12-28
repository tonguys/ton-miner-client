#include <cstdint>
#include <string>
#include <variant>
#include <chrono>
#include <vector>

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#ifndef MODELS_HPP
#define MODELS_HPP

namespace crypto::model {

struct Err {
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
    explicit Timestamp(long unix_timestamp): unix(unix_timestamp) {}
    explicit Timestamp(time_point<steady_clock> tp) {
        unix = duration_cast<seconds>(tp.time_since_epoch()).count();
    }

    Timestamp& operator=(const Timestamp&) = default;
    Timestamp& operator=(Timestamp&&) = default;
    Timestamp(const Timestamp&) = default;
    Timestamp(Timestamp&&) = default;
    ~Timestamp() = default;

    long GetUnix() const {
        return unix;
    }
    auto GetChrono() const {
        return time_point<steady_clock>{seconds{unix}};
    }
};

}

//get/task
struct Task {
    std::string seed;
    std::string complexity;
    std::string giver_address;
    std::string pool_address;
    util::Timestamp expires;
};
using TaskResponse = std::variant<Err, Task>;

//post/send_answer
struct AnswerStatus {
    bool accepted;
};
using SendAnswerResponse = std::variant<Err, AnswerStatus>;
struct Answer {
    using Byte = uint8_t;
    std::vector<Byte> boc;
    std::string giver_address;
};

struct Config {
    std::string token;
    std::string url;
    spdlog::level::level_enum logLevel;

    template <class ...Args>
    explicit Config(Args ...args) {
        (args.Set(*this), ...);
    }
};

class TokenOption {
    std::string data;

    public:
    void Set(Config &cfg) {
        cfg.token = std::move(data);
    }

    TokenOption& operator=(std::string token) {
        data = std::move(token);
        return *this;
    }
};

class UrlOption {
    std::string data;

    public:
    void Set(Config &cfg) {
        cfg.url = std::move(data);
    }

    UrlOption& operator=(std::string url) {
        data = std::move(url);
        return *this;
    }
};

class LogLevelOption {
    spdlog::level::level_enum data;

    public:
    void Set(Config &cfg) {
        cfg.logLevel = data;
    }

    LogLevelOption& operator=(spdlog::level::level_enum level) {
        data = level;
        return *this;
    }
};

inline TokenOption Token;
inline UrlOption Url;
inline LogLevelOption LogLevel;


std::string Dump(const Err&);
std::string Dump(const Ok&);
std::string Dump(const UserInfo&);
std::string Dump(const Task&);
std::string Dump(const AnswerStatus&);
std::string Dump(const Answer&);
std::string Dump(const Config&);

using json = nlohmann::json;

void to_json(json& j, const UserInfo &info);
void from_json(const json& j, UserInfo &info);

void to_json(json& j, const Task &task);
void from_json(const json& j, Task &task);

void to_json(json& j, const AnswerStatus &status);
void from_json(const json& j, AnswerStatus &status);

void to_json(json& j, const Answer &answer);
void from_json(const json& j, Answer &answer);

template <class T>
inline typename std::enable_if<std::is_same<decltype(Dump(T{})), std::string>::value, std::ostream&>::type 
operator<<(std::ostream &os, const T &x) {
	os << Dump(x);
	return os;
}

}

#endif