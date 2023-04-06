#include <iostream>
#include "version.h"
#include <vector>
#include <map>
#include<list>

#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

//Разобраться с этой херней
//#include "src/functions.cpp"

int main() {
    auto max_size = 1048576 * 5; // 5 Mb
    auto max_files = 5;
    auto logger = spdlog::rotating_logger_mt("mqtt_broker_logger", "mqtt_broker.log", max_size, max_files);

    spdlog::info("START BROKER");

    return EXIT_FAILURE;
}
