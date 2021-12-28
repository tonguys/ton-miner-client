#include "httpClient.hpp"
#include "nlohmann/json_fwd.hpp"
#include "models.hpp"

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
        model::Err exceptionToErr() {
            auto eptr = std::current_exception();
            try {
                if (eptr) {
                    std::rethrow_exception(eptr);
                }
            } catch (std::exception &e) {
                return model::Err{-1, fmt::format("exception thrown: {}", e.what())};
            }
            return model::Err{-1, "unknown exception thrown"};
        }
    }

    class HTTPClient::HTTPClientImpl {
        private:
        httplib::SSLClient client;
        std::string token;

        public:
        using Response = std::variant<
            model::Err,
            model::Ok>;

        explicit HTTPClientImpl(std::string_view url, std::string_view _token): 
            client(url.data()),
            token(_token) {
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
        static Response processResponse(httplib::Result &res, int expectedCode) {
            if (!res) {
                auto err = res.error();
                std::stringstream ss;
                ss << err;

                model::Err ret;
                ret.code = -1;
                ret.msg = ss.str();
                return ret;
            }

            if (res->status != expectedCode) {
                model::Err ret;
                ret.code = res->status;
                ret.msg = "server returned non-200 code";
                return ret;
            }
            
            // TODO: check this string -> json conversion about safety/exceptions and so
            return model::Ok{ nlohmann::json::parse(res->body) };
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

        model::RegisterResponse Register() {
            std::string request = fmt::format("/api/v1/register?auth_token={}", token);
            auto res = Get(request);

            model::RegisterResponse resp;
            std::visit(overload{
                [&resp](const model::Err &err){ resp = err; },
                [&resp](const model::Ok &ok){ resp = ok.body; }}, res);
            return resp;
        }

        model::TaskResponse GetTask() {
            std::string request = fmt::format("/api/v1/task?auth_token={}", token);
            auto res = Get(request);
            
            model::TaskResponse resp;
            std::visit(overload{
                [&resp](const model::Err &err){ resp = err; },
                [&resp](const model::Ok &ok){ resp = ok.body; }}, res);
            return resp;
        }

        model::SendAnswerResponse SendAnswer(const model::Answer &a) {
            try {
                std::string path = fmt::format("/api/v1/send_answer?auth_token={}", token);
                nlohmann::json request = a;
                spdlog::debug("Sending answer: {}", request.dump());
                auto res = client.Post(path.c_str(), request.dump(), "application/json");
                auto processed = processResponse(res, 202);
            
                model::SendAnswerResponse resp;
                std::visit(overload{
                    [&resp](const model::Err &err){ resp = err; },
                    [&resp](const model::Ok &ok){ resp = ok.body; }}, processed);
                return resp;;
            } catch (...) {
                return exceptionToErr();
            }
        }

    };

    HTTPClient::HTTPClient(std::string_view url, std::string_view token): pImpl(std::make_unique<HTTPClientImpl>(url, token)) {
    }

    HTTPClient::~HTTPClient() = default;

    std::optional<model::UserInfo> HTTPClient::doRegister() {
        auto resp = pImpl->Register();
    
        std::optional<model::UserInfo> res = std::nullopt;
        std::visit(overload{
            [](const model::Err& err) {
                spdlog::critical("Can`t register: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const model::UserInfo& info) {res = info;} }, resp);
        return res;
    }

    std::optional<model::Task> HTTPClient::doGetTask() {
        auto resp = pImpl->GetTask();

        std::optional<model::Task> res = std::nullopt;
        std::visit(overload{
            [](const model::Err& err) {
                spdlog::critical("Can`t get task: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const model::Task& info) {res = info;} }, resp);
        return res;
    }

    std::optional<model::AnswerStatus> HTTPClient::doSendAnswer(const model::Answer &answer) {
        auto resp = pImpl->SendAnswer(answer);

        std::optional<model::AnswerStatus> res = std::nullopt;
        std::visit(overload{
            [](const model::Err& err) {
                spdlog::critical("Can`t send answer: code ({}), msg: {}", err.code, err.msg);
            },
            [&res](const model::AnswerStatus& info) {res = info;} }, resp);
        return res;
    }
}