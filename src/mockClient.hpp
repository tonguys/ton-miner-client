#include <chrono>
#include <string_view>

#include "boost/core/ignore_unused.hpp"

#include "client.hpp"

#ifndef MACK_CLIENT_HPP
#define MACK_CLIENT_HPP

namespace crypto::mock {

auto inline defaultUserInfo() {
  return model::UserInfo{"pool_address", "user_address", 10};
}

auto inline defaultTask() {
  model::Task res;
  res.expires = model::util::Timestamp(std::chrono::steady_clock::now() +
                                       std::chrono::seconds(5));
  res.pool_address = "kQBWkNKqzCAwA9vjMwRmg7aY75Rf8lByPA9zKXoqGkHi8SM7";
  res.seed = "229760179690128740373110445116482216837";
  res.complexity =
      "53919893334301279589334030174039261347274288845081144962207220498432";
  res.giver_address = "kf-kkdY_B7p-77TLn2hUhM6QidWrrsl8FYWCIvBMpZKprBtN";

  return res;
}

auto inline defaultAnswerStatus() { return model::AnswerStatus{true}; }

class MockClient final : public Client {
private:
  std::string url;

public:
  explicit MockClient(std::string_view url,
                      std::string_view token){
                        boost::ignore_unused(url);
                        boost::ignore_unused(token);
                      };
  ~MockClient() final = default;

private:
  std::optional<model::UserInfo> doRegister() final {
    return defaultUserInfo();
  };
  std::optional<model::Task> doGetTask() final { return defaultTask(); };
  std::optional<model::AnswerStatus>
  doSendAnswer(const model::Answer &answer) final {
    boost::ignore_unused(answer);
    return defaultAnswerStatus();
  };
};

} // namespace crypto::mock

#endif