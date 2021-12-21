#include "httpClient.hpp"
#include "nlohmann/json_fwd.hpp"
#include "responses.hpp"

#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <variant>
#include <optional>
#include <algorithm>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "fmt/core.h"
#include "spdlog/spdlog.h"

namespace crypto {

    namespace {
        template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
        template<class... Ts> overload(Ts...) -> overload<Ts...>;
    }

    namespace  {
        response::Err exceptionToErr() {
            auto eptr = std::current_exception();
            try {
                if (eptr) {
                    std::rethrow_exception(eptr);
                }
            } catch (std::exception &e) {
                return response::Err{-1, fmt::format("exception thrown: {}", e.what())};
            }
            return response::Err{-1, "unknown exception thrown"};
        }
    }

    class HTTPClient::HTTPClientImpl {
        private:
        httplib::SSLClient client;

        public:
        using Response = std::variant<
            response::Err,
            response::Ok>;

        explicit HTTPClientImpl(std::string_view url): client(url.data()) {
            client.set_default_headers({
                { "accept", "application/json"},
            });
        };
        ~HTTPClientImpl() = default;

        HTTPClientImpl(HTTPClientImpl&) = delete;
        HTTPClientImpl(HTTPClientImpl&&) = delete;

        HTTPClientImpl& operator=(HTTPClientImpl&) = delete;
        HTTPClientImpl& operator=(HTTPClientImpl&&) = delete;

        private:
        Response processResponse(httplib::Result &res, int expectedCode) {
            if (!res) {
                auto err = res.error();
                std::stringstream ss;
                ss << err;

                response::Err ret;
                ret.code = -1;
                ret.msg = ss.str();
                return ret;
            }

            if (res->status != expectedCode) {
                response::Err ret;
                ret.code = res->status;
                ret.msg = "server returned non-200 code";
                return ret;
            }

            // TODO: check this string -> json conversion about safety/exceptions and so
            return response::Ok{res->body};
        }

        public:
        Response Get(std::string_view request) {
            try {
                auto res = client.Get(request.data());
                return processResponse(res, 200);
            } catch (...) {
                return exceptionToErr();
            };
        }

        response::RegisterResponse Register(std::string_view token) {
            std::string request = fmt::format("/register?token={}", token);
            auto res = Get(request);
            std::fill(std::begin(request), std::end(request), 'a');

            response::RegisterResponse resp;
            std::visit(overload{
                [&resp](const response::Err &err){ resp = err; },
                [&resp](const response::Ok &ok){ resp = ok.body; }}, res);
            return resp;
        }

        response::TaskResponse GetTask() {
            auto res = Get("/task");
            response::TaskResponse resp;
            std::visit(overload{
                [&resp](const response::Err &err){ resp = err; },
                [&resp](const response::Ok &ok){ resp = ok.body; }}, res);
            return resp;
        }

        response::SendAnswerResponse SendAnswer(const response::Answer &a) {
            try {
                nlohmann::json request = a;
                auto res = client.Post("/send_answer", request, "application/json");
                auto processed = processResponse(res, 202);
            
                response::SendAnswerResponse resp;
                std::visit(overload{
                    [&resp](const response::Err &err){ resp = err; },
                    [&resp](const response::Ok &ok){ resp = ok.body; }}, processed);
                return resp;;
            } catch (...) {
                return exceptionToErr();
            }
        }

    };

    HTTPClient::HTTPClient(std::string_view url): pImpl(std::make_unique<HTTPClientImpl>(url)) {
    }

    HTTPClient::~HTTPClient() = default;

    std::optional<response::UserInfo> HTTPClient::doRegister(std::string_view token) {
        auto resp = pImpl->Register(token);
    
        std::optional<response::UserInfo> res = std::nullopt;
        std::visit(overload{
            [](const response::Err& err) {
                spdlog::critical("Can`t register: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const response::UserInfo& info) {res = info;} }, resp);
        return res;
    }

    std::optional<response::Task> HTTPClient::doGetTask() {
        auto resp = pImpl->GetTask();

        std::optional<response::Task> res = std::nullopt;
        std::visit(overload{
            [](const response::Err& err) {
                spdlog::critical("Can`t get task: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const response::Task& info) {res = info;} }, resp);
        return res;
    }

    std::optional<response::AnswerStatus> HTTPClient::doSendAnswer(const response::Answer &answer) {
        auto resp = pImpl->SendAnswer(answer);

        std::optional<response::AnswerStatus> res = std::nullopt;
        std::visit(overload{
            [](const response::Err& err) {
                spdlog::critical("Can`t send answer: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const response::AnswerStatus& info) {res = info;} }, resp);
        return res;
    }
}