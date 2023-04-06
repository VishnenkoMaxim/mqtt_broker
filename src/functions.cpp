//
// Created by Vishnenko Maxim on 06.04.2023.
//
#include <iostream>

using namespace std;

template <typename T>
void PrintType [[maybe_unused]](T) {
    cout << __PRETTY_FUNCTION__ << endl;
}