#include <memory>
#include <string>
#include <string_view>
#include <optional>

#include "models.hpp"
#include "client.hpp"

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

namespace crypto {
    class HTTPClient final: public Client {
        class HTTPClientImpl;

        private:
        std::unique_ptr<HTTPClientImpl> pImpl;
        std::string token;

        public: 
        explicit HTTPClient(std::string_view, std::string_view); 
        ~HTTPClient() final;

        private:
        std::optional<model::UserInfo> doRegister() final;
        std::optional<model::Task> doGetTask() final;
        std::optional<model::AnswerStatus> doSendAnswer(const model::Answer &answer) final;

        };
}

#endif
