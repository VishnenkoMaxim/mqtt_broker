
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#include "mqtt_broker.h"

vector<string> pack_type_names{"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
                               "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT", "AUTH"};

//Publisher publisher;

void SenderThread(int id){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start Sender Thread {}", id);

    while(true){
        broker.Execute();
        broker.lg->debug("Sender Thread {} executed", id);
    }
}

void QoSThread(){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start QoSThread{}");

    shared_lock<shared_mutex> lock(broker.qos_mutex);
    lock.unlock();

    while(true){
        if (!broker.QoS_events.empty()){
            lock.lock();

            shared_lock<shared_mutex> lock_2(broker.clients_mtx);
            for(const auto &it : broker.clients){
                //
            }
            lock_2.unlock();

            lock.unlock();
        }


//        for(const auto& it_fd : fd_lst){
//            //auto it = broker.QoS_events.find(it_fd);
//            auto range = broker.QoS_events.equal_range(it_fd);
//
//            for_each(range.first, range.second, [&broker, &it_fd](const auto &x){
//                MqttStringEntity e_string(x.second.GetString());
//                auto pData = x.second.GetPtr();
//                broker.NotifyClient(it_fd, e_string, *pData, x.second.GetQoS(), x.second.GetID());
//            });
//        }


        this_thread::sleep_for(chrono::seconds(1));
    }
}

