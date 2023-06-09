#ifndef MQTT_BROKER_MQTT_PROTOCOL_H
#define MQTT_BROKER_MQTT_PROTOCOL_H

#include <strings.h>
#include <stdlib.h>
#include "functions.h"
#include <sys/types.h>
#include <memory>
#include <utility>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

namespace mqtt_protocol{
    //MQTT Control Packet types
    namespace mqtt_pack_type {
        enum mqtt_pack_type_enum : int {
            RESERVED,
            CONNECT,
            CONNACK,
            PUBLISH,
            PUBACK,
            PUBREC,
            PUBREL,
            PUBCOMP,
            SUBSCRIBE,
            SUBACK,
            UNSUBSCRIBE,
            UNSUBACK,
            PINGREQ,
            PINGRESP,
            DISCONNECT,
            AUTH
        };
    }

    using namespace mqtt_pack_type;

    enum mqtt_property_id : uint8_t {
        payload_format_indicator = 0x01,
        message_expiry_interval = 0x02,
        content_type = 0x03,
        response_topic = 0x08,
        correlation_data = 0x09,
        subscription_identifier = 0x0B,
        session_expiry_interval = 0x11,
        assigned_client_identifier = 0x12,
        server_keep_alive = 0x13,
        authentication_method = 0x15,
        authentication_data = 0x16,
        request_problem_information = 0x17,
        will_delay_interval = 0x18,
        request_response_information = 0x19,
        response_information = 0x1A,
        server_reference = 0x1C,
        reason_string = 0x1F,
        receive_maximum = 0x21,
        topic_alias_maximum = 0x22,
        topic_alias = 0x23,
        maximum_qos = 0x24,
        retain_available = 0x25,
        user_property = 0x26,
        maximum_packet_size = 0x27,
        wildcard_subscription_available = 0x28,
        subscription_identifier_available = 0x29,
        shared_subscription_available = 0x2A
    };

    enum mqtt_data_type : uint8_t {
        undefined,
        byte,
        two_byte,
        four_byte,
        mqtt_string,
        binary_data,
        mqtt_string_pair,
        variable_int
    };

    enum mqtt_err : int {
        ok,
        read_err,
        var_int_err
    };

    struct FixedHeader{
        uint8_t first;
        uint32_t remaining_len;

        FixedHeader() : first(0), remaining_len(0) {};

        [[nodiscard]] bool    isDUP() const {return first & 0x08;}
        [[nodiscard]] uint8_t QoS() const {return first & 0x06;}
        [[nodiscard]] bool    isRETAIN() const {return first & 0x01;}
        [[nodiscard]] uint8_t GetType() const { return first>>4;}
        [[nodiscard]] uint8_t GetFlags() const { return first & 0x0F; }

        bool    isIdentifier() const {
            uint8_t type = GetType();
            if ((type >= PUBACK && type <= UNSUBACK) || (type == PUBLISH && QoS() > 0)){
                return true;
            }
            return false;
        }

        bool    isProperties() const {
            uint8_t type = GetType();
            if ((type >= CONNECT && type <= UNSUBACK) || (type == DISCONNECT) || (type == AUTH)){
                return true;
            }
            return false;
        }
    };

    struct ConnectVH {
        uint16_t prot_name_len;
        char name[4];
        uint8_t version;
        uint8_t conn_flags;
        uint16_t alive;

        ConnectVH() : prot_name_len(0), version(0), conn_flags(0), alive(0) {
            bzero(name, 4);
        }

        void CopyFromNet(const uint8_t *buf);
    };

    class MqttDataEntity{
    private:
        uint8_t type;
        shared_ptr<uint8_t> data;
    public:
        MqttDataEntity() : type(mqtt_data_type::undefined){data = nullptr;};
        MqttDataEntity(uint8_t _type, const uint8_t *_data);

        uint8_t GetType();
        uint8_t* GetData();
        ~MqttDataEntity() = default;
    };

    class MqttStringEntity;

    class MqttEntity{
    protected:
        uint8_t type;
    public:
        virtual uint32_t Size() = 0;
        virtual uint8_t* GetData() = 0;
        virtual shared_ptr<pair<MqttStringEntity, MqttStringEntity>> GetPair();
        virtual uint8_t GetType();

        virtual ~MqttEntity() = default;
    };

    class MqttByteEntity : public MqttEntity{
    private:
        shared_ptr<uint8_t> data{};
    public:
        MqttByteEntity() = delete;

        explicit MqttByteEntity(const uint8_t* _data){
            type = mqtt_data_type::byte;
            data = shared_ptr<uint8_t>(new uint8_t);
            memcpy(data.get(), _data, sizeof(uint8_t));
        }

        uint32_t Size() override;
        uint8_t* GetData() override;

        ~MqttByteEntity() override {
            data.reset();
        }
    };

