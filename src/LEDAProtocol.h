#pragma once

#include <ACAN2515.h>
#include "DataModel.h"

class LEDAProtocol {
public:
    static void parseFrame(const CANMessage &msg, Data &data);
private: // WICHTIG: Diese Sektion muss existieren
    static void runPostProcessing(Data &data);
    static const char* getOvenStateText(uint8_t code);
};