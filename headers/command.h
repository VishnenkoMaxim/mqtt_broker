#ifndef MQTT_BROKER_SENDER_H
#define MQTT_BROKER_SENDER_H

#include <iostream>
#include <memory>
#include <string>
#include <memory>
#include <cstring>
#include <ctime>
#include <queue>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "functions.h"

using namespace std;

class Writer {
public:
    Writer()= default;

    int Write(int fd, const shared_ptr<uint8_t>& buf, const uint32_t buf_len);
};

class ICommand{
public:
    virtual void Execute() = 0;
    virtual ~ICommand() = default;
};

class WriteCommand: public ICommand{
protected:
    shared_ptr<Writer> stream;
    explicit WriteCommand(shared_ptr<Writer> _stream): stream(std::move(_stream)) {}
public:
    ~WriteCommand() override = default;
};

class FdWriteCommand : public WriteCommand{
private:
    tuple<uint32_t, shared_ptr<uint8_t>> cmd;
    static int count;
    int fd;
public:
    FdWriteCommand(shared_ptr<Writer> _writer, const int _fd, tuple<uint32_t, shared_ptr<uint8_t>> _cmd) : WriteCommand(std::move(_writer)), cmd(std::move(_cmd)), fd(_fd) {
        count++;
    }
    void Execute() override;
};

class Commands{
private:
    vector<shared_ptr<ICommand>> commands;
    unsigned short N;
    int state;
    shared_ptr<Writer> stream;
public:
    explicit Commands(const unsigned short n) : N(n), state(0) {
        stream = std::make_shared<Writer>();
    }

    void Add(int fd, tuple<uint32_t, shared_ptr<uint8_t>> _cmd);
    void Execute();
    void PostActions();
    void Exit();
};

#endif //MQTT_BROKER_SENDER_H
