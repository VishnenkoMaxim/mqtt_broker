
#ifndef MQTT_VARIABLE_HEADER_H
#define MQTT_VARIABLE_HEADER_H

#include "mqtt_protocol.h"

class IVariableHeader{
public:
    [[nodiscard]] virtual uint32_t GetSize() const = 0;
    virtual void Serialize(uint8_t* dst_buf, uint32_t &offset) = 0;
    virtual void ReadFromBuf(const uint8_t* buf, uint32_t &offset) = 0;
};

class ConnectVH1 : public IVariableHeader{
public:
    uint16_t prot_name_len;
    char name[4];
    uint8_t version;
    uint8_t conn_flags;
    uint16_t alive;

    ConnectVH1();

    [[nodiscard]] uint32_t GetSize() const override;
    void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
    void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
};

class ConnactVH1 : public IVariableHeader{
public:
    uint8_t conn_acknowledge_flags;
    uint8_t reason_code;

    ConnactVH1() = default;
    explicit ConnactVH1(uint8_t _caf, uint8_t _rc);

    [[nodiscard]] uint32_t GetSize() const override;
    void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
    void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
};

class DisconnectVH1 : public IVariableHeader{
public:
    uint8_t reason_code;

    explicit DisconnectVH1(uint8_t _reason_code);

    [[nodiscard]] uint32_t GetSize() const override;
    void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
    void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;
};

class VariableHeader1 final : public IVariableHeader{
private:
    shared_ptr<IVariableHeader> v_header;
public:
    VariableHeader1() = delete;
    explicit VariableHeader1(shared_ptr<IVariableHeader> entity){
        v_header = std::move(entity);
    }

    VariableHeader1(const VariableHeader1& _vh);
    VariableHeader1(VariableHeader1&& _vh) noexcept;

    [[nodiscard]] uint32_t GetSize() const override;
    void Serialize(uint8_t* dst_buf, uint32_t &offset) override;
    void ReadFromBuf(const uint8_t* buf, uint32_t &offset) override;

    ~VariableHeader1() = default;
};

#endif //MQTT_VARIABLE_HEADER_H
