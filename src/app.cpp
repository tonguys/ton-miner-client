#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

#include "app.hpp"

#include "client.hpp"
#include "executor.hpp"
#include "httpClient.hpp"
#include "mockClient.hpp"

#include "boost/filesystem.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace crypto {

void printStatistic(std::optional<model::Statistic> st_opt) {
  try {
    static int count = 0;
    if (!st_opt) {
      spdlog::warn("No statistic parsed");
      return;
    }

    count++;
    auto st = st_opt.value();
    st.count = count;
    nlohmann::json j = st;
    spdlog::info("JSON STATISTIC: {}", j.dump());
  } catch (...) {
    spdlog::warn("Cant print statistic");
  }
}

void configureLogger(const model::Config &cfg) {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      cfg.logPath.c_str(), 1024 * 1024, 10, false));

  auto log = std::make_shared<spdlog::logger>("client", std::begin(sinks),
                                              std::end(sinks));
  std::string jsonpattern = {
      "{\"time\": \"%Y-%m-%dT%H:%M:%S.%f%z\", \"name\": \"%n\", \"level\": "
      "\"%^%l%$\", \"thread\": %t, \"message\": \"%v\"}"};
  log->set_pattern(jsonpattern);

  spdlog::register_logger(log);
  spdlog::set_default_logger(log);
}

int App::Run(const model::Config &cfg) {
  if (running.load()) {
    spdlog::critical("Starting already running App");
    throw std::runtime_error("Already started");
  }
  running.store(true);

  configureLogger(cfg);
  spdlog::info("Starting with {}", cfg);

  this->exec = std::make_unique<Executor>(cfg);

  std::unique_ptr<Client> client =
      std::make_unique<HTTPClient>(cfg.url, cfg.token);

  auto auth = client->Register();
  if (!auth) {
    spdlog::critical("Registration failed, inspect logs for details");
    return 1;
  }
  spdlog::info("Registered with {}", auth.value());

  std::optional<crypto::model::Task> task;
  while (running.load()) {
    spdlog::debug("Request new task");
    task = client->GetTask();
    if (!task) {
      spdlog::critical(
          "Can`t get new task from server, inspect logs for details");
      return 1;
    }
    spdlog::debug("Got task: {}", task.value());

    auto minerTask = model::MinerTask(cfg.iterations, task.value(), cfg.gpu);
    spdlog::debug("Starting miner with task: {}", Dump(minerTask));

    auto res = exec->Run(minerTask);
    exec->Stop();
    if (res) {
      spdlog::debug("Found answer: {}", Dump(res.value()));
      model::Answer answer = res->answer;
      printStatistic(answer.statistic);

      spdlog::debug("Sending answer");
      auto status = client->SendAnswer(answer);
      if (!status) {
        spdlog::critical("Cant send answer, inspect logs for details");
        return 1;
      }
      spdlog::info("Result: {}", status.value());
    }
  }
  return 0;
}

void App::Stop() {
  if (!running.load()) {
    return;
  }
  exec->Stop();
  running.store(false);
}

} // namespace crypto