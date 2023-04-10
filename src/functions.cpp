//
// Created by Vishnenko Maxim on 06.04.2023.
//
#include <iostream>
#include "mqtt_broker.h"

using namespace std;

template <typename T>
void PrintType [[maybe_unused]](T) {
    cout << __PRETTY_FUNCTION__ << endl;
}

ServerCfgData ReadConfig(int &err){
    Config cfg;

    try {
        cfg.readFile(DEFAULT_CFG_FILE);
    } catch(const FileIOException &fioex){
        std::cerr << "I/O error while reading file." << std::endl;
        err = ERR_BAD_CONFIG;
        return ServerCfgData();
    }

    const Setting &root = cfg.getRoot();
    const Setting &log_cfg = root["log_file"];

    string path;
    int size(0);
    int max_file(0);

    log_cfg.lookupValue("path", path);
    log_cfg.lookupValue("size", size);
    log_cfg.lookupValue("max_files", max_file);

    err = ERR_OK;
    return (ServerCfgData(path, size, max_file));
}
