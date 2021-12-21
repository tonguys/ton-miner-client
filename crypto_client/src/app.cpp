#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <optional>
#include <memory>

#include "app.hpp"

#include "client.hpp"
#include "httpClient.hpp"
#include "executor.hpp"

namespace crypto {

namespace {

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

}

int run(std::string &&token) {
    //TODO: fill url
    std::unique_ptr<Client> client = std::make_unique<HTTPClient>("");

    auto auth = client->Register(token);
    if (!auth) {
        std::cout << "Registration failed, inspect logs for details." << std::endl;
        return 1;
    }

    // TODO: fille path
    Executor exec("path");
    int execCrashesCount = 0;
    bool shouldRequestNewTask = true;

    while (true) {
        std::optional<crypto::response::Task> task;
        if (shouldRequestNewTask) {
            task = client->GetTask();
            if (!task) {
               std::cout << "Can`t get new task from server, inspect logs for details." << std::endl;
                return 1;
            }
        }
        shouldRequestNewTask = true;

        auto res = exec.Exec(task.value());
        std::optional<response::Answer> answer = std::nullopt;
        std::visit(overload{
            [&execCrashesCount](exec_res::Timeout){ execCrashesCount = 0; },
            [&execCrashesCount, &shouldRequestNewTask](exec_res::Crash){ 
                execCrashesCount++;
                shouldRequestNewTask = false; },
            [&answer, &execCrashesCount](exec_res::Ok res){ 
                answer = res.answer;
                execCrashesCount = 0;
            }
        }, res);

        if (execCrashesCount > 5) {
            std::cout << "Exec crashed much times, exiting..." << std::endl;
            return 1;
        }

        if (answer) {
            auto status = client->SendAnswer(answer.value());
            if (!status) {
                std::cout << "Cant send answer, inspect logs for details" << std::endl;
                return 2;
            }
        }

    }
}

}