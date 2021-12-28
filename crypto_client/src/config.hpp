
#include "fmt/format.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace crypto::cfg {

struct Config {
    std::string token;
    std::string url;
    spdlog::level::level_enum logLevel;

    template <class ...Args>
    explicit Config(Args ...args) {
        (args.Set(*this), ...);
    }
};

class TokenOption {
    std::string data;

    public:
    void Set(Config &cfg) {
        cfg.token = std::move(data);
    }

    TokenOption& operator=(std::string token) {
        data = std::move(token);
        return *this;
    }
};

class UrlOption {
    std::string data;

    public:
    void Set(Config &cfg) {
        cfg.url = std::move(data);
    }

    UrlOption& operator=(std::string url) {
        data = std::move(url);
        return *this;
    }
};

class LogLevelOption {
    spdlog::level::level_enum data;

    public:
    void Set(Config &cfg) {
        cfg.logLevel = data;
    }

    LogLevelOption& operator=(spdlog::level::level_enum level) {
        data = level;
        return *this;
    }
};

inline TokenOption Token;
inline UrlOption Url;
inline LogLevelOption LogLevel;


inline std::string Dump(const Config &cfg) {
    return fmt::format(
        "Config{{url:{}, logLevel:{}, token:NOT_PRINTED}}",
        cfg.url,
        cfg.logLevel);
}


}