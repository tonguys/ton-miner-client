#include <string>
#include <variant>
#include <chrono>

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#ifndef MODELS_HPP
#define MODELS_HPP

namespace crypto::response {

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
    std::string boc;
    std::string giver_address;
};


using json = nlohmann::json;

void to_json(json& j, const UserInfo &info);
void from_json(const json& j, UserInfo &info);

void to_json(json& j, const Task &task);
void from_json(const json& j, Task &task);

void to_json(json& j, const AnswerStatus &status);
void from_json(const json& j, AnswerStatus &status);

void to_json(json& j, const Answer &answer);
void from_json(const json& j, Answer &answer);


}

#endif