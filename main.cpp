#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <memory>

#include "version.h"
#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

using namespace std;
using namespace spdlog;
using namespace libconfig;

int main() {
    cfg_err err;
    shared_ptr<logger> lg;
    Broker& broker = Broker::GetInstance();
    ServerCfgData cfg_data = ReadConfig(DEFAULT_CFG_FILE, err);

    if (err != cfg_err::ok){
        cerr << "Error reading cfg file. Terminate" << endl;
        return 0;
    }
    lg = spdlog::rotating_logger_mt("main", cfg_data.log_file_path, cfg_data.log_max_size, cfg_data.log_max_files);
    lg->info("START BROKER");
    lg->info("logger file:{} size:{} Kb, max_files:{} level:{}", cfg_data.log_file_path,  cfg_data.log_max_size/1024, cfg_data.log_max_files, cfg_data.level);
    SetLogLevel(lg, cfg_data.level);
    broker.SetPort(cfg_data.port);

    broker.InitLogger(cfg_data.log_file_path, cfg_data.log_max_size, cfg_data.log_max_files, cfg_data.level);

    broker.SetEraseOldValues(false);
    int sock_fd = broker.InitSocket();
    if (sock_fd <= 0) exit(0);
    broker.InitControlSocket(cfg_data.control_socket_path);

	signal(SIGPIPE, SIG_IGN);

    while (true){
        lg->info("Waiting for a client..."); lg->flush();
        char ip[16];
        auto newsock_fd = broker.WaitForClient(ip);
        if(newsock_fd > 0) {
            lg->info("New client has connected: {}", ip);
            broker_err status = broker.AddClient(newsock_fd, ip);
            if (status != broker_err::ok) {
                lg->error("Insertion error, close connection");
                close(newsock_fd);
                continue;
            }
            lg->info("New client has been added: fd:{}", newsock_fd); lg->flush();
            if (broker.GetState() == broker_states::init) broker.Start();
        }
        lg->flush();
    }
    return EXIT_FAILURE;
}
