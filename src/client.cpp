#include "client.h"

Client::Client(string _ip) : ip(std::move(_ip)), client_id(string{}), state(0), flags(0), alive(0){
    //
}

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
void Client::SetID(const string& _id){
    client_id = _id;
}

string Client::GetID(){
    return client_id.GetString();
}

string& Client::GetIP(){
    return ip;
}