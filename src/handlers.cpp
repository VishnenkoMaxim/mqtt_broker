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
    uint32_t properties_len = 0;
    uint8_t size = 0;

    uint8_t stat = DeCodeVarInt(buf.get() + offset, properties_len, size);
    if (stat != mqtt_err::ok) {
        lg->error("Read DeCodeVarInt error");
        return stat;
    }
    lg->debug("properties len:{}", properties_len);
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
    auto id = CreateMqttStringEntity(buf.get() + offset, id_len);
    if (id != nullptr){
        pClient->SetID(id->GetString());
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
    lg->debug("topic name:'{}' packet_id:{} property_count:{}", p_vh.topic_name.GetString(), p_vh.packet_id, p_vh.p_chain.Count());
    vh = std::move(p_vh);

    //read Payload
    lg->debug("message:{}", string((char *)(buf.get() + offset), fh.remaining_len - offset));
    message = MqttBinaryDataEntity(fh.remaining_len - offset, buf.get() + offset);

    return mqtt_err::ok;
}

int HandleMqttSubscribe([[maybe_unused]] const FixedHeader &fh, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg, SubscribeVH &vh){
    lg->debug("HandleMqttSubscribe");
    uint32_t offset = 0;

    SubscribeVH s_vh(buf, offset);
    vh = std::move(s_vh);

    return mqtt_err::ok;
}