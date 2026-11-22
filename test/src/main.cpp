#include <iostream>
#include "../include/Engine.h"
#include "../include/Car.h"

int main() {
    Engine e;
    e.model_ = std::string("v8");
    e.say_model();
    Car c;
    c.e_ = e;
    c.gas();
}
