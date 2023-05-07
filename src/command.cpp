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

void Commands::Add(const int fd, tuple<uint32_t, shared_ptr<uint8_t>> _cmd){
    pthread_mutex_lock(&mutex);
    commands.emplace_back(new FdWriteCommand(stream, fd, std::move(_cmd)));
    pthread_mutex_unlock(&mutex);
    if (N>0 && commands.size() >= N) Execute();
}

void Commands::Execute(){
    if (commands.empty()) return;
    for (auto &it : commands){
        it->Execute();
    }
    PostActions();
}

void Commands::PostActions(){
    pthread_mutex_lock(&mutex);
    commands.clear();
    pthread_mutex_unlock(&mutex);
}

void Commands::Exit(){
    Execute();
    stream.reset();
}

int FdWriteCommand::count = 0;