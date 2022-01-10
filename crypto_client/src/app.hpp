
#include <atomic>
#include <memory>
#include <string>

#include "executor.hpp"
#include "models.hpp"

#ifndef APP_HPP
#define APP_HPP

namespace crypto {

class App {
private:
  std::unique_ptr<Executor> exec;
  std::atomic_bool running;

public:
  int Run(const model::Config &cfg);
  void Stop();
};

} // namespace crypto

#endif