void* ServerThread([[maybe_unused]] void *arg){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start ServerThread");
    vector<struct pollfd> vec_fds;
    while(true){
        switch (broker.state){
            case broker_states::started : {
                broker.lg->debug("state started");
                vec_fds.clear();
                vec_fds.reserve(10);
                uint32_t client_num = broker.GetClientCount();
                if (client_num == 0){
                    broker.SetState(broker_states::init);
                    broker.lg->info("Stop ServerThread");
                    broker.Execute();
                    return nullptr;
                }
                client_num++; // for control socket
                for(auto const &it : broker.clients){
                    struct pollfd tmp_fd{it.first, POLLIN, 0};
                    vec_fds.push_back(tmp_fd);
                }
                struct pollfd tmp_fd_control{broker.control_sock, POLLIN, 0};
                vec_fds.push_back(tmp_fd_control);
                broker.SetState(broker_states::wait);
                broker.lg->flush();
            }; break;

            case broker_states::wait : {
                //broker.lg->debug("state wait"); broker.lg->flush();
                int ready;
                uint32_t client_num = broker.GetClientCount() + 1;
                ready = poll(vec_fds.data(), client_num, 1000);
                if (ready == -1){
                    broker.lg->critical("poll error");
                    sleep(1);
                    broker.SetState(broker_states::started);
                }
                list<int> fd_to_delete;
                time_t current_time;
                time(&current_time);
                for(unsigned int i=0; i<client_num; i++) {
                    if (vec_fds[i].revents != 0) {
//                        broker.lg->debug("Event: fd:{} events:{}{}{}", vec_fds[i].fd,
//                                  (vec_fds[i].revents & POLLIN) ? "POLLIN " : "",
//                                  (vec_fds[i].revents & POLLHUP) ? "POLLHUP " : "",
//                                  (vec_fds[i].revents & POLLERR) ? "POLLERR " : "");

                        if (vec_fds[i].revents & POLLIN) {
                            vec_fds[i].revents = 0;
                            if (vec_fds[i].fd == broker.control_sock){
                                broker.lg->debug("Got control command");
                                int data_socket = accept(broker.control_sock, NULL, NULL);
                                if (data_socket != -1) {
                                    char c_buf[16] = "";
                                    int ret = read(data_socket, c_buf, sizeof(c_buf));
                                    if (ret > 0) {
                                        if (!strncmp(c_buf, "ADD", sizeof(c_buf))) {
                                            broker.lg->debug("add new client, reinit fds");
                                            broker.SetState(broker_states::started);
                                            close(data_socket);
                                            break;
                                        } else  broker.lg->warn("Ignore command. Unknown command in command socket.");
                                    } else broker.lg->error("data_socket read error");
                                    close(data_socket);
                                } else broker.lg->error("control socket accept error");
                            } else {
                                broker.lg->debug("Have data");
                                int fd = vec_fds[i].fd;
                                broker.clients[fd]->SetPacketLastTime(current_time);
                                FixedHeader f_head;
                                int ret = broker.ReadFixedHeader(fd, f_head);
                                if (ret == broker_err::ok){
                                    broker.lg->debug("action:{}", broker.GetControlPacketTypeName(f_head.GetType()));
                                    shared_ptr<uint8_t> buf(new uint8_t[f_head.remaining_len], default_delete<uint8_t[]>());
                                    ret = ReadData(fd, buf.get(), f_head.remaining_len, 0);
                                    broker.lg->debug("remaining_len:{}", f_head.remaining_len);
                                    if (ret != (int) f_head.remaining_len){
                                        broker.lg->error("Read error. Read {} bytes instead of {}", ret, f_head.remaining_len);
                                        fd_to_delete.push_back(fd);
                                        continue;
                                    }
                                    broker.lg->flush();
                                    switch (f_head.GetType()){
                                        case mqtt_pack_type::CONNECT: {
                                              int handle_stat = HandleMqttConnect(broker.clients[fd], buf, broker.lg);
                                              if (handle_stat != mqtt_err::ok){
                                                  broker.lg->error("handleConnect error");
                                                  fd_to_delete.push_back(fd);
                                                  break;
                                              }

                                              uint32_t answer_size;
                                              MqttPropertyChain p_chain;
                                              p_chain.AddProperty(make_shared<MqttProperty>(assigned_client_identifier,
                                                                                            shared_ptr<MqttEntity>(new MqttStringEntity(broker.clients[fd]->GetID()))));
                                              VariableHeader answer_vh{shared_ptr<IVariableHeader>(new ConnactVH(0,success, std::move(p_chain)))};
                                              broker.AddCommand(fd, make_tuple(answer_size, CreateMqttPacket(CONNACK << 4, answer_vh, answer_size)));
                                        }; break;

                                        case mqtt_pack_type::PUBLISH:{
                                            PublishVH vh;
                                            auto pMessage = make_shared<MqttBinaryDataEntity>();

                                            int handle_stat = HandleMqttPublish(f_head, buf, broker.lg, vh, pMessage);
                                            if (handle_stat != mqtt_err::ok){
                                                broker.lg->error("handle PUBLISH error");
                                                fd_to_delete.push_back(fd);
                                                break;
                                            }
                                            broker.lg->info("{} topic name:'{}' packet_id:{} property_count:{}",broker.clients[fd]->GetIP(), vh.topic_name.GetString(), vh.packet_id, vh.p_chain.Count());
                                            broker.lg->info("{} sent {} bytes",broker.clients[fd]->GetIP(), pMessage->Size()-2);
                                            if (f_head.isRETAIN()){
                                                broker.StoreTopicValue(f_head.QoS(), vh.packet_id, vh.topic_name.GetString(), pMessage);
                                            }
                                            if (f_head.QoS() != mqtt_QoS::QoS_0){
                                                uint32_t answer_size;
                                                VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PubackVH(vh.packet_id,success, MqttPropertyChain()))};
                                                broker.AddCommand(fd, make_tuple(answer_size, CreateMqttPacket(PUBACK << 4, answer_vh, answer_size)));
                                            }
                                            broker.NotifyClients(MqttTopic(f_head.QoS(), vh.packet_id, vh.topic_name.GetString(), pMessage));
                                        }; break;

                                        case mqtt_pack_type::SUBSCRIBE : {
                                            SubscribeVH vh;
                                            vector<uint8_t> reason_codes;
                                            list<string> tpcs;

                                            int handle_stat = HandleMqttSubscribe(broker.clients[fd], f_head, buf, broker.lg, vh, reason_codes, tpcs);
                                            if (handle_stat != mqtt_err::ok){
                                                broker.lg->error("handle SUBSCRIBE error");
                                                fd_to_delete.push_back(fd);
                                                break;
                                            }
                                            broker.lg->info("Subscribe. id:{} property count:{}", vh.packet_id, vh.p_chain.Count());
                                            for(auto it = vh.p_chain.Cbegin(); it != vh.p_chain.Cend(); ++it) {
                                                broker.lg->debug("property id:{} val:{}", it->first,
                                                                 it->second->GetUint());
                                            }
                                            VariableHeader answer_vh{shared_ptr<IVariableHeader>(new SubackVH(vh.packet_id, MqttPropertyChain(), reason_codes))};
                                            uint32_t answer_size;
                                            broker.AddCommand(fd, make_tuple(answer_size, CreateMqttPacket(SUBACK << 4, answer_vh, answer_size)));

                                            for (const auto& it : tpcs){
                                                bool found;
                                                auto retain_topic = broker.GetTopic(it, found);

                                                if(found){
                                                    broker.lg->debug("Found retain topic:{} qos:{}", it, retain_topic.GetQoS());
                                                    retain_topic.SetPacketID(vh.packet_id);
                                                    broker.NotifyClient(fd, retain_topic);
                                                }
                                            }
                                        }; break;

                                        case mqtt_pack_type::PUBACK : {
                                            PubackVH p_vh;
                                            HandleMqttPuback(buf, broker.lg, p_vh);
                                            broker.lg->debug("puback: id:{} reason_code:{}", p_vh.packet_id, p_vh.reason_code);
                                            auto pClient = broker.clients[vec_fds[i].fd];
                                            broker.DelQosEvent(pClient->GetID(), p_vh.packet_id);
                                        } break;

                                        case mqtt_pack_type::DISCONNECT : {
                                            broker.lg->info("{}: Client has disconnected", broker.clients[fd]->GetIP());
                                            fd_to_delete.push_back(fd);
                                        }; break;

                                        case mqtt_pack_type::PINGREQ : {
                                            broker.lg->info("{}: PINGREQ", broker.clients[fd]->GetIP());
                                            uint32_t answer_size;
                                            broker.AddCommand(fd, make_tuple(answer_size, CreateMqttPacket(PINGRESP << 4, answer_size)));
                                        }; break;

                                        default : {
                                            broker.lg->warn("Unsupported mqtt packet. Ignore it.");
                                        }
                                    }
                                } else {
                                    broker.lg->error("Mqtt protocol error. Can't read FixedHeader");
                                    fd_to_delete.push_back(fd);
                                    auto pClient = broker.clients[vec_fds[i].fd];
                                    if (pClient->isWillFlag()){
                                        broker.NotifyClients(pClient->will_topic);
                                    }
                                }
                            }
                        } else {
                            broker.lg->debug("client closed socket fd:{}", vec_fds[i].fd);
                            fd_to_delete.push_back(vec_fds[i].fd);
                            auto pClient = broker.clients[vec_fds[i].fd];
                            if (pClient->isWillFlag()){
                                broker.NotifyClients(pClient->will_topic);
                            }
                        }
                    } else {
                        // no data from current socket
                        int fd = vec_fds[i].fd;
                        if (fd != broker.control_sock){
                            auto pClient = broker.clients[fd];
                            if (current_time - pClient->GetPacketLastTime() >= pClient->GetAlive() + 5){
                                broker.lg->info("{} ({}) time out, disconnect", pClient->GetIP(), pClient->GetID());
                                VariableHeader answer_vh{shared_ptr<IVariableHeader>(new DisconnectVH(keep_alive_timeout, MqttPropertyChain()))};
                                uint32_t answer_size;
                                broker.AddCommand(fd, make_tuple(answer_size, CreateMqttPacket(DISCONNECT << 4, answer_vh, answer_size)));
                                fd_to_delete.push_back(fd);
                                if (pClient->isWillFlag()){
                                    broker.NotifyClients(pClient->will_topic);
                                }
                            }
                        }
                    }
                }
                if (!fd_to_delete.empty()){
                    broker.SetState(broker_states::started);
                    for(const auto &it : fd_to_delete){
                        auto pClient = broker.clients[it];
                        broker.CloseConnection(it);
                    }
                }
                broker.Notify();
                broker.lg->flush();
            }; break;
        }
    }
    return nullptr;
}

