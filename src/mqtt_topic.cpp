#include "mqtt_protocol.h"

mqtt_protocol::MqttTopic::MqttTopic(uint16_t _id, string _name, uint16_t _len, const uint8_t * _data) : id(_id), name(_name), data(_len, _data){
    //
}
