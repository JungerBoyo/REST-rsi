#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    argv[1][0] = 'a';
    spdlog::info("Hello {}", argv[1]);
}

