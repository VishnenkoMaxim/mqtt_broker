#include "mqtt_protocol.h"

#include <memory>

using namespace mqtt_protocol;
using namespace temp_funcs;
using namespace std;

[[nodiscard]] uint8_t mqtt_protocol::ReadVariableInt(const int fd, int &value){
    uint8_t single_byte;
    uint32_t multiplayer = 1;
    value = 0;
    while(true){
        int ret = read(fd, &single_byte, sizeof(single_byte));
        if (ret != 1) {
            return mqtt_err::read_err;
        }
        value += (single_byte & 0x7F)*multiplayer;
        if (multiplayer > mult<128,3>::value){
            return mqtt_err::var_int_err;
        }
        multiplayer *= 128;
        if ((single_byte & 0x80) == 0) break;
    }
    return mqtt_err::ok;
}

[[nodiscard]] uint8_t mqtt_protocol::DeCodeVarInt(const uint8_t *buf, uint32_t &value, uint8_t &size){
    uint8_t single_byte;
    uint32_t multiplayer = 1;
    value = 0;
    uint8_t offset = 0;
    while(true){
        single_byte = *(buf + offset);
        value += (single_byte & 0x7F)*multiplayer;
        if (multiplayer > mult<128,3>::value){
            return mqtt_err::var_int_err;
        }
        multiplayer *= 128;
        offset++;
        if ((single_byte & 0x80) == 0) break;
    }
    size = offset;
    return mqtt_err::ok;
}

uint8_t mqtt_protocol::CodeVarInt(uint8_t *buf, uint32_t value, uint8_t &size){
    uint8_t single_byte;
    uint8_t offset = 0;
    while(true){
        single_byte = value % 0x80;
        value /= 0x80;
        if (value > 0) single_byte |= 0x80;
        memcpy(buf + offset, &single_byte, sizeof(single_byte));
        offset++;
        if (value == 0) break;
    }
    size = offset;
    return mqtt_err::ok;
}

[[nodiscard]] uint8_t mqtt_protocol::GetVarIntSize(uint32_t value){
    if (value == 0) return 1;
    uint8_t count = 0;
    while(value > 0){
        count++;
        value /= 0x80;
    }
    return count;
}

shared_ptr<pair<MqttStringEntity, MqttStringEntity>> MqttEntity::GetPair() {
    return nullptr;
}

uint8_t MqttEntity::GetType() const{
    return type;
}

uint32_t MqttEntity::GetUint() const{
    return 0;
}

string MqttEntity::GetString() const {
    return string{};
}

pair<string, string> MqttEntity::GetStringPair() const {
    return pair{string{}, string{}};
}

MqttByteEntity::MqttByteEntity(const MqttByteEntity& _obj) noexcept {
    data.reset();
    data = std::make_shared<std::uint8_t>(_obj.GetUint());
    type = _obj.type;
}

MqttByteEntity::MqttByteEntity(const uint8_t value) {
    type = mqtt_data_type::byte;
    data = shared_ptr<uint8_t>(new uint8_t);
    memcpy(data.get(), &value, sizeof(uint8_t));
}

MqttByteEntity::MqttByteEntity(const uint8_t* _data){
    type = mqtt_data_type::byte;
    data = shared_ptr<uint8_t>(new uint8_t);
    memcpy(data.get(), _data, sizeof(uint8_t));
}

MqttByteEntity::MqttByteEntity(MqttByteEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    _obj.data = nullptr;
}
MqttByteEntity& MqttByteEntity::operator=(const MqttByteEntity& _obj) noexcept{
    data.reset();
    data = std::make_shared<std::uint8_t>(_obj.GetUint());
    return *this;
}
MqttByteEntity& MqttByteEntity::operator=(MqttByteEntity&& _obj) noexcept{
    data.reset();
    data = _obj.data;
    _obj.data = nullptr;
    return *this;
}

uint32_t MqttByteEntity::Size() const {
    return sizeof(uint8_t);
}

uint8_t* MqttByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttByteEntity::GetUint() const {
    return (uint32_t) *data;
}

void MqttByteEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    memcpy(dst_buf, GetData(), Size());
    offset += Size();
}

MqttTwoByteEntity::MqttTwoByteEntity(const uint8_t * _data){
    type = mqtt_data_type::two_byte;
    data = shared_ptr<uint16_t>(new uint16_t);
    memcpy(data.get(), _data, 2);
}

