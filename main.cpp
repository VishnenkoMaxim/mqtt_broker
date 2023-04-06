#include <iostream>
#include "version.h"

#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"


int main() {
    Config cfg;

    try {
        cfg.readFile(DEFAULT_CFG_FILE);
    } catch(const FileIOException &fioex){
        std::cerr << "I/O error while reading file." << std::endl;
        return(EXIT_FAILURE);
    }

    const Setting &root = cfg.getRoot();
    const Setting &log_cfg = root["log_file"];

    string path;
    size_t size(0);
    size_t max_file(0);

    log_cfg.lookupValue("path", path);
    log_cfg.lookupValue("size", size);
    log_cfg.lookupValue("max_files", max_file);
    ServerCfgData cfg_data(path, size, max_file);

    cout << cfg_data.log_file_path << " " << cfg_data.log_max_size << " " << cfg_data.log_max_files << endl;

    auto logger = spdlog::rotating_logger_mt("main", cfg_data.log_file_path, cfg_data.log_max_size, cfg_data.log_max_files);
    logger->info("START BROKER");

    return EXIT_FAILURE;
}
