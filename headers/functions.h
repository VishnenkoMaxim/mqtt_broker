//
// Created by Vishnenko Maxim on 06.04.2023.
//

#ifndef MQTT_BROKER_FUNCTIONS_H
#define MQTT_BROKER_FUNCTIONS_H

#include <memory>
#include <list>
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

void SetLogLevel(const shared_ptr<spdlog::logger>& lg, int _level) noexcept;

template <class T>
struct PoolData{
    shared_ptr<T> pool;
    int32_t used_elements;

    PoolData() = delete;

    PoolData(unsigned int num_elements) : used_elements(0) {
        pool.reset(static_cast<T*>(::operator new(num_elements*sizeof(T))));
    }
};

template <class T, int num>
class PoolAllocator{
private:
    list<PoolData<T>> pools;
    size_t pool_size;
    T* cur_pointer = nullptr;

public:
    using value_type = T;
    void AllocateNewPool(){
        PoolData<T> pd(pool_size);
        pools.push_back(pd);
        cur_pointer = static_cast<T*>(pools.back().pool.get());
    }

    PoolAllocator() noexcept : pool_size(num) {
        //
    };

    template <class U> PoolAllocator (const PoolAllocator<U, num>&) noexcept {}

    T* allocate (size_t n){
        if (n > pool_size) throw bad_alloc();
        if ((pools.size() == 0) || (pools.back().used_elements + n > num)) AllocateNewPool();

        pools.back().used_elements += n;
        return static_cast<T*>(::operator new(n, cur_pointer + pools.back().used_elements - n));
    }

    void deallocate (T* p, size_t n) {
        if (n == 0) return;
        if (p == nullptr) return;

        for(auto it=pools.begin(); it != pools.end(); it++){
            if (it->pool.get() <= p && it->pool.get()+pool_size > p) {
                it->used_elements -= n;
                if (it->used_elements == 0) {
                    it->pool.reset();
                    pools.erase(it);
                    if (pools.size()) {
                        cur_pointer = static_cast<T*>(pools.back().pool.get());
                    }
                    break;
                }
            }
        }
    }

    template<class U>
    struct rebind {
        typedef PoolAllocator<U, num> other;
    };

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type; //UB if std::false_type and a1 != a2;
};

std::string GenRandom(const uint8_t len);

template <class Type>
ostream& operator << (ostream &os, const vector<Type> &_vec);

#endif //MQTT_BROKER_FUNCTIONS_H
