#include "mqtt_protocol.h"

using namespace mqtt_protocol;

ConnectVH::ConnectVH() : prot_name_len(0), version(0), conn_flags(0), alive(0){
    memset(name, 0, 4);
};

uint32_t ConnectVH::GetSize() const {
    return sizeof(prot_name_len) + sizeof(version) + 4 + sizeof(conn_flags) + sizeof(alive);
}

void ConnectVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    auto tmp = htons(prot_name_len);
    memcpy(dst_buf + local_offset, &tmp, sizeof(prot_name_len));
    local_offset += sizeof(prot_name_len);
    memcpy(dst_buf + local_offset, name, 4 + sizeof(version) + sizeof(conn_flags));
    local_offset += 4 + sizeof(version) + sizeof(conn_flags);
    tmp = htons(alive);
    memcpy(dst_buf + local_offset, &tmp, sizeof(alive));
    offset += local_offset;
}

void ConnectVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    uint32_t local_offset = 0;
    memcpy(&prot_name_len, buf + local_offset, sizeof(prot_name_len));
    auto tmp = ntohs(prot_name_len);
    prot_name_len = tmp;
    local_offset += sizeof(prot_name_len);
    memcpy(name, buf + local_offset, 4);
    local_offset += 4;
    memcpy(&version, buf + local_offset, sizeof(version));
    local_offset += sizeof(version);
    memcpy(&conn_flags, buf + local_offset, sizeof(conn_flags));
    local_offset += sizeof(conn_flags);
    memcpy(&alive, buf + local_offset, sizeof(alive));
    local_offset += sizeof(alive);
    tmp = ntohs(alive);
    alive = tmp;
    offset += GetSize();
}

ConnactVH::ConnactVH() :conn_acknowledge_flags(0), reason_code(0) {}

ConnactVH::ConnactVH(uint8_t _caf, uint8_t _rc) : conn_acknowledge_flags(_caf), reason_code(_rc) {}

uint32_t ConnactVH::GetSize() const {
    return sizeof(conn_acknowledge_flags) + sizeof(reason_code);
}

void ConnactVH::Serialize(uint8_t *dst_buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    memcpy(dst_buf, &conn_acknowledge_flags, sizeof(conn_acknowledge_flags));
    local_offset += sizeof(conn_acknowledge_flags);
    memcpy(dst_buf + local_offset, &reason_code, sizeof(reason_code));
    local_offset += sizeof(reason_code);
    offset += local_offset;
}

void ConnactVH::ReadFromBuf(const uint8_t *buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    memcpy(&conn_acknowledge_flags, buf, sizeof(conn_acknowledge_flags));
    local_offset += sizeof(conn_acknowledge_flags);
    memcpy(&reason_code, buf + local_offset, sizeof(reason_code));
    local_offset += sizeof(reason_code);
    offset += local_offset;
}

DisconnectVH::DisconnectVH(uint8_t _reason_code) : reason_code(_reason_code){}

uint32_t DisconnectVH::GetSize() const{
    return sizeof(reason_code);
}

void DisconnectVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    memcpy(dst_buf, &reason_code, sizeof(reason_code));
    offset += sizeof(reason_code);
}

void DisconnectVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    memcpy(&reason_code, buf, sizeof(reason_code));
    offset += sizeof(reason_code);
}

PublishVH::PublishVH(bool is_packet_id_present, const shared_ptr<uint8_t>& buf, uint32_t &offset) : topic_name(ConvertToHost2Bytes(buf.get()), buf.get() + sizeof(uint16_t)), packet_id(0){
    offset = topic_name.Size();
    if (is_packet_id_present){
        packet_id = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(packet_id);
    }
    uint32_t property_size;
    p_chain.Create(buf.get() + offset, property_size);
    offset += property_size;
}

