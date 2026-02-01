#include "LEDAProtocol.h"
#include <Arduino.h>

// Schwellenwerte für die Logik (wie im alten can_handler)
const int16_t CRITICAL_TEMPERATURE_THRESHOLD = 750;
const int16_t CRITICAL_TEMPERATURE_HYSTERESIS = 10;

/**
 * @brief Decodes two bytes from a buffer into a 16-bit unsigned integer (Little-Endian).
 */
static uint16_t decodeUInt16(const uint8_t* buffer, size_t size) {
    if (size < 2) {
        Serial.println(F("ERROR: Insufficient buffer size for 16-bit unsigned decode."));
        return 0;
    }
    // Little-Endian: buffer[0] ist LSB, buffer[1] ist MSB
    return (uint16_t)((buffer[1] << 8) | buffer[0]);
}

/**
 * @brief Processes and decodes a received CAN message based on its ID.
 *
 * @param msg The CAN message object.
 * @param data Reference to the Data structure to update.
 * @return true if the message ID and type were recognized and processed, false otherwise.
 */
void LEDAProtocol::parseFrame(const CANMessage &msg, Data &data) {
    int len;
    char printBuffer[256];
    switch (msg.id) {
        case 0x281: {
            // Byte 0 & 1: Combustion Temperature (16-bit, LSB first in °C)
            data.combustion_temp.set((int16_t)decodeUInt16(msg.data + 0, 2));
            
            // Byte 2: Air Flap Actual Position
            data.air_flap_act.set(msg.data[2]);

            // Byte 3: Air Flap Target Position
            data.air_flap_target.set(msg.data[3]);

            // Byte 4: Oven State Number
            data.oven_state_num.set(msg.data[4]);

            // Byte 5: Raw byte for reference
            data.byte281_5.set(msg.data[5]);

            // Byte 6: Raw byte for reference
            data.byte281_6.set(msg.data[6]);
            
            // Byte 7: Controller Version
            data.controller_version.set(msg.data[7]);

            // Debug-Ausgabe der dekodierten Werte (Added leading \n for blank line)
            len = snprintf(
                printBuffer, 
                sizeof(printBuffer), 
                "\n[CAN] Message 0x281 decoded:\n"
                "  %-22s : %d °C\n"
                "  %-22s : %u %%\n"
                "  %-22s : %u %%\n"
                "  %-22s : %u\n"
                "  %-22s : %u\n",
                "Combustion Temp",  data.combustion_temp.value,
                "Air Flap Actual",  data.air_flap_act.value,
                "Air Flap Target",  data.air_flap_target.value,
                "Oven State Num",   data.oven_state_num.value,
                "Controller Ver",   data.controller_version.value
            );
            Serial.print(printBuffer);
            break;
        } 

        case 0x283: {
            uint8_t message_type = msg.data[0];
            
            switch (message_type) {
                case 1: { // Use braces for the inner case to guarantee scope
                    // Byte 1 & 2: Max Combustion Temperature (16-bit, LSB first in °C)
                    data.max_combustion_temp.set((int16_t)decodeUInt16(msg.data + 1, 2));
                    
                    // Byte 3 & 4: Grundglut Temperature (16-bit, LSB first in °C)
                    data.smoldering_temp.set((int16_t)decodeUInt16(msg.data + 3, 2));

                    // Byte 5: Trend
                    data.trend.set((int8_t)msg.data[5]); 

                    // Byte 6: Store raw byte for reference
                    data.byte283_1_6.set(msg.data[6]);
                    
                    // Debug-Ausgabe der dekodierten Werte
                    len = snprintf(
                        printBuffer, 
                        sizeof(printBuffer), 
                        "\n[CAN] Message 0x283 Type 1 decoded:\n"
                        "  %-22s : %d °C\n"
                        "  %-22s : %d °C\n"
                        "  %-22s : %d\n",
                        "Max Combustion Temp", data.max_combustion_temp.value,
                        "Smoldering Temp",     data.smoldering_temp.value,
                        "Trend",               data.trend.value
                    );
                    Serial.print(printBuffer);
                    break;
                }
                    
                case 2: { // Statistics Data (Cycles, Errors)

                    // Byte 1 & 2: Unkown Counter
                    data.unknown_counter.set(decodeUInt16(msg.data + 1, 2));

                    
                    // Byte 3 & 4: Burn Cycles
                    data.burn_cycles.set(decodeUInt16(msg.data + 3, 2));

                    // Byte 5 & 6: Heating Error Count
                    data.heating_error_count.set(decodeUInt16(msg.data + 5, 2));

                    // Debug-Ausgabe der dekodierten Werte für 0x283 Type 2
                    len = snprintf(
                        printBuffer, 
                        sizeof(printBuffer), 
                        "\n[CAN] Message 0x283 Type 2 decoded:\n"
                        "  %-22s : %u\n"
                        "  %-22s : %u\n"
                        "  %-22s : %u\n",
                        "Unknown Counter",       data.unknown_counter.value,
                        "Burn Cycles",           data.burn_cycles.value,
                        "Heating Error Count",   data.heating_error_count.value
                    );
                    Serial.print(printBuffer);
                    break;
                }

                case 3: { // TBD/Diagnostics Data

                    // Byte 1 & 2: Heating Errors
                    data.byte283_3_1.set(msg.data[1]);
                    data.byte283_3_2.set(msg.data[2]);
                    
                    // Byte 3-6: Raw bytes
                    data.byte283_3_3.set(msg.data[3]);
                    data.byte283_3_4.set(msg.data[4]);
                    data.byte283_3_5.set(msg.data[5]);
                    data.byte283_3_6.set(msg.data[6]);
                    
                    // Debug-Ausgabe der dekodierten Werte für 0x283 Type 3
                    len = snprintf(
                        printBuffer, 
                        sizeof(printBuffer), 
                        "\n[CAN] Message 0x283 Type 3 decoded:\n"
                        "  %-22s : 0x%02X\n"
                        "  %-22s : 0x%02X\n"
                        "  %-22s : 0x%02X\n"
                        "  %-22s : 0x%02X\n"
                        "  %-22s : 0x%02X\n"
                        "  %-22s : 0x%02X\n",
                        "Raw Byte 1", data.byte283_3_1.value,
                        "Raw Byte 2", data.byte283_3_2.value,
                        "Raw Byte 3", data.byte283_3_3.value,
                        "Raw Byte 4", data.byte283_3_4.value,
                        "Raw Byte 5", data.byte283_3_5.value,
                        "Raw Byte 6", data.byte283_3_6.value
                    );
                    Serial.print(printBuffer);
                    break;
                }

                default:
                    Serial.print("Unrecognized 0x283 Type: ");
                    Serial.println(msg.data[0]);
                    break;
            }
        } 

        default:
            Serial.print("Unknown CAN ID: ");
            Serial.println(msg.id, HEX);
            break;
    }
}


