#include <string_view>
#include <string>
#include <memory>
#include <variant>
#include <filesystem>
#include <vector>

#include "responses.hpp"

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
    response::Answer answer;
};

using ExecRes = std::variant<Timeout, Crash, Ok>;

inline std::string Dump([[maybe_unused]] const Timeout &time) {
    return fmt::format("Timeout{{}}");
}

inline std::string Dump(const Crash &crash) {
    return fmt::format("Crash{{code:{}, msg:{}}}", crash.code, crash.msg);
}

inline std::string Dump(const Ok &ok) {
    return fmt::format("Ok{{answer:{}}}", Dump(ok.answer));
}

}

class Executor {
    private:
    std::filesystem::path path;
    std::filesystem::path result_path;
    inline static const char* const resName = "mined.boc";

    public:
    explicit Executor(std::filesystem::path _path): path(std::move(_path)) {
        result_path = path.parent_path() / resName;
    };
    explicit Executor(std::string_view _path): Executor(std::filesystem::path(_path)) {};

    ~Executor() = default;

    Executor(Executor&) = delete;
    Executor(Executor&&) = delete;

    Executor& operator=(Executor&) = delete;
    Executor& operator=(Executor&&) = delete;

    private:
    static std::string taskToArgs(const response::Task &t);
    bool AnswerExists();
    std::vector<response::Answer::Byte> GetAnswer();
    exec_res::ExecRes ExecImpl(const response::Task &task);

    public:
    exec_res::ExecRes Exec(const response::Task &task);
};

}

#endif