#include "client.h"

Client::Client(string _ip) : ip(std::move(_ip)), client_id(string{}), state(0), flags(0), alive(0) {
    time_t _cur_time;
    time(&_cur_time);
    SetPacketLastTime(_cur_time);
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
    return flags & 0x18 >> 3;
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

string Client::GetID() const {
    return client_id.GetString();
}

string& Client::GetIP(){
    return ip;
}

time_t Client::GetPacketLastTime() const {
    return time_last_packet;
}
void Client::SetPacketLastTime(time_t _cur_time){
    time_last_packet = _cur_time;
}

uint16_t Client::GetAlive() const{
    return alive;
}

void  Client::AddSubscription(const string &_topic_name, uint8_t options){
    subscribed_topics.insert(make_pair(_topic_name, options));
}

unordered_map<string, uint8_t>::const_iterator Client::CFind(const string &_topic_name) {
    return subscribed_topics.find(_topic_name);
}

unordered_map<string, uint8_t>::const_iterator Client::CEnd(){
    return subscribed_topics.cend();
}

bool Client::MyTopic(const string &_topic){
    if (subscribed_topics.find(_topic) != subscribed_topics.end()) return true;
    return false;
}