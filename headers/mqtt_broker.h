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
#include <thread>
#include <chrono>
#include <unordered_set>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"

#include "functions.h"
#include "mqtt_protocol.h"
#include "client.h"
#include "command.h"
#include "topic_storage.h"
#include "MqttPacketHandler.h"

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
        for(const auto& it : topics_to_pub){
            cout << "topic: " << it.GetString() << endl;
        }
    }
};

void SenderThread(int id);

class Broker : public Commands, public CTopicStorage, public MqttPacketHandler {
private:
    shared_mutex clients_mtx;
    unsigned int current_clients;
    unordered_multimap<string, int> subscribe_data;
    map<int, shared_ptr<Client>> clients;

    int state;
    int control_sock;
    int port;
    shared_ptr<logger> lg;

    Broker();

    Broker(const Broker& root)          = delete;
    Broker& operator=(const Broker&)    = delete;
    Broker(Broker&& root)               = delete;
    Broker& operator=(Broker&&)         = delete;

    int SendCommand(const char *buf, int buf_size);
    int ReadFixedHeader(int fd, FixedHeader &f_hed);
    string GetControlPacketTypeName(uint8_t _packet);
    void CloseConnection(int fd);

    int NotifyClients(MqttTopic& topic);
    int NotifyClient(int fd, MqttTopic& topic);

    unordered_multimap<string, MqttTopic> QoS_events;
    //unordered_map<string, queue<MqttTopic>> QoS_events;

    shared_mutex qos_mutex;
    bool CheckTopicPresence(const string& client_id, const MqttTopic& topic);
    thread qos_thread;
    bool qos_thread_started{false};
    bool erase_old_values_in_queue{false};
    bool CheckIfMoreMessages(const string& client_id);
    MqttTopic GetKeptTopic(const string& client_id, bool &found);


    friend MqttConnectPacketHandler;
    friend MqttPublishPacketHandler;
    friend MqttSubscribePacketHandler;
    friend MqttPubAckPacketHandler;
    friend MqttDisconnectPacketHandler;
    friend MqttPingPacketHandler;
    friend MqttUnsubscribePacketHandler;

public:
    friend void ServerThread ();
    friend void SenderThread(int id);
    friend void QoSThread();

    static Broker& GetInstance(){
        static Broker instance;
        return instance;
    }

    int AddClient(int sock, const string &_ip);
    void DelClient(int sock);
    int InitControlSocket();

    void AddQosEvent(const string& client_id, const MqttTopic& mqtt_message);
    void DelQosEvent(const string& client_id, uint16_t packet_id);
    void DelClientQosEvents(const string& client_id);
    void SetEraseOldValues(const bool val) noexcept;

    uint32_t GetClientCount() noexcept;
    int     GetState() noexcept;

    void    SetState(int _state) noexcept;
    void    SetPort(int _port) noexcept;
    void    InitLogger(const string & _path, size_t  _size, size_t _max_files, int _level);

    void Start();
};

int HandleMqttConnect(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg);
int HandleMqttPublish(const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PublishVH &vh, shared_ptr<MqttBinaryDataEntity> &message);
int HandleMqttSubscribe(shared_ptr<Client>& pClient, const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg,
                        SubscribeVH &vh, vector<uint8_t> &_reason_codes, list<pair<string, uint8_t>>& subscribe_topics);
int HandleMqttPuback(const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PubackVH&  p_vh);
int HandleMqttUnsubscribe(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, const FixedHeader &fh,
                          shared_ptr<logger>& lg, UnsubscribeVH&  p_vh, list<string> &topics_to_unsubscribe);


#endif //MQTT_BROKER_H
