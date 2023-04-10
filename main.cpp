#include <iostream>
#include "version.h"

#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

int main() {
    int err = 0;
    ServerCfgData cfg_data = ReadConfig(err);

    auto logger = spdlog::rotating_logger_mt("main", cfg_data.log_file_path, cfg_data.log_max_size, cfg_data.log_max_files);
    logger->info("START BROKER");

    logger->info("logger size:{} Kb, max_files:{}", cfg_data.log_max_size/1024, cfg_data.log_max_files);

    return EXIT_FAILURE;
}
