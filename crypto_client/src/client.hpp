#include <optional>
#include <string_view>

#include "responses.hpp"

#ifndef CLIENT_HPP
#define CLIENT_HPP

namespace crypto {

class Client {
    private:
    virtual std::optional<response::UserInfo> doRegister(std::string_view) = 0;
    virtual std::optional<response::Task> doGetTask() = 0;
    virtual std::optional<response::AnswerStatus> doSendAnswer(const response::Answer&) = 0;


    public:
    Client() = default;

    Client(Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
    Client& operator=(Client&) = delete;

    virtual ~Client() = default;

    public:
    std::optional<response::UserInfo> Register(std::string_view path) {
        return doRegister(path);
    }
    std::optional<response::Task> GetTask() {
        return doGetTask();
    }
    std::optional<response::AnswerStatus> SendAnswer(const response::Answer &answer) {
        return doSendAnswer(answer);
    }

};

}

#endif