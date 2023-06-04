#include "mqtt_protocol.h"

using namespace mqtt_protocol;

FixedHeader::FixedHeader() noexcept{
    first = 0;
    remaining_len = 0;
}

FixedHeader::FixedHeader(uint8_t _first) noexcept{
    first = _first;
    remaining_len = 0;
}

[[nodiscard]] bool    FixedHeader::isDUP() const { return first & 0x08; }
[[nodiscard]] uint8_t FixedHeader::QoS() const {return first & 0x06;}
[[nodiscard]] bool    FixedHeader::isRETAIN() const {return first & 0x01;}
[[nodiscard]] uint8_t FixedHeader::GetType() const { return first>>4;}
[[nodiscard]] uint8_t FixedHeader::GetFlags() const { return first & 0x0F; }

[[nodiscard]] bool FixedHeader::isIdentifier() const {
    uint8_t type = GetType();
    if ((type >= PUBACK && type <= UNSUBACK) || (type == PUBLISH && QoS() > 0)){
        return true;
    }
    return false;
}

[[nodiscard]] bool FixedHeader::isProperties() const {
    uint8_t type = GetType();
    if ((type >= CONNECT && type <= UNSUBACK) || (type == DISCONNECT) || (type == AUTH)){
        return true;
    }
    return false;
}

void FixedHeader::SetRemainingLen(uint32_t _len){
    remaining_len = _len;
}

void FixedHeader::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint8_t size = 0;
    memcpy(dst_buf, &first, sizeof(first));
    CodeVarInt(dst_buf + sizeof(first), remaining_len, size);
    offset += sizeof(first) + size;
}

uint32_t FixedHeader::Size() const{
    return GetVarIntSize(remaining_len) + sizeof(first);
}
