#pragma once

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

#define DEFAULT_CFG_FILE    "/home/cfg/mqtt_broker.cfg"
#define DEFAULT_LOG_FILE    "/home/logs/mqtt_broker.log"
#define DEFAULT_PORT        1883
#define CONTROL_SOCKET_NAME "/tmp/9Lq7BNBnBycd6nxy.socket"

#define _1MB_                1048576

enum class cfg_err {
    ok,
    bad_file,
    bad_arg
};

enum class broker_err {
    ok,
    add_error,
    sock_create_err,
    sock_bind_err,
    sock_listen_err,
    read_err,
    mqtt_err,
    connect_err,
    write_err
};

enum broker_states : int {
    init,
    started,
    wait
};

class ServerCfgData{
public:
    std::string log_file_path;
    size_t log_max_size;
    size_t log_max_files;
    int port;
    int level;
    std::string control_socket_path;

    ServerCfgData() : log_file_path(DEFAULT_LOG_FILE), log_max_size(10*_1MB_), log_max_files(5), port(DEFAULT_PORT), level(spdlog::level::info), control_socket_path(CONTROL_SOCKET_NAME) {}

    ServerCfgData(const std::string &_path, const size_t &_log_max_size, const size_t &log_max_files, const int _port, const int _log_level, const std::string& _control_socket_path)
                    : log_file_path(_path), log_max_size(_log_max_size), log_max_files(log_max_files), port(_port), level(_log_level), control_socket_path(_control_socket_path) {

    }
};

ServerCfgData ReadConfig(const char *path, cfg_err &err);
[[noreturn]] void SenderThread(int id);

class Broker : public Commands, public CTopicStorage, public MqttPacketHandler {
private:
    std::shared_mutex clients_mtx;
    unsigned int current_clients;
    std::unordered_multimap<std::string, int> subscribe_data;
    std::map<int, std::shared_ptr<Client>> clients;

    int state;
    int control_sock;
    std::string control_sock_path;
    int port{};
    std::shared_ptr<spdlog::logger> lg;

    Broker();

    broker_err SendCommand(const char *buf, int buf_size);
    broker_err ReadFixedHeader(int fd, FixedHeader &f_hed);
    std::string GetControlPacketTypeName(uint8_t _packet);
    void CloseConnection(int fd);

    int NotifyClients(MqttTopic& topic);
    int NotifyClient(int fd, MqttTopic& topic);

    std::unordered_map<std::string, std::list<std::tuple<uint32_t, std::shared_ptr<uint8_t>, uint16_t>>> postponed_events;

    std::shared_mutex qos_mutex;
    std::thread qos_thread;
    bool qos_thread_started{false};
    bool erase_old_values_in_queue{false};
    bool CheckIfMoreMessages(const std::string& client_id);
    std::pair<uint32_t, std::shared_ptr<uint8_t>> GetPacket(const std::string& client_id, bool &found);

    friend MqttConnectPacketHandler;
    friend MqttPublishPacketHandler;
    friend MqttSubscribePacketHandler;
    friend MqttPubAckPacketHandler;
    friend MqttDisconnectPacketHandler;
    friend MqttPingPacketHandler;
    friend MqttUnsubscribePacketHandler;
    friend MqttPubRelPacketHandler;
    friend MqttPubRecPacketHandler;
    friend MqttPubCompPacketHandler;

public:
    Broker(const Broker& root)          = delete;
    Broker& operator=(const Broker&)    = delete;
    Broker(Broker&& root)               = delete;
    Broker& operator=(Broker&&)         = delete;

    static void ServerThread();
    [[noreturn]] static void SenderThread(int id);
    static void QoSThread();

    static Broker& GetInstance(){
        static Broker instance;
        return instance;
    }

    broker_err AddClient(int sock, const std::string &_ip);
    void DelClient(int sock);
    broker_err InitControlSocket(const std::string& sock_path);

    void AddQosEvent(const std::string& client_id, const std::tuple<uint32_t, std::shared_ptr<uint8_t>, uint16_t>& mqtt_message);
    void DelQosEvent(const std::string& client_id, uint16_t packet_id);
    void SetEraseOldValues(bool val) noexcept;

    uint32_t GetClientCount() noexcept;
    int     GetState() const noexcept;

    void    SetState(int _state) noexcept;
    void    SetPort(int _port) noexcept;
    void    InitLogger(const std::string & _path, size_t  _size, size_t _max_files, int _level);

    void Start();
};

int HandleMqttConnect(std::shared_ptr<Client>& pClient, const std::shared_ptr<uint8_t>& buf, std::shared_ptr<spdlog::logger>& lg);

int HandleMqttPublish(const FixedHeader &fh, const std::shared_ptr<uint8_t>& buf, std::shared_ptr<spdlog::logger>& lg, PublishVH &vh, std::shared_ptr<MqttBinaryDataEntity> &message);

int HandleMqttSubscribe(std::shared_ptr<Client>& pClient, const FixedHeader &fh, const std::shared_ptr<uint8_t>& buf, std::shared_ptr<spdlog::logger>& lg,
                        SubscribeVH &vh, std::vector<uint8_t> &_reason_codes, std::list<std::pair<std::string, uint8_t>>& subscribe_topics);

int HandleMqttPuback(const std::shared_ptr<uint8_t>& buf, std::shared_ptr<spdlog::logger>& lg, PubackVH&  p_vh);

int HandleMqttUnsubscribe(std::shared_ptr<Client>& pClient, const std::shared_ptr<uint8_t>& buf, const FixedHeader &fh,
                          std::shared_ptr<spdlog::logger>& lg, UnsubscribeVH&  p_vh, std::list<std::string> &topics_to_unsubscribe);

int HandleMqttPubrel(const std::shared_ptr<uint8_t>& buf, std::shared_ptr<spdlog::logger>& lg, TypicalVH&  t_vh);
