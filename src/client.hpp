#include <optional>
#include <string_view>

#include "models.hpp"

#ifndef CLIENT_HPP
#define CLIENT_HPP

namespace crypto {

class Client {
private:
  virtual std::optional<model::UserInfo> doRegister() = 0;
  virtual std::optional<model::Task> doGetTask() = 0;
  virtual std::optional<model::AnswerStatus>
  doSendAnswer(const model::Answer &) = 0;

public:
  Client() = default;

  Client(Client &) = delete;
  Client(Client &&) = delete;
  Client &operator=(Client &&) = delete;
  Client &operator=(Client &) = delete;

  virtual ~Client() = default;

public:
  std::optional<model::UserInfo> Register() { return doRegister(); }
  std::optional<model::Task> GetTask() { return doGetTask(); }
  std::optional<model::AnswerStatus> SendAnswer(const model::Answer &answer) {
    return doSendAnswer(answer);
  }
};

} // namespace crypto

#endif