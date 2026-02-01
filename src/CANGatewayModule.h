#pragma once

#include "OpenKNX.h"
#include "DataModel.h"
#include "CANInterface.h"
#include "LEDAProtocol.h"
#include <string>

class CANGateway : public OpenKNX::Module
{
public:
    CANGateway();

    const std::string name() override { return "CANGateway"; }
    const std::string version() override { return "0.0.1"; }

    void setup() override;
    void loop1() override;


private:
    Data _data;
    void syncDataToKNX();
    uint32_t _lastCyclicSend = 0;
    const uint32_t CYCLIC_SEND_INTERVAL = 300000;
};