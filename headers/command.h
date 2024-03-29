#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include <iostream>
#include <memory>
#include <string>
#include <memory>
#include <cstring>
#include <ctime>
#include <queue>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "functions.h"

class Writer {
public:
    Writer()= default;
    int Write(int fd, const std::shared_ptr<uint8_t>& buf, uint32_t buf_len);
};

class ICommand{
public:
    virtual void Execute() = 0;
    virtual ~ICommand() = default;
};

class WriteCommand: public ICommand{
protected:
    std::shared_ptr<Writer> stream;
    explicit WriteCommand(std::shared_ptr<Writer> _stream): stream(std::move(_stream)) {}
public:
    ~WriteCommand() override = default;
};

class FdWriteCommand : public WriteCommand{
private:
    std::tuple<uint32_t, std::shared_ptr<uint8_t>> cmd;
    static int count;
    int fd;
public:
    FdWriteCommand(std::shared_ptr<Writer> _writer, const int _fd, std::tuple<uint32_t, std::shared_ptr<uint8_t>> _cmd) : WriteCommand(std::move(_writer)), cmd(std::move(_cmd)), fd(_fd) {
        count++;
    }
    void Execute() override;
};

class Commands{
protected:
    std::queue<std::shared_ptr<ICommand>> commands;
    std::shared_ptr<Writer> stream;

    std::mutex com_mutex;
    std::condition_variable cond;
public:
    Commands(){
        stream = std::make_shared<Writer>();
    }

    void AddCommand(int fd, std::tuple<uint32_t, std::shared_ptr<uint8_t>> _cmd);
    void Execute();
    void Notify();
};
