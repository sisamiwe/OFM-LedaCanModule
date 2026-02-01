#include "CANInterface.h"
#include "hardware.h" // Für PIN-Definitionen


//--- CAN Quartz frequency
const uint32_t QUARTZ_FREQUENCY = 16 * 1000 * 1000 ; // 16 MHz
const uint32_t BIT_RATE = 125 * 1000 ; // 125 kbit/s

// CAN Objekt
ACAN2515 can(CAN0_CS_PIN, SPI, CAN0_INT_PIN);

bool CANInterface::begin() {
   //--- Set the SPI pins
    SPI.setSCK(CAN0_SPI_SCK_PIN);    // SCK
    SPI.setTX(CAN0_SPI_MOSI_PIN);    // MOSI
    SPI.setRX(CAN0_SPI_MISO_PIN);    // MISO
    SPI.setCS(CAN0_CS_PIN);          // Chip Select
    SPI.begin();

    // Configure the CAN controller settings
    ACAN2515Settings settings(QUARTZ_FREQUENCY, BIT_RATE);
    // Request Normal Mode for standard CAN communication
    settings.mRequestedMode = ACAN2515Settings::NormalMode;

    // Begin the CAN module initialization
    // Passes the settings and a lambda function for the interrupt service routine (ISR)
    const uint16_t errorCode = can.begin(settings, [] { can.isr(); });

    // Check for initialization errors
    if (0 == errorCode) {
        // Initialization succeeded
        Serial.println(F("CAN module initialized successfully"));
    } else {
        // Initialization failed
        Serial.print(F("Error: CAN module initialization failed with error: 0x"));
        Serial.println(errorCode, HEX); // Print the error code in hexadecimal
    }
    return (0 == errorCode);
}

bool CANInterface::available() {
    return can.available();
}

void CANInterface::getNextMessage(CANMessage &msg) {
    can.receive(msg);
}