int Broker::AddClient(int sock, const string &_ip){
    current_clients++;
    shared_ptr<Client> new_client = std::make_shared<Client>(_ip);

    unique_lock<shared_mutex> lock(clients_mtx);
    auto ret = clients.insert(make_pair(sock, new_client));
    lock.unlock();

    if (ret.second == true) {
        if (state == broker_states::wait) SendCommand("ADD", 3);
        return broker_err::ok;
    } else return broker_err::add_error;
}

void Broker::DelClient(int sock){
    lg->debug("DelClient");
    current_clients--;
    lg->debug("Total cl_num: {}", current_clients);

    unique_lock<shared_mutex> lock(clients_mtx);
    clients.erase(sock);
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
    lg->debug("Close connection fd:{}", fd);
    close(fd);
    DelClient(fd);
}

int Broker::NotifyClients(const MqttTopic& topic){
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
    uint32_t answer_size;

    shared_ptr<uint8_t> data = CreateMqttPacket(PUBLISH << 4 | topic.GetQoS() << 1, answer_vh, topic.GetPtr(), answer_size);

    for(const auto& it : clients){
        string topic_name_str = topic.GetName();
        if(it.second->MyTopic(topic_name_str)){
            lg->debug("Add topic to send :{}", topic_name_str);
            AddCommand(it.first, make_tuple(answer_size, data));
            if (topic.GetQoS() > mqtt_QoS::QoS_0){
                AddQosEvent(it.second->GetID(), topic);
            }
        }
    }
    return mqtt_err::ok;
}

