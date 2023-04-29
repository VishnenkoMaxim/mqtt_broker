#include "conn_properties.h"

#include <utility>

void ConnProperties::SetFlags(const uint8_t _flags) {
    flags = _flags;
}

bool ConnProperties::isUserNameFlag() const {
    return flags & 0x80;
}

bool ConnProperties::isPwdFlag() const {
    return flags & 0x40;
}

bool ConnProperties::isWillRetFlag() const {
    return flags & 0x20;
}

uint8_t ConnProperties::WillQoSFlag() const {
    return flags & 0x18;
}

bool ConnProperties::isWillFlag() const {
    return flags & 0x4;
}
bool ConnProperties::isCleanFlag() const {
    return flags & 0x2;
}

void ConnProperties::AddProperty(shared_ptr<MqttProperty> property){
    properties.AddProperty(property);
}

