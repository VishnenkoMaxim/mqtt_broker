
#include "mqtt_broker.h"

using namespace std;
using namespace mqtt_pack_type;

//IMqttPacketHandler
IMqttPacketHandler::IMqttPacketHandler(uint8_t _type) : type(_type){}

uint8_t IMqttPacketHandler::GetType() const {
    return type;
}

//Connect
MqttConnectPacketHandler::MqttConnectPacketHandler() : IMqttPacketHandler(mqtt_pack_type::CONNECT) {}

int MqttConnectPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, const int fd){
    broker->lg->debug("handleConnect");
    auto pClient = broker->clients[fd];
	
	if (pClient == nullptr){
		broker->lg->error("handleConnect error, pClient is nullptr"); 
		return 	mqtt_err::handle_error;
	}

    int handle_stat = HandleMqttConnect(pClient, data, broker->lg, broker);
    if (handle_stat != mqtt_err::ok){
        broker->lg->error("[{}] handleConnect error, handle_stat: {}", pClient->GetIP(), handle_stat);
        return handle_stat;
    }

    for(auto it = pClient->conn_properties.Cbegin(); it != pClient->conn_properties.Cend(); ++it) {
        broker->lg->debug("[{}] property id:{} val:{}", pClient->GetIP(), it->first, it->second->GetUint());
    }

    uint32_t answer_size;
    MqttPropertyChain p_chain;

//    if (pClient->isRandomID()) p_chain.AddProperty(make_shared<MqttProperty>(assigned_client_identifier, shared_ptr<MqttEntity>(new MqttStringEntity(pClient->GetID()))));
//    p_chain.AddProperty(make_shared<MqttProperty>(retain_available, shared_ptr<MqttEntity>(new MqttByteEntity(1))));
//    p_chain.AddProperty(make_shared<MqttProperty>(maximum_packet_size, shared_ptr<MqttEntity>(new MqttFourByteEntity(65535))));
//    p_chain.AddProperty(make_shared<MqttProperty>(wildcard_subscription_available, shared_ptr<MqttEntity>(new MqttByteEntity((uint8_t)0))));
//    p_chain.AddProperty(make_shared<MqttProperty>(shared_subscription_available, shared_ptr<MqttEntity>(new MqttByteEntity((uint8_t)0))));

    MqttPropertyChainBuilder property_builder;
    if (pClient->isRandomID()) p_chain = std::move(property_builder.withClientIdentifier(pClient->GetID()).withRetainAvailable(1).withMaxPocketSize(65535).withWildCard(0).withSharedSubAvailable(0).build());
    else p_chain = std::move(property_builder.withRetainAvailable(1).withMaxPocketSize(65535).withWildCard(0).withSharedSubAvailable(0).build());

    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new ConnactVH(!pClient->isCleanFlag(),success, std::move(p_chain)))};
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(CONNACK).Build(), answer_vh, answer_size)});
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(CONNACK));
    return mqtt_err::ok;
}

//Publish
MqttPublishPacketHandler::MqttPublishPacketHandler(): IMqttPacketHandler(mqtt_pack_type::PUBLISH) {}

int MqttPublishPacketHandler::HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    PublishVH vh;
    auto pMessage = make_shared<MqttBinaryDataEntity>();
    auto pClient = broker->clients[fd];

    int handle_stat = HandleMqttPublish(pClient, f_header, data, broker->lg, vh, pMessage);
    if (handle_stat != mqtt_err::ok){
        broker->lg->error("[{}] handle PUBLISH error", pClient->GetIP());
        return handle_stat;
    }
    broker->lg->info("[{}] topic name:'{}' packet_id:{} property_count:{}",pClient->GetIP(), vh.topic_name.GetString(), vh.packet_id, vh.p_chain.Count());
    if (f_header.isRETAIN()){
        broker->lg->info("[{}] Store topic:{}",pClient->GetIP(), vh.topic_name.GetString());
        broker->StoreTopicValue(f_header.QoS(), vh.packet_id, vh.topic_name.GetString(), pMessage);
    }
    if (f_header.QoS() == mqtt_QoS::QoS_1){
        uint32_t answer_size;
        VariableHeader answer_vh{shared_ptr<IVariableHeader>(new PubackVH(vh.packet_id, success, MqttPropertyChain()))};
        broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(PUBACK).Build(), answer_vh, answer_size)});
    } else if (f_header.QoS() == mqtt_QoS::QoS_2){
        uint32_t answer_size;
        VariableHeader answer_vh{shared_ptr<IVariableHeader>(new TypicalVH(vh.packet_id, success, MqttPropertyChain()))};
        if (!broker->CheckIfMoreMessages(pClient->GetID())){
            auto data_packet = CreateMqttPacket(FHBuilder().PacketType(PUBREC).Build(), answer_vh, answer_size);
            broker->AddCommand(fd, tuple{answer_size, data_packet});
        } else {
            auto data_packet = CreateMqttPacket(FHBuilder().PacketType(PUBREC).WithDup().Build(), answer_vh, answer_size);
            broker->AddQosEvent(pClient->GetID(), mqtt_packet{answer_size, data_packet, vh.packet_id});
        }
        broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(PUBREC));
    }
    auto topic = MqttTopic(f_header.QoS(), vh.packet_id, vh.topic_name.GetString(), pMessage);

    if (f_header.QoS() != mqtt_QoS::QoS_2) broker->NotifyClients(topic);
    else broker->AddQoSTopic(pClient->GetID(), topic);

    return mqtt_err::ok;
}

