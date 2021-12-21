#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <optional>
#include <memory>
#include <filesystem>

#include "app.hpp"

#include "client.hpp"
#include "mockClient.hpp"
#include "executor.hpp"

#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace crypto {

namespace {

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

}

int run(std::string &&token) {
    spdlog::set_level(spdlog::level::debug);

    //TODO: fill url
    std::string url("url");
    spdlog::info("Starting with url {}", url);

    std::unique_ptr<Client> client = std::make_unique<mock::MockClient>(url);

    auto auth = client->Register(token);
    if (!auth) {
        spdlog::critical("Registration failed, inspect logs for details");
        return 1;
    }

    // TODO: fill path
    auto currentDirectory = std::filesystem::current_path();
    auto minerPath = currentDirectory / "pow-miner-cuda";
    spdlog::debug("Creating exec with path {}", minerPath.string());
    Executor exec(minerPath);
    
    int execCrashesCount = 0;
    bool shouldRequestNewTask = true;
    std::optional<crypto::response::Task> task;

    while (true) {
        if (shouldRequestNewTask) {
            spdlog::debug("Request new task");
            task = client->GetTask();
            if (!task) {
                spdlog::critical("Can`t get new task from server, inspect logs for details");
                return 1;
            }
        }
        shouldRequestNewTask = true;

        spdlog::debug("Execing miner...");
        auto res = exec.Exec(task.value());
        std::optional<response::Answer> answer = std::nullopt;
        std::visit(overload{
            [&execCrashesCount](exec_res::Timeout){ 
                spdlog::info("Miner timeout");
                execCrashesCount = 0;
            },
            [&execCrashesCount, &shouldRequestNewTask](const exec_res::Crash &c){
                spdlog::warn("Miner crashed; code: {}", c.code);
                execCrashesCount++;
                shouldRequestNewTask = false; 
            },
            [&answer, &execCrashesCount](exec_res::Ok res){
                spdlog::info("Answer found");
                answer = res.answer;
                execCrashesCount = 0;
            }
        }, res);

        if (execCrashesCount > 5) {
            spdlog::critical("Exec crashed much times, exiting...");
            return 1;
        }

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