MqttTwoByteEntity::MqttTwoByteEntity(const uint16_t value){
    type = mqtt_data_type::two_byte;
    data = shared_ptr<uint16_t>(new uint16_t);
    memcpy(data.get(), &value, 2);
}

MqttTwoByteEntity::MqttTwoByteEntity(const MqttTwoByteEntity& _obj) noexcept{
    data = std::make_shared<std::uint16_t>(_obj.GetUint());
    type = _obj.type;
}

MqttTwoByteEntity::MqttTwoByteEntity(MqttTwoByteEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    _obj.data = nullptr;
}

MqttTwoByteEntity& MqttTwoByteEntity::operator=(const MqttTwoByteEntity& _obj) noexcept{
    data.reset();
    data = std::make_shared<std::uint16_t>(_obj.GetUint());
    return *this;
}

MqttTwoByteEntity& MqttTwoByteEntity::operator=(MqttTwoByteEntity&& _obj) noexcept{
    data.reset();
    data = _obj.data;
    _obj.data = nullptr;
    return *this;
}

uint32_t MqttTwoByteEntity::Size() const {
    return sizeof(uint16_t);
}

uint8_t* MqttTwoByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttTwoByteEntity::GetUint() const {
    return (uint32_t) *data;
}

void MqttTwoByteEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint16_t raw_val = htons((uint16_t) GetUint());
    memcpy(dst_buf, &raw_val, Size());
    offset += Size();
}

MqttFourByteEntity::MqttFourByteEntity(const uint8_t * _data){
    type = mqtt_data_type::four_byte;
    data = shared_ptr<uint32_t>(new uint32_t);
    memcpy(data.get(), _data, 4);
}

MqttFourByteEntity::MqttFourByteEntity(const uint32_t value){
    type = mqtt_data_type::four_byte;
    data = shared_ptr<uint32_t>(new uint32_t);
    memcpy(data.get(), &value, 4);
}

MqttFourByteEntity::MqttFourByteEntity(const MqttFourByteEntity& _obj) noexcept{
    data = std::make_shared<std::uint32_t>(_obj.GetUint());
    type = _obj.type;
}

MqttFourByteEntity::MqttFourByteEntity(MqttFourByteEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    _obj.data = nullptr;
}

MqttFourByteEntity& MqttFourByteEntity::operator=(const MqttFourByteEntity& _obj) noexcept{
    data.reset();
    data = std::make_shared<std::uint32_t>(_obj.GetUint());
    return *this;
}

MqttFourByteEntity& MqttFourByteEntity::operator=(MqttFourByteEntity&& _obj) noexcept{
    data.reset();
    data = _obj.data;
    _obj.data = nullptr;
    return *this;
}

uint32_t MqttFourByteEntity::Size() const {
    return sizeof(uint32_t);
}

uint8_t* MqttFourByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttFourByteEntity::GetUint() const {
    return (uint32_t) *data;
}

void MqttFourByteEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint32_t raw_val = htonl(GetUint());
    memcpy(dst_buf, &raw_val, Size());
    offset += Size();
}

MqttStringEntity::MqttStringEntity(const MqttStringEntity& _obj) noexcept {
    data = std::make_shared<std::string>(_obj.GetString());
    type = _obj.type;
}

MqttStringEntity::MqttStringEntity(const string& _str) noexcept{
    type = mqtt_data_type::mqtt_string;
    data = std::make_shared<std::string>(_str);
}

MqttStringEntity::MqttStringEntity(MqttStringEntity&& _obj) noexcept {
    data = _obj.data;
    type = _obj.type;
    _obj.data = nullptr;
}

MqttStringEntity& MqttStringEntity::operator=(const MqttStringEntity& _obj) noexcept {
    data.reset();
    data = std::make_shared<std::string>(_obj.GetString());
    return *this;
}

MqttStringEntity& MqttStringEntity::operator=(MqttStringEntity&& _obj) noexcept{
    data.reset();
    data = _obj.data;
    _obj.data = nullptr;
    return *this;
}

MqttStringEntity& MqttStringEntity::operator=(const string& _obj) noexcept {
    data.reset();
    data = std::make_shared<std::string>(_obj);
    return *this;
}

uint32_t MqttStringEntity::Size() const {
    return sizeof(uint16_t) + data->size();
}

uint8_t* MqttStringEntity::GetData(){
    return (uint8_t *) data->data();
}

string MqttStringEntity::GetString() const {
    return *data;
}

void MqttStringEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint16_t len = htons(data->size());
    memcpy(dst_buf, &len, sizeof(len));
    memcpy(dst_buf + sizeof(len), data->c_str(), data->size());
    offset += Size();
}

MqttBinaryDataEntity::MqttBinaryDataEntity(const uint16_t _len, const uint8_t * _data){
    size = _len;
    type = mqtt_data_type::binary_data;
    data = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    memcpy(data.get(), _data, size);
}

MqttBinaryDataEntity::MqttBinaryDataEntity(const MqttBinaryDataEntity& _obj) noexcept{
    size = _obj.size;
    data = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    memcpy(data.get(), _obj.data.get(), size);
    type = _obj.type;
}

MqttBinaryDataEntity::MqttBinaryDataEntity(MqttBinaryDataEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    size = _obj.size;
    _obj.size = 0;
    _obj.data = nullptr;
}

MqttBinaryDataEntity& MqttBinaryDataEntity::operator=(const MqttBinaryDataEntity& _obj) noexcept{
    data.reset();
    size = _obj.size;
    data = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    memcpy(data.get(), _obj.data.get(), size);
    type = _obj.type;
    return *this;
}

MqttBinaryDataEntity& MqttBinaryDataEntity::operator=(MqttBinaryDataEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    size = _obj.size;
    _obj.size = 0;
    _obj.data = nullptr;
    return *this;
}

uint32_t MqttBinaryDataEntity::Size() const {
    return sizeof(uint16_t) + size;
}

uint8_t* MqttBinaryDataEntity::GetData(){
    return (uint8_t *) data.get();
}

[[nodiscard]] string MqttBinaryDataEntity::GetString() const {
    return string((char *) data.get(), size);
}

void MqttBinaryDataEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint16_t len = htons(size);
    memcpy(dst_buf, &len, sizeof(len));
    memcpy(dst_buf + sizeof(len), data.get(), size);
    offset += Size();
}

void MqttBinaryDataEntity::SerializeWithoutLen(uint8_t* dst_buf, uint32_t &offset){
    memcpy(dst_buf, data.get(), size);
    offset += Size()-sizeof(size);
}

bool MqttBinaryDataEntity::isEmpty(){
    return size == 0;
}

MqttStringPairEntity::MqttStringPairEntity(const MqttStringPairEntity& _obj) noexcept{
    type = _obj.type;
    data = std::make_shared<pair<MqttStringEntity, MqttStringEntity>>(_obj.GetStringPair());
}

MqttStringPairEntity::MqttStringPairEntity(MqttStringPairEntity&& _obj) noexcept{
    data = _obj.data;
    type = _obj.type;
    _obj.data = nullptr;
}

MqttStringPairEntity& MqttStringPairEntity::operator=(const MqttStringPairEntity& _obj) noexcept{
    data.reset();
    data = std::make_shared<pair<MqttStringEntity, MqttStringEntity>>(_obj.GetStringPair());
    return *this;
}

MqttStringPairEntity& MqttStringPairEntity::operator=(MqttStringPairEntity&& _obj) noexcept{
    data.reset();
    data = _obj.data;
    _obj.data = nullptr;
    return *this;
}

MqttStringPairEntity::MqttStringPairEntity(const string &_str_1, const string &_str_2) noexcept{
    type = mqtt_data_type::mqtt_string_pair;
    data = std::make_shared<pair<MqttStringEntity, MqttStringEntity>>(pair<MqttStringEntity, MqttStringEntity>(_str_1, _str_2));
}

uint32_t MqttStringPairEntity::Size() const {
    return data->first.Size() + data->second.Size();
}

uint8_t* MqttStringPairEntity::GetData(){
    return data->first.GetData();
}

pair<string, string> MqttStringPairEntity::GetStringPair() const {
    return pair{(const_cast<MqttStringPairEntity *>(this))->GetPair()->first.GetString(), (const_cast<MqttStringPairEntity *>(this))->GetPair()->second.GetString()};
}

void MqttStringPairEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    auto pair = GetPair();
    pair->first.Serialize(dst_buf, offset);
    pair->second.Serialize(dst_buf + pair->first.Size(), offset);
}

MqttVIntEntity::MqttVIntEntity(const uint8_t* _data){
    type = mqtt_data_type::variable_int;
    data = shared_ptr<uint32_t>(new uint32_t);
    memcpy(data.get(), _data, sizeof(uint32_t));
}

