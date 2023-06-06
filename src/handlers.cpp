#include "mqtt_broker.h"

int HandleMqttConnect(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg){
    ConnectVH con_vh;
    uint32_t offset = 0;

    con_vh.ReadFromBuf(buf.get(), offset);

    char name_tmp[5] = "";
    memcpy(name_tmp, con_vh.name, 4);
    lg->debug(
            "Connect VH: len:{} name:{} version:{} flags:{:X} alive:{}", con_vh.prot_name_len, name_tmp, con_vh.version,
            con_vh.conn_flags, con_vh.alive);

    pClient->SetConnAlive(con_vh.alive);
    pClient->SetConnFlags(con_vh.conn_flags);

    //read properties
    uint32_t property_size;
    int create_status = pClient->conn_properties.Create(buf.get() + offset, property_size);
    if (create_status != mqtt_err::ok){
        lg->error("Read properties error!");
        return create_status;
    }
    offset += property_size;

    //read ClientID
    uint8_t id_len;
    auto id = CreateMqttStringEntity(buf.get() + offset, id_len);
    if (id != nullptr){
        pClient->SetID(id->GetString());
        lg->debug("id: {}", pClient->GetID());
    } else {
        lg->info("No ClientID provided");
        pClient->SetID(GenRandom(23));
        lg->info("Create new ID:{}", pClient->GetID());
    }
    return mqtt_err::ok;
}

int HandleMqttPublish(const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, PublishVH &vh, MqttBinaryDataEntity &message){
    lg->debug("HandleMqttPublish");
    uint32_t offset = 0;

    PublishVH p_vh(fh.QoS(), buf, offset);
    lg->info("topic name:'{}' packet_id:{} property_count:{}", p_vh.topic_name.GetString(), p_vh.packet_id, p_vh.p_chain.Count());
    vh = std::move(p_vh);

    //read Payload
    lg->debug("message:{}", string((char *)(buf.get() + offset), fh.remaining_len - offset));
    message = MqttBinaryDataEntity(fh.remaining_len - offset, buf.get() + offset);

    return mqtt_err::ok;
}

int HandleMqttSubscribe(shared_ptr<Client>& pClient, const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, SubscribeVH &vh, vector<uint8_t> &_reason_codes){
    lg->debug("HandleMqttSubscribe");
    uint32_t offset = 0;

    SubscribeVH s_vh(buf, offset);
    vh = std::move(s_vh);

    while(offset < fh.remaining_len){
        uint8_t options;
        uint16_t name_len = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(name_len);
        string topic_name((char *) buf.get() + offset, name_len);
        offset += name_len;
        memcpy(&options, buf.get() + offset, sizeof(options));
        offset += sizeof(options);
        pClient->AddSubscription(topic_name, options);
        _reason_codes.push_back(0);
        lg->info("{}: subscribed to topic:'{}'", pClient->GetIP(), topic_name);
    }
    return mqtt_err::ok;
}