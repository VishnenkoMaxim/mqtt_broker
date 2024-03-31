#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h> 

#include "mqtt_broker.h"

using namespace std;
using namespace mqtt_pack_type;

vector<string> pack_type_names{"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP", "SUBSCRIBE", "SUBACK",
                               "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT", "AUTH"};

[[noreturn]] void Broker::SenderThread(int id){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start Sender Thread {}", id);

    while(true){      
		broker.Execute();
		broker.lg->trace("Sender Thread {} executed", id);  
		broker.lg->flush(); 
    }
}

[[noreturn]] void Broker::QoSThread(){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start QoSThread{}");

    shared_lock lock{broker.qos_mutex};
    lock.unlock();
    map<int, string> clients_id_map;

    while(true){
        broker.lg->trace("QoSThread execute"); broker.lg->flush();
        if (!broker.postponed_events.empty()){
            clients_id_map.clear();
            lock.lock();

            shared_lock lock_2{broker.clients_mtx};
            for(const auto &it : broker.clients){
                clients_id_map.insert(make_pair(it.first, it.second->GetID()));
            }
            lock_2.unlock();

            for(const auto& it : clients_id_map){
                auto it_cli = broker.postponed_events.find(it.second);
                if (it_cli != broker.postponed_events.end() && !it_cli->second.empty()){
                    broker.AddCommand(it.first, tuple{it_cli->second.front().data_len, it_cli->second.front().pData});
                }
            }
            lock.unlock();
            broker.lg->flush();
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void Broker::ServerThread(){
    Broker& broker = Broker::GetInstance();
    broker.lg->debug("Start ServerThread");  broker.lg->flush();
    vector<struct pollfd> vec_fds;
    while(true){
        switch (broker.state){
            case broker_states::started : {
                broker.lg->debug("state started"); broker.lg->flush();
                vec_fds.clear();
                vec_fds.reserve(50);
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
                broker.SetState(broker_states::wait_state);
                broker.lg->info("Current fds: {}", vec_fds.size()); broker.lg->flush();
            }; break;

            case broker_states::wait_state : {
                broker.lg->trace("state wait"); broker.lg->flush();
                int ready;
                uint32_t client_num = broker.GetClientCount() + 1;
                assert(vec_fds.data() != nullptr);

                ready = poll(vec_fds.data(), client_num, 1000);

                if (ready == -1){
                    broker.lg->critical("poll error"); broker.lg->flush();
                    sleep(1);
                    broker.SetState(broker_states::started);
                } else broker.lg->trace("poll ok"); broker.lg->flush();

                list<int> fd_to_delete;
                time_t current_time;
                time(&current_time);
                for(unsigned int i=0; i<client_num; i++) {
                    if (vec_fds[i].revents != 0) {
                        if (vec_fds[i].revents & POLLIN) {
                            vec_fds[i].revents = 0;
                            if (vec_fds[i].fd == broker.control_sock){
                                broker.lg->debug("Got control command"); broker.lg->flush();
                                int data_socket = accept(broker.control_sock, nullptr, nullptr);
                                if (data_socket != -1) {
                                    char c_buf[16] = "";
                                    int ret = read(data_socket, c_buf, sizeof(c_buf));
                                    if (ret > 0) {
                                        if (!strncmp(c_buf, "ADD", sizeof(c_buf))) {
                                            broker.lg->debug("add new client, reinit fds"); broker.lg->flush();
                                            broker.SetState(broker_states::started);
                                            close(data_socket);
                                            break;
                                        } else  broker.lg->warn("Ignore command. Unknown command in command socket."); broker.lg->flush();
                                    } else broker.lg->error("data_socket read error"); broker.lg->flush();
                                    close(data_socket);
                                } else broker.lg->error("control socket accept error"); broker.lg->flush();
                            } else {
                                broker.lg->debug("Have data"); broker.lg->flush();
                                int fd = vec_fds[i].fd;
                                auto pClient = broker.clients[fd];
                                pClient->SetPacketLastTime(current_time);
                                FixedHeader f_head;
                                broker_err ret = broker.ReadFixedHeader(fd, f_head);
                                if (ret == broker_err::ok){
                                    broker.lg->info("[{}] {} <------", pClient->GetIP(), broker.GetControlPacketTypeName(f_head.GetType()));
                                    shared_ptr<uint8_t> buf(new uint8_t[f_head.remaining_len], default_delete<uint8_t[]>());
                                    int read_status = ReadData(fd, buf.get(), f_head.remaining_len, 0);
                                    broker.lg->debug("remaining_len:{}", f_head.remaining_len);
                                    if (read_status != (int) f_head.remaining_len){
                                        broker.lg->error("Read error. Read {} bytes instead of {}", read_status, f_head.remaining_len);
                                        fd_to_delete.push_back(fd);
                                        continue;
                                    }
                                    broker.lg->flush();
                                    int handle_stat = broker.HandlePacket(f_head, buf, &broker, fd);
                                    if (handle_stat != mqtt_err::ok){
                                        broker.HandleError(handle_stat, broker, fd);
                                        fd_to_delete.push_back(fd);
                                    } else broker.lg->debug("handle_stat OK");
                                    broker.lg->flush();
                                } else {
                                    broker.lg->error("Mqtt protocol error. Can't read FixedHeader. status:{}", static_cast<int>(ret));
                                    fd_to_delete.push_back(fd);
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
                            if (current_time - pClient->GetPacketLastTime() >= uint32_t (pClient->GetAlive() + 5)){
                                broker.lg->warn("[{}] time out, disconnect", pClient->GetIP());
                                VariableHeader answer_vh{shared_ptr<IVariableHeader>(new DisconnectVH(keep_alive_timeout, MqttPropertyChain()))};
                                uint32_t answer_size;
                                broker.AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(DISCONNECT).Build(), answer_vh, answer_size)});
                                broker.lg->info("[{}] {} ------>", pClient->GetIP(), broker.GetControlPacketTypeName(DISCONNECT));
                                fd_to_delete.push_back(fd);
                                if (pClient->isWillFlag()){
                                    broker.NotifyClients(pClient->will_topic);
                                }
                            }
                        }
                    }
                }
                if (!fd_to_delete.empty()){
					broker.lg->warn("close connections"); broker.lg->flush();
                    broker.SetState(broker_states::started);
                    for(const auto &it : fd_to_delete){
						auto pClient = broker.clients[it];
						broker.lg->warn("close connection fd:{} ID:{}", it, pClient->GetID()); broker.lg->flush();
                        broker.CloseConnection(it);
                    }
                }
                broker.Notify();
                broker.lg->flush();
            }; break;
        }
    }
}

Broker::Broker() : Commands(), current_clients(0), state(0), control_sock(-1), port(1883) {
    AddHandler(new MqttPublishPacketHandler());
    AddHandler(new MqttPubAckPacketHandler());
    AddHandler(new MqttPubRelPacketHandler());
    AddHandler(new MqttPubRecPacketHandler());
    AddHandler(new MqttPubCompPacketHandler());
    AddHandler(new MqttPingPacketHandler());
    AddHandler(new MqttSubscribePacketHandler());
    AddHandler(new MqttDisconnectPacketHandler());
    AddHandler(new MqttUnsubscribePacketHandler());
    AddHandler(new MqttConnectPacketHandler());

    AddErrorHandler(make_shared<MqttDisconnectErr>());
    AddErrorHandler(make_shared<MqttProtocolVersionErr>());
    AddErrorHandler(make_shared<MqttHandleErr>());
    AddErrorHandler(make_shared<MqttDuplicateIDErr>());
}

broker_err Broker::AddClient(int sock, const string &_ip){
    current_clients++;
    shared_ptr<Client> new_client = std::make_shared<Client>(_ip);

    unique_lock lock{clients_mtx};
    auto ret = clients.insert(make_pair(sock, new_client));
    lock.unlock();

    if (ret.second) {
        if (state == broker_states::wait_state) SendCommand("ADD", 3);
        return broker_err::ok;
    } else return broker_err::add_error;
}

void Broker::DelClient(int sock){
    lg->debug("DelClient fd:{}", sock); lg->flush();
    current_clients--;

    unique_lock lock{clients_mtx};
    clients.erase(sock);
}

uint32_t Broker::GetClientCount() noexcept {
    return clients.size();
}

int Broker::GetState() const noexcept {
    return state;
}

void Broker::Start() {
    if (state == broker_states::init) {
        lg->debug("Broker::Start()"); lg->flush();
        state = broker_states::started;

        thread thread_broker(ServerThread);
        thread_broker.detach();
        if (!qos_thread_started) {
            lg->debug("start threads"); lg->flush();
            qos_thread = std::thread(QoSThread);
            thread thread1(SenderThread, 1);
            thread thread2(SenderThread, 2);
            thread1.detach();
            thread2.detach();
            qos_thread_started = true;
        }
        lg->debug("Broker has started");
    } else {
        lg->warn("Broker has already started!");
    }
    lg->flush();
}

void Broker::SetState(int _state) noexcept {
    state = _state;
}

broker_err Broker::InitControlSocket(const string& sock_path) {
    struct sockaddr_un serv_addr;
    unlink(sock_path.c_str());
    control_sock_path = sock_path;
    control_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (control_sock < 0) {
        lg->error("Error opening control socket: {}", strerror(errno));
        return broker_err::sock_create_err;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family  = AF_UNIX;
    strncpy(serv_addr.sun_path, sock_path.c_str(), sizeof(serv_addr.sun_path) - 1);
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

int Broker::InitSocket(){
    int sock_fd;
    struct sockaddr_in serv_addr;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        lg->error("Error opening socket: {}", strerror(errno));lg->flush();
        return 0;
    }
	int on = 1;
	ioctl(sock_fd, FIONBIO, &on);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    while(true) {
        if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            lg->error("Error binding socket: {}", strerror(errno)); lg->flush();
            sleep(5);
            continue;
        }
        break;
    }
    if (listen(sock_fd, 40) < 0){
        lg->error("Error listening socket: {}", strerror(errno)); lg->flush();
        return 0;
    }
    main_socket = sock_fd;
    return sock_fd;
}

int Broker::WaitForClient(char* _ip){
    struct pollfd fds;
    fds.fd = main_socket;
    fds.events = POLLIN;

    struct sockaddr_in cli_addr;
    socklen_t c_len = sizeof(cli_addr);
    if (poll(&fds, 1, -1) == -1) {
        lg->error("Error poll(): {}", strerror(errno));
        return -1;
    }
    int newsock_fd = accept(fds.fd, (struct sockaddr *) &cli_addr, &c_len);
    if (newsock_fd < 0){
        lg->error("Error accept socket: {}", strerror(errno));
        return -2;
    }
    strcpy(_ip, inet_ntoa(cli_addr.sin_addr));
    return newsock_fd;
}

void Broker::SetPort(int _port) noexcept {
    port = _port;
}

broker_err Broker::SendCommand(const char *buf, const int buf_size) {
    struct sockaddr_un addr;
    int ret;

    int data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        lg->error("Error opening socket: {}", strerror(errno));
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, control_sock_path.c_str(), sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *) &addr,sizeof(addr));
    if (ret == -1) {
        lg->error("Control socket is down: {}", strerror(errno));
        return broker_err::connect_err;
    }
    ret = write(data_socket, buf, buf_size);
    if (ret == -1) {
        lg->error("write error: {}", strerror(errno));
        close(data_socket);
        return broker_err::write_err;
    }
    lg->debug("sent control command: {}", buf);
    close(data_socket);
    return broker_err::ok;
}

