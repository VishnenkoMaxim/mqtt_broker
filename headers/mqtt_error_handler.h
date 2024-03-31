#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include "mqtt_protocol.h"
#include "client.h"
#include "command.h"
#include "topic_storage.h"

class IMqttErrorHandler{
public:
    explicit IMqttErrorHandler(int _err);
    int GetErr();
    virtual void HandleError(Broker& broker, int fd) = 0;

    virtual ~IMqttErrorHandler() = default;
protected:
    int error;
};

class MqttDisconnectErr : public IMqttErrorHandler{
public:
    MqttDisconnectErr();
    void HandleError(Broker& broker, int fd) override;
};

class MqttProtocolVersionErr : public IMqttErrorHandler{
public:
    MqttProtocolVersionErr();
    void HandleError(Broker& broker, int fd) override;
};

class MqttDuplicateIDErr : public IMqttErrorHandler{
public:
    MqttDuplicateIDErr();
    void HandleError(Broker& broker, int fd) override;
};

class MqttHandleErr : public IMqttErrorHandler{
public:
    MqttHandleErr();
    void HandleError(Broker& broker, int fd) override;
};

//-----------------------------------------------------------------------------------
class MqttErrorHandler{
public:
    void AddErrorHandler(std::shared_ptr<IMqttErrorHandler>);
    void HandleError(int error, Broker& broker, int fd);
private:
    std::list<std::shared_ptr<IMqttErrorHandler>> handlers;
};