MqttVIntEntity::MqttVIntEntity(const uint32_t value){
    type = mqtt_data_type::variable_int;
    data = shared_ptr<uint32_t>(new uint32_t);
    memcpy(data.get(), &value, sizeof(uint32_t));
}

uint32_t MqttVIntEntity::GetUint() const {
    return *data;
}

uint32_t MqttVIntEntity::Size() const {
    return GetVarIntSize(GetUint());
}

uint8_t* MqttVIntEntity::GetData(){
    return (uint8_t *) data.get();
}

void MqttVIntEntity::Serialize(uint8_t* dst_buf, uint32_t &offset){
    uint8_t size = 0;
    CodeVarInt(dst_buf, *data, size);
    offset += size;
}

shared_ptr<pair<MqttStringEntity, MqttStringEntity>> MqttStringPairEntity::GetPair(){
    return data;
}

MqttPropertyChain::MqttPropertyChain(const MqttPropertyChain & _chain){
    for(const auto &it: _chain.properties){
        shared_ptr<MqttProperty> p;
        auto p1 = *it.second;
        switch(it.second->GetType()){
            case mqtt_data_type::byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::two_byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttTwoByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::four_byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttFourByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::mqtt_string : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttStringEntity(it.second->GetString())));};break;
            case mqtt_data_type::mqtt_string_pair : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttStringPairEntity(it.second->GetPair()->first, it.second->GetPair()->second)));};break;
            case mqtt_data_type::variable_int : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttVIntEntity(it.second->GetData())));};break;
            case mqtt_data_type::binary_data : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(it.second->Size(), it.second->GetData())));};break;
        }
        assert(p->GetId() > 0);
        AddProperty(p);
    }
}

MqttPropertyChain& MqttPropertyChain::operator = (const MqttPropertyChain & _chain){
    properties.clear();
    for(const auto &it: _chain.properties){
        shared_ptr<MqttProperty> p;

        switch(it.second->GetType()){
            case mqtt_data_type::byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::two_byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttTwoByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::four_byte : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttFourByteEntity(it.second->GetUint())));};break;
            case mqtt_data_type::mqtt_string : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttStringEntity(it.second->GetString())));};break;
            case mqtt_data_type::mqtt_string_pair : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttStringPairEntity(it.second->GetPair()->first, it.second->GetPair()->second)));};break;
            case mqtt_data_type::variable_int : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttVIntEntity(it.second->GetData())));};break;
            case mqtt_data_type::binary_data : {p = make_shared<MqttProperty>(it.first, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(it.second->Size(), it.second->GetData())));};break;
        }
        assert(p->GetId() > 0);
        AddProperty(p);
    }
    return *this;
}

MqttPropertyChain::MqttPropertyChain(MqttPropertyChain && _chain) noexcept {
    properties = std::move(_chain.properties);
}

MqttPropertyChain& MqttPropertyChain::operator = (MqttPropertyChain && _chain) noexcept {
    properties = std::move(_chain.properties);
    return *this;
}

void MqttPropertyChain::Clear(){
    for (auto &it : properties){
        it.second.reset();
    }
    properties.clear();
}

MqttProperty::MqttProperty(uint8_t _id, shared_ptr<MqttEntity> entity){
    property = std::move(entity);
    id = _id;
}

uint8_t*    MqttProperty::GetData(){
    return property->GetData();
}

uint32_t    MqttProperty::Size() const {
    return property->Size();
}

uint8_t MqttProperty::GetId() const{
    return id;
}

uint8_t MqttProperty::GetType() const {
    return property->GetType();
}

shared_ptr<pair<MqttStringEntity, MqttStringEntity>>     MqttProperty::GetPair(){
    return property->GetPair();
}

uint32_t MqttProperty::GetUint() const {
    return property->GetUint();
}

string MqttProperty::GetString() const {
    return property->GetString();
}

pair<string, string> MqttProperty::GetStringPair() const {
    return property->GetStringPair();
}

void MqttProperty::Serialize(uint8_t* buf_dst, uint32_t &offset) {
    uint8_t _id = GetId();
    memcpy(buf_dst, &_id, sizeof(_id));
    offset++;
    property->Serialize(buf_dst + sizeof(_id), offset);
}

void MqttPropertyChain::AddProperty(const shared_ptr<MqttProperty>& entity){
    uint8_t _id = entity->GetId();
    properties.insert(make_pair(_id, entity));
}

