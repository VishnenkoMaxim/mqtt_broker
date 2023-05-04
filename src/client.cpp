#include "client.h"

void Client::SetConnFlags(uint8_t _flags){
    flags = _flags;
}

void Client::SetConnAlive(uint16_t _alive){
    alive = _alive;
}

bool Client::isUserNameFlag() const {
    return flags & 0x80;
}

bool Client::isPwdFlag() const {
    return flags & 0x40;
}

bool Client::isWillRetFlag() const {
    return flags & 0x20;
}

uint8_t Client::WillQoSFlag() const {
    return flags & 0x18;
}

bool Client::isWillFlag() const {
    return flags & 0x4;
}
bool Client::isCleanFlag() const {
    return flags & 0x2;
}