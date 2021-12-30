#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <optional>
#include <memory>
#include <filesystem>

#include "app.hpp"

#include "client.hpp"
#include "httpClient.hpp"
#include "mockClient.hpp"
#include "executor.hpp"

#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace crypto {

namespace {
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;
}

int run(const model::Config &cfg) {
    spdlog::set_level(cfg.logLevel);

    spdlog::info("Starting with {}", cfg);

    std::unique_ptr<Client> client = std::make_unique<HTTPClient>(cfg.url, cfg.token);

    auto auth = client->Register();
    if (!auth) {
        spdlog::critical("Registration failed, inspect logs for details");
        return 1;
    }

    spdlog::debug("Creating exec with path {}", cfg.miner.string());
    Executor exec(cfg);
    
    std::optional<crypto::model::Task> task;
    while (true) {
        spdlog::debug("Request new task");
        task = client->GetTask();
        if (!task) {
            spdlog::critical("Can`t get new task from server, inspect logs for details");
            return 1;
        }

        spdlog::debug("Execing miner with task: {}", task.value());
        auto res = exec.Exec(task.value());
        std::optional<model::Answer> answer = std::nullopt;
        std::visit(overload{
            [](exec_res::Timeout){ 
                spdlog::info("Miner timeout");
            },
            [](const exec_res::Crash &c){
                spdlog::error("Miner crashed: {}", exec_res::Dump(c));
            },
            [&answer](exec_res::Ok res){
                spdlog::info("Answer found");
                answer = res.answer;
            }
        }, res);

        if (answer) {
            spdlog::debug("Sending answer");
            auto status = client->SendAnswer(answer.value());
            if (!status) {
                spdlog::critical("Cant send answer, inspect logs for details");
                return 1;
            }
        }

    }
}

}