void    Broker::InitLogger(const string & _path, const size_t  _size, const size_t _max_files, const int _level){
    lg = spdlog::rotating_logger_mt("broker", _path, _size, _max_files);
    //lg = spdlog::basic_logger_mt<spdlog::async_factory>("broker", _path);
    SetLogLevel(lg, _level);
}

broker_err Broker::ReadFixedHeader(const int fd, FixedHeader &f_hed){
    int ret = read(fd, &f_hed.first, 1);
    if (ret != 1) {
        lg->error("read error: {} {}", ret, strerror(errno));
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
    lg->debug("Close connection fd:{}", fd); lg->flush();
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

            uint32_t answer_size;
			
            VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
            if (topic.GetQoS() > mqtt_QoS::QoS_0){
                if (!CheckIfMoreMessages(it.second->GetID())){
                    auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).Build(), answer_vh, topic.GetPtr(), answer_size);
                    AddCommand(it.first, tuple{answer_size, data});
                } else {
                    auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).WithDup().Build(), answer_vh, topic.GetPtr(), answer_size);
                    AddQosEvent(it.second->GetID(), mqtt_packet{answer_size, data, topic.GetID()});
                }
            } else {
                auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).Build(), answer_vh, topic.GetPtr(), answer_size);
                AddCommand(it.first, tuple{answer_size, data});
            }

            lg->info("[{}] fd:{} {} ------>", it.second->GetIP(), it.first, GetControlPacketTypeName(PUBLISH));
        }
    }
    return mqtt_err::ok;
}

