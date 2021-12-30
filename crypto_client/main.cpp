#include "app.hpp"
#include "fmt/core.h"
#include "lyra/arg.hpp"
#include "lyra/help.hpp"
#include "lyra/opt.hpp"
#include "models.hpp"

#include "lyra/lyra.hpp"

int main(int argc, char *argv[]) {
  using namespace crypto;

  std::string token;
  std::string url = "test-server1.tonguys.com";
  std::string logLevel = "debug";
  std::string factor = "1";
  bool showHelp = false;

  auto currentDirectory = std::filesystem::current_path();
  auto miner = currentDirectory / "pow-miner-cuda";

  auto cli =
      lyra::help(showHelp) |
      lyra::opt(token, "token")["-t"]["--token"]("Your tg token").required() |
      lyra::opt(url, "url")["-u"]["--url"](
          fmt::format("Server url (default to {})", url))
          .optional() |
      lyra::opt(logLevel, "logLevel")["-l"]["--level"]("Log level")
          .optional()
          .choices("debug", "info", "err") |
      lyra::opt(miner, "miner")["-m"]["--miner"]("Path to ton miner")
          .optional() |
      lyra::opt(factor, "factor")["-F"]["--boost-factor"]("Boost factor")
          .optional();

  auto result = cli.parse({argc, argv});
  if (!result) {
    std::cerr << "Error in command line: " << result.message() << std::endl;
    std::cout << cli << std::endl;
    return 1;
  }

  if (showHelp) {
    std::cout << cli << std::endl;
    return 0;
  }

  return crypto::run(crypto::model::Config(
      model::Token = token, model::Url = url, model::LogLevel = logLevel,
      model::MinerPath = miner, model::BoostFactor = factor));
}