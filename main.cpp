#include <iostream>
#include "version.h"
#include <vector>
#include <map>
#include<list>

#include "spdlog/spdlog.h"

using namespace std;

template <typename T>
void PrintType [[maybe_unused]](T) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

int main(int argc, char *argv[]) {


    return EXIT_FAILURE;
}
