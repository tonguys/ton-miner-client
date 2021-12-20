#include "executor.hpp"
#include "responses.hpp"

#include <istream>
#include <sstream>
#include <string>

#include "boost/process.hpp"
#include "boost/process/detail/child_decl.hpp"
#include "boost/process/io.hpp"
#include "boost/process/pipe.hpp"
#include "boost/asio.hpp"

namespace crypto {

namespace bp = boost::process; 

class Executor::ExecutorImpl {
    private:
    std::string path;

    public:
    ExecutorImpl(std::string_view _path): 
        path(_path) {};
    ~ExecutorImpl() = default;

    ExecutorImpl(ExecutorImpl&) = delete;
    ExecutorImpl(ExecutorImpl&&) = delete;

    ExecutorImpl& operator=(ExecutorImpl&) = delete;
    ExecutorImpl& operator=(ExecutorImpl&&) = delete;

    public:
    exec_res::ExecRes Exec(const response::Task &task) {
        boost::asio::io_service ios;
        std::vector<char> buf;

        bp::group g;
        bp::child ch(path, g, bp::std_out > boost::asio::buffer(buf), ios);
        ios.run();

        if (!g.wait_until(task.expires)) {
            g.terminate();
            g.wait();
            return exec_res::Timeout{};
        }
        int code = ch.exit_code();
        if (code != 0) {
            return exec_res::Crash{code};
        }

        response::Answer answer;
        // TODO: read buf, parse and fill up answer
        return exec_res::Ok{answer};
    }

};

Executor::Executor(std::string_view _path): 
    pImpl(std::make_unique<ExecutorImpl>(path))
    {}

exec_res::ExecRes Executor::Exec(const response::Task &task) {
    return pImpl->Exec(task);
}

Executor::~Executor() = default;

}