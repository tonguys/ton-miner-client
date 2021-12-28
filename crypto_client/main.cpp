#include "spdlog/common.h"
#include "src/app.hpp"

int main() {
    using namespace crypto;

    std::string token = "9cae7663b25e9f91e989cb250a9174a3998e2bffd76d23df";
    std::string url = "test-server1.tonguys.com";

    crypto::run(crypto::cfg::Config(
        cfg::Token = token,
        cfg::Url = url,
        cfg::LogLevel = spdlog::level::debug));
}