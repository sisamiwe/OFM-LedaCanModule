#pragma once

#include <stdint.h>

template<typename T>
struct Field {
    T value;
    bool updated = false;

    void set(T newValue) {
        if (value != newValue) {
            value = newValue;
            updated = true;
        }
    }
    
    bool isDirty() { return updated; }
    void clearDirty() { updated = false; }
};

/**
 * @brief Zentralstruktur für dekodierte Daten
 */
struct Data {
    // Messwerte
    Field<int16_t>  combustion_temp;
    Field<uint8_t>  air_flap_act;
    Field<uint8_t>  air_flap_target;
    Field<uint8_t>  oven_state_num;
    Field<uint8_t>  controller_version;
    Field<int8_t>   trend;
    Field<int16_t>  max_combustion_temp;
    Field<int16_t>  smoldering_temp;
    Field<uint16_t> burn_cycles;
    Field<uint16_t> heating_error_count;
    Field<uint16_t> unknown_counter;
    
    // Roh-Bytes / Diagnose
    Field<uint8_t>  byte281_5;
    Field<uint8_t>  byte281_6;
    Field<uint8_t>  byte283_1_6;
    Field<uint8_t>  byte283_3_1;
    Field<uint8_t>  byte283_3_2;
    Field<uint8_t>  byte283_3_3;
    Field<uint8_t>  byte283_3_4;
    Field<uint8_t>  byte283_3_5;
    Field<uint8_t>  byte283_3_6;
    
    // Status-Flags (Bools)
    Field<bool>     oven_heated;
    Field<bool>     ember_bed;
    Field<bool>     heating_error;
    Field<bool>     critical_temperature;
    Field<bool>     can_bus_error;
    Field<bool>     is_online;
    Field<bool>     connection_lost_error;

    // Sonderbehandlung String
    char oven_state_text[15];
    bool oven_state_text_updated;

    uint32_t lastUpdateTimestamp;
};

/**
 * @brief Standardwerte für den Systemstart.
 * Format: { Wert, Updated-Flag }
 */
static constexpr Data DEFAULT_VALUES = {
    .combustion_temp       = {-1000, false},
    .air_flap_act          = {255,   false},
    .air_flap_target       = {255,   false},
    .oven_state_num        = {255,   false},
    .controller_version    = {255,   false},
    .trend                 = {127,   false},
    .max_combustion_temp   = {-1000, false},
    .smoldering_temp       = {-1000, false},
    .burn_cycles           = {0,     false},
    .heating_error_count   = {0,     false},
    .unknown_counter       = {0,     false},
    
    .byte281_5             = {0,     false},
    .byte281_6             = {0,     false},
    .byte283_1_6           = {0,     false},
    .byte283_3_1           = {0,     false},
    .byte283_3_2           = {0,     false},
    .byte283_3_3           = {0,     false},
    .byte283_3_4           = {0,     false},
    .byte283_3_5           = {0,     false},
    .byte283_3_6           = {0,     false},
    
    .oven_heated           = {false, false},
    .ember_bed             = {false, false},
    .heating_error         = {false, false},
    .critical_temperature  = {false, false},
    .can_bus_error         = {false, false},
    .is_online             = {false, false},
    .connection_lost_error = {false, false},

    .oven_state_text       = "Initial",
    .oven_state_text_updated = false,
    
    .lastUpdateTimestamp   = 0
};

/**
 * @brief Enumeration der Ofenzustände.
 */

enum class OvenState : uint8_t {
    Ready = 0,
    Start = 1,
    PreHeating = 2, // Fall 2 & 3
    Heating = 4,
    End = 5,
    Pause = 6,
    EmberBed = 7,
    Reload = 8,
    HeatingError = 97,
    DoorOpen = 98,
    SensorError = 99,
    Unknown = 255
};