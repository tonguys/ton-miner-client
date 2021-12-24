#include "src/app.hpp"

int main() {
    std::string token = "9cae7663b25e9f91e989cb250a9174a3998e2bffd76d23df";
    crypto::run(std::move(token));
}