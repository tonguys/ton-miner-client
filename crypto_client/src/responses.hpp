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
struct UserInfo {};
using RegisterResponse = std::variant<Err, UserInfo>;

//get/task
struct Task {
    std::chrono::time_point<std::chrono::system_clock> expires;
};
using TaskResponse = std::variant<Err, Task>;

//post/send_answer
struct AnswerStatus {};
using SendAnswerResponse = std::variant<Err, AnswerStatus>;
struct Answer {};


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