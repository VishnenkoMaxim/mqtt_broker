//
// Created by Vishnenko Maxim on 06.04.2023.
//

#ifndef MQTT_BROKER_FUNCTIONS_H
#define MQTT_BROKER_FUNCTIONS_H

#include <memory>
#include "spdlog/spdlog.h"

using namespace std;

namespace temp_funcs {
    template<int V, int num>
    struct mult {
        static const int value = V * mult<V, num - 1>::value;
    };

    template<int V>
    struct mult<V, 0> {
        static const int value = 1;
    };
}

template <typename T>
void PrintType [[maybe_unused]] (T);

int WriteData(int fd, uint8_t * data, unsigned int size);
int ReadData(int fd, uint8_t* data, int size, unsigned int timeout);
uint16_t ConvertToHost2Bytes(const uint8_t* buf);
uint32_t ConvertToHost4Bytes(const uint8_t* buf);

void SetLogLevel(shared_ptr<spdlog::logger> lg, int _level) noexcept;

#endif //MQTT_BROKER_FUNCTIONS_H