int Broker::NotifyClient(const int fd, const MqttTopic& topic){
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
    uint32_t answer_size;
    shared_ptr<uint8_t> data = CreateMqttPacket(PUBLISH << 4 | topic.GetQoS() << 1, answer_vh, topic.GetPtr(), answer_size);
    lg->debug("Add topic to send :{}", topic.GetName());
    AddCommand(fd, make_tuple(answer_size, data));

    if (topic.GetQoS() > mqtt_QoS::QoS_0){
        auto pClient = clients[fd];
        AddQosEvent(pClient->GetID(), topic);
    }

    return mqtt_err::ok;
}

void Broker::AddQosEvent(const string& client_id, const MqttTopic& mqtt_message){
    unique_lock<shared_mutex> lock(qos_mutex);
    QoS_events.insert(make_pair(client_id, mqtt_message));
    lg->debug("AddQosEvent for client_id:{} qos:{} topic:{} packet_id:{}", client_id, mqtt_message.GetQoS(), mqtt_message.GetName(), mqtt_message.GetID());
    lg->debug("total count:{}", QoS_events.size());
}

void Broker::DelQosEvent(const string& client_id, const uint16_t packet_id){
    unique_lock<shared_mutex> lock(qos_mutex);
    auto it = QoS_events.find(client_id);
    while(it != QoS_events.end() && it->first == client_id){
        if (it->second.GetID() == packet_id) {
            QoS_events.erase(it);
            lg->debug("DelQosEvent for client_id:{} qos:{} topic:{} packet_id:{}", client_id, it->second.GetQoS(), it->second.GetName(), it->second.GetID());
            lg->debug("total count:{}", QoS_events.size());
            return;
        }
        it++;
    }
}

void Broker::DelClientQosEvents(const string& client_id){
    unique_lock<shared_mutex> lock(qos_mutex);
    QoS_events.erase(client_id);
}
