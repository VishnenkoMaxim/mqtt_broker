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

template <class T>
struct PoolData{
    shared_ptr<T> pool;
    int32_t used_elements;

    PoolData() = delete;

    PoolData(unsigned int num_elements) : used_elements(0) {
        pool.reset(static_cast<T*>(::operator new(num_elements*sizeof(T))));
    }
};

template <class T, int num>
class PoolAllocator{
private:
    list<PoolData<T>> pools;
    size_t pool_size;
    T* cur_pointer = nullptr;

public:
    using value_type = T;
    void AllocateNewPool(){
        PoolData<T> pd(pool_size);
        pools.push_back(pd);
        cur_pointer = static_cast<T*>(pools.back().pool.get());
    }

    PoolAllocator() noexcept : pool_size(num) {
        //
    };

    template <class U> PoolAllocator (const PoolAllocator<U, num>&) noexcept {}

    T* allocate (size_t n){
        if (n > pool_size) throw bad_alloc();
        if ((pools.size() == 0) || (pools.back().used_elements + n > num)) AllocateNewPool();

        pools.back().used_elements += n;
        return static_cast<T*>(::operator new(n, cur_pointer + pools.back().used_elements - n));
    }

    void deallocate (T* p, size_t n) {
        if (n == 0) return;
        if (p == nullptr) return;

        for(auto it=pools.begin(); it != pools.end(); it++){
            if (it->pool.get() <= p && it->pool.get()+pool_size > p) {
                it->used_elements -= n;
                if (it->used_elements == 0) {
                    it->pool.reset();
                    pools.erase(it);
                    if (pools.size()) {
                        cur_pointer = static_cast<T*>(pools.back().pool.get());
                    }
                    break;
                }
            }
        }
    }

    template<class U>
    struct rebind {
        typedef PoolAllocator<U, num> other;
    };

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type; //UB if std::false_type and a1 != a2;
};

class Broker{
private:
    pthread_mutex_t clients_mtx;
    unsigned int current_clients;
    map<int, Client, less<>, PoolAllocator<pair<const int, Client>, 10>> clients;
    int state;
    int control_sock;
    pthread_t server_tid;
    unique_ptr<struct pollfd> fds;
    int port;
    shared_ptr<logger> lg;

    Broker() : current_clients(0), state(0), control_sock(-1) {};

    Broker(const Broker& root)          = delete;
    Broker& operator=(const Broker&)    = delete;
    Broker(Broker&& root)               = delete;
    Broker& operator=(Broker&&)         = delete;

    int SendCommand(const char *buf, int buf_size);
    int ReadFixedHeader(int fd, FixedHeader &f_hed);
    string GetControlPacketTypeName(const uint8_t _packet);
    void CloseConnection(int fd);

    //void ParseMqttPacket(shared_ptr<uint8_t> data, uint32_t data_size);
    //int ReadVariableHeader(int fd, ConnectVH &con_vh);
public:
    friend void* ServerThread (void *arg);

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

#endif //MQTT_BROKER_H
