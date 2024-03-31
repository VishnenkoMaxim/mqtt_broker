#include <utility>

#include "mqtt_protocol.h"

using namespace mqtt_protocol;
using namespace std;

ConnectVH::ConnectVH() : prot_name_len(0), version(0), conn_flags(0), alive(0){
    memset(name, 0, 4);
}

ConnectVH::ConnectVH(uint8_t _flags, uint16_t _alive) : prot_name_len(4), version(5), conn_flags(_flags), alive(_alive)  {
    memset(name, 0, 4);
    strncpy(name, "MQTT", 4);
}

uint32_t ConnectVH::GetSize() const {
    return sizeof(prot_name_len) + sizeof(version) + 4 + sizeof(conn_flags) + sizeof(alive);
}

void ConnectVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    uint16_t tmp = htons(prot_name_len);
    memcpy(dst_buf + local_offset, &tmp, sizeof(prot_name_len));
    local_offset += sizeof(prot_name_len);
    memcpy(dst_buf + local_offset, name, 4 + sizeof(version) + sizeof(conn_flags));
    local_offset += 4 + sizeof(version) + sizeof(conn_flags);
    tmp = htons(alive);
    memcpy(dst_buf + local_offset, &tmp, sizeof(alive));
    local_offset += sizeof(alive);
    offset += local_offset;
}

void ConnectVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    uint32_t local_offset = 0;
    memcpy(&prot_name_len, buf + local_offset, sizeof(prot_name_len));
    uint16_t tmp = ntohs(prot_name_len);
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
ConnactVH::ConnactVH(uint8_t _caf, uint8_t _rc, MqttPropertyChain &_properties) : conn_acknowledge_flags(_caf), reason_code(_rc), p_chain(_properties) {}
ConnactVH::ConnactVH(uint8_t _caf, uint8_t _rc, MqttPropertyChain &&_properties) : conn_acknowledge_flags(_caf), reason_code(_rc), p_chain(std::move(_properties)) {}

uint32_t ConnactVH::GetSize() const {
    return sizeof(conn_acknowledge_flags) + sizeof(reason_code) + p_chain.GetSize() + GetVarIntSize(p_chain.GetSize());
}

void ConnactVH::Serialize(uint8_t *dst_buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    memcpy(dst_buf, &conn_acknowledge_flags, sizeof(conn_acknowledge_flags));
    local_offset += sizeof(conn_acknowledge_flags);
    memcpy(dst_buf + local_offset, &reason_code, sizeof(reason_code));
    local_offset += sizeof(reason_code);
    offset += local_offset;

    p_chain.Serialize(dst_buf + local_offset, offset);
}

void ConnactVH::ReadFromBuf(const uint8_t *buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    memcpy(&conn_acknowledge_flags, buf, sizeof(conn_acknowledge_flags));
    local_offset += sizeof(conn_acknowledge_flags);
    memcpy(&reason_code, buf + local_offset, sizeof(reason_code));
    local_offset += sizeof(reason_code);
    offset += local_offset;
    uint32_t size;
    p_chain.Create(buf + local_offset, size);
    offset += size;
}

DisconnectVH::DisconnectVH(uint8_t _reason_code,  MqttPropertyChain &_properties) : reason_code(_reason_code), p_chain(_properties){}
DisconnectVH::DisconnectVH(uint8_t _reason_code,  MqttPropertyChain &&_properties) : reason_code(_reason_code), p_chain(std::move(_properties)){}

uint32_t DisconnectVH::GetSize() const{
    return sizeof(reason_code) + GetVarIntSize(p_chain.GetSize()) + p_chain.GetSize();
}

void DisconnectVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    memcpy(dst_buf, &reason_code, sizeof(reason_code));
    local_offset += sizeof(reason_code);

    p_chain.Serialize(dst_buf + local_offset, local_offset);
    offset += local_offset;
}

void DisconnectVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    memcpy(&reason_code, buf, sizeof(reason_code));
    offset += sizeof(reason_code);

    uint32_t size;
    p_chain.Create(buf + sizeof(reason_code), size);
    offset += size;
}

PublishVH::PublishVH(bool is_packet_id_present, const std::shared_ptr<uint8_t>& buf, uint32_t &offset) : topic_name(ConvertToHost2Bytes(buf.get()), buf.get() + sizeof(uint16_t)), packet_id(0){
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
PublishVH::PublishVH(MqttStringEntity &_topic_name, uint16_t _packet_id, MqttPropertyChain &&_p_chain) : topic_name(_topic_name), packet_id(_packet_id), p_chain(std::move(_p_chain)){}
PublishVH::PublishVH(MqttStringEntity &&_topic_name, uint16_t _packet_id, MqttPropertyChain &&_p_chain) : topic_name(std::move(_topic_name)), packet_id(_packet_id), p_chain(std::move(_p_chain)) {}
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
    return topic_name.Size() + sizeof(packet_id) + GetVarIntSize(p_chain.GetSize()) + p_chain.GetSize();
}

void PublishVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    topic_name.Serialize(dst_buf, local_offset);

    offset += local_offset;
    if (packet_id != 0){
        uint16_t tmp = ntohs(packet_id);
        memcpy(dst_buf + local_offset, &tmp, sizeof(tmp));
        offset += sizeof(tmp);
        local_offset += sizeof(tmp);
    }
    p_chain.Serialize(dst_buf + local_offset, offset);
}

void PublishVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    //todo
    (void) buf;
    (void) offset;
}

//--------------------------SubscribeVH-------------------------------------------
SubscribeVH::SubscribeVH(const std::shared_ptr<uint8_t>& buf, uint32_t &offset){
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
    return sizeof(packet_id) + p_chain.GetSize();
}

void SubscribeVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;

    if (packet_id != 0){
        uint16_t tmp = ntohs(packet_id);
        memcpy(dst_buf, &tmp, sizeof(tmp));
        offset += sizeof(tmp);
        local_offset += sizeof(tmp);
    }

    p_chain.Serialize(dst_buf + local_offset, offset);
}

void SubscribeVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    p_chain.Clear();
    uint32_t local_offset = 0;
    memcpy(&packet_id, buf + local_offset, sizeof(packet_id));
    uint16_t tmp = ntohs(packet_id);
    packet_id = tmp;
    local_offset += sizeof(packet_id);
    offset += local_offset;
    uint32_t property_len = 0;
    p_chain.Create(buf + local_offset, property_len);
    offset += property_len;
}

//----------------------SubackVH-------------------------------
SubackVH::SubackVH(uint16_t _packet_id, MqttPropertyChain _p_chain, vector<uint8_t>& _reason_codes) : packet_id(_packet_id), p_chain(std::move(_p_chain)), reason_codes(_reason_codes){}

uint32_t SubackVH::GetSize() const{
    return sizeof(packet_id) + p_chain.GetSize() + GetVarIntSize(p_chain.GetSize()) + reason_codes.size();
}

void SubackVH::Serialize(uint8_t* dst_buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    auto tmp = htons(packet_id);
    memcpy(dst_buf, &tmp, sizeof(packet_id));
    local_offset += sizeof(packet_id);

    p_chain.Serialize(dst_buf + local_offset, local_offset);
    for(unsigned int i=0; i<reason_codes.size(); i++){
        memcpy(dst_buf + local_offset, &reason_codes[i], sizeof(uint8_t));
        local_offset++;
    }
    offset += local_offset;
}

void SubackVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    //todo
    (void)buf;
    (void) offset;
}

PubackVH::PubackVH(uint16_t _packet_id, uint8_t _reason_code, MqttPropertyChain  _p_chain) : packet_id(_packet_id), reason_code(_reason_code), p_chain(std::move(_p_chain)){}

[[nodiscard]] uint32_t PubackVH::GetSize() const{
    return sizeof(packet_id) + sizeof(reason_code) + p_chain.GetSize() + GetVarIntSize(p_chain.GetSize());
}

void PubackVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    uint16_t tmp = htons(packet_id);
    memcpy(dst_buf, &tmp, sizeof(packet_id));
    local_offset += sizeof(packet_id);

    memcpy(dst_buf + local_offset, &reason_code, sizeof(reason_code));
    local_offset += sizeof(reason_code);

    p_chain.Serialize(dst_buf + local_offset, local_offset);
    offset += local_offset;
}

void PubackVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    p_chain.Clear();
    uint32_t local_offset = 0;

    memcpy(&packet_id, buf + local_offset, sizeof(packet_id));
    uint16_t tmp = ntohs(packet_id);
    packet_id = tmp;
    local_offset += sizeof(packet_id);

    offset += local_offset;
}

//-----------------------------UnsubscribeVH---------------------------------
UnsubscribeVH::UnsubscribeVH(uint16_t _packet_id, MqttPropertyChain _p_chain) : packet_id(_packet_id), p_chain(std::move(_p_chain)){}

[[nodiscard]] uint32_t UnsubscribeVH::GetSize() const{
    return sizeof(packet_id) + p_chain.GetSize() + GetVarIntSize(p_chain.GetSize());
}

void UnsubscribeVH::Serialize(uint8_t* dst_buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    uint16_t tmp = htons(packet_id);
    memcpy(dst_buf, &tmp, sizeof(packet_id));
    local_offset += sizeof(packet_id);

    p_chain.Serialize(dst_buf + local_offset, local_offset);
    offset += local_offset;
}

void UnsubscribeVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    p_chain.Clear();
    uint32_t local_offset = 0;

    memcpy(&packet_id, buf + local_offset, sizeof(packet_id));
    uint16_t tmp = ntohs(packet_id);
    packet_id = tmp;
    local_offset += sizeof(packet_id);
    offset += local_offset;

    uint32_t property_len = 0;
    p_chain.Create(buf + local_offset, property_len);
    offset += property_len;
}

//--------------------------Pubrec Pubrel Pubcomp----------------------------------------------------------
TypicalVH::TypicalVH(uint16_t _packet_id, uint8_t _reason_code, MqttPropertyChain _p_chain) : PubackVH(_packet_id, _reason_code, std::move(_p_chain)){}

//---------------------------------UnsubAckVH--------------------------------------------------
UnsubAckVH::UnsubAckVH(uint16_t _packet_id, MqttPropertyChain _p_chain, vector<uint8_t>& _reason_codes) : packet_id(_packet_id), p_chain(std::move(_p_chain)), reason_codes(_reason_codes){}

[[nodiscard]] uint32_t UnsubAckVH::GetSize() const {
    return sizeof(packet_id) + p_chain.GetSize() + GetVarIntSize(p_chain.GetSize()) + reason_codes.size();
}

void UnsubAckVH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    uint16_t tmp = htons(packet_id);
    memcpy(dst_buf, &tmp, sizeof(packet_id));
    local_offset += sizeof(packet_id);

    p_chain.Serialize(dst_buf + local_offset, local_offset);

    for(const auto& reason_code : reason_codes){
        memcpy(dst_buf + local_offset, &reason_code, sizeof(uint8_t));
        local_offset++;
    }
    offset += local_offset;
}

void UnsubAckVH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    //todo
    (void)buf;
    (void) offset;
}


//---------------------------V3--------------------
TypicalV3VH::TypicalV3VH(uint16_t _packet_id) : packet_id(_packet_id){}

uint32_t TypicalV3VH::GetSize() const {
    return sizeof(packet_id);
}

void TypicalV3VH::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    uint16_t tmp = htons(packet_id);
    memcpy(dst_buf, &tmp, sizeof(packet_id));
    local_offset += sizeof(packet_id);
    offset += local_offset;
}

void TypicalV3VH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    uint32_t local_offset = 0;

    memcpy(&packet_id, buf + local_offset, sizeof(packet_id));
    uint16_t tmp = ntohs(packet_id);
    packet_id = tmp;
    local_offset += sizeof(packet_id);

    offset += local_offset;
}

PublishV3VH::PublishV3VH() : topic_name(""), packet_id(0){}

PublishV3VH::PublishV3VH(bool is_packet_id_present, const std::shared_ptr<uint8_t>& buf, uint32_t &offset) : topic_name(ConvertToHost2Bytes(buf.get()), buf.get() + 																												sizeof(uint16_t)), packet_id(0){
	offset = topic_name.Size();
    if (is_packet_id_present){
        packet_id = ConvertToHost2Bytes(buf.get() + offset);
        offset += sizeof(packet_id);
    }
}

PublishV3VH::PublishV3VH(MqttStringEntity &_topic_name, uint16_t _packet_id) : topic_name(_topic_name), packet_id(_packet_id){}
PublishV3VH::PublishV3VH(MqttStringEntity &&_topic_name, uint16_t _packet_id) : topic_name(std::move(_topic_name)), packet_id(_packet_id){}

uint32_t PublishV3VH::GetSize() const {
	return topic_name.Size() + sizeof(packet_id);
}

void PublishV3VH::Serialize(uint8_t* dst_buf, uint32_t &offset){
 	uint32_t local_offset = 0;
    topic_name.Serialize(dst_buf, local_offset);
    offset += local_offset;
    if (packet_id != 0){
        uint16_t tmp = ntohs(packet_id);
        memcpy(dst_buf + local_offset, &tmp, sizeof(tmp));
        offset += sizeof(tmp);
    }
}

void PublishV3VH::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
	//todo
    (void) buf;
    (void) offset;
}

//---------------------------------VariableHeader-----------------------------------------------
uint32_t VariableHeader::GetSize() const {
    return v_header->GetSize();
}

void VariableHeader::Serialize(uint8_t* dst_buf, uint32_t &offset){
    v_header->Serialize(dst_buf, offset);
}

void VariableHeader::ReadFromBuf(const uint8_t* buf, uint32_t &offset) {
    v_header->ReadFromBuf(buf, offset);
}
