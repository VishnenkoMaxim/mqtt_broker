
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#include "mqtt_broker.h"

vector<string> pack_type_names{"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
                               "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT", "AUTH"};

void* ServerThread([[maybe_unused]] void *arg){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start ServerThread");
    while(true){
        switch (broker.state){
            case broker_states::started : {
                broker.lg->debug("state started");
                uint32_t client_num = broker.GetClientCount();
                if (client_num == 0){
                    broker.SetState(broker_states::init);
                    broker.lg->info("Stop ServerThread");
                    return nullptr;
                }
                client_num++; // for control socket
                broker.fds.reset(static_cast<struct pollfd*>(::operator new(client_num*sizeof(struct pollfd))));
                int i=0;
                for(auto const &it : broker.clients){
                    struct pollfd tmp_fd;
                    tmp_fd.fd = it.first;
                    tmp_fd.events = POLLIN;
                    tmp_fd.revents = 0;
                    memcpy(broker.fds.get() + i, &tmp_fd, sizeof(tmp_fd));
                    i++;
                }
                struct pollfd tmp_fd_control;
                tmp_fd_control.fd = broker.control_sock;
                tmp_fd_control.events = POLLIN;
                tmp_fd_control.revents = 0;
                memcpy(broker.fds.get() + i, &tmp_fd_control, sizeof(tmp_fd_control));
                broker.SetState(broker_states::wait);
                broker.lg->flush();
            }; break;

            case broker_states::wait : {
                broker.lg->debug("state wait");
                int ready;
                uint32_t client_num = broker.GetClientCount();
                broker.lg->debug("Waiting for the events..."); broker.lg->flush();
                client_num++;
                ready = poll(broker.fds.get(), client_num, -1);
                if (ready == -1){
                    broker.lg->critical("poll error");
                    sleep(1);
                    broker.SetState(broker_states::started);
                }
                broker.lg->debug("Have data");
                for(unsigned int i=0; i<client_num; i++) {
                    if (broker.fds.get()[i].revents != 0) {
                        broker.lg->debug("Event: fd:{} events:{}{}{}", broker.fds.get()[i].fd,
                                  (broker.fds.get()[i].revents & POLLIN) ? "POLLIN " : "",
                                  (broker.fds.get()[i].revents & POLLHUP) ? "POLLHUP " : "",
                                  (broker.fds.get()[i].revents & POLLERR) ? "POLLERR " : "");

                        if (broker.fds.get()[i].revents & POLLIN) {
                            broker.fds.get()[i].revents = 0;
                            if (broker.fds.get()[i].fd == broker.control_sock){
                                broker.lg->debug("Got control command");
                                int data_socket = accept(broker.control_sock, NULL, NULL);
                                if (data_socket != -1) {
                                    char c_buf[16] = "";
                                    int ret = read(data_socket, c_buf, sizeof(c_buf));
                                    if (ret > 0) {
                                        if (!strncmp(c_buf, "ADD", sizeof(c_buf))) {
                                            broker.lg->debug("add new client, reinit fds");
                                            broker.fds.reset();
                                            broker.SetState(broker_states::started);
                                            close(data_socket);
                                            break;
                                        } else {
                                            broker.lg->warn("Ignore command. Unknown command in command socket.");
                                        }
                                    } else broker.lg->error("data_socket read error");
                                    close(data_socket);
                                } else broker.lg->error("control socket accept error");
                            } else {
                                FixedHeader f_head;
                                int ret = broker.ReadFixedHeader(broker.fds.get()[i].fd, f_head);
                                if (ret == broker_err::ok){
                                    broker.lg->debug("action:{}", broker.GetControlPacketTypeName(f_head.GetType()));
                                    shared_ptr<uint8_t> buf(new uint8_t[f_head.remaining_len], default_delete<uint8_t[]>());
                                    ret = ReadData(broker.fds.get()[i].fd, buf.get(), f_head.remaining_len, 0);
                                    broker.lg->debug("remaining_len:{}", f_head.remaining_len);
                                    if (ret != (int) f_head.remaining_len){
                                        broker.lg->error("Read error. Read {} bytes instead of {}", ret, f_head.remaining_len);
                                        broker.CloseConnection(broker.fds.get()[i].fd);
                                        broker.fds.reset();
                                        broker.SetState(broker_states::started);
                                        break;
                                    }
                                    switch (f_head.GetType()){
                                        case mqtt_pack_type::CONNECT: {
                                            int fd = broker.fds.get()[i].fd;
                                            shared_ptr<Client> pClient = broker.clients[fd];

                                            ConnectVH con_vh;
                                            uint32_t offset = 0;

                                            con_vh.CopyFromNet(buf.get());
                                            offset += sizeof(con_vh);
                                            char name_tmp[5] = "";
                                            memcpy(name_tmp, con_vh.name, 4);
                                            broker.lg->debug(
                                                    "Connect VH: len:{} name:{} version:{} flags:{:X} alive:{}",
                                                    con_vh.prot_name_len, name_tmp, con_vh.version, con_vh.conn_flags,
                                                    con_vh.alive);

                                            pClient->SetConnAlive(con_vh.alive);
                                            pClient->SetConnFlags(con_vh.conn_flags);
                                            //read properties
                                            int properties_len = 0;
                                            uint8_t size = 0;

                                            uint8_t stat = DeCodeVarInt(buf.get() + offset, properties_len, size);
                                            if (stat != mqtt_err::ok) {
                                                broker.lg->error("Read DeCodeVarInt error");
                                                broker.CloseConnection(broker.fds.get()[i].fd);
                                                broker.fds.reset();
                                                broker.SetState(broker_states::started);
                                                break;
                                            }
                                            broker.lg->debug("properties len:{}", properties_len);
                                            offset += size;
                                            uint32_t p_len = properties_len;
                                            while (p_len > 0) {
                                                uint8_t size_property;
                                                auto property = CreateProperty(buf.get() + offset, size_property);
                                                pClient->conn_properties.AddProperty(property);
                                                p_len -= size_property;
                                                offset += size_property;
                                            }
                                            //read ClientID
                                            uint8_t id_len;
                                            shared_ptr<MqttStringEntity> id = CreateMqttStringEntity(buf.get() + offset, id_len);
                                            if (id != nullptr){
                                                pClient->SetID(id->GetString());
                                            } else broker.lg->info("No ClientID provided");
                                        }; break;

                                        default : {
                                            broker.lg->warn("Unsupported mqtt packet. Ignore it.");
                                        }
                                    }
                                } else {
                                    broker.CloseConnection(broker.fds.get()[i].fd);
                                    broker.fds.reset();
                                    broker.SetState(broker_states::started);
                                    break;
                                }
                            }
                        } else {
                            broker.lg->debug("client closed socket fd:{}\n", broker.fds.get()[i].fd);
                            broker.CloseConnection(broker.fds.get()[i].fd);
                            broker.fds.reset();
                            broker.SetState(broker_states::started);
                            break;
                        }
                    }
                }
                broker.lg->flush();
            }; break;
        }
    }
    return nullptr;
}