PublishVH::PublishVH(MqttStringEntity &_topic_name, uint16_t _packet_id, MqttPropertyChain &_p_chain) : topic_name(_topic_name), packet_id(_packet_id), p_chain(_p_chain){}
PublishVH::PublishVH(PublishVH &&_vh) noexcept : topic_name(std::move(_vh.topic_name)), packet_id(_vh.packet_id), p_chain(std::move(_vh.p_chain)){}
PublishVH::PublishVH(const PublishVH &_vh) : topic_name(_vh.topic_name), packet_id(_vh.packet_id), p_chain(_vh.p_chain){}

PublishVH& PublishVH::operator=(const PublishVH &_vh) {
    topic_name = _vh.topic_name;
    packet_id = _vh.packet_id;
    p_chain = _vh.p_chain;
    return *this;
}

PublishVH& PublishVH::operator=(PublishVH &&_vh) noexcept {
    topic_name = std::move(_vh.topic_name);
    packet_id = _vh.packet_id;
    p_chain = std::move(_vh.p_chain);
    return *this;
}

uint32_t PublishVH::GetSize() const {
    return topic_name.Size() + sizeof(packet_id) + p_chain.GetSize();
}

void PublishVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    topic_name.Serialize(dst_buf, local_offset);
    offset += local_offset;
    if (packet_id != 0){
        auto tmp = ntohs(packet_id);
        memcpy(dst_buf + local_offset, &tmp, sizeof(tmp));
        offset += sizeof(tmp);
    }
    p_chain.Serialize(dst_buf, offset);
}

void PublishVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    //todo
    (void) buf;
    (void) offset;
}

//--------------------------SubscribeVH-------------------------------------------
SubscribeVH::SubscribeVH(const shared_ptr<uint8_t>& buf, uint32_t &offset){
    offset = 0;
    packet_id = ConvertToHost2Bytes(buf.get());
    offset += sizeof(packet_id);

    uint32_t property_size;
    p_chain.Create(buf.get() + offset, property_size);
    offset += property_size;
}

SubscribeVH::SubscribeVH(uint16_t _packet_id, MqttPropertyChain &_p_chain) : packet_id(_packet_id), p_chain(_p_chain){}
SubscribeVH::SubscribeVH(const SubscribeVH &_vh) : packet_id(_vh.packet_id), p_chain(_vh.p_chain){}
SubscribeVH::SubscribeVH(SubscribeVH &&_vh) noexcept : packet_id(_vh.packet_id), p_chain(std::move(_vh.p_chain)){}

SubscribeVH& SubscribeVH::operator =(const SubscribeVH &_vh){
    packet_id = _vh.packet_id;
    p_chain = _vh.p_chain;
    return *this;
}

SubscribeVH& SubscribeVH::operator =(SubscribeVH &&_vh) noexcept{
    packet_id = _vh.packet_id;
    p_chain = std::move(_vh.p_chain);
    return *this;
}

[[nodiscard]] uint32_t SubscribeVH::GetSize() const {
    return sizeof(packet_id) + + p_chain.GetSize();
}

void SubscribeVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    if (packet_id != 0){
        auto tmp = ntohs(packet_id);
        memcpy(dst_buf, &tmp, sizeof(tmp));
        offset += sizeof(tmp);
    }
    p_chain.Serialize(dst_buf, offset);
}

void SubscribeVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    p_chain.Clear();
    uint32_t local_offset = 0;
    memcpy(&packet_id, buf + local_offset, sizeof(packet_id));
    auto tmp = ntohs(packet_id);
    packet_id = tmp;
    local_offset += sizeof(packet_id);
    offset += local_offset;
    uint32_t property_len = 0;
    p_chain.Create(buf + local_offset, property_len);
    offset += property_len;
}

uint32_t VariableHeader::GetSize() const {
    return v_header->GetSize();
}

void VariableHeader::Serialize(uint8_t* dst_buf, uint32_t &offset){
    v_header->Serialize(dst_buf, offset);
}

void VariableHeader::ReadFromBuf(const uint8_t* buf, uint32_t &offset) {
    v_header->ReadFromBuf(buf, offset);
}
