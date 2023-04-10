//
// Created by vishn on 10.04.2023.
//

#include <gtest/gtest.h>
#include "functions.h"
#include "mqtt_broker.h"

using namespace std;

TEST(BasicTests, Test_read_config){
    int err = 0;
    ServerCfgData cfg_data = ReadConfig(err);

    EXPECT_EQ(err, 0);
    EXPECT_EQ(cfg_data.log_max_size, 10485760);
    EXPECT_EQ(cfg_data.log_max_files, 11);
    EXPECT_EQ(cfg_data.log_file_path, "/home/logs/mqtt_broker.log");
}