//Subscribe
MqttSubscribePacketHandler::MqttSubscribePacketHandler() : IMqttPacketHandler(mqtt_pack_type::SUBSCRIBE){}

int MqttSubscribePacketHandler::HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    SubscribeVH vh;
    vector<uint8_t> reason_codes;
    list<pair<string, uint8_t>> tpcs;
    auto pClient = broker->clients[fd];

    int handle_stat = HandleMqttSubscribe(pClient, f_header, data, broker->lg, vh, reason_codes, tpcs);
    if (handle_stat != mqtt_err::ok){
        broker->lg->error("[{}] handle SUBSCRIBE error", broker->clients[fd]->GetIP());
        return handle_stat;
    }

    //broker->lg->info("[{}] Subscribe. id:{} property count:{}", broker->clients[fd]->GetIP(), vh.packet_id, vh.p_chain.Count());
    for(auto it = vh.p_chain.Cbegin(); it != vh.p_chain.Cend(); ++it) {
        broker->lg->debug("[{}] property id:{} val:{}", pClient->GetIP(), it->first,
                         it->second->GetUint());
    }
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new SubackVH(vh.packet_id, MqttPropertyChain(), reason_codes))};
    uint32_t answer_size;
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(SUBACK).Build(), answer_vh, answer_size)});
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(SUBACK));

    for (const auto& it : tpcs){
        broker->lg->info("[{}] serach for topic name:'{}' among retained", pClient->GetIP(), it.first);
        bool found;
        auto retain_topic = broker->GetTopic(it.first, found);

        if(found){
            broker->lg->debug("[{}] Found retain topic:{}",  pClient->GetIP(), it.first); broker->lg->flush();
            retain_topic.SetQos(it.second);
            broker->NotifyClient(fd, retain_topic);
        }
    }
    return mqtt_err::ok;
}

//puback
MqttPubAckPacketHandler::MqttPubAckPacketHandler() : IMqttPacketHandler(mqtt_pack_type::PUBACK){}

int MqttPubAckPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    PubackVH p_vh;
    auto pClient = broker->clients[fd];

    HandleMqttPuback(data, broker->lg, p_vh);
    broker->lg->debug("[{}] puback: id:{}",  broker->clients[fd]->GetIP(), p_vh.packet_id); broker->lg->flush();
    broker->DelQosEvent(pClient->GetID(), p_vh.packet_id);

    if (broker->CheckIfMoreMessages(pClient->GetID())){
        //broker->lg->debug("[{}] There are more messages", broker->clients[fd]->GetIP()); broker->lg->flush();
        bool found;
        auto data_mes = broker->GetPacket(pClient->GetID(), found);
        if (found){
            broker->lg->debug("[{}] found kept message", pClient->GetIP()); broker->lg->flush();
            broker->AddCommand(fd, tuple{data_mes.first, data_mes.second});
        }
    }
    broker->lg->flush();
    return mqtt_err::ok;
}

//disconnect
MqttDisconnectPacketHandler::MqttDisconnectPacketHandler() : IMqttPacketHandler(mqtt_pack_type::DISCONNECT){}

int MqttDisconnectPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, [[maybe_unused]] const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    broker->lg->info("[{}] Client has disconnected", broker->clients[fd]->GetIP());
    if (data && f_header.remaining_len > 0) {
        broker->lg->info("[{}] reason code: {}", broker->clients[fd]->GetIP(), data.get()[0]);
    }
    return mqtt_err::disconnect;
}

//ping
MqttPingPacketHandler::MqttPingPacketHandler() : IMqttPacketHandler(mqtt_pack_type::PINGREQ){}

int MqttPingPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, [[maybe_unused]] const shared_ptr<uint8_t> &data, Broker *broker, int fd){
	auto pClient = broker->clients[fd];   
	broker->lg->info("[{}] PINGREQ", pClient->GetIP());
    uint32_t answer_size;
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(PINGRESP).Build(), answer_size)});
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(PINGRESP));
    return mqtt_err::ok;
}

//unsubscribe
MqttUnsubscribePacketHandler::MqttUnsubscribePacketHandler() : IMqttPacketHandler(mqtt_pack_type::UNSUBSCRIBE){}

