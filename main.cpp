#include <iostream>
#include "version.h"
#include <vector>
#include <map>
#include<list>

#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

int main() {
    auto max_size = 1048576 * 10; // 10 Mb
    auto max_files = 5;
    auto logger = spdlog::rotating_logger_mt("main", "mqtt_broker.log", max_size, max_files);

    logger->info("START BROKER");

    return EXIT_FAILURE;
}
