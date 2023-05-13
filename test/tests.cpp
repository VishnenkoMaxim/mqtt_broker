//
// Created by vishn on 10.04.2023.
//

#include <gtest/gtest.h>
#include <string>
//#include <stdio.h>
#include <sys/types.h>

#include "functions.h"
#include "mqtt_broker.h"
#include "mqtt_protocol.h"
#include "command.h"

using namespace std;

TEST(BasicTests, Test_1){
    int err = 0;
    ServerCfgData cfg_data = ReadConfig(DEFAULT_CFG_FILE, err);

    EXPECT_EQ(err, 0);
    EXPECT_EQ(cfg_data.log_max_size, 10485760);
    EXPECT_EQ(cfg_data.log_max_files, 5);
    EXPECT_EQ(cfg_data.log_file_path, "/home/logs/mqtt_broker.log");
    EXPECT_EQ(cfg_data.level, 1);
    EXPECT_EQ(cfg_data.port, 1883);
}

TEST(BasicTests, Test_2){
    uint32_t val = 0x7F;
    uint32_t val2 = 0xBF;
    uint32_t val3 = 0xBF0F;
    uint32_t val4 = 0xBFFF0A;
    uint8_t size;

    uint8_t buf[8];
    bzero(buf, 8);

    CodeVarInt(buf, val2, size);
    EXPECT_EQ(GetVarIntSize(val), 1);
    EXPECT_EQ(GetVarIntSize(val2), 2);
    EXPECT_EQ(GetVarIntSize(val2), size);

    CodeVarInt(buf, val4, size);
    EXPECT_EQ(GetVarIntSize(val4), 4);
    EXPECT_EQ(GetVarIntSize(val4), size);

    uint32_t val5 = 0;
    uint8_t size5 = 0;
    int ret = DeCodeVarInt(buf, val5, size5);
    EXPECT_EQ(ret, mqtt_err::ok);
    EXPECT_EQ(val4, val5);
}

TEST(MqttEntity, Test_1){
    uint16_t var = 0xFFFA;
    auto *entity = new MqttTwoByteEntity((uint8_t *) &var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);

    MqttTwoByteEntity pp((uint8_t *) &var);
    MqttTwoByteEntity pp2 = std::move(pp);
    EXPECT_EQ(pp2.GetUint(), var);

    delete entity;
}

TEST(MqttEntity, Test_2){
    uint32_t var = 0xFFAAFA;
    auto *entity = new MqttFourByteEntity((uint8_t *) &var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);

    MqttFourByteEntity pp((uint8_t *) &var);
    MqttFourByteEntity pp2 = std::move(pp);
    EXPECT_EQ(pp2.GetUint(), var);

    delete entity;
}
TEST(MqttEntity, Test_3){
    char str[] = "test_var";
    auto *entity = new MqttStringEntity(strlen(str), (uint8_t *) str);

    EXPECT_EQ(entity->Size(), strlen(str) + 2);
    EXPECT_EQ(entity->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(strcmp((char *) (entity->GetData()), str), 0);

    MqttStringEntity pp = MqttStringEntity(strlen(str), (uint8_t *) str);
    MqttStringEntity pp2 = std::move(pp);
    EXPECT_EQ(strcmp((char *) (pp2.GetData()), str), 0);

    delete entity;
}

TEST(MqttEntity, Test_4){
    uint8_t buf[16] = {2,32,13};
    auto *entity = new MqttBinaryDataEntity(sizeof(buf), buf);

    EXPECT_EQ(entity->Size(), 2 + sizeof(buf));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(memcmp(entity->GetData(), buf, sizeof(buf)), 0);

    MqttBinaryDataEntity pp(sizeof(buf), buf);
    MqttBinaryDataEntity pp2 = std::move(pp);
    EXPECT_EQ(memcmp(pp2.GetData(), buf, sizeof(buf)), 0);

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

    MqttStringPairEntity pp{string(p_str_1), string(p_str_2)};
    auto pp2 = pp;
    auto pp3 = std::move(pp2);
    auto t = pp3.GetStringPair();
    EXPECT_EQ(strcmp((char *) (t.first.c_str()), p_str_1), 0);
    EXPECT_EQ(strcmp((char *) (t.second.c_str()), p_str_2), 0);
    EXPECT_EQ(t.first == p_str_1, true);
    EXPECT_EQ(t.second == p_str_2, true);

    delete entity;
}

TEST(MqttEntity, Test_6){
    uint8_t var = 0x21;
    auto *entity = new MqttByteEntity(&var);

    EXPECT_EQ(entity->Size(), sizeof(var));
    EXPECT_EQ(entity->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(memcmp(entity->GetData(), &var, sizeof(var)), 0);

    MqttByteEntity pp(&var);
    MqttByteEntity pp2 = std::move(pp);
    EXPECT_EQ(pp2.GetUint(), var);

    delete entity;
}

TEST(MqttEntity, Test_7){
    uint32_t val = 0xBFFF0A;
    uint8_t size;
    uint8_t buf[8];
    bzero(buf, 8);

    memcpy(buf, &val, sizeof(val));
    //CodeVarInt(buf, val, size);
    auto *entity = new MqttVIntEntity(buf);
    EXPECT_EQ(entity->GetType(), mqtt_data_type::variable_int);
    EXPECT_EQ(val, entity->GetUint());
    EXPECT_EQ(GetVarIntSize(val), entity->Size());

    delete entity;
}

TEST(MqttProperties, Test_1){
    uint16_t var = 0xFFFA;

    auto *prop = new MqttProperty(1, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var)));
    EXPECT_EQ(prop->GetId(), 1);
    EXPECT_EQ(prop->Size(), 2);
    EXPECT_EQ(prop->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(memcmp(prop->GetData(), &var, sizeof(var)), 0);
    EXPECT_EQ(prop->GetUint(), var);
    delete prop;
}

TEST(MqttProperties, Test_2){
    uint32_t var = 0xAB136501;
    auto *prop = new MqttProperty(2, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *) &var)));

    EXPECT_EQ(prop->GetId(), 2);
    EXPECT_EQ(prop->Size(), 4);
    EXPECT_EQ(prop->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(memcmp(prop->GetData(), &var, sizeof(var)), 0);
    EXPECT_EQ(prop->GetUint(), var);
    delete prop;
}

