#ifndef MQTT_BROKER_H
#define MQTT_BROKER_H

#include <libconfig.h++>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <memory>
#include <pthread.h>
#include <poll.h>
#include <sys/un.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"

#include "functions.h"
#include "mqtt_protocol.h"
#include "client.h"
#include "command.h"

using namespace std;
using namespace libconfig;
using namespace spdlog;
using namespace temp_funcs;
using namespace mqtt_protocol;

#define DEFAULT_CFG_FILE    "/home/cfg/mqtt_broker.cfg"
#define DEFAULT_LOG_FILE    "/home/logs/mqtt_broker.log"
#define DEFAULT_PORT        1883
#define CONTROL_SOCKET_NAME "/tmp/9Lq7BNBnBycd6nxy.socket"

#define _1MB                1048576

namespace cfg_err{
    enum cfg_err_enum : int {
        ok,
        bad_file,
        bad_arg
    };
}

namespace broker_err{
    enum broker_err_enum : int {
        ok,
        add_error,
        sock_create_err,
        sock_bind_err,
        sock_listen_err,
        read_err,
        mqtt_err
    };
}

namespace broker_states{
    enum broker_states_enum : int {
        init,
        started,
        wait
    };
}

class ServerCfgData{
public:
    string log_file_path;
    size_t log_max_size;
    size_t log_max_files;
    int port;
    int level;

    ServerCfgData() : log_file_path(DEFAULT_LOG_FILE), log_max_size(10*_1MB), log_max_files(5), port(DEFAULT_PORT), level(spdlog::level::info) {}

    ServerCfgData(const string &_path, const size_t &_log_max_size, const size_t &log_max_files, const int _port, const int _log_level)
                    : log_file_path(_path), log_max_size(_log_max_size), log_max_files(log_max_files), port(_port), level(_log_level) {

    }
};

ServerCfgData ReadConfig(const char *path, int &err);

class Publisher{
private:
    list<MqttTopic> topics_to_pub;

public:
    void Add(const MqttTopic &topic){
        topics_to_pub.push_back(topic);
    }

    void ShowTopics(){
        for(auto it : topics_to_pub){
            cout << "topic: " << it.GetString() << endl;
        }
    }
};

[[noreturn]] void SenderThread();

class Broker : public Commands {
private:
    pthread_mutex_t clients_mtx;
    unsigned int current_clients;
    map<int, shared_ptr<Client>> clients;
    unordered_multimap<string, int> subscribe_data;

    int state;
    int control_sock;
    pthread_t server_tid;
    int port;
    shared_ptr<logger> lg;

    Broker() : Commands(), current_clients(0), state(0), control_sock(-1) {};

    Broker(const Broker& root)          = delete;
    Broker& operator=(const Broker&)    = delete;
    Broker(Broker&& root)               = delete;
    Broker& operator=(Broker&&)         = delete;

    int SendCommand(const char *buf, int buf_size);
    int ReadFixedHeader(int fd, FixedHeader &f_hed);
    string GetControlPacketTypeName(uint8_t _packet);
    void CloseConnection(int fd);

    int NotifyClients(MqttStringEntity &topic_name, MqttBinaryDataEntity &_message);
public:
    friend void* ServerThread (void *arg);
    friend void SenderThread();

    static Broker& GetInstance(){
        static Broker instance;
        return instance;
    }

    int AddClient(int sock, const string &_ip);
    void DelClient(int sock);
    int InitControlSocket();

    uint32_t GetClientCount() noexcept;
    int     GetState()noexcept;

    void    SetState(int _state) noexcept;
    void    SetPort(int _port) noexcept;
    void    InitLogger(const string & _path, size_t  _size, size_t _max_files, size_t _level);

    void Start();
};

int HandleMqttConnect(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg);
int HandleMqttPublish(const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PublishVH &vh, MqttBinaryDataEntity &message);
int HandleMqttSubscribe(shared_ptr<Client>& pClient, const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, SubscribeVH &vh, vector<uint8_t> &_reason_codes);


#endif //MQTT_BROKER_H
