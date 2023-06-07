#include "command.h"

using namespace std;

int Writer::Write(const int fd, const shared_ptr<uint8_t>& buf, const uint32_t buf_len){
    return WriteData(fd, buf.get(), buf_len);
}

void FdWriteCommand::Execute() {
    uint32_t len;
    shared_ptr<uint8_t> ptr;
    tie(len, ptr) = cmd;
    stream->Write(fd, ptr, len);
    ptr.reset();
    count--;
}

void Commands::AddCommand(const int fd, tuple<uint32_t, shared_ptr<uint8_t>> _cmd){
    lock_guard<mutex> guard{com_mutex};
    commands.emplace(new FdWriteCommand(stream, fd, std::move(_cmd)));
    cond.notify_all();
}

void Commands::Execute(){
    unique_lock<mutex> lock{com_mutex};
    while(commands.empty()) cond.wait(lock);

    commands.front()->Execute();
    commands.pop();
}

void Commands::Notify(){
    cond.notify_all();
}

int FdWriteCommand::count = 0;