/**
 * @brief Verarbeitet und validiert die dekodierten CAN-Bus-Werte des Ofens.
 * * Diese Funktion führt ein Post-Processing der empfangenen Rohdaten durch:
 * - Setzt Status-Flags für Heizbetrieb und Fehlerzustände.
 * - Überwacht die Verbrennungstemperatur mit einer Hysterese 
 * - Übersetzt den numerischen Ofenstatus (oven_state_num) in einen menschenlesbaren Text.
 */
void LEDAProtocol::runPostProcessing(Data &data) {
    uint8_t s = data.oven_state_num.value;
    int16_t temp = data.combustion_temp.value;

    // 1. Status-Text Zuordnung
    const char* text = getOvenStateText(s);
    if (strcmp(data.oven_state_text, text) != 0) {
        strncpy(data.oven_state_text, text, sizeof(data.oven_state_text) - 1);
        data.oven_state_text[sizeof(data.oven_state_text) - 1] = '\0';
        // Wir setzen hier manuell ein Flag, falls das Modul auch den Text senden soll
    }

    // 2. Logische Zustände ableiten 
    bool isActive = (s >= 1 && s <= 4) || s == 8; 
    data.oven_heated.set(isActive && (data.air_flap_act.value > 0));

    // 3. Kritische Temperatur mit Hysterese
    if (temp > CRITICAL_TEMPERATURE_THRESHOLD && !data.critical_temperature.value) {
        data.critical_temperature.set(true);
    } 
    else if (temp < (CRITICAL_TEMPERATURE_THRESHOLD - CRITICAL_TEMPERATURE_HYSTERESIS) && data.critical_temperature.value) {
        data.critical_temperature.set(false);
    }

    // 4. Fehler-Flags
    data.heating_error.set(s == 97 || s == 99); // Beispielhafte Fehler-IDs
    data.ember_bed.set(s == 7); // Grundglut
}

const char* LEDAProtocol::getOvenStateText(uint8_t code) {
    switch (code) {
        case 0:  return "Bereit";
        case 1:  return "Start";
        case 2:
        case 3:  return "Anheizen";
        case 4:  return "Heizbetrieb";
        case 5:  return "Ende";
        case 6:  return "Pause";
        case 7:  return "Grundglut";
        case 8:  return "Nachlegen";
        case 97: return "Anheizfehler";
        case 99: return "Sicherheitsaus";
        default: return "Unbekannt";
    }
}