int Broker::NotifyClient(const int fd, MqttTopic& topic){
    lg->debug("NotifyClient()"); lg->flush();
    auto pClient = clients[fd];

    if (topic.GetQoS() == mqtt_QoS::QoS_0) topic.SetPacketID(0);
    else topic.SetPacketID(pClient->GenPacketID());

    //lg->debug("NotifyClient(): {} {}", topic.GetID(), topic.GetQoS()); lg->flush();

    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PublishVH(MqttStringEntity(topic.GetName()), topic.GetID(), MqttPropertyChain()))};
    uint32_t answer_size;
    lg->debug("Add topic to send :{}", topic.GetName()); lg->flush();

    if (topic.GetQoS() > mqtt_QoS::QoS_0){
        if (!CheckIfMoreMessages(pClient->GetID())) {
            auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).Build(), answer_vh, topic.GetPtr(), answer_size);
            AddCommand(fd, tuple{answer_size, data});
        } else {
            auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).WithDup().Build(), answer_vh, topic.GetPtr(), answer_size);
            AddQosEvent(pClient->GetID(), mqtt_packet{answer_size, data, topic.GetID()});
        }
    } else {
        auto data = CreateMqttPacket(FHBuilder().PacketType(PUBLISH).WithQoS(topic.GetQoS()).Build(), answer_vh, topic.GetPtr(), answer_size);
        AddCommand(fd, tuple{answer_size, data});
    }

    lg->info("[{}] fd:{} {} ------>", pClient->GetIP(), fd, GetControlPacketTypeName(PUBLISH));
    return mqtt_err::ok;
}

