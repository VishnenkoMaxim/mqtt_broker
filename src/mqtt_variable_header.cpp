#include "mqtt_variable_header.h"

ConnectVH1::ConnectVH1() : prot_name_len(0), version(0), conn_flags(0), alive(0){
    memset(name, 0, 4);
};

uint32_t ConnectVH1::GetSize() const {
    return sizeof(prot_name_len) + sizeof(version) + 4 + sizeof(conn_flags) + sizeof(alive);
}

void ConnectVH1::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t local_offset = 0;
    uint16_t tmp = htons(prot_name_len);
    memcpy(dst_buf + local_offset, &tmp, sizeof(prot_name_len));
    local_offset += sizeof(prot_name_len);
    memcpy(dst_buf + local_offset, name, 4 + sizeof(version) + sizeof(conn_flags));
    local_offset += 4 + sizeof(version) + sizeof(conn_flags);
    tmp = htons(alive);
    memcpy(dst_buf + local_offset, &tmp, sizeof(alive));
    offset += local_offset;
}

void ConnectVH1::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    memcpy((void*)this, buf, sizeof(ConnectVH1));
    uint16_t tmp = ntohs(prot_name_len);
    prot_name_len = tmp;
    tmp = ntohs(alive);
    alive = tmp;
    offset += GetSize();
}

ConnactVH1::ConnactVH1(uint8_t _caf, uint8_t _rc) {
    conn_acknowledge_flags = _caf;
    reason_code = _rc;
}

uint32_t ConnactVH1::GetSize() const {
    return sizeof(conn_acknowledge_flags) + sizeof(reason_code);
}

void ConnactVH1::Serialize(uint8_t *dst_buf, uint32_t &offset) {
    memcpy(dst_buf, &conn_acknowledge_flags, sizeof(ConnactVH1));
    offset += sizeof(ConnactVH1);
}

void ConnactVH1::ReadFromBuf(const uint8_t *buf, uint32_t &offset) {
    uint32_t local_offset = 0;
    memcpy(&conn_acknowledge_flags, buf, sizeof(conn_acknowledge_flags));
    local_offset += sizeof(conn_acknowledge_flags);
    memcpy(&reason_code, buf + local_offset, sizeof(reason_code));
    local_offset += sizeof(reason_code);
    offset += local_offset;
}

DisconnectVH1::DisconnectVH1(uint8_t _reason_code) : reason_code(_reason_code){}

uint32_t DisconnectVH1::GetSize() const{
    return sizeof(reason_code);
}

void DisconnectVH1::Serialize(uint8_t* dst_buf, uint32_t &offset){
    memcpy(dst_buf, &reason_code, sizeof(reason_code));
    offset += sizeof(reason_code);
}

void DisconnectVH1::ReadFromBuf(const uint8_t* buf, uint32_t &offset){
    memcpy(&reason_code, buf, sizeof(reason_code));
    offset += sizeof(reason_code);
}

uint32_t VariableHeader1::GetSize() const {
    return v_header->GetSize();
}

void VariableHeader1::Serialize(uint8_t* dst_buf, uint32_t &offset){
    v_header->Serialize(dst_buf, offset);
}

void VariableHeader1::ReadFromBuf(const uint8_t* buf, uint32_t &offset) {
    v_header->ReadFromBuf(buf, offset);
}

