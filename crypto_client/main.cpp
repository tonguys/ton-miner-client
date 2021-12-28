#include "spdlog/common.h"
#include "src/app.hpp"

int main() {
    using namespace crypto;

    std::string token = "9cae7663b25e9f91e989cb250a9174a3998e2bffd76d23df";
    std::string url = "test-server1.tonguys.com";

    crypto::run(crypto::model::Config(
        model::Token = token,
        model::Url = url,
        model::LogLevel = spdlog::level::debug));
}