int Broker::AddClient(int sock, const string &_ip){
    current_clients++;
    shared_ptr<Client> new_client = std::make_shared<Client>(_ip);
    pthread_mutex_lock(&clients_mtx);
    auto ret = clients.insert(make_pair(sock, new_client));
    pthread_mutex_unlock(&clients_mtx);
    if (ret.second == true) {
        if (state == broker_states::wait) SendCommand("ADD", 3);
        return broker_err::ok;
    } else return broker_err::add_error;
}

void Broker::DelClient(int sock){
    lg->debug("DelClient");
    current_clients--;
    pthread_mutex_lock(&clients_mtx);
    clients.erase(sock);
    pthread_mutex_unlock(&clients_mtx);
}

uint32_t Broker::GetClientCount() noexcept {
    return clients.size();
}

int Broker::GetState() noexcept {
    return state;
}

void Broker::Start() {
    if (state == broker_states::init) {
        int ret = pthread_create(&server_tid, nullptr, &ServerThread, nullptr);
        if (ret != 0){
            lg->critical("Couldn't start thread");
            return;
        }
        pthread_detach(server_tid);
        state = broker_states::started;
        lg->debug("Broker has started {}", server_tid);
    } else {
        lg->warn("Broker has already started!");
    }
    lg->flush();
}

void Broker::SetState(int _state) noexcept {
    state = _state;
}

int Broker::InitControlSocket() {
    struct sockaddr_un serv_addr;
    unlink(CONTROL_SOCKET_NAME);
    control_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (control_sock < 0) {
        lg->error("Error opening control socket: {}", strerror(errno));
        return broker_err::sock_create_err;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family  = AF_UNIX;
    strncpy(serv_addr.sun_path, CONTROL_SOCKET_NAME, sizeof(serv_addr.sun_path) - 1);
    if (bind(control_sock, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        lg->error("Error binding control socket: {}", strerror(errno));
        return broker_err::sock_bind_err;
    }
    if (listen(control_sock, 10) < 0){
        lg->error("Error listening control socket: {}", strerror(errno));
        return broker_err::sock_listen_err;
    }
    return broker_err::ok;
}

void Broker::SetPort(int _port) noexcept {
    port = _port;
}

int Broker::SendCommand(const char *buf, const int buf_size) {
    struct sockaddr_un addr;
    int ret;
    int data_socket;

    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        lg->error("Error opening socket: {}", strerror(errno));
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CONTROL_SOCKET_NAME, sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *) &addr,sizeof(addr));
    if (ret == -1) {
        lg->error("Control socket is down: {}", strerror(errno));
        return -1;
    }
    ret = write(data_socket, buf, buf_size);
    if (ret == -1) {
        lg->error("write error: {}", strerror(errno));
        close(data_socket);
        return -2;
    }
    lg->debug("sent control command: {}", buf);
    close(data_socket);
    return broker_err::ok;
}

void    Broker::InitLogger(const string & _path, const size_t  _size, const size_t _max_files, const size_t _level){
    lg = spdlog::rotating_logger_mt("broker", _path, _size, _max_files);
    SetLogLevel(lg, _level);
}

int Broker::ReadFixedHeader(const int fd, FixedHeader &f_hed){
    size_t ret = read(fd, &f_hed.first, 1);
    if (ret != 1) {
        lg->error("read error");
        return broker_err::read_err;
    }
    ret = ReadVariableInt(fd, reinterpret_cast<int &>(f_hed.remaining_len));
    if (ret != mqtt_err::ok){
        lg->error("read variable value error");
        if (ret == mqtt_err::read_err) return broker_err::read_err;
        else                           return broker_err::mqtt_err;
    }
    lg->debug("Fixed header type:0x{0:X} len:{1}", f_hed.first, f_hed.remaining_len);
    lg->debug("DUP:{} QoS:{} RETAIN:{}", f_hed.isDUP(), f_hed.QoS(), f_hed.isRETAIN());
    return broker_err::ok;
}

string Broker::GetControlPacketTypeName(const uint8_t _packet){
    return pack_type_names[_packet];
}

void Broker::CloseConnection(int fd){
    lg->debug("Close connection");
    close(fd);
    DelClient(fd);
}

