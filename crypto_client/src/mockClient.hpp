#include <chrono>
#include <string_view>

#include "client.hpp"

#ifndef MACK_CLIENT_HPP
#define MACK_CLIENT_HPP

namespace crypto::mock {

auto inline defaultUserInfo() {
    return model::UserInfo{
        "pool_address",
        "user_address",
        10 };
}

auto inline defaultTask() {
    auto expires = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    return model::Task{
        "100000",
        "100000",
        "giver_address",
        "pool_address",
        model::util::Timestamp(expires)};
}

auto inline defaultAnswerStatus() {
    return model::AnswerStatus{
        true };
}

class MockClient final: public Client {
    private:
    std::string url;

    public:
    explicit MockClient([[maybe_unused]] std::string_view url,[[maybe_unused]] std::string_view token) {};
    ~MockClient() final = default;

    private:
    std::optional<model::UserInfo> doRegister() final {
        return defaultUserInfo();
    };
    std::optional<model::Task> doGetTask() final {
        return defaultTask();
    };
    std::optional<model::AnswerStatus> doSendAnswer([[maybe_unused]] const model::Answer &answer) final {
        return defaultAnswerStatus();
    };
};

}

#endif