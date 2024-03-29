#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <memory>
#include <utility>
#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <set>
#include <queue>

#include "functions.h"

//mqtt flags
#define RETAIN_FLAG     0x01;
#define DUP_FLAG        0x08;

#define MQTT_VERSION_5      5
#define MQTT_VERSION_3      4

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

    enum mqtt_QoS : uint8_t {
        QoS_0,
        QoS_1,
        QoS_2
    };

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

    enum mqtt_reason_code : uint8_t{
        success,
        granted_quos_1,
        granted_quos_2,
        disconnect_with_will = 0x04,
        no_subscription_existed = 0x11,
        unspecified_error = 0x80,
        malformed_error,
        protocol_error,
        imp_specific_error,
        unsupported_protocol_version,
        client_identifier_not_valid,
        bad_user_name_passwd,
        not_authorized,
        server_unavailable,
        server_busy,
        banned,
        server_shutting_down,
        bad_authentication_method = 0x8C,
        keep_alive_timeout,
        session_taken_over,
        topic_filter_invalid,
        topic_name_invalid,
        receive_maximum_exceeded = 0x93,
        topic_alias_invalid,
        packet_too_large,
        message_rate_too_high,
        quota_exceeded,
        administrative_action,
        payload_format_invalid,
        retain_not_supported,
        qos_not_supported,
        use_another_server,
        server_moved,
        shared_subscription_not_supported,
        connection_rate_exceeded,
        maximum_connection_time,
        subscription_identifiers_not_supported,
        wildcard_subscriptions_not_supported
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
        var_int_err,
        mqtt_property_err,
        disconnect,
        handle_error,
        protocol_version_err,
        duplicate_client_id
    };

    class FixedHeader{
    public:
        friend class FHBuilder;

        uint8_t first;
        uint32_t remaining_len;

        FixedHeader() noexcept;
        explicit FixedHeader(uint8_t _first) noexcept;

        [[nodiscard]] bool    isDUP() const;
        [[nodiscard]] uint8_t QoS() const;
        [[nodiscard]] bool    isRETAIN() const;
        [[nodiscard]] uint8_t GetType() const;
        [[nodiscard]] uint8_t GetFlags() const;
        [[nodiscard]] bool    isIdentifier() const;
        [[nodiscard]] bool    isProperties() const;

        void SetRemainingLen(uint32_t _len);
        void Serialize(uint8_t* dst_buf, uint32_t &offset);
        [[nodiscard]] uint32_t Size() const noexcept;
        [[nodiscard]] uint8_t Get() const noexcept;
    };

    class FHBuilder{
    public:
        FHBuilder& PacketType(const uint8_t type);
        FHBuilder& WithDup();
        FHBuilder& WithQoS(const uint8_t qos);
        FHBuilder& WithRetain();

        uint8_t Build();
    private:
        FixedHeader header;
    };

    class MqttStringEntity;

    class MqttEntity{
    protected:
        uint8_t type = 0;
    public:
        [[nodiscard]] virtual uint32_t  Size() const = 0;
        virtual uint8_t*                GetData() = 0;
        virtual void                    Serialize(uint8_t* dst_buf, uint32_t &offset) = 0;

        virtual std::shared_ptr<std::pair<MqttStringEntity, MqttStringEntity>> GetPair();
        [[nodiscard]] virtual uint8_t     GetType() const;
        [[nodiscard]] virtual uint32_t    GetUint() const;
        [[nodiscard]] virtual std::string      GetString() const;
        [[nodiscard]] virtual std::pair<std::string, std::string>      GetStringPair() const;
        virtual ~MqttEntity() = default;
    };

    class MqttByteEntity : public MqttEntity{
    private:
        std::shared_ptr<uint8_t> data{};
    public:
        MqttByteEntity() = delete;

        explicit MqttByteEntity(const uint8_t* _data);
        explicit MqttByteEntity(uint8_t value);

        MqttByteEntity(const MqttByteEntity& _obj) noexcept;
        MqttByteEntity(MqttByteEntity&& _obj) noexcept;
        MqttByteEntity& operator=(const MqttByteEntity& _obj) noexcept;
        MqttByteEntity& operator=(MqttByteEntity&& _obj) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData() override;
        [[nodiscard]] uint32_t GetUint() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        ~MqttByteEntity() override {
            data.reset();
        }
    };

    class MqttTwoByteEntity : public MqttEntity{
    private:
        std::shared_ptr<uint16_t> data{};
    public:
        MqttTwoByteEntity() = delete;

        explicit MqttTwoByteEntity(const uint8_t * _data);
        explicit MqttTwoByteEntity(uint16_t value);

        MqttTwoByteEntity(const MqttTwoByteEntity& _obj) noexcept;
        MqttTwoByteEntity(MqttTwoByteEntity&& _obj) noexcept;
        MqttTwoByteEntity& operator=(const MqttTwoByteEntity& _obj) noexcept;
        MqttTwoByteEntity& operator=(MqttTwoByteEntity&& _obj) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData() override;
        [[nodiscard]] uint32_t GetUint() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;

        ~MqttTwoByteEntity() override {
            data.reset();
        }
    };

    class MqttFourByteEntity : public MqttEntity{
    private:
        std::shared_ptr<uint32_t> data{};
    public:
        MqttFourByteEntity() = delete;

        explicit MqttFourByteEntity(const uint8_t * _data);
        explicit MqttFourByteEntity(uint32_t value);

        MqttFourByteEntity(const MqttFourByteEntity& _obj) noexcept;
        MqttFourByteEntity(MqttFourByteEntity&& _obj) noexcept;
        MqttFourByteEntity& operator=(const MqttFourByteEntity& _obj) noexcept;
        MqttFourByteEntity& operator=(MqttFourByteEntity&& _obj) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData() override;
        [[nodiscard]] uint32_t GetUint() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;

        ~MqttFourByteEntity() override {
            data.reset();
        }
    };

    class MqttStringEntity : public MqttEntity{
    private:
        std::shared_ptr<std::string> data{};
    public:
        MqttStringEntity() = delete;

        MqttStringEntity(const uint16_t _len, const uint8_t * _data){
            type = mqtt_data_type::mqtt_string;
            data = std::make_shared<std::string>((char *)_data, _len);
        }

        explicit MqttStringEntity(const std::string& _str) noexcept;
        MqttStringEntity(const MqttStringEntity& _obj) noexcept;
        MqttStringEntity(MqttStringEntity&& _obj) noexcept;
        MqttStringEntity& operator=(const MqttStringEntity& _obj) noexcept;
        MqttStringEntity& operator=(MqttStringEntity&& _obj) noexcept;
        MqttStringEntity& operator=(const std::string& _str) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData() override;
        [[nodiscard]] std::string GetString() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;

        ~MqttStringEntity() override {
            data.reset();
        }
    };

    class MqttBinaryDataEntity : public MqttEntity{
    private:
        std::shared_ptr<uint8_t> data{};
        uint16_t size{};
    public:
        MqttBinaryDataEntity() = default;
        MqttBinaryDataEntity(uint16_t _len, const uint8_t * _data);
        MqttBinaryDataEntity(const MqttBinaryDataEntity& _obj) noexcept;
        MqttBinaryDataEntity(MqttBinaryDataEntity&& _obj) noexcept;
        MqttBinaryDataEntity& operator=(const MqttBinaryDataEntity& _obj) noexcept;
        MqttBinaryDataEntity& operator=(MqttBinaryDataEntity&& _obj) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData() override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void SerializeWithoutLen(uint8_t* dst_buf, uint32_t &offset);
        [[nodiscard]] std::string GetString() const override;

        bool isEmpty();

        ~MqttBinaryDataEntity() override {
            data.reset();
        }
    };

    class MqttStringPairEntity : public MqttEntity {
    private:
        std::shared_ptr<std::pair<MqttStringEntity, MqttStringEntity>> data{};
    public:
        MqttStringPairEntity(const MqttStringEntity& str_1, const MqttStringEntity& str_2){
            type = mqtt_data_type::mqtt_string_pair;
            data = std::make_shared<std::pair<MqttStringEntity, MqttStringEntity>>(str_1, str_2);
        }

        MqttStringPairEntity(const MqttStringPairEntity& _obj) noexcept;
        MqttStringPairEntity(const std::string &_str_1, const std::string &_str_2) noexcept;
        MqttStringPairEntity(MqttStringPairEntity&& _obj) noexcept;
        MqttStringPairEntity& operator=(const MqttStringPairEntity& _obj) noexcept;
        MqttStringPairEntity& operator=(MqttStringPairEntity&& _obj) noexcept;

        [[nodiscard]] uint32_t Size() const override;
        uint8_t* GetData()  override;
        std::shared_ptr<std::pair<MqttStringEntity, MqttStringEntity>> GetPair() override;
        [[nodiscard]] std::pair<std::string, std::string> GetStringPair() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;

        ~MqttStringPairEntity() override {
            data.reset();
        }
    };

    class MqttVIntEntity : public MqttEntity{
    private:
        std::shared_ptr<uint32_t> data{};
    public:
        MqttVIntEntity() = delete;

        explicit MqttVIntEntity(const uint8_t* _data);
        explicit MqttVIntEntity(uint32_t value);

        [[nodiscard]] uint32_t Size() const override;
        [[nodiscard]] uint32_t GetUint() const override;
        uint8_t* GetData() override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;

        ~MqttVIntEntity() override {
            data.reset();
        }
    };

    class MqttProperty final : public MqttEntity {
    private:
        std::shared_ptr<MqttEntity> property;
        uint8_t id;
    public:
        MqttProperty() = delete;

        MqttProperty(uint8_t _id, std::shared_ptr<MqttEntity> entity);

        [[nodiscard]] uint8_t     GetId() const;
        uint8_t*    GetData() override;
        [[nodiscard]] uint32_t    Size() const override;
        [[nodiscard]] uint8_t     GetType() const override;
        std::shared_ptr<std::pair<MqttStringEntity, MqttStringEntity>>     GetPair() override;
        [[nodiscard]] uint32_t    GetUint() const override;
        [[nodiscard]] std::string      GetString() const override;
        [[nodiscard]] std::pair<std::string, std::string> GetStringPair() const override;
        void        Serialize(uint8_t* buf_dst, uint32_t &offset) override;

        ~MqttProperty() override {
            //property->~MqttEntity();
        };
    };

    class MqttPropertyChain{
    private:
        //map<uint8_t, shared_ptr<MqttProperty>, less<>, PoolAllocator<pair<uint8_t, shared_ptr<MqttProperty>>, 5>> properties;
        std::map<uint8_t, std::shared_ptr<MqttProperty>> properties;
    public:
        MqttPropertyChain() = default;
        MqttPropertyChain(const MqttPropertyChain & _chain);
        MqttPropertyChain& operator = (const MqttPropertyChain & _chain);
        MqttPropertyChain(MqttPropertyChain && _chain) noexcept;
        MqttPropertyChain& operator = (MqttPropertyChain && _chain) noexcept;

        [[nodiscard]] uint32_t    Count() const;
        [[nodiscard]] uint16_t    GetSize() const;
        std::shared_ptr<MqttProperty>   GetProperty(uint8_t _id);
        std::shared_ptr<MqttProperty>   operator[](uint8_t _id);

        int  Create(const uint8_t *buf, uint32_t &size);
        void AddProperty(const std::shared_ptr<MqttProperty>& entity);
        void Serialize(uint8_t *buf, uint32_t &offset);
        void Clear();

        decltype(properties)::const_iterator Cbegin(){
            return properties.cbegin();
        }

        decltype(properties)::const_iterator Cend(){
            return properties.cend();
        }

        ~MqttPropertyChain() {
            for (auto &it : properties){
                it.second.reset();
            }
        }
    };

    class IVariableHeader{
    public:
        [[nodiscard]] virtual uint32_t GetSize() const = 0;
        virtual void Serialize(uint8_t* dst_buf, uint32_t &offset) = 0;
        virtual void ReadFromBuf(const uint8_t* buf, uint32_t &offset) = 0;

        virtual ~IVariableHeader() = default;
    };

    class ConnectVH : public IVariableHeader{
    public:
        uint16_t prot_name_len;
        char name[4];
        uint8_t version;
        uint8_t conn_flags;
        uint16_t alive;

        ConnectVH();
        ConnectVH(uint8_t _flags, uint16_t _alive);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
    };

    class ConnactVH : public IVariableHeader{
    public:
        uint8_t conn_acknowledge_flags;
        uint8_t reason_code;
        MqttPropertyChain   p_chain;

        ConnactVH();
        ConnactVH(uint8_t _caf, uint8_t _rc, MqttPropertyChain &_properties);
        ConnactVH(uint8_t _caf, uint8_t _rc, MqttPropertyChain &&_properties);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
    };

    class DisconnectVH : public IVariableHeader{
    public:
        uint8_t reason_code;
        MqttPropertyChain p_chain;

        DisconnectVH(uint8_t _reason_code, MqttPropertyChain &_properties);
        DisconnectVH(uint8_t _reason_code, MqttPropertyChain &&_properties);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
    };

    class PublishVH: public IVariableHeader{
    public:
        MqttStringEntity topic_name;
        uint16_t packet_id;
        MqttPropertyChain p_chain;

        PublishVH() : topic_name(""), packet_id(0){};

        PublishVH(bool is_packet_id_present, const std::shared_ptr<uint8_t>& buf, uint32_t &offset);
        PublishVH(MqttStringEntity &_topic_name, uint16_t _packet_id, MqttPropertyChain &_p_chain);
        PublishVH(MqttStringEntity &_topic_name, uint16_t _packet_id, MqttPropertyChain &&_p_chain);
        PublishVH(MqttStringEntity &&_topic_name, uint16_t _packet_id, MqttPropertyChain &&_p_chain);

        PublishVH(const PublishVH &_vh);
        PublishVH(PublishVH &&_vh) noexcept;
        PublishVH& operator =(const PublishVH &_vh);
        PublishVH& operator =(PublishVH &&_vh) noexcept;

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~PublishVH() override = default;
    };

    class SubscribeVH: public IVariableHeader{
    public:
        uint16_t packet_id;
        MqttPropertyChain p_chain;

        SubscribeVH() : packet_id(0){};

        SubscribeVH(const std::shared_ptr<uint8_t>& buf, uint32_t &offset);
        SubscribeVH(const std::shared_ptr<uint8_t>& buf, uint32_t &offset, const uint8_t version);
        SubscribeVH(uint16_t _packet_id, MqttPropertyChain &_p_chain);
        SubscribeVH(const SubscribeVH &_vh);
        SubscribeVH(SubscribeVH &&_vh) noexcept;
        SubscribeVH& operator =(const SubscribeVH &_vh);
        SubscribeVH& operator =(SubscribeVH &&_vh) noexcept;

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~SubscribeVH() override = default;
    };

    class SubackVH : public IVariableHeader{
    public:
        uint16_t packet_id;
        MqttPropertyChain p_chain;
        std::vector<uint8_t> &reason_codes;

        SubackVH() = delete;
        SubackVH(uint16_t _packet_id, MqttPropertyChain  _p_chain, std::vector<uint8_t>& _reason_codes);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~SubackVH() override = default;
    };

    class PubackVH : public IVariableHeader{
    public:
        uint16_t packet_id;
        uint8_t reason_code;
        MqttPropertyChain p_chain;

        PubackVH() = default;
        PubackVH(uint16_t _packet_id, uint8_t _reason_code, MqttPropertyChain  _p_chain);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~PubackVH() override = default;
    };

    class UnsubscribeVH : public IVariableHeader{
    public:
        uint16_t packet_id{0};
        MqttPropertyChain p_chain;

        UnsubscribeVH() = default;
        UnsubscribeVH(uint16_t _packet_id, MqttPropertyChain  _p_chain);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~UnsubscribeVH() override = default;
    };

    class UnsubAckVH: public IVariableHeader {
    public:
        uint16_t packet_id;
        MqttPropertyChain p_chain;
        std::vector<uint8_t> &reason_codes;

        UnsubAckVH() = delete;
        UnsubAckVH(uint16_t _packet_id, MqttPropertyChain _p_chain, std::vector<uint8_t>& _reason_codes);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~UnsubAckVH() override = default;
    };

    class TypicalVH: public PubackVH {
    public:
        TypicalVH() = default;
        TypicalVH(uint16_t _packet_id, uint8_t _reason_code, MqttPropertyChain _p_chain);

        ~TypicalVH() override = default;
    };

    class VariableHeader final : public IVariableHeader{
    private:
        std::shared_ptr<IVariableHeader> v_header;
    public:
        VariableHeader() = delete;
        explicit VariableHeader(std::shared_ptr<IVariableHeader> entity){
            v_header = std::move(entity);
        }

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~VariableHeader() override = default;
    };

    class MqttTopic{
    private:
        uint8_t qos;
        uint16_t id;
        std::string name;
        std::shared_ptr<MqttBinaryDataEntity> data;

    public:
        MqttTopic() = default;
        MqttTopic(uint8_t _qos, uint16_t _id, const std::string &_name, const std::shared_ptr<MqttBinaryDataEntity> &_data);

        MqttTopic(const MqttTopic &_topic) = default;
        MqttTopic(MqttTopic &&_topic) noexcept = default;
        MqttTopic& operator=(const MqttTopic &_topic) = default;
        MqttTopic& operator=(MqttTopic &&_topic) noexcept = default;
        bool operator == (const std::string &str);
        bool operator<(const MqttTopic& _topic) const;

        uint32_t GetSize();
        const uint8_t* GetData();
        [[nodiscard]] std::shared_ptr<MqttBinaryDataEntity> GetPtr() const;
        [[nodiscard]] std::string GetString() const;
        [[nodiscard]] MqttBinaryDataEntity GetValue() const;
        [[nodiscard]] uint16_t GetID() const;
        [[nodiscard]] uint8_t GetQoS() const;
        [[nodiscard]] std::string GetName() const;

        void SetPacketID(uint16_t new_id);
        void SetQos(uint8_t _qos);
        void SetName(const std::string& _name);
    };


    class TypicalV3VH : public IVariableHeader{
    public:
        uint16_t packet_id;

        TypicalV3VH() = default;
        TypicalV3VH(uint16_t _packet_id);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~TypicalV3VH() override = default;
    };

	class PublishV3VH: public IVariableHeader{
    public:
        MqttStringEntity topic_name;
        uint16_t packet_id;
        
        PublishV3VH();
        PublishV3VH(bool is_packet_id_present, const std::shared_ptr<uint8_t>& buf, uint32_t &offset);
        PublishV3VH(MqttStringEntity &_topic_name, uint16_t _packet_id);
        PublishV3VH(MqttStringEntity &&_topic_name, uint16_t _packet_id);

        [[nodiscard]] uint32_t GetSize() const override;
        void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
        void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

        ~PublishV3VH() override = default;
    };

    [[nodiscard]] uint8_t ReadVariableInt(int fd, int &value);
    [[nodiscard]] uint8_t DeCodeVarInt(const uint8_t *buf, uint32_t &value, uint8_t &size);
    uint8_t               CodeVarInt(uint8_t *buf, uint32_t value, uint8_t &size);
    [[nodiscard]] uint8_t GetVarIntSize(uint32_t value);
    [[nodiscard]] std::shared_ptr<MqttProperty> CreateProperty(const uint8_t *buf, uint8_t &size);
    [[nodiscard]] std::shared_ptr<MqttStringEntity> CreateMqttStringEntity(const uint8_t *buf, uint8_t &size);

    std::shared_ptr<uint8_t> CreateMqttPacket(uint8_t pack_type, uint32_t &size);
    std::shared_ptr<uint8_t> CreateMqttPacket(uint8_t pack_type, VariableHeader &vh, uint32_t &size);
    std::shared_ptr<uint8_t> CreateMqttPacket(uint8_t pack_type, VariableHeader &vh, const std::shared_ptr<MqttBinaryDataEntity> &message, uint32_t &size);
}
