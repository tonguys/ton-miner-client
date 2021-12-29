#include "fmt/core.h"
#include "lyra/arg.hpp"
#include "lyra/help.hpp"
#include "models.hpp"
#include "app.hpp"

#include "lyra/lyra.hpp"


int main(int argc, char *argv[]) {
    using namespace crypto;

    std::string token = "9cae7663b25e9f91e989cb250a9174a3998e2bffd76d23df";
    std::string url = "test-server1.tonguys.com";
    std::string logLevel = "debug";
    bool showHelp = false;
    auto cli
        = lyra::help(showHelp)
        | lyra::opt(token, "token")
            ["-t"]["--token"]
            ("Your tg token").required()
        | lyra::opt(url, "url")
            ["-u"]["--url"]
            (fmt::format("Server url (default to {})", url)).optional()
        | lyra::opt(logLevel, "logLevel")
            ["-l"]["--level"]
            ("Log level").optional().choices(
                "debug",
                "info",
                "err",
                "critical");
        
    auto result = cli.parse({argc, argv});
    if ( !result ) { 
    	std::cerr << "Error in command line: " << result.message() << std::endl;
        std::cout << cli << std::endl;
    	return 1;
    }

    if (showHelp) {
        std::cout << cli << std::endl;
        return 0;
    }
    
    crypto::run(crypto::model::Config(
        model::Token = token,
        model::Url = url,
        model::LogLevel = logLevel));
}