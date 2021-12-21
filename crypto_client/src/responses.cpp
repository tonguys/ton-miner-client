#include "responses.hpp"
#include <chrono>
#include <string>


namespace crypto::response {

void to_json(json& j, const UserInfo &info) {

}

void from_json(const json& j, UserInfo &info) {

}

void to_json(json& j, const Task &task) {
    using namespace std::chrono;
    j["seed"] = task.seed;
    j["complexity"] = task.complexity;
    j["giver_address"] = task.giver_address;
    j["pool_adress"] = task.pool_address;

    auto timestamp =  duration_cast<seconds>(task.expires.time_since_epoch());
    j["expires"] = std::to_string(timestamp.count());
}

void from_json(const json& j, Task &task) {
    task.seed = j.at("seed");
    task.complexity = j.at("complexity");
    task.giver_address = j.at("giver_address");
    task.pool_address = j.at("pool_adress");

    long timestamp = j.at("expires");
}

void to_json(json& j, const AnswerStatus &status) {

}

void from_json(const json& j, AnswerStatus &status) {
    
}

void to_json(json& j, const Answer &answer) {

}

void from_json(const json& j, Answer &answer) {
    
}

}