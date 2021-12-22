#include "responses.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>


namespace crypto::response {

void to_json(json& j, const UserInfo &info) {
    j["pool_address"] = info.pool_address;
    j["user_address"] = info.user_address;
    j["shares"] = info.shares;
}

void from_json(const json& j, UserInfo &info) {
    j["pool_address"].get_to(info.pool_address);
    j["user_address"].get_to(info.user_address);
    j["shares"].get_to(info.shares);
}

void to_json(json& j, const Task &task) {
    using namespace std::chrono;
    j["seed"] = task.seed;
    j["complexity"] = task.complexity;
    j["giver_address"] = task.giver_address;
    j["pool_address"] = task.pool_address;
    j["expires"] = std::to_string(task.expires.GetUnix());
}

void from_json(const json& j, Task &task) {
    j.at("seed").get_to(task.seed);
    j.at("complexity").get_to(task.complexity);
    j.at("giver_address").get_to(task.giver_address);
    j.at("pool_address").get_to(task.pool_address);

    std::string tmp;
    j.at("expires").get_to(tmp);
    task.expires = util::Timestamp(std::stol(tmp));
}

namespace {
    const char* const ACCEPTED = "ACCEPTED";
    const char* const DECLINED = "DECLINED";
}

void to_json(json& j, const AnswerStatus &status) {
    if (status.accepted) {
        j["status"] = ACCEPTED;
        return;
    }
    j["status"] = DECLINED;
}

void from_json(const json& j, AnswerStatus &status) {
    std::string statusString;
    j["status"].get_to(statusString);
    if (ACCEPTED == statusString) {
        status.accepted = true;
        return;
    }
    status.accepted = false;
}

void to_json(json& j, const Answer &answer) {
    j["boc_data"] = answer.boc;
    j["giver_address"] = answer.giver_address;
}

void from_json(const json& j, Answer &answer) {
    j["boc_data"].get_to(answer.boc);
    j["giver_address"].get_to(answer.giver_address);
}

}