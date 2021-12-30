#include <string_view>
#include <string>
#include <memory>
#include <variant>
#include <filesystem>
#include <vector>

#include "models.hpp"

#include "fmt/format.h"

#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

namespace crypto {

namespace exec_res {
struct Timeout {};

struct Crash {
    std::string msg;
    int code;
};

struct Ok {
    model::Answer answer;
};

using ExecRes = std::variant<Timeout, Crash, Ok>;

inline std::string Dump([[maybe_unused]] const Timeout &time) {
    return fmt::format("Timeout{{}}");
}

inline std::string Dump(const Crash &crash) {
    return fmt::format("Crash{{code:{}, msg:{}}}", crash.code, crash.msg);
}

inline std::string Dump(const Ok &ok) {
    return fmt::format("Ok{{answer:{}}}", ok.answer);
}

}

class Executor {
    private:
    const std::string factor;
    const std::filesystem::path path;
    std::filesystem::path result_path;
    inline static const char* const resName = "mined.boc";

    public:
    explicit Executor(const model::Config &cfg): factor(cfg.boostFactor), path(cfg.miner) {
        result_path = path.parent_path() / resName;
    };

    ~Executor() = default;

    Executor(Executor&) = delete;
    Executor(Executor&&) = delete;

    Executor& operator=(Executor&) = delete;
    Executor& operator=(Executor&&) = delete;

    private:
    std::string taskToArgs(const model::Task &t);
    bool AnswerExists();
    std::vector<model::Answer::Byte> GetAnswer();
    exec_res::ExecRes ExecImpl(const model::Task &task);

    public:
    exec_res::ExecRes Exec(const model::Task &task);
};

}

#endif