uint32_t MqttPropertyChain::Count() const {
    return properties.size();
}

shared_ptr<MqttProperty> MqttPropertyChain::GetProperty(uint8_t _id){
    auto it = properties.find(_id);
    if (it != properties.end()) return it->second;
    return nullptr;
}

shared_ptr<MqttProperty>   MqttPropertyChain::operator[](uint8_t _id){
    return properties[_id];
}

uint16_t MqttPropertyChain::GetSize() const {
    uint16_t size = 0;
    for(const auto &it: properties){
        size += it.second->Size() + 1; //+1 because property_id
    }
    return size;
}

void MqttPropertyChain::Serialize(uint8_t *buf, uint32_t &offset){
    uint32_t size = 0;

    uint32_t p_len = GetSize();
    uint8_t p_len_size;
    CodeVarInt(buf, p_len, p_len_size);
    size += p_len_size;
    offset += size;

    for(const auto &it: properties){
        it.second->Serialize(buf + size, offset);
        size += it.second->Size() + 1; //+1 because property_id
    }
}

int MqttPropertyChain::Create(const uint8_t *buf, uint32_t &size){
    uint32_t properties_len;
    uint8_t properties_len_size;
    size = 0;
    if (DeCodeVarInt(buf, properties_len, properties_len_size) == mqtt_err::ok) {
        size += properties_len_size;
        if (properties_len > 0){
            for (auto &it : properties){
                it.second.reset();
            }
            properties.clear();
            uint32_t p_len = properties_len;
            while (p_len > 0) {
                uint8_t size_property;
                auto property = CreateProperty(buf + size, size_property);
                if (property == nullptr) {
                    return mqtt_err::mqtt_property_err;
                }
                AddProperty(property);
                p_len -= size_property;
                size += size_property;
            }
            return mqtt_err::ok;
        } else return mqtt_err::ok;
    } else return mqtt_err::var_int_err;
}

shared_ptr<MqttStringEntity> mqtt_protocol::CreateMqttStringEntity(const uint8_t *buf, uint8_t &size){
    if (buf == nullptr){
        size = 0;
        return nullptr;
    }
    uint16_t len;
    len = ConvertToHost2Bytes(buf);
    if (len == 0){
        size = 0;
        return nullptr;
    }
    size = sizeof(len);
    size += len;
    return make_shared<MqttStringEntity>(len, &buf[2]);
}

std::shared_ptr<MqttBinaryDataEntity> mqtt_protocol::CreateMqttBinaryDataEntity(const uint8_t *buf, uint8_t &size){
    if (buf == nullptr){
        size = 0;
        return nullptr;
    }
    uint16_t len;
    len = ConvertToHost2Bytes(buf);
    if (len == 0){
        size = 0;
        return nullptr;
    }
    size = sizeof(len);
    size += len;
    return make_shared<MqttBinaryDataEntity>(len, &buf[2]);
}

shared_ptr<MqttProperty> mqtt_protocol::CreateProperty(const uint8_t *buf, uint8_t &size){
    if (buf == nullptr){
        size = 0;
        return nullptr;
    }

    uint8_t id = buf[0];
    size = 1;
    switch (id){
        case payload_format_indicator: case request_problem_information: case request_response_information: case maximum_qos:
        case retain_available: case wildcard_subscription_available: case subscription_identifier_available: case shared_subscription_available: {
            size += 1;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttByteEntity(&buf[1])));
        }

        case server_keep_alive: case receive_maximum: case topic_alias_maximum: case topic_alias: {
            size += 2;
            uint16_t val = ConvertToHost2Bytes(&buf[1]);
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *)&val)));
        }

        case message_expiry_interval: case session_expiry_interval: case will_delay_interval: case maximum_packet_size:{
            size += 4;
            uint32_t val = ConvertToHost4Bytes(&buf[1]);
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *)&val)));
        }

        case content_type: case response_topic: case assigned_client_identifier: case authentication_method:
        case response_information: case server_reference: case reason_string: {
            uint16_t len;
            len = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len);
            uint16_t offset = size;
            size += len;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttStringEntity(len, &buf[offset])));
        }

        case correlation_data: case authentication_data: {
            uint16_t len;
            len = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len);
            uint16_t offset = size;
            size += len;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(len, &buf[offset])));
        }

        case user_property:{
            uint16_t len_1;
            uint16_t len_2;
            len_1 = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len_1);
            len_2 = ConvertToHost2Bytes(&buf[size + len_1]);
            size += len_1;
            size += sizeof(len_2);
            size += len_2;

            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttStringPairEntity(MqttStringEntity(len_1, &buf[1 + sizeof(len_1)]),
                                                                                         MqttStringEntity(len_2, &buf[1 + sizeof(len_1) + len_1 + sizeof(len_2)]))));
        }

        case subscription_identifier:{
            uint32_t val;
            uint8_t vint_size;
            uint8_t res = DeCodeVarInt(&buf[1], val, vint_size);
            size += vint_size;
            if (res == mqtt_err::ok){
                return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttVIntEntity((uint8_t *)&val)));
            } else {
                size = 0;
                return nullptr;
            }
        }

        default: {
            size = 0;
            return nullptr;
        }
    }
}

