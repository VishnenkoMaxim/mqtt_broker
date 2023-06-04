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

ServerCfgData ReadConfig(const char *cfg_path, int &err){
    Config cfg;

    try {
        cfg.readFile(cfg_path);
    } catch(const FileIOException &fioex){
        std::cerr << "I/O error while reading file." << std::endl;
        err = cfg_err::bad_file;
        return ServerCfgData{};
    }

    const Setting &root = cfg.getRoot();
    const Setting &log_cfg = root["log_file"];
    const Setting &broker_cfg = root["broker"];

    string path;
    int size(0);
    int max_file(0);
    int log_level;
    int port;

    if (!log_cfg.lookupValue("path", path)) {
        std::cerr << "path error" << std::endl;
        err = cfg_err::bad_arg;
        return ServerCfgData{};
    }

    if (!log_cfg.lookupValue("size", size)) {
        std::cerr << "Size arg error. Set default" << std::endl;
        size = 10*_1MB;
    }

    if (!log_cfg.lookupValue("max_files", max_file)) {
        std::cerr << "max_files arg error. Set default." << std::endl;
        max_file = 3;
    }

    if (!broker_cfg.lookupValue("port", port)) {
        std::cerr << "port arg error. Set default (" << DEFAULT_PORT << ")" << std::endl;
        port = DEFAULT_PORT;
    }

    if (!log_cfg.lookupValue("level", log_level)) {
        std::cerr << "level arg error. Set default." << std::endl;
        log_level = level::level_enum::info;
    }

    err = cfg_err::ok;
    return (ServerCfgData{path, size, max_file, port, log_level});
}

void SetLogLevel(const shared_ptr<logger>& lg, int _level) noexcept {
    switch(_level){
        case 0 : {lg->set_level(spdlog::level::trace);} break;
        case 1 : {lg->set_level(spdlog::level::debug);} break;
        case 2 : {lg->set_level(spdlog::level::info);} break;
        case 3 : {lg->set_level(spdlog::level::warn);} break;
        case 4 : {lg->set_level(spdlog::level::err);} break;
        case 5 : {lg->set_level(spdlog::level::critical);} break;
        case 6 : {lg->set_level(spdlog::level::off);} break;
        default : {lg->set_level(spdlog::level::n_levels);}
    }
}

int WriteData(int fd, uint8_t* data, unsigned int size) {
    int res = 0;
    unsigned int sent_bytes = 0;
    while (sent_bytes < size) {
        res = write(fd, data + sent_bytes, size - sent_bytes);
        if (res <= 0) return -1;
        sent_bytes += res;
    }
    return sent_bytes;
}

int ReadData(int fd, uint8_t* data, int size, unsigned int timeout){
    int co = 0;
    int rval = -1;
    struct timeval tv;
    fd_set rfd;

    if( fd < 0 || !data || size <= 0 ) return 0;
    for(co = 0; ;){
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        FD_ZERO(&rfd);
        FD_SET( fd, &rfd );
        //usleep(10);
        rval = select(fd + 1, &rfd, nullptr, nullptr, &tv);
        if( rval < 0 ) return -1;
        if (rval == 0) return co;

        rval = read(fd, data + co, size - co );
        if(rval <= 0) return co;

        co+=rval;
        if( co >= size ) break;
    }
    return co;
}

uint16_t ConvertToHost2Bytes(const uint8_t* buf){
    uint16_t val;
    memcpy(&val, buf, sizeof(val));
    return ntohs(val);
}

uint32_t ConvertToHost4Bytes(const uint8_t* buf){
    uint32_t val;
    memcpy(&val, buf, sizeof(val));
    return ntohl(val);
}

string GenRandom(const uint8_t len) {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return tmp_s;
}

template <class Type>
ostream& operator << (ostream &os, const vector<Type> &_vec){
    for(auto &it : _vec){
        os << it << " ";
    }
    os << endl;
    return os;
}