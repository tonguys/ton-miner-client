#include "app.hpp"
#include "models.hpp"

#include "boost/lexical_cast.hpp"
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

const int gpuInvalid = -1;

std::string parseGPU(std::vector<int> &gpu, const int gpuSingle,
                     std::string_view gpuRange) {
  // TODO: possible to make better?
  bool single = gpuSingle != gpuInvalid;
  bool range = !gpuRange.empty();

  if (single && range) {
    return "you can`t set -g and -G both";
  }

  if (single) {
    gpu = std::vector<int>{gpuSingle};
    return "";
  }

  // Yes, this stupid boilerplate code is needed just to parse
  // two numbers and a dash in square brackets (like [1-4]).
  // Yes, it`s dumb, as soon, as we could make one simple regex.
  // But, first of all, regex is always a technical debt, learn to live without
  // them. Secondly, I want to make user friendly cli with user friendly error
  // messages. There are always a donkey: user or developer.
  if (range) {
    if (gpuRange.empty()) {
      return "empty range";
    }
    if (gpuRange[0] != '[') {
      return "invalid format: not [ on first place";
    }
    if (gpuRange.back() != ']') {
      return "invalid format: not ] on the last place";
    }

    // Remove brackets, now we are working with range, not gpuRange
    auto range = gpuRange.substr(1, gpuRange.size() - 2);

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

    if (r <= l) {
      return "right <= left number";
    }

    gpu.resize(r - l + 1);
    std::iota(gpu.begin(), gpu.end(), l);

    return "";
  }

  gpu = std::vector<int>{0};

  return "";
}

int main(int argc, char *argv[]) {
  using namespace crypto;

  std::string token;
  std::string url = "test-server1.tonguys.com";
  std::string logLevel = "debug";
  long factor = 1;
  bool showHelp = false;

  auto currentDirectory = std::filesystem::current_path();
  auto miner = currentDirectory / "pow-miner-cuda";

  int gpuSingle = gpuInvalid;
  std::string gpuRange;

  auto cli = lyra::help(showHelp) |
             lyra::opt(token, "token")["-t"]["--token"](
                 "Your token, get it using bot @pooltonbot")
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
             lyra::opt(gpuSingle,
                       "gpu")["-g"]["--gpu"]("Device numbre (defaults to #0)")
                 .optional() |
             lyra::opt(gpuRange, "gpuRange")["-G"]["--gpu-range"](
                 "Devices range: [0-2] will use #0,#1,#2")
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
  std::string report = parseGPU(gpu, gpuSingle, gpuRange);
  if (!report.empty()) {
    std::cerr << "Error: gpu range parsing error: " << report << std::endl;
    return 1;
  }

  const long long iterations = 100000000000;
  return crypto::run(crypto::model::Config(
      model::Token = std::move(token), model::Url = std::move(url),
      model::LogLevel = logLevel, model::MinerPath = std::move(miner),
      model::BoostFactor = factor, model::GPU = std::move(gpu),
      model::Iterations = iterations));
}