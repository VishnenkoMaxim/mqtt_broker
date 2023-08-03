
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#include "mqtt_broker.h"

vector<string> pack_type_names{"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
                               "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT", "AUTH"};

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

    map<int, string> clients_id_map;

    while(true){
        //broker.lg->debug("QoSThread execute"); broker.lg->flush();
        if (!broker.QoS_events.empty()){
            clients_id_map.clear();
            lock.lock();

            shared_lock<shared_mutex> lock_2(broker.clients_mtx);
            for(const auto &it : broker.clients){
                clients_id_map.insert(make_pair(it.first, it.second->GetID()));
            }
            lock_2.unlock();

            for(const auto& it : clients_id_map){

                auto it_cli = broker.QoS_events.find(it.second);
                if (it_cli != broker.QoS_events.end() && !it_cli->second.empty()){
                    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(it_cli->second.front().GetName()), it_cli->second.front().GetID(), MqttPropertyChain()))};
                    uint32_t answer_size;
                    shared_ptr<uint8_t> data = CreateMqttPacket(PUBLISH << 4 | it_cli->second.front().GetQoS() << 1, answer_vh, it_cli->second.front().GetPtr(), answer_size);
                    broker.lg->debug("QoSThread, Add topic to send :{} value:{}", it_cli->second.front().GetName(), it_cli->second.front().GetString());
                    broker.AddCommand(it.first, make_tuple(answer_size, data));
                }
            }
            lock.unlock();
            broker.lg->flush();
        }

        this_thread::sleep_for(chrono::seconds(1));
    }
}

void ServerThread(){
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
                    return;
                }
                client_num++; // for control socket
                for(auto const &it : broker.clients){
                    struct pollfd tmp_fd{it.first, POLLIN, 0};
                    vec_fds.push_back(tmp_fd);
                }
                struct pollfd tmp_fd_control{broker.control_sock, POLLIN, 0};
                vec_fds.push_back(tmp_fd_control);
                broker.SetState(broker_states::wait);
                broker.lg->info("Current fds: {}", vec_fds.size()); broker.lg->flush();
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
                        if (vec_fds[i].revents & POLLIN) {
                            vec_fds[i].revents = 0;
                            if (vec_fds[i].fd == broker.control_sock){
                                broker.lg->debug("Got control command");
                                int data_socket = accept(broker.control_sock, nullptr, nullptr);
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
                                broker.lg->debug("Have data"); broker.lg->flush();
                                int fd = vec_fds[i].fd;
                                broker.clients[fd]->SetPacketLastTime(current_time);
                                FixedHeader f_head;
                                int ret = broker.ReadFixedHeader(fd, f_head);
                                if (ret == broker_err::ok){
                                    broker.lg->debug("action:{}", broker.GetControlPacketTypeName(f_head.GetType())); broker.lg->flush();
                                    shared_ptr<uint8_t> buf(new uint8_t[f_head.remaining_len], default_delete<uint8_t[]>());
                                    ret = ReadData(fd, buf.get(), f_head.remaining_len, 0);
                                    broker.lg->debug("remaining_len:{}", f_head.remaining_len);
                                    if (ret != (int) f_head.remaining_len){
                                        broker.lg->error("Read error. Read {} bytes instead of {}", ret, f_head.remaining_len);
                                        fd_to_delete.push_back(fd);
                                        continue;
                                    }
                                    broker.lg->flush();

                                    int handle_stat = broker.HandlePacket(f_head, buf, &broker, fd);
                                    if (handle_stat != mqtt_err::ok){
                                        if (handle_stat == mqtt_err::handle_error){
                                            broker.lg->warn("Unsupported mqtt packet. Ignore it.");
                                        } else {
                                            if (handle_stat != mqtt_err::disconnect) broker.lg->error("handle error {}", handle_stat);
                                            fd_to_delete.push_back(fd);
                                        }
                                    } else broker.lg->debug("handle_stat OK");
                                    broker.lg->flush();
                                } else {
                                    broker.lg->error("Mqtt protocol error. Can't read FixedHeader. status:{}", ret);
                                    fd_to_delete.push_back(fd);
                                    auto pClient = broker.clients[vec_fds[i].fd];
                                    if (pClient->isWillFlag()){
                                        broker.NotifyClients(pClient->will_topic);
                                    }
                                }
                            }
                        } else {
                            broker.lg->debug("[{}] Client closed socket", broker.clients[vec_fds[i].fd]->GetID());
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
                                broker.lg->warn("[{}] time out, disconnect", pClient->GetIP());
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
    return;
}