TEST(MqttProperties, Test_3){
    char str[] = "test_var";
    auto *prop = new MqttProperty(2, shared_ptr<MqttEntity>(new MqttStringEntity(strlen(str), (uint8_t *) str)));
    EXPECT_EQ(prop->GetId(), 2);
    EXPECT_EQ(prop->Size(), 2 + strlen(str));
    EXPECT_EQ(prop->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(strcmp((char *) (prop->GetData()), str), 0);
    EXPECT_EQ(strcmp(prop->GetString().c_str(), str), 0);
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

    EXPECT_EQ(strcmp(prop->GetStringPair().first.c_str(), p_str_1), 0);
    EXPECT_EQ(strcmp(prop->GetStringPair().second.c_str(), p_str_2), 0);

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
    EXPECT_EQ(prop->GetUint(), var);
    delete prop;
}

TEST(MqttPropertiesChain, Test_1){
    uint16_t var = 0xFFFA;
    auto p_chain = new MqttPropertyChain;

    p_chain->AddProperty(make_shared<MqttProperty>(response_information, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var))));
    p_chain->AddProperty(make_shared<MqttProperty>(3, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var))));

    EXPECT_EQ(p_chain->GetProperty(response_information)->GetId(), response_information);
    EXPECT_EQ(p_chain->GetProperty(response_information)->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(p_chain->GetProperty(response_information)->Size(), sizeof(var));
    EXPECT_EQ(memcmp(p_chain->GetProperty(response_information)->GetData(), &var, sizeof(var)), 0);

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

    EXPECT_EQ(p_chain->GetProperty(1)->GetId(), 1);
    EXPECT_EQ(p_chain->GetProperty(1)->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(p_chain->GetProperty(1)->Size(), sizeof(var));
    EXPECT_EQ(memcmp(p_chain->GetProperty(1)->GetData(), &var, sizeof(var)), 0);

    EXPECT_EQ(p_chain->GetProperty(2)->GetId(), 2);
    EXPECT_EQ(p_chain->GetProperty(2)->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(p_chain->GetProperty(2)->Size(), sizeof(var_2));
    EXPECT_EQ(memcmp(p_chain->GetProperty(2)->GetData(), &var_2, sizeof(var_2)), 0);

    EXPECT_EQ(p_chain->GetProperty(3)->GetId(), 3);
    EXPECT_EQ(p_chain->GetProperty(3)->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(p_chain->GetProperty(3)->Size(), sizeof(var_3));
    EXPECT_EQ(memcmp(p_chain->GetProperty(3)->GetData(), &var_3, sizeof(var_3)), 0);

    EXPECT_EQ(p_chain->GetProperty(4)->GetId(), 4);
    EXPECT_EQ(p_chain->GetProperty(4)->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(p_chain->GetProperty(4)->Size(), 2 + strlen(str));
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(4)->GetData()), str), 0);

    EXPECT_EQ(p_chain->GetProperty(5)->GetId(), 5);
    EXPECT_EQ(p_chain->GetProperty(5)->GetType(), mqtt_data_type::mqtt_string_pair);
    EXPECT_EQ(p_chain->GetProperty(5)->Size(), 2 + strlen(p_str_1) + 2 + strlen(p_str_2));
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(5)->GetPair()->first.GetData()), p_str_1), 0);
    EXPECT_EQ(strcmp((char *) (p_chain->GetProperty(5)->GetPair()->second.GetData()), p_str_2), 0);

    EXPECT_EQ(p_chain->GetProperty(6)->GetId(), 6);
    EXPECT_EQ(p_chain->GetProperty(6)->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(p_chain->GetProperty(6)->Size(), 2 + sizeof(buf));
    EXPECT_EQ(memcmp(p_chain->GetProperty(6)->GetData(), buf, sizeof(buf)), 0);

    delete p_chain;
}

