#pragma once

#include <ACAN2515.h>

class CANInterface {
public:
    static bool begin();
    static bool available();
    static void getNextMessage(CANMessage &msg);
};