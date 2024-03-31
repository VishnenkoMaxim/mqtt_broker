#include "mqtt_broker.h"

using namespace std;
using namespace mqtt_protocol;

IMqttErrorHandler::IMqttErrorHandler(const int _err) : error(_err) {}

int IMqttErrorHandler::GetErr() {return error;}

//Disconnect
MqttDisconnectErr::MqttDisconnectErr() : IMqttErrorHandler(mqtt_err::disconnect) {}

void MqttDisconnectErr::HandleError(Broker& broker, [[maybe_unused]] const int fd){
    broker.lg->debug("handle_stat disconnect OK");
}

//unsupported_protocol_version
MqttProtocolVersionErr::MqttProtocolVersionErr() : IMqttErrorHandler(mqtt_err::protocol_version_err) {}

void MqttProtocolVersionErr::HandleError(Broker& broker, const int fd){
    broker.lg->error("Protocol version err");
    uint32_t answer_size;
    VariableHeader answer_vh{unique_ptr<IVariableHeader>(new ConnactVH(0, unsupported_protocol_version, MqttPropertyChain{}))};
    auto data = CreateMqttPacket(FHBuilder().PacketType(mqtt_pack_type::CONNACK).Build(), answer_vh, answer_size);

    WriteData(fd, data.get(), answer_size);
}

//unsupported_protocol_version
MqttDuplicateIDErr::MqttDuplicateIDErr() : IMqttErrorHandler(mqtt_err::duplicate_client_id) {}

void MqttDuplicateIDErr::HandleError(Broker& broker, const int fd){
    broker.lg->error("Protocol version err");
    uint32_t answer_size;
    VariableHeader answer_vh{unique_ptr<IVariableHeader>(new ConnactVH(0, client_identifier_not_valid, MqttPropertyChain{}))};
    auto data = CreateMqttPacket(FHBuilder().PacketType(mqtt_pack_type::CONNACK).Build(), answer_vh, answer_size);

    WriteData(fd, data.get(), answer_size);
}

//handle error
MqttHandleErr::MqttHandleErr() : IMqttErrorHandler(mqtt_err::handle_error) {}

void MqttHandleErr::HandleError(Broker& broker, [[maybe_unused]] const int fd){
    broker.lg->warn("Unsupported mqtt packet. Ignore it.");
}

//------------------------------------------------------------------------------------------
void MqttErrorHandler::AddErrorHandler(std::shared_ptr<IMqttErrorHandler> handler){
    handlers.push_back(handler);
}

void MqttErrorHandler::HandleError(int error, Broker& broker, int fd){
    for(const auto& it : handlers){
        if (it->GetErr() == error) {
            it->HandleError(broker, fd);
            return;
        }
    }
}