TEST(MqttPropertiesChain, Test_3){
    uint8_t var = 0xFA;
    uint16_t var_2 = 0xFFFA;
    uint32_t var_3 = 324;
    char str[] = "test_var";
    char p_str_1[] = "test_var_1";
    char p_str_2[] = "test_var_2";
    uint8_t buf[10];


    uint32_t val = 0xBFFF0A;
    uint8_t size;
    uint8_t buf2[8];
    bzero(buf2, 8);

    memcpy(buf, &val, sizeof(val));

    auto p_chain = new MqttPropertyChain;
    p_chain->AddProperty(make_shared<MqttProperty>(1, shared_ptr<MqttEntity>(new MqttByteEntity(&var))));
    p_chain->AddProperty(make_shared<MqttProperty>(2, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *) &var_2))));
    p_chain->AddProperty(make_shared<MqttProperty>(3, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *) &var_3))));
    p_chain->AddProperty(make_shared<MqttProperty>(4, shared_ptr<MqttEntity>(new MqttStringEntity(strlen(str), (uint8_t *) str))));
    p_chain->AddProperty(make_shared<MqttProperty>(5, shared_ptr<MqttEntity>(new MqttStringPairEntity(MqttStringEntity(strlen(p_str_1), (uint8_t *) p_str_1),
                                                                                                      MqttStringEntity(strlen(p_str_2), (uint8_t *) p_str_2)))));
    p_chain->AddProperty(make_shared<MqttProperty>(6, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(sizeof(buf), buf))));
    p_chain->AddProperty(make_shared<MqttProperty>(7, shared_ptr<MqttEntity>(new MqttVIntEntity(buf))));

    EXPECT_EQ(p_chain->GetSize(), 64);
    delete p_chain;
}

