#include "client.h"

using namespace std;

Client::Client(string _ip) : ip(std::move(_ip)), client_id(string{}), flags(0), alive(0), packet_id_gen(1) {
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
    return (flags & 0x18) >> 3;
}

bool Client::isWillFlag() const {
    return flags & 0x4;
}
bool Client::isCleanFlag() const {
    return flags & 0x2;
}

bool Client::isRandomID() const {
    return id_was_random_generated;
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

bool Client::MyTopic(const string &_topic, uint8_t& options){
    auto it = subscribed_topics.find(_topic);
    if (it != subscribed_topics.end()) {
        options = it->second;
        return true;
    }
    return false;
}

uint16_t Client::GenPacketID(){
    return packet_id_gen++;
}

uint8_t Client::DelSubscription(const string &_topic_name){
    return subscribed_topics.erase(_topic_name);
}

void Client::SetRandomID(){
    id_was_random_generated = true;
}

uint8_t Client::GetClientMQTTVersion() const noexcept{
    return mqtt_version;
}

void Client::SetClientMQTTVersion(const uint8_t version) noexcept {
    mqtt_version = version;
}
