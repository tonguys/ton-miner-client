#include <chrono>
#include <string_view>

#include "client.hpp"

#ifndef MACK_CLIENT_HPP
#define MACK_CLIENT_HPP

namespace crypto::mock {

auto inline defaultUserInfo() {
    return response::UserInfo{};
}

auto inline defaultTask() {
    auto expires = std::chrono::system_clock::now() + std::chrono::seconds(5);
    return response::Task{ expires };
}

auto inline defaultAnswerStatus() {
    return response::AnswerStatus{};
}

class MockClient final: public Client {
    private:
    std::string url;

    public:
    explicit MockClient([[maybe_unused]] std::string_view url) {};
    ~MockClient() final = default;

    private:
    std::optional<response::UserInfo> doRegister([[maybe_unused]] std::string_view token) final {
        return defaultUserInfo();
    };
    std::optional<response::Task> doGetTask() final {
        return defaultTask();
    };
    std::optional<response::AnswerStatus> doSendAnswer([[maybe_unused]] const response::Answer &answer) final {
        return defaultAnswerStatus();
    };
};

}

#endif