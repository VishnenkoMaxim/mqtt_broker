#include "mqtt_broker.h"

int HandleMqttConnect(shared_ptr<Client>& pClient, const shared_ptr<uint8_t>& buf, shared_ptr<logger>& lg){
    ConnectVH con_vh;
    uint32_t offset = 0;

    con_vh.CopyFromNet(buf.get());
    offset += sizeof(con_vh);
    char name_tmp[5] = "";
    memcpy(name_tmp, con_vh.name, 4);
    lg->debug(
            "Connect VH: len:{} name:{} version:{} flags:{:X} alive:{}",
            con_vh.prot_name_len, name_tmp, con_vh.version, con_vh.conn_flags,
            con_vh.alive);

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