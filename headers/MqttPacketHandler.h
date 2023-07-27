
#ifndef MQTTPACKETHANDLER_H
#define MQTTPACKETHANDLER_H

#include "mqtt_protocol.h"
#include "client.h"
#include "command.h"
#include "topic_storage.h"


class Broker;

class IMqttPacketHandler{
public:
    IMqttPacketHandler(uint8_t _type);
    uint8_t GetType() const;
    virtual int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) = 0;
protected:
    uint8_t type;
};

class MqttConnectPacketHandler : public IMqttPacketHandler{
public:
    MqttConnectPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPublishPacketHandler : public IMqttPacketHandler{
public:
    MqttPublishPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttSubscribePacketHandler : public IMqttPacketHandler{
public:
    MqttSubscribePacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPubAckPacketHandler : public IMqttPacketHandler{
public:
    MqttPubAckPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttDisconnectPacketHandler : public IMqttPacketHandler{
public:
    MqttDisconnectPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPingPacketHandler : public IMqttPacketHandler{
public:
    MqttPingPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPacketHandler{
public:
    void AddHandler(IMqttPacketHandler *);
    int HandlePacket(const FixedHeader& f_header, const shared_ptr<uint8_t> &data, Broker *broker, int fd);

private:
    list<IMqttPacketHandler *> handlers;
};

#endif //MQTTPACKETHANDLER_H
