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
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

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

int App::Run(const model::Config &cfg) {
  spdlog::set_level(cfg.logLevel);
  if (running.load()) {
    spdlog::critical("Starting already running App");
    throw std::runtime_error("Already started");
  }
  spdlog::info("Starting with {}", cfg);
  running.store(true);

  this->exec = std::make_unique<Executor>(cfg);

  std::unique_ptr<Client> client =
      std::make_unique<mock::MockClient>(cfg.url, cfg.token);

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