void Broker::AddQosEvent(const string& client_id, const mqtt_packet& mqtt_message) {
    unique_lock lock{qos_mutex};

    if (postponed_events.find(client_id) == postponed_events.end()){
        postponed_events.insert(make_pair(client_id, list<mqtt_packet>{}));
    }
    postponed_events[client_id].push_back(mqtt_message);

    lg->debug("Add posteponed event for client_id:{} packet_id:{}", client_id, mqtt_message.packet_id);
    lg->debug("total count:{}", postponed_events[client_id].size());
}

void Broker::DelQosEvent(const string& client_id, uint16_t packet_id){
    unique_lock lock{qos_mutex};

    auto it = postponed_events.find(client_id);
    if (it != postponed_events.end() && !it->second.empty()) {
        it->second.remove_if([&packet_id](const mqtt_packet& packet){return packet.packet_id == packet_id;});
        lg->debug("DelQosEvent client_id:{} packet_id:{}", client_id, packet_id);
        lg->debug("postponed packets:{}", it->second.size());
        return;
    }
}

void Broker::SetEraseOldValues(const bool val) noexcept {
    erase_old_values_in_queue = val;
}

bool Broker::CheckIfMoreMessages(const string& client_id){
    //lg->debug("CheckIfMoreMessages {}", client_id); lg->flush();
    auto it = postponed_events.find(client_id);
    if (it != postponed_events.end() && !it->second.empty()) return true;

    return false;
}

pair<uint32_t, shared_ptr<uint8_t>> Broker::GetPacket(const string& client_id, bool &found){
    found = false;
    auto it = postponed_events.find(client_id);

    if (it != postponed_events.end()){
        found = true;
        return make_pair(it->second.front().data_len, it->second.front().pData);
    }
    return make_pair(0, nullptr);
}

bool Broker::CheckClientID(const std::string& client_id) const noexcept {
    for (const auto& it : clients){
        if (it.second->GetID() == client_id) return true;
    }
    return false;
}

int Broker::GetClientFd(const std::string& client_id) const noexcept {
    for (const auto& it : clients){
        if (it.second->GetID() == client_id) return it.first;
    }
    return -1;
}


