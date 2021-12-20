#include <memory>
#include <string>
#include <string_view>
#include <optional>

#include "responses.hpp"

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

namespace crypto {
    class HTTPClient {
        class HTTPClientImpl;

        private:
        std::unique_ptr<HTTPClientImpl> pImpl;

        public: 
        HTTPClient(std::string_view); 
        ~HTTPClient();

        HTTPClient(HTTPClient&) = delete;
        HTTPClient(HTTPClient&&) noexcept;

        HTTPClient& operator=(HTTPClient&) = delete;
        HTTPClient& operator=(HTTPClient&&) noexcept;

        public:
        std::optional<response::UserInfo> Register(std::string_view);
        std::optional<response::Task> GetTask();
        std::optional<response::AnswerStatus> SendAnswer(const response::Answer &answer);

        };
}

#endif