Broker::Broker() : Commands(), current_clients(0), state(0), control_sock(-1) {
    AddHandler(new MqttConnectPacketHandler());
    AddHandler(new MqttPublishPacketHandler());
    AddHandler(new MqttSubscribePacketHandler());
    AddHandler(new MqttPubAckPacketHandler());
    AddHandler(new MqttDisconnectPacketHandler());
    AddHandler(new MqttPingPacketHandler());
    AddHandler(new MqttUnsubscribePacketHandler());
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
        thread thread_broker(ServerThread);
        thread_broker.detach();

        state = broker_states::started;

        if (!qos_thread_started) {
            qos_thread = std::thread(QoSThread);
            thread thread1(SenderThread, 1);
            thread thread2(SenderThread, 2);
            thread1.detach();
            thread2.detach();

            qos_thread_started = true;
        }
        lg->debug("Broker has started {}");
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

void    Broker::InitLogger(const string & _path, const size_t  _size, const size_t _max_files, const int _level){
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

int Broker::NotifyClients(MqttTopic& topic){
    lg->debug("NotifyClients()"); lg->flush();
    for(const auto& it : clients){
        string topic_name_str = topic.GetName();
        uint8_t options;
        if(it.second->MyTopic(topic_name_str, options)){
            topic.SetQos(options & 0x03);
            if (topic.GetQoS() == mqtt_QoS::QoS_0) topic.SetPacketID(0);
            else topic.SetPacketID(it.second->GenPacketID());
            lg->debug("Add topic to send :{} QoS:{} packet_id:{}", topic_name_str, topic.GetQoS(), topic.GetID()); lg->flush();

            VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
            uint32_t answer_size;
            shared_ptr<uint8_t> data = CreateMqttPacket(PUBLISH << 4 | topic.GetQoS() << 1, answer_vh, topic.GetPtr(), answer_size);

            AddCommand(it.first, make_tuple(answer_size, data));
            if (topic.GetQoS() > mqtt_QoS::QoS_0){
                AddQosEvent(it.second->GetID(), topic);
            }
        }
    }
    return mqtt_err::ok;
}

int Broker::NotifyClient(const int fd, MqttTopic& topic){
    lg->debug("NotifyClient()"); lg->flush();
    auto pClient = clients[fd];

    if (topic.GetQoS() == mqtt_QoS::QoS_0) topic.SetPacketID(0);
    else topic.SetPacketID(pClient->GenPacketID());

    lg->debug("NotifyClient(): {} {}", topic.GetID(), topic.GetQoS()); lg->flush();
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
    uint32_t answer_size;
    shared_ptr<uint8_t> data = CreateMqttPacket(PUBLISH << 4 | topic.GetQoS() << 1, answer_vh, topic.GetPtr(), answer_size);
    lg->debug("Add topic to send :{}", topic.GetName()); lg->flush();
    AddCommand(fd, make_tuple(answer_size, data));

    if (topic.GetQoS() > mqtt_QoS::QoS_0){
        AddQosEvent(pClient->GetID(), topic);
    }

    return mqtt_err::ok;
}

void Broker::AddQosEvent(const string& client_id, const MqttTopic& mqtt_message) {
    unique_lock<shared_mutex> lock(qos_mutex);

    if (erase_old_values_in_queue) {
        auto it = QoS_events.find(client_id);
        while (it != QoS_events.end() && it->first == client_id) {
            if (it->second.front().GetName() == mqtt_message.GetName()) {
                QoS_events.erase(it);
                lg->debug("Found old value, DelQosEvent for client_id:{} qos:{} topic:{} packet_id:{}", client_id,
                          it->second.front().GetQoS(), it->second.front().GetName(), it->second.front().GetID());
                break;
            }
            it++;
        }
    }
    //QoS_events.insert(make_pair(client_id, mqtt_message));
    if (QoS_events.find(client_id) == QoS_events.end()){
        QoS_events.insert(make_pair(client_id, queue<MqttTopic>{}));
    }
    QoS_events[client_id].push(mqtt_message);

    lg->debug("AddQosEvent for client_id:{} qos:{} topic:{} packet_id:{}", client_id, mqtt_message.GetQoS(), mqtt_message.GetName(), mqtt_message.GetID());
    lg->debug("total count:{}", QoS_events.size());
}

void Broker::DelQosEvent(const string& client_id, [[maybe_unused]]  uint16_t packet_id){
    unique_lock<shared_mutex> lock(qos_mutex);
    QoS_events[client_id].pop();
}

void Broker::DelClientQosEvents(const string& client_id){
    unique_lock<shared_mutex> lock(qos_mutex);
    QoS_events.erase(client_id);
}

bool Broker::CheckTopicPresence(const string& client_id, const MqttTopic& topic){
    shared_lock<shared_mutex> lock(qos_mutex);

    auto it = QoS_events.equal_range(client_id);
    while ( it.first != it.second ){
        if (it.first->second.front().GetName() == topic.GetName()) return true;
        *it.first++;
    }
    return false;
}

void Broker::SetEraseOldValues(const bool val) noexcept {
    erase_old_values_in_queue = val;
}

bool Broker::CheckIfMoreMessages(const string& client_id){
    auto it = QoS_events.find(client_id);
    if (it->second.size()) return true;

    return false;
}

MqttTopic Broker::GetKeptTopic(const string& client_id, bool &found){
    found = false;
    auto it = QoS_events.find(client_id);

    if (it != QoS_events.end()){
        found = true;
        return it->second.front();
    }

    return MqttTopic{0, 0, string{""}, nullptr};
}