TEST(MqttGetProperty, Test_1){
    uint8_t buf[2];
    uint8_t size = 0;

    buf[0] = payload_format_indicator;
    buf[1] = 0xA4;

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), payload_format_indicator);
    EXPECT_EQ(property->GetType(), mqtt_data_type::byte);
    EXPECT_EQ(memcmp(property->GetData(), &buf[1], 1), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(MqttGetProperty, Test_2){
    uint8_t buf[3];
    uint8_t size = 0;

    buf[0] = server_keep_alive;
    buf[1] = 0xA4;
    buf[2] = 0x32;

    uint8_t buf_2[2];
    buf_2[0] = buf[2];
    buf_2[1] = buf[1];

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), server_keep_alive);
    EXPECT_EQ(property->GetType(), mqtt_data_type::two_byte);
    EXPECT_EQ(memcmp(property->GetData(), &buf_2[0], 2), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(MqttGetProperty, Test_3){
    uint8_t buf[5];
    uint8_t size = 0;

    buf[0] = message_expiry_interval;
    uint32_t val = 1235122;
    uint32_t val2 = htonl(val);
    memcpy(&buf[1], &val2, 4);

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), message_expiry_interval);
    EXPECT_EQ(property->GetType(), mqtt_data_type::four_byte);
    EXPECT_EQ(memcmp(property->GetData(), &val, sizeof(val)), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(MqttGetProperty, Test_4){
    string str = "test_value";
    uint8_t size = 0;
    uint8_t buf[3 + str.size()];
    uint16_t len = str.size();
    uint16_t len2 = htons(len);
    memcpy(&buf[1], &len2, 2);
    memcpy(&buf[3], str.c_str(), str.size());
    buf[0] = content_type;

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), content_type);
    EXPECT_EQ(property->GetType(), mqtt_data_type::mqtt_string);
    EXPECT_EQ(strcmp((char *) property->GetData(), str.c_str()), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(MqttGetProperty, Test_5){
    uint8_t size = 0;
    uint8_t buf[3 + 10];
    uint16_t len = 10;
    uint16_t len2 = htons(len);
    memcpy(&buf[1], &len2, 2);
    buf[0] = correlation_data;

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), correlation_data);
    EXPECT_EQ(property->GetType(), mqtt_data_type::binary_data);
    EXPECT_EQ(memcmp((char *) property->GetData(), &buf[3], 10), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(MqttGetProperty, Test_6){
    char str[] = "test_value";
    char str_2[] = "test_value2";

    uint8_t size = 0;
    uint8_t buf[1 + 2 + strlen(str) + 2 + strlen(str_2)];

    uint16_t len = strlen(str);
    uint16_t len2 = htons(len);
    memcpy(&buf[1], &len2, 2);
    memcpy(&buf[3], str, strlen(str));

    uint16_t len_2 = strlen(str_2);
    len2 = htons(len_2);
    memcpy(&buf[1 + 2 + len], &len2, sizeof(len_2));
    memcpy(&buf[1 + 2 + len + sizeof(len_2)], str_2, strlen(str_2));
    buf[0] = user_property;

    auto property = CreateProperty(buf, size);
    EXPECT_NE(property, nullptr);
    EXPECT_EQ(property->GetId(), user_property);
    EXPECT_EQ(property->GetType(), mqtt_data_type::mqtt_string_pair);
    EXPECT_EQ(strcmp((char *) (property->GetPair()->first.GetData()), str), 0);
    EXPECT_EQ(strcmp((char *) (property->GetPair()->second.GetData()), str_2), 0);
    EXPECT_EQ(size, sizeof(buf));

    property.reset();
}

TEST(Command, Test_1){
    Commands command(1);

    int fd;
    string file_name = "file";
    fd = open(file_name.c_str(), O_WRONLY | O_CREAT);
    auto buf = shared_ptr<uint8_t>(new uint8_t[16], default_delete<uint8_t[]>());
    uint32_t size = 16;
    memcpy(buf.get(), "test 123", 8);

    command.AddCommand(fd, make_tuple(size, buf));
    close(fd);
    fd = open(file_name.c_str(), O_RDONLY);
    EXPECT_GE(fd, 0);
    close(fd);
    remove(file_name.c_str());
}

TEST(VariableHeaders, Test_1){
    VariableHeader vh{ConnactVH(1,2)};
    VariableHeader vh2{ConnectVH()};

    EXPECT_EQ(sizeof(ConnactVH), vh.GetSize());
    EXPECT_EQ(sizeof(ConnectVH), vh2.GetSize());

    uint8_t buf[32] = "";
    uint32_t offset = 0;
    vh.Serialize(buf, offset);
    EXPECT_EQ(sizeof(ConnactVH), offset);
}

TEST(CreateMqttPacket, Test_1){
    uint8_t buf[10] = "";

    VariableHeader vh{ConnactVH(11,33)};
    uint32_t packet_size = 0;
    MqttPropertyChain p_chain;
    FixedHeader fh(CONNACK << 4);


    p_chain.AddProperty(make_shared<MqttProperty>(11, shared_ptr<MqttEntity>(new MqttByteEntity(0xFA))));
    p_chain.AddProperty(make_shared<MqttProperty>(22, shared_ptr<MqttEntity>(new MqttByteEntity(12))));
    p_chain.AddProperty(make_shared<MqttProperty>(33, shared_ptr<MqttEntity>(new MqttTwoByteEntity(0xAA00))));
    p_chain.AddProperty(make_shared<MqttProperty>(44, shared_ptr<MqttEntity>(new MqttFourByteEntity(0xAA001100))));
    p_chain.AddProperty(make_shared<MqttProperty>(55, shared_ptr<MqttEntity>(new MqttStringEntity(string("hello")))));
    p_chain.AddProperty(make_shared<MqttProperty>(66, shared_ptr<MqttEntity>(new MqttStringPairEntity(MqttStringEntity(string("test")), MqttStringEntity(string("test"))))));
    p_chain.AddProperty(make_shared<MqttProperty>(77, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(sizeof(buf), buf))));
//    p_chain->AddProperty(make_shared<MqttProperty>(7, shared_ptr<MqttEntity>(new MqttVIntEntity(buf))));

    auto data_to_send = CreateMqttPacket(fh, vh, p_chain, packet_size);

    cout << packet_size << endl;
    for (unsigned int i=0; i<packet_size; i++){
        cout << int(data_to_send.get()[i]) << " ";
    }
    cout << endl;
    EXPECT_GE(packet_size, 0);
}