#include "executor.hpp"
#include "boost/exception/exception.hpp"
#include "boost/process/exception.hpp"
#include "openssl/bio.h"
#include "responses.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>

#include "boost/process.hpp"
#include "boost/process/detail/child_decl.hpp"
#include "boost/process/io.hpp"
#include "boost/process/pipe.hpp"
#include "boost/asio.hpp"
#include "boost/asio/streambuf.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace crypto {

namespace bp = boost::process; 

std::string Executor::taskToArgs(const response::Task &t) {
    // TODO: conigurable -g option
    const long long iterations = 100000000000;
    return fmt::format(
        "-g 0 -e {} {} {} {} {} {} {}",
        t.expires.GetUnix(),
        t.giver_address,
        t.seed,
        t.complexity,
        iterations,
        t.giver_address,
        resName);
}

exec_res::ExecRes Executor::ExecImpl(const response::Task &task) {
    boost::asio::io_service ios;
    std::future<std::string> outData;
    boost::asio::streambuf errData;

    auto args = taskToArgs(task);
    spdlog::info("Miner args: {}", args); 
    bp::child ch(
        path.string(),
        args,
        bp::std_in.close(),
        bp::std_err > errData,
        bp::std_out > outData,
        ios);

    ios.run();
    auto status =  outData.wait_until(task.expires.GetChrono());
    if (status == std::future_status::timeout) {
        ch.terminate();
        ch.wait();
        return exec_res::Timeout{};
    }

    auto out = outData.get();
    std::string err((std::istreambuf_iterator<char>(&errData)), std::istreambuf_iterator<char>(nullptr));
    spdlog::info("Miner stdout:\n{}", out);
    spdlog::info("Miner stderr:\n{}", err);

    ch.wait();
    auto code = ch.exit_code();
    if (code != 0) {
        return exec_res::Crash{"non-nil exit code", code};
    }

    if (!AnswerExists()) {
        return exec_res::Crash{"can`t locate boc file", -1};
    }

    response::Answer answer;
    answer.giver_address = task.giver_address;
    answer.boc = GetAnswer();
    spdlog::debug(response::Dump(answer));
    return exec_res::Ok{answer};
}

exec_res::ExecRes Executor::Exec(const response::Task &task) {
    try {
        spdlog::info("Exec miner");
        return ExecImpl(task);
    } catch (boost::process::process_error &e) {
        spdlog::error("Exec got boost exception: {}; code: {}", e.what(), e.code().message());
    } catch (std::exception &e) {
        spdlog::error("Exec got exception: {}", e.what());
    } catch (...) {
        spdlog::error("Exec got unknown exception");
    }
    return exec_res::Crash{};
}

bool Executor::AnswerExists() {
    return std::filesystem::exists(result_path);
}

std::vector <response::Answer::Byte> Executor::GetAnswer() {
    std::ifstream file(result_path, std::ios::binary | std::ios::in);
    return std::vector<response::Answer::Byte>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

}