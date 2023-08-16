#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <memory>

#include "version.h"
#include "mqtt_broker.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

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

    int sock_fd, newsock_fd;
    struct sockaddr_in serv_addr, cli_addr;

    sock_fd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    if (sock_fd < 0) {
        lg->error("Error opening socket: {}", strerror(errno));
        return 0;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(cfg_data.port);
    while(true) {
        if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            lg->error("Error binding socket: {}", strerror(errno));
            lg->flush();
            sleep(1);
            continue;
        }
        break;
    }

    if (listen(sock_fd, 40) < 0){
        lg->error("Error listening socket: {}", strerror(errno));
        return 0;
    }
    socklen_t c_len = sizeof(cli_addr);

    struct pollfd fds;
    fds.fd = sock_fd;
    fds.events = POLLIN;
    int ret;

    broker.InitControlSocket(cfg_data.control_socket_path);

    while(true){
        lg->info("Waiting for a client..."); lg->flush();
        if ((ret = poll(&fds, 1, -1)) == -1) {
            lg->error("Error poll(): {}", strerror(errno));
            sleep(1);
            continue;
        }
        newsock_fd = accept(fds.fd, (struct sockaddr *) &cli_addr, &c_len);
        if (newsock_fd < 0){
            lg->error("Error accept socket: {}", strerror(errno));
            continue;
        }
        char * ipStr = inet_ntoa(cli_addr.sin_addr);
        lg->info("New client has connected: {}", ipStr);
        broker_err status = broker.AddClient(newsock_fd, ipStr);
        if (status != broker_err::ok){
            lg->error("Insertion error, close connection");
            close(newsock_fd);
            continue;
        }
        lg->debug("New client has been added: fd:{}", newsock_fd);
        if (broker.GetState() == broker_states::init) broker.Start();
        lg->flush();
    }
    return EXIT_FAILURE;
}
