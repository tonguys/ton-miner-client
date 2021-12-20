#include <string_view>
#include <string>
#include <memory>
#include <variant>

#include "responses.hpp"

#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

namespace crypto {

namespace exec_res {
struct Timeout {};

struct Crash {
    int code;
};

struct Ok {
    response::Answer answer;
};

using ExecRes = std::variant<Timeout, Crash, Ok>;
}

class Executor {
    class ExecutorImpl;

    private:
    std::unique_ptr<ExecutorImpl> pImpl;
    std::string path;

    public:
    explicit Executor(std::string_view);
    ~Executor();

    Executor(Executor&) = delete;
    Executor(Executor&&) = delete;

    Executor& operator=(Executor&) = delete;
    Executor& operator=(Executor&&) = delete;

    public:
    exec_res::ExecRes Exec(const response::Task &task);
};

}

#endif