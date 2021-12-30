#include "models.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include "cppcodec/base64_rfc4648.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace crypto::model {

std::string Dump(const Err &e) {
  return fmt::format("Err{{code: {}, msg:{}, body: {}}}", e.code, e.msg,
                     e.body.value_or("(no body)"));
}

std::string Dump([[maybe_unused]] const Ok &ok) {
  // we will not print body, as internal json
  // 1) Possibly may contain sensitive info
  // 2) Likely will be logged after json unpacking to struct
  return fmt::format("Ok");
}

std::string Dump(const UserInfo &info) {
  return fmt::format(
      "UserInfo{{user_addres: {}, pool_address: {}, shares: {}}}",
      info.user_address, info.pool_address, info.shares);
}

std::string Dump(const Task &task) {
  return fmt::format(
      "Task{{complexity: {}, expires: {}, giver: {}, pool: {}, seed: {}}}",
      task.complexity, task.expires.GetUnix(), task.giver_address,
      task.pool_address, task.seed);
}

std::string Dump(const AnswerStatus &status) {
  return fmt::format("Status{{accepted: {}}}", status.accepted);
}

std::string Dump(const Answer &answer) {
  return fmt::format("Answer{{giver: {}, boc: {}}}", answer.giver_address,
                     fmt::join(answer.boc, ""));
}

std::string Dump(const Config &cfg) {
  return fmt::format("Config{{url:{}, logLevel:{}, token:NOT_PRINTED}}",
                     cfg.url, cfg.logLevel);
}

void to_json(json &j, const UserInfo &info) {
  j["pool_address"] = info.pool_address;
  j["user_address"] = info.user_address;
  j["shares"] = info.shares;
}

void from_json(const json &j, UserInfo &info) {
  j.at("pool_address").get_to(info.pool_address);
  j.at("user_address").get_to(info.user_address);
  j.at("shares").get_to(info.shares);
}

void to_json(json &j, const Task &task) {
  j.at("seed") = task.seed;
  j.at("complexity") = task.complexity;
  j.at("giver_address") = task.giver_address;
  j.at("pool_address") = task.pool_address;
  j.at("expires") = std::to_string(task.expires.GetUnix());
}

void from_json(const json &j, Task &task) {
  j.at("seed").get_to(task.seed);
  j.at("complexity").get_to(task.complexity);
  j.at("giver_address").get_to(task.giver_address);
  j.at("pool_address").get_to(task.pool_address);

  long tmp = 0;
  j.at("expires").get_to(tmp);
  task.expires = util::Timestamp(tmp);
}

namespace {
const char *const ACCEPTED = "ACCEPTED";
const char *const DECLINED = "DECLINED";
} // namespace

void to_json(json &j, const AnswerStatus &status) {
  if (status.accepted) {
    j["status"] = ACCEPTED;
    return;
  }
  j["status"] = DECLINED;
}

void from_json(const json &j, AnswerStatus &status) {
  std::string statusString;
  j["status"].get_to(statusString);
  if (ACCEPTED == statusString) {
    status.accepted = true;
    return;
  }
  status.accepted = false;
}

void to_json(json &j, const Answer &answer) {
  using codec = cppcodec::base64_rfc4648;
  j["boc_data"] = codec::encode(answer.boc);
  j["giver_address"] = answer.giver_address;
}

void from_json(const json &j, Answer &answer) {
  using codec = cppcodec::base64_rfc4648;
  j["giver_address"].get_to(answer.giver_address);

  std::vector<Answer::Byte> tmp;
  j["boc_data"].get_to(tmp);
  answer.boc = codec::decode(tmp);
}

} // namespace crypto::model