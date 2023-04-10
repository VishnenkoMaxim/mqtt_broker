//
// Created by Vishnenko Maxim on 05.04.2023.
//

#ifndef MQTT_BROKER_H
#define MQTT_BROKER_H

#include "functions.h"
#include <libconfig.h++>
#include <string>
#include <vector>
#include <map>
#include <list>

using namespace std;
using namespace libconfig;

#define ERR_OK                  0
#define ERR_BAD_CONFIG          -1

#define DEFAULT_CFG_FILE "/home/cfg/mqtt_broker.cfg"
#define _1MB                1048576

class ServerCfgData{
public:
    const string log_file_path;
    const size_t log_max_size;
    const size_t log_max_files;

    ServerCfgData() : log_file_path("/home/logs/mqtt_broker.log"), log_max_size(10*_1MB), log_max_files(5) {}

    ServerCfgData(const string &_path, const size_t &_log_max_size, const size_t &log_max_files)
                    : log_file_path(_path), log_max_size(_log_max_size), log_max_files(log_max_files) {

    }
};

ServerCfgData ReadConfig(int &err);

#endif //MQTT_BROKER_H