int MqttUnsubscribePacketHandler::HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    auto pClient = broker->clients[fd];
    broker->lg->info("[{}] UNSUBSCRIBE", pClient->GetIP());
    UnsubscribeVH u_vh;
    list<string> topics_to_unsubscribe;
    HandleMqttUnsubscribe(pClient, data, f_header, broker->lg, u_vh, topics_to_unsubscribe);

    vector<uint8_t> reason_codes;
    for(const auto& it : topics_to_unsubscribe){
        if (pClient->DelSubscription(it)) reason_codes.push_back(mqtt_reason_code::success);
        else reason_codes.push_back(mqtt_reason_code::no_subscription_existed);
    }

    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new UnsubAckVH(u_vh.packet_id, MqttPropertyChain(), reason_codes))};
    uint32_t answer_size;
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(UNSUBACK).Build(), answer_vh, answer_size)});
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(UNSUBACK));
    return mqtt_err::ok;
}

//pubrel
MqttPubRelPacketHandler::MqttPubRelPacketHandler() : IMqttPacketHandler(mqtt_pack_type::PUBREL) {}

int MqttPubRelPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    TypicalVH t_vh;
    auto pClient = broker->clients[fd];

    HandleMqttPubrel(data, broker->lg, t_vh);
    broker->lg->debug("[{}] pubrel: id:{}", pClient->GetIP(), t_vh.packet_id);

    uint32_t answer_size;
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new TypicalVH(t_vh.packet_id, success, MqttPropertyChain()))};
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(PUBCOMP).Build(), answer_vh, answer_size)});
    broker->DelQosEvent(pClient->GetID(), t_vh.packet_id);
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(PUBCOMP));

    if (broker->CheckIfMoreMessages(pClient->GetID())){
        bool found;
        auto data_mes = broker->GetPacket(pClient->GetID(), found);
        if (found){
            broker->lg->debug("[{}] found kept message", pClient->GetIP()); broker->lg->flush();
            broker->AddCommand(fd, tuple{data_mes.first, data_mes.second});
        }
    }

    bool found;
    auto topic = broker->GetQoSTopic(pClient->GetID(), t_vh.packet_id, found);
    if (found){
        broker->lg->debug("[{}] Have found packet_id", pClient->GetIP());
        broker->NotifyClients(topic);
        broker->DelQoSTopic(pClient->GetID(), t_vh.packet_id);
        broker->lg->debug("[{}] Delete from storage. topic count:{}",  pClient->GetIP(), broker->GetQoSTopicCount());
    } else {
        broker->lg->error("[{}] Did not find packet_id:{}", pClient->GetIP(), t_vh.packet_id);
    }

    broker->lg->flush();
    return mqtt_err::ok;
}

//pubrec
MqttPubRecPacketHandler::MqttPubRecPacketHandler() : IMqttPacketHandler(mqtt_pack_type::PUBREC) {}

int MqttPubRecPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    TypicalVH t_vh;
    HandleMqttPuback(data, broker->lg, t_vh);
    broker->lg->debug("[{}] pubrec: id:{}",  broker->clients[fd]->GetIP(), t_vh.packet_id);
    auto pClient = broker->clients[fd];
    broker->DelQosEvent(pClient->GetID(), t_vh.packet_id);
    uint32_t answer_size;
    VariableHeader answer_vh{shared_ptr<IVariableHeader>(new TypicalVH(t_vh.packet_id, success, MqttPropertyChain()))};
    broker->AddCommand(fd, tuple{answer_size, CreateMqttPacket(FHBuilder().PacketType(PUBREL).Build() | 1 << 1, answer_vh, answer_size)});
    broker->lg->info("[{}] {} ------>", pClient->GetIP(), broker->GetControlPacketTypeName(PUBREL));
    return mqtt_err::ok;
}

//pubcomp
MqttPubCompPacketHandler::MqttPubCompPacketHandler() : IMqttPacketHandler(mqtt_pack_type::PUBCOMP) {}

int MqttPubCompPacketHandler::HandlePacket([[maybe_unused]] const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd){
    TypicalVH t_vh;
    HandleMqttPuback(data, broker->lg, t_vh);
    broker->lg->debug("[{}] pubcomp: id:{} reason_code:{}",  broker->clients[fd]->GetIP(), t_vh.packet_id, t_vh.reason_code);

    auto pClient = broker->clients[fd];
    if (broker->CheckIfMoreMessages(pClient->GetID())){
        bool found;
        auto data_mes = broker->GetPacket(pClient->GetID(), found);
        if (found){
            broker->lg->debug("[{}] found kept message", broker->clients[fd]->GetIP()); broker->lg->flush();
            broker->AddCommand(fd, tuple{data_mes.first, data_mes.second});
        }
    }

    broker->lg->flush();
    return mqtt_err::ok;
}

//Handler
void MqttPacketHandler::AddHandler(IMqttPacketHandler *handler){
    handlers.push_back(handler);
}

int MqttPacketHandler::HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, const int fd){
    for(const auto& it : handlers){
        if (f_header.GetType() == it->GetType()) return it->HandlePacket(f_header, data, broker, fd);
    }
    return mqtt_err::handle_error;
}
