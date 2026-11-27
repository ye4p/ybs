#include <iostream>
#include "../include/Engine.h"
#include "../include/Car.h"

int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
    }
    Engine e;
    e.model_ = std::string("v8");
    e.say_model();
    Car c;

    c.e_ = e;
    c.gas();
}
