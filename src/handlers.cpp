#include "mqtt_broker.h"

using namespace spdlog;
using namespace std;

int HandleMqttConnect(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, Broker *broker){
    ConnectVH con_vh;
    uint32_t offset = 0;
    con_vh.ReadFromBuf(buf.get(), offset);

    char name_tmp[5] = "";
    memcpy(name_tmp, con_vh.name, 4);
    lg->debug("Connect VH: len:{} name:{} version:{} flags:{:X} alive:{}", con_vh.prot_name_len, name_tmp, con_vh.version, con_vh.conn_flags, con_vh.alive);

    if (con_vh.version != MQTT_VERSION_5 && con_vh.version != MQTT_VERSION_3){
        return mqtt_err::protocol_version_err;
    }

    pClient->SetClientMQTTVersion(con_vh.version);
    pClient->SetConnAlive(con_vh.alive);
    pClient->SetConnFlags(con_vh.conn_flags);
    lg->debug("Connection flags:{}{}{}{}{}{}", (pClient->isCleanFlag()) ? "CleanStart " : "", (pClient->isWillFlag()) ? "WillFlag " : "",
                                            (pClient->WillQoSFlag()) ? "WillQoS " : "", (pClient->isWillRetFlag()) ? "WillRetainFlag " : "",
                                            (pClient->isPwdFlag()) ? "PWDFlag " : "", (pClient->isUserNameFlag()) ? "USRNameFlag " : "");
    //read properties
    if (pClient->GetClientMQTTVersion() == MQTT_VERSION_5) {
        uint32_t property_size;
        int create_status = pClient->conn_properties.Create(buf.get() + offset, property_size);
        if (create_status != mqtt_err::ok) {
            lg->error("Read properties error!");
            return create_status;
        } else lg->debug("[{}] Property count: {}", pClient->GetIP(), pClient->conn_properties.Count());
        offset += property_size;
    }

    //read ClientID
    uint8_t id_len;
    auto id = CreateMqttStringEntity(buf.get() + offset, id_len);
    if (id != nullptr){
		//auto check_fd = broker->GetClientFd(id->GetString());	
		if (broker->CheckClientID(id->GetString()) == true){        
    		lg->warn("[{}] client sent already existing id client {}", pClient->GetIP(), id->GetString());
			//broker->CloseConnection(check_fd);            
			return mqtt_err::duplicate_client_id;
        }
        pClient->SetID(id->GetString());
        lg->debug("[{}] ID: {}", pClient->GetIP(), pClient->GetID());
    } else {
        pClient->SetID(GenRandom(23));
        lg->info("[{}] No ClientID provided, create new ID:{}", pClient->GetIP(), pClient->GetID());
        pClient->SetRandomID();
    }

    if (id_len) offset += id_len;
    else offset += 2;

    if (pClient->isWillFlag()){
        uint32_t will_property_size;
        int will_create_status = pClient->will_properties.Create(buf.get() + offset, will_property_size);
        if (will_create_status != mqtt_err::ok){
            lg->error("Read will properties error!");
            return will_create_status;
        }
        offset += will_property_size;
        lg->debug("will properties count: {} ", pClient->will_properties.Count());
        lg->flush();

        uint16_t str_len = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(str_len);
        string will_topic_name = string((char *) (buf.get() + offset), str_len);
        offset += str_len;
        lg->debug("will topic: {} WillQoS:{}", will_topic_name, pClient->WillQoSFlag());
        lg->flush();

        uint16_t data_len = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(data_len);

        auto pMessage = make_shared<MqttBinaryDataEntity>(MqttBinaryDataEntity(data_len, buf.get() + offset));

        pClient->will_topic = MqttTopic(pClient->WillQoSFlag(), 1, will_topic_name, pMessage);
        offset += data_len;
        lg->debug("will message len: {} ", pClient->will_topic.GetSize());
        lg->flush();
    }

    return mqtt_err::ok;
}

int HandleMqttPublish(std::shared_ptr<Client>& pClient, const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PublishVH &vh, shared_ptr<MqttBinaryDataEntity> &message){
    lg->debug("HandleMqttPublish");
    uint32_t offset = 0;
    PublishVH p_vh;

    if (pClient->GetClientMQTTVersion() == MQTT_VERSION_5){
        PublishVH tmp_vh(fh.QoS(), buf, offset);
        p_vh = tmp_vh;
    } else {
        p_vh.topic_name = MqttStringEntity(ConvertToHost2Bytes(buf.get()), buf.get() + sizeof(uint16_t));
        offset = p_vh.topic_name.Size();
        if (fh.QoS() > mqtt_QoS::QoS_0){
            p_vh.packet_id = ConvertToHost2Bytes(buf.get() + offset);
            offset += sizeof(p_vh.packet_id);
        }
    }

    lg->info("topic name:'{}' packet_id:{} property_count:{}", p_vh.topic_name.GetString(), p_vh.packet_id, p_vh.p_chain.Count());
    vh = std::move(p_vh);

    //read Payload
    lg->debug("message:{} retained:{}", string((char *)(buf.get() + offset), fh.remaining_len - offset), fh.isRETAIN() ? true : false);
    *message = MqttBinaryDataEntity(fh.remaining_len - offset, buf.get() + offset);

    return mqtt_err::ok;
}

int HandleMqttSubscribe(shared_ptr<Client>& pClient, const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg,
                        SubscribeVH &vh, vector<uint8_t> &_reason_codes, list<pair<string, uint8_t>>& subscribe_topics){
    lg->debug("HandleMqttSubscribe");
    uint32_t offset = 0;
    vh = SubscribeVH(buf, offset, pClient->GetClientMQTTVersion());

    while(offset < fh.remaining_len){
        uint8_t options;
        uint16_t name_len = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(name_len);
        string topic_name((char *) (buf.get() + offset), name_len);
        offset += name_len;
        memcpy(&options, buf.get() + offset, sizeof(options));
        offset += sizeof(options);

        pClient->AddSubscription(topic_name, options);
        _reason_codes.push_back(mqtt_QoS::QoS_2);
        subscribe_topics.emplace_back(topic_name, options);
        lg->info("[{}] subscribed to topic:'{}' QoS:{}", pClient->GetIP(), topic_name, options);
    }
    lg->flush();
    return mqtt_err::ok;
}

int HandleMqttPuback(const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PubackVH&  p_vh){
    lg->debug("HandleMqttPuback");
    uint32_t offset = 0;

    p_vh.ReadFromBuf(buf.get(), offset);
    return mqtt_err::ok;
}

int HandleMqttUnsubscribe(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, const FixedHeader &fh,
                          shared_ptr<logger>& lg, UnsubscribeVH&  p_vh, list<string> &topics_to_unsubscribe){
    lg->debug("HandleMqttUnsubscribe");
    uint32_t offset = 0;
    p_vh.ReadFromBuf(buf.get(), offset);
    while(offset < fh.remaining_len){
        uint16_t name_len = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(name_len);
        string topic_name((char *) buf.get() + offset, name_len);
        offset += name_len;
        topics_to_unsubscribe.push_back(topic_name);
        lg->info("{}: Unsubscribe topic:'{}'", pClient->GetIP(), topic_name);
    }
    lg->flush();
    return mqtt_err::ok;
}

int HandleMqttPubrel(const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, TypicalVH&  t_vh){
    lg->debug("HandleMqttPubrel");
    uint32_t offset = 0;
    t_vh.ReadFromBuf(buf.get(), offset);
    return mqtt_err::ok;
}
