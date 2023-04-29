//
// Created by vishn on 10.04.2023.
//

#include <gtest/gtest.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#include "functions.h"
#include "mqtt_broker.h"
#include "mqtt_protocol.h"

using namespace std;

TEST(BasicTests, Test_read_config){
    int err = 0;
    ServerCfgData cfg_data = ReadConfig(DEFAULT_CFG_FILE, err);

    EXPECT_EQ(err, 0);
    EXPECT_EQ(cfg_data.log_max_size, 10485760);
    EXPECT_EQ(cfg_data.log_max_files, 5);
    EXPECT_EQ(cfg_data.log_file_path, "/home/logs/mqtt_broker.log");
    EXPECT_EQ(cfg_data.level, 1);
}

TEST(MqttProtocol, Test_1){
    uint16_t var = 17;
    MqttDataEntity data(mqtt_data_type::two_byte, (uint8_t *) &var);
    EXPECT_EQ(data.GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(*((uint16_t *)(data.GetData())), var);

    uint16_t var2 = 0;
    MqttDataEntity data2(mqtt_data_type::two_byte, (uint8_t *) &var2);
    EXPECT_EQ(data2.GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(*((uint16_t *)(data2.GetData())), var2);

    uint16_t var3 = 0xFFFF;
    MqttDataEntity data3(mqtt_data_type::two_byte, (uint8_t *) &var3);
    EXPECT_EQ(data3.GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(*((uint16_t *)(data3.GetData())), var3);
}

TEST(MqttProtocol, Test_2){
    uint32_t var = 3825;
    MqttDataEntity data(mqtt_data_type::four_byte, (uint8_t *) &var);
    EXPECT_EQ(data.GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(*((uint32_t *)(data.GetData())), var);

    uint32_t var2 = 0;
    MqttDataEntity data2(mqtt_data_type::four_byte, (uint8_t *) &var2);
    EXPECT_EQ(data2.GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(*((uint32_t *)(data2.GetData())), var2);

    uint32_t var3 = 0xFFFF;
    MqttDataEntity data3(mqtt_data_type::four_byte, (uint8_t *) &var3);
    EXPECT_EQ(data3.GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(*((uint32_t *)(data3.GetData())), var3);

    uint32_t var4 = 0xFFFFFFFA;
    MqttDataEntity data4(mqtt_data_type::four_byte, (uint8_t *) &var4);
    EXPECT_EQ(data4.GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(*((uint32_t *)(data4.GetData())), var4);
}

TEST(MqttEntity, Test_1){
    uint16_t var = 0xFFFA;
    auto *entity = new MqttTwoByteEntity((uint8_t *) &var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);

    delete entity;
}

TEST(MqttEntity, Test_2){
    uint32_t var = 0xFFAAFA;
    auto *entity = new MqttFourByteEntity((uint8_t *) &var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);
    delete entity;
}
TEST(MqttEntity, Test_3){
    char str[] = "test_var";
    auto *entity = new MqttStringEntity(strlen(str), (uint8_t *) str);

    EXPECT_EQ(entity->Size(), strlen(str) + 2);
    EXPECT_EQ(entity->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(strcmp((char *) (entity->GetData()), str), 0);
    delete entity;
}

TEST(MqttEntity, Test_4){
    uint8_t buf[16] = {2,32,13};
    auto *entity = new MqttBinaryDataEntity(sizeof(buf), buf);

    EXPECT_EQ(entity->Size(), 2 + sizeof(buf));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(memcmp(entity->GetData(), buf, sizeof(buf)), 0);
    delete entity;
}

TEST(MqttEntity, Test_5){
    char p_str_1[] = "test_var_1";
    char p_str_2[] = "test_var_2";
    auto *entity = new MqttStringPairEntity(MqttStringEntity(strlen(p_str_1), (uint8_t *) p_str_1), MqttStringEntity(strlen(p_str_2), (uint8_t *) p_str_2));

    EXPECT_EQ(entity->Size(), 2 + strlen(p_str_1) + 2 + strlen(p_str_2));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::mqtt_string_pair);
    auto test_pair = entity->GetPair();
    EXPECT_EQ(strcmp((char *) (test_pair->first.GetData()), p_str_1), 0);
    EXPECT_EQ(strcmp((char *) (test_pair->second.GetData()), p_str_2), 0);
    delete entity;
}

TEST(MqttEntity, Test_6){
    uint8_t var = 0x21;
    auto *entity = new MqttByteEntity(&var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);

    delete entity;
}

TEST(MqttProperties, Test_1){
    uint16_t var = 0xFFFA;

    auto *prop = new MqttProperty(1, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var)));
    EXPECT_EQ(prop->GetId(), 1);
    EXPECT_EQ(prop->Size(), 2);
    EXPECT_EQ(prop->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(memcmp(prop->GetData(), &var, sizeof(var)), 0);
    delete prop;
}

TEST(MqttProperties, Test_2){
    uint32_t var = 324;
    auto *prop = new MqttProperty(2, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *) &var)));

    EXPECT_EQ(prop->GetId(), 2);
    EXPECT_EQ(prop->Size(), 4);
    EXPECT_EQ(prop->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(memcmp(prop->GetData(), &var, sizeof(var)), 0);
    delete prop;
}

TEST(MqttProperties, Test_3){
    char str[] = "test_var";
    auto *prop = new MqttProperty(2, shared_ptr<MqttEntity>(new MqttStringEntity(strlen(str), (uint8_t *) str)));

    EXPECT_EQ(prop->GetId(), 2);
    EXPECT_EQ(prop->Size(), 2 + strlen(str));
    EXPECT_EQ(prop->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(strcmp((char *) (prop->GetData()), str), 0);
    delete prop;
}

TEST(MqttProperties, Test_4){
    char p_str_1[] = "test_var_1";
    char p_str_2[] = "test_var_2";

    auto *prop = new MqttProperty(2, shared_ptr<MqttEntity>(new MqttStringPairEntity(
            MqttStringEntity(strlen(p_str_1), (uint8_t *) p_str_1),
            MqttStringEntity(strlen(p_str_2), (uint8_t *) p_str_2))));

    EXPECT_EQ(prop->GetId(), 2);
    EXPECT_EQ(prop->Size(), 2 + strlen(p_str_1) + 2 + strlen(p_str_2));
    EXPECT_EQ(prop->GetType(), mqtt_data_type::mqtt_string_pair);
    auto test_pair = prop->GetPair();
    EXPECT_EQ(strcmp((char *) (test_pair->first.GetData()), p_str_1), 0);
    EXPECT_EQ(strcmp((char *) (test_pair->second.GetData()), p_str_2), 0);
    delete prop;
}

TEST(MqttProperties, Test_5){
    uint8_t buf[1000];
    auto *prop = new MqttProperty(11, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(sizeof(buf), buf)));

    EXPECT_EQ(prop->GetId(), 11);
    EXPECT_EQ(prop->Size(), 2 + sizeof(buf));
    EXPECT_EQ(prop->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(memcmp(prop->GetData(), buf, sizeof(buf)), 0);
    delete prop;
}

TEST(MqttProperties, Test_6){
    uint8_t var = 0xFA;

    auto *prop = new MqttProperty(1, shared_ptr<MqttEntity>(new MqttByteEntity(&var)));
    EXPECT_EQ(prop->GetId(), 1);
    EXPECT_EQ(prop->Size(), sizeof(var));
    EXPECT_EQ(prop->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(memcmp(prop->GetData(), &var, sizeof(var)), 0);
    delete prop;
}

TEST(MqttPropertiesChain, Test_1){
    uint16_t var = 0xFFFA;
    auto p_chain = new MqttPropertyChain;

    p_chain->AddProperty(make_shared<MqttProperty>(2, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var))));
    p_chain->AddProperty(make_shared<MqttProperty>(2, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var))));

    EXPECT_EQ(p_chain->GetProperty(0)->GetId(), 2);
    EXPECT_EQ(p_chain->GetProperty(0)->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(p_chain->GetProperty(0)->Size(), sizeof(var));
    EXPECT_EQ(memcmp(p_chain->GetProperty(0)->GetData(), &var, sizeof(var)), 0);

    delete p_chain;
}

TEST(MqttPropertiesChain, Test_2){
    uint8_t var = 0xFA;
    uint16_t var_2 = 0xFFFA;
    uint32_t var_3 = 324;
    char str[] = "test_var";
    char p_str_1[] = "test_var_1";
    char p_str_2[] = "test_var_2";
    uint8_t buf[1000];

    auto p_chain = new MqttPropertyChain;

    p_chain->AddProperty(make_shared<MqttProperty>(1, shared_ptr<MqttEntity>(new MqttByteEntity(&var))));
    p_chain->AddProperty(make_shared<MqttProperty>(2, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var_2))));
    p_chain->AddProperty(make_shared<MqttProperty>(3, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *) &var_3))));
    p_chain->AddProperty(make_shared<MqttProperty>(4, shared_ptr<MqttEntity>(new MqttStringEntity(strlen(str), (uint8_t *) str))));
    p_chain->AddProperty(make_shared<MqttProperty>(5, shared_ptr<MqttEntity>(new MqttStringPairEntity(MqttStringEntity(strlen(p_str_1), (uint8_t *) p_str_1),
                                                                                                      MqttStringEntity(strlen(p_str_2), (uint8_t *) p_str_2)))));
    p_chain->AddProperty(make_shared<MqttProperty>(6, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(sizeof(buf), buf))));


    EXPECT_EQ(p_chain->GetProperty(0)->GetId(), 1);
    EXPECT_EQ(p_chain->GetProperty(0)->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(p_chain->GetProperty(0)->Size(), sizeof(var));
    EXPECT_EQ(memcmp(p_chain->GetProperty(0)->GetData(), &var, sizeof(var)), 0);

    EXPECT_EQ(p_chain->GetProperty(1)->GetId(), 2);
    EXPECT_EQ(p_chain->GetProperty(1)->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(p_chain->GetProperty(1)->Size(), sizeof(var_2));
    EXPECT_EQ(memcmp(p_chain->GetProperty(1)->GetData(), &var_2, sizeof(var_2)), 0);

    EXPECT_EQ(p_chain->GetProperty(2)->GetId(), 3);
    EXPECT_EQ(p_chain->GetProperty(2)->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(p_chain->GetProperty(2)->Size(), sizeof(var_3));
    EXPECT_EQ(memcmp(p_chain->GetProperty(2)->GetData(), &var_3, sizeof(var_3)), 0);

    EXPECT_EQ(p_chain->GetProperty(3)->GetId(), 4);
    EXPECT_EQ(p_chain->GetProperty(3)->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(p_chain->GetProperty(3)->Size(), 2 + strlen(str));
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(3)->GetData()), str), 0);

    EXPECT_EQ(p_chain->GetProperty(4)->GetId(), 5);
    EXPECT_EQ(p_chain->GetProperty(4)->GetType(), mqtt_data_type::mqtt_string_pair);
    EXPECT_EQ(p_chain->GetProperty(4)->Size(), 2 + strlen(p_str_1) + 2 + strlen(p_str_2));
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(4)->GetPair()->first.GetData()), p_str_1), 0);
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(4)->GetPair()->second.GetData()), p_str_2), 0);

    EXPECT_EQ(p_chain->GetProperty(5)->GetId(), 6);
    EXPECT_EQ(p_chain->GetProperty(5)->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(p_chain->GetProperty(5)->Size(), 2 + sizeof(buf));
    EXPECT_EQ(memcmp(p_chain->GetProperty(5)->GetData(), buf, sizeof(buf)), 0);

    delete p_chain;
}