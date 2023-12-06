#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include "mqtt_protocol.h"
#include "client.h"
#include "command.h"
#include "topic_storage.h"

class Broker;

class IMqttPacketHandler{
public:
    explicit IMqttPacketHandler(uint8_t _type);
    [[nodiscard]] uint8_t GetType() const;
    virtual int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) = 0;
protected:
    uint8_t type;
};

class MqttConnectPacketHandler : public IMqttPacketHandler{
public:
    MqttConnectPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPublishPacketHandler : public IMqttPacketHandler{
public:
    MqttPublishPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttSubscribePacketHandler : public IMqttPacketHandler{
public:
    MqttSubscribePacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPubAckPacketHandler : public IMqttPacketHandler{
public:
    MqttPubAckPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttDisconnectPacketHandler : public IMqttPacketHandler{
public:
    MqttDisconnectPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPingPacketHandler : public IMqttPacketHandler{
public:
    MqttPingPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttUnsubscribePacketHandler : public IMqttPacketHandler{
public:
    MqttUnsubscribePacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPubRelPacketHandler : public IMqttPacketHandler{
public:
    MqttPubRelPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPubRecPacketHandler : public IMqttPacketHandler{
public:
    MqttPubRecPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPubCompPacketHandler : public IMqttPacketHandler{
public:
    MqttPubCompPacketHandler();
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd) override;
};

class MqttPacketHandler{
public:
    void AddHandler(IMqttPacketHandler *);
    int HandlePacket(const FixedHeader& f_header, const std::shared_ptr<uint8_t> &data, Broker *broker, int fd);

private:
    std::list<IMqttPacketHandler *> handlers;
};