shared_ptr<uint8_t> mqtt_protocol::CreateMqttPacket(uint8_t pack_type, VariableHeader &vh, uint32_t &size){
    FixedHeader fh(pack_type);

    size = vh.GetSize();
    fh.remaining_len = size;
    size += fh.Size();

    auto ptr = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    if (ptr == nullptr){
        size = 0;
        return nullptr;
    }
    uint32_t offset = 0;
    fh.Serialize(ptr.get(), offset);
    vh.Serialize(ptr.get() + offset, offset);

    assert(size == offset);
    return ptr;
}

shared_ptr<uint8_t> mqtt_protocol::CreateMqttPacket(uint8_t pack_type, uint32_t &size){
    FixedHeader fh(pack_type);
    size = 0;

    fh.remaining_len = size;
    size += fh.Size();

    auto ptr = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    if (ptr == nullptr){
        size = 0;
        return nullptr;
    }
    uint32_t offset = 0;
    fh.Serialize(ptr.get(), offset);

    return ptr;
}

shared_ptr<uint8_t> mqtt_protocol::CreateMqttPacket(uint8_t pack_type, VariableHeader &vh, const shared_ptr<MqttBinaryDataEntity> &message, uint32_t &size){
    FixedHeader fh(pack_type);

    size = vh.GetSize() + message->Size()-2;
    if(fh.QoS() == 0) size -= 2; //delete id packet if quos is 0
	
    fh.remaining_len = size;
    size += fh.Size();

    auto ptr = shared_ptr<uint8_t>(new uint8_t[size], default_delete<uint8_t[]>());
    if (ptr == nullptr){
        size = 0;
        return nullptr;
    }
    uint32_t offset = 0;
	
    fh.Serialize(ptr.get(), offset);
    vh.Serialize(ptr.get() + offset, offset);
    message->SerializeWithoutLen(ptr.get() + offset, offset);
    
	if (size != offset){
		cout << "mqtt_broker assertion error " << size << " " << offset << endl;	
	}

    assert(size == offset);
    return ptr;
}

MqttPropertyChainBuilder& MqttPropertyChainBuilder::withClientIdentifier(const std::string &id) {
    properties.AddProperty(make_shared<MqttProperty>(assigned_client_identifier, shared_ptr<MqttEntity>(new MqttStringEntity(id))));
    return *this;
}

MqttPropertyChainBuilder& MqttPropertyChainBuilder::withMaxPocketSize(const unsigned int max_pocket_size) {
    properties.AddProperty(make_shared<MqttProperty>(maximum_packet_size, shared_ptr<MqttEntity>(new MqttFourByteEntity(max_pocket_size))));
    return *this;
}

MqttPropertyChainBuilder& MqttPropertyChainBuilder::withRetainAvailable(const uint8_t value){
    properties.AddProperty(make_shared<MqttProperty>(retain_available, shared_ptr<MqttEntity>(new MqttByteEntity(value))));
    return *this;
}


MqttPropertyChainBuilder& MqttPropertyChainBuilder::withWildCard(const uint8_t value){
    properties.AddProperty(make_shared<MqttProperty>(wildcard_subscription_available, shared_ptr<MqttEntity>(new MqttByteEntity(value))));
    return *this;
}

MqttPropertyChainBuilder& MqttPropertyChainBuilder::withSharedSubAvailable(const uint8_t value){
    properties.AddProperty(make_shared<MqttProperty>(shared_subscription_available, shared_ptr<MqttEntity>(new MqttByteEntity(value))));
    return *this;
}

MqttPropertyChain& MqttPropertyChainBuilder::build() {
    return properties;
}