    class MqttTwoByteEntity : public MqttEntity{
    private:
        shared_ptr<uint16_t> data{};
    public:
        MqttTwoByteEntity() = delete;

        explicit MqttTwoByteEntity(const uint8_t * _data){
            type = mqtt_data_type::two_byte;
            data = shared_ptr<uint16_t>(new uint16_t);
            memcpy(data.get(), _data, 2);
        }

        uint32_t Size() override;
        uint8_t* GetData() override;

        ~MqttTwoByteEntity() override {
            data.reset();
        }
    };

    class MqttFourByteEntity : public MqttEntity{
    private:
        shared_ptr<uint32_t> data{};
    public:
        MqttFourByteEntity() = delete;

        explicit MqttFourByteEntity(const uint8_t * _data){
            type = mqtt_data_type::four_byte;
            data = shared_ptr<uint32_t>(new uint32_t);
            memcpy(data.get(), _data, 4);
        }

        uint32_t Size() override;
        uint8_t* GetData() override;
        ~MqttFourByteEntity() override {
            data.reset();
        }
    };

    class MqttStringEntity : public MqttEntity{
    private:
        shared_ptr<string> data{};
    public:
        MqttStringEntity() = delete;

        MqttStringEntity(const uint16_t _len, const uint8_t * _data){
            type = mqtt_data_type::mqtt_string;
            data = std::make_shared<std::string>((char *)_data, _len);
        }

        MqttStringEntity(MqttStringEntity&& _obj) noexcept {
            data = _obj.data;
            type = _obj.type;
        }

        uint32_t Size() override;
        uint8_t* GetData() override;

        ~MqttStringEntity() override {
            data.reset();
        }
    };

    class MqttBinaryDataEntity : public MqttEntity{
    private:
        shared_ptr<uint8_t> data{};
        uint16_t size;
    public:
        MqttBinaryDataEntity()= delete;

        MqttBinaryDataEntity(const uint16_t _len, const uint8_t * _data){
            size = _len;
            type = mqtt_data_type::binary_data;
            data = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
            memcpy(data.get(), _data, size);
        }

        uint32_t Size() override;
        uint8_t* GetData() override;
        ~MqttBinaryDataEntity() override {
            data.reset();
        }
    };

    class MqttStringPairEntity : public MqttEntity {
    private:
        shared_ptr<pair<MqttStringEntity, MqttStringEntity>> data{};
    public:
        MqttStringPairEntity(MqttStringEntity str_1, MqttStringEntity str_2){
            type = mqtt_data_type::mqtt_string_pair;
            data = std::make_shared<pair<MqttStringEntity, MqttStringEntity>>(std::move(str_1), std::move(str_2));
        }

        uint32_t Size() override;
        uint8_t* GetData()  override;
        shared_ptr<pair<MqttStringEntity, MqttStringEntity>> GetPair() override;
        ~MqttStringPairEntity() override {
            data.reset();
        }
    };

    class MqttVIntEntity : public MqttEntity{
    private:
        shared_ptr<uint32_t> data{};
    public:
        MqttVIntEntity() = delete;

        explicit MqttVIntEntity(const uint8_t* _data){
            type = mqtt_data_type::variable_int;
            data = shared_ptr<uint32_t>(new uint32_t);
            memcpy(data.get(), _data, sizeof(uint32_t));
        }

        uint32_t Size() override;
        uint8_t* GetData() override;

        ~MqttVIntEntity() override {
            data.reset();
        }
    };

    class MqttProperty : public MqttEntity {
    private:
        shared_ptr<MqttEntity> property;
        uint8_t id;
    public:
        MqttProperty() = delete;

        MqttProperty(uint8_t _id, shared_ptr<MqttEntity> entity){
            property = std::move(entity);
            id = _id;
        }

        uint8_t     GetId() const;
        uint8_t*    GetData() override;
        uint32_t    Size() override;
        uint8_t     GetType() override;
        shared_ptr<pair<MqttStringEntity, MqttStringEntity>>     GetPair() override;

        ~MqttProperty() override {
            property->~MqttEntity();
        };
    };

    class MqttPropertyChain {
    private:
        vector<shared_ptr<MqttProperty>> properties{};
    public:
        MqttPropertyChain() = default;

        void            AddProperty(shared_ptr<MqttProperty> entity);
        uint32_t        Count();
        shared_ptr<MqttProperty>   GetProperty(unsigned int i);

        ~MqttPropertyChain() {
            for (auto &it : properties){
                it.reset();
            }
        }
    };

    [[nodiscard]] uint8_t ReadVariableInt(int fd, int &value);
    [[nodiscard]] uint8_t ReadVariableInt(const uint8_t *buf, int &value, uint8_t &size);
    [[nodiscard]] shared_ptr<MqttProperty> CreateProperty(const uint8_t *buf, uint8_t &size);
}

#endif //MQTT_BROKER_MQTT_PROTOCOL_H