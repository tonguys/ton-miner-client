#include "app.hpp"
#include "models.hpp"

#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/range/algorithm.hpp"
#include "fmt/core.h"
#include "lyra/arg.hpp"
#include "lyra/arguments.hpp"
#include "lyra/help.hpp"
#include "lyra/lyra.hpp"
#include "lyra/opt.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

std::string parseGPU(std::vector<int> &result_gpus, std::string_view gpuRange) {
  // Yes, this stupid boilerplate code is needed just to parse
  // two numbers and a dash in square brackets (like [1-4]).
  // Yes, it`s dumb, as soon, as we could make one simple regex.
  // But, first of all, regex is always a technical debt, learn to live without
  // them. Secondly, I want to make user friendly cli with user friendly error
  // messages. There are always a donkey: user or developer.
  std::vector<int> gpus;
  if (gpuRange.empty()) {
    return "empty range";
  }
  if (gpuRange[0] != '[') {
    return "invalid format: not [ on first place";
  }
  if (gpuRange.back() != ']') {
    return "invalid format: not ] on the last place";
  }

  // Remove brackets, now we are working with range_str, not gpuRange
  auto range_str = gpuRange.substr(1, gpuRange.size() - 2);

  // split ranges by comma
  std::vector<std::string> ranges;
  boost::split(ranges, range_str, boost::is_any_of(","));

  // now we are working on range(it is just one number or range like `1-4`)
  for (const auto &range : ranges) {
    bool is_number = false;
    int gpu_number = -1;
    try {
      gpu_number = boost::lexical_cast<int>(range);
      is_number = true;
    } catch (boost::bad_lexical_cast &) {
    }

    // if it is just number then just push to back its value
    if (is_number) {
      gpus.emplace_back(gpu_number);
      continue;
    }

    const auto dash = range.find('-');
    if (dash == std::basic_string_view<char>::npos) {
      return "dash is missed";
    }
    if (dash == 0) {
      return "first number is missed or negative";
    }

    auto ls = range.substr(0, dash);
    auto rs = range.substr(dash + 1);
    int l = 0;
    int r = 0;
    try {
      l = boost::lexical_cast<int>(ls);
      r = boost::lexical_cast<int>(rs);
    } catch (...) {
      return fmt::format("can`t parse numbers: {} and {}", ls, rs);
    }

    if (r < l) {
      return "right < left number";
    }

    const auto old_size = gpus.size();
    gpus.resize(old_size + r - l + 1);
    std::iota(std::next(gpus.begin(), (long)old_size), gpus.end(), l);
  }

  // make gpus unique
  auto unique_gpus = boost::range::unique(gpus);

  boost::range::copy(unique_gpus, std::back_inserter(result_gpus));

  return "";
}

int main(int argc, char *argv[]) {
  using namespace crypto;

  std::string token;
  std::string url = "server.tonguys.com";
  std::string logLevel = "debug";
  long factor = 64;
  bool showHelp = false;

  auto currentDirectory = std::filesystem::current_path();
  auto miner = currentDirectory / "pow-miner-cuda";
  std::string gpuRange = "[0-0]";

  auto cli = lyra::help(showHelp) |
             lyra::opt(token, "token")["-t"]["--token"](
                 "Your token, get it using bot https://t.me/tonguys_pool_bot")
                 .required() |
             lyra::opt(url, "url")["-u"]["--url"](
                 fmt::format("Server url (default to {})", url))
                 .optional() |
             lyra::opt(logLevel, "logLevel")["-l"]["--level"]("Log level")
                 .optional()
                 .choices("trace", "debug", "info", "err") |
             lyra::opt(miner, "miner")["-m"]["--miner"]("Path to ton miner")
                 .optional() |
             lyra::opt(factor, "factor")["-F"]["--boost-factor"]("Boost factor")
                 .optional() |
             lyra::opt(gpuRange, "gpuRange")["-G"]["--gpu-range"](
                 "Devices range: [0-2,4,7-9] will use #0,#1,#2,#4,#7,#8,#9; "
                 "[0,3] is #0,#3; [0] is #0")
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

  std::vector<int> gpu;
  std::string report = parseGPU(gpu, gpuRange);
  if (!report.empty()) {
    std::cerr << "Error: gpu range parsing error: " << report << std::endl;
    return 1;
  }

  const long long iterations = 100000000000;
  crypto::App app;
  return app.Run(crypto::model::Config(
      model::Token = std::move(token), model::Url = std::move(url),
      model::LogLevel = logLevel, model::MinerPath = std::move(miner),
      model::BoostFactor = factor, model::GPU = std::move(gpu),
      model::Iterations = iterations));
}