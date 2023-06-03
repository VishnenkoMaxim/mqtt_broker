//#include "mqtt_protocol.h"
//
//mqtt_protocol::MqttSubscription::MqttSubscription(const uint8_t * _data, uint32_t &offset) : MqttStringEntity(""){
//    uint16_t len = ConvertToHost2Bytes(_data);
//    offset += sizeof(len);
//
//    MqttStringEntity::operator=(string((char *) (_data + sizeof(len)), len));
//    offset += len;
//    memcpy(&options, _data + sizeof(len) + len, sizeof(options));
//}
//
//mqtt_protocol::MqttSubscription::MqttSubscription(const MqttSubscription &_sub) : MqttStringEntity(_sub.GetString()), options(_sub.options){}
//mqtt_protocol::MqttSubscription::MqttSubscription(const MqttSubscription &&_sub) noexcept : MqttStringEntity(std::move(_sub)), options(_sub.options) {}
//
//mqtt_protocol::MqttSubscription& mqtt_protocol::MqttSubscription::operator = (const MqttSubscription &_sub){
//    options = _sub.options;
//    MqttStringEntity::operator=(_sub.GetString());
//    return *this;
//}
//
//string mqtt_protocol::MqttSubscription::GetName() const{
//    return GetString();
//}
//
//uint8_t mqtt_protocol::MqttSubscription::GetOptions() const {
//    return options;
//}