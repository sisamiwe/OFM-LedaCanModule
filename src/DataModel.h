#pragma once

#include <stdint.h>
#include <Arduino.h> // Für millis()

/**
 * @brief Intelligentes Datenfeld, das Wert, Status und Zeitstempel kapselt.
 */
template<typename T>
struct Field {
    T value;
    bool updated = false;
    uint32_t lastSentMillis = 0;

    /**
     * @brief Aktualisiert den Wert und setzt das Dirty-Flag nur bei echter Änderung.
     */
    void set(T newValue) {
        if (value != newValue) {
            value = newValue;
            updated = true;
        }
    }

    /**
     * @brief Prüft, ob seit dem letzten Senden mehr Zeit vergangen ist als im Intervall definiert.
     * @param intervalMinutes Minuten aus dem KNX-Parameter (0 = deaktiviert)
     */
    bool cycleElapsed(uint32_t intervalMinutes) {
        if (intervalMinutes == 0) return false;
        return (millis() - lastSentMillis >= (intervalMinutes * 60000));
    }

    /**
     * @brief Setzt den Zeitstempel auf 'jetzt' und löscht das Dirty-Flag.
     * Muss nach jedem erfolgreichen KNX-Senden aufgerufen werden.
     */
    void markAsSent() {
        lastSentMillis = millis();
        updated = false;
    }
};

/**
 * @brief Zentralstruktur für alle Ofendaten.
 */
struct Data {
    // Numerische Messwerte (Hysterese + Zyklus möglich)
    Field<int16_t>  combustion_temp;
    Field<int16_t>  max_combustion_temp;
    Field<int16_t>  smoldering_temp;
    Field<uint8_t>  air_flap_act;
    Field<uint8_t>  air_flap_target;
    Field<int8_t>   trend;
    
    // Status-Werte (Meist nur Zyklus)
    Field<uint8_t>  oven_state_num;
    char            oven_state_text[20]; // Text braucht Sonderbehandlung (kein Field-Template)
    uint32_t        oven_state_text_lastSent = 0;

    // Zähler und Versionen (Meist Event-basiert / updated-Flag)
    Field<uint16_t> burn_cycles;
    Field<uint16_t> heating_error_count;
    Field<uint8_t>  controller_version;

    // Binäre Zustände (DPT 1.x)
    Field<bool>     oven_heated;
    Field<bool>     heating_error;
    Field<bool>     ember_bed;
    Field<bool>     critical_temperature;
    Field<bool>     can_bus_error;
    Field<bool>     is_online;

    // Noch unbekannte Werte (nur für Debugging, nicht an KNX)
    Field<uint8_t>  byte281_5;
    Field<uint8_t>  byte281_6;
    Field<uint16_t> unknown_counter;
    Field<uint8_t>  byte283_1_6;
    Field<uint8_t>  byte283_3_1;
    Field<uint8_t>  byte283_3_2;
    Field<uint8_t>  byte283_3_3;
    Field<uint8_t>  byte283_3_4;
    Field<uint8_t>  byte283_3_5;
    Field<uint8_t>  byte283_3_6;

    // Zeitstempel für den Online-Check
    uint32_t        lastUpdateTimestamp = 0;
};

/**
 * @brief Standard-Initialisierungswerte
 */
const Data DEFAULT_VALUES = {
    .combustion_temp        = {-1000, false, 0},
    .max_combustion_temp    = {-1000, false, 0},
    .smoldering_temp        = {-1000, false, 0},
    .air_flap_act           = {255,   false, 0},
    .air_flap_target        = {255,   false, 0},
    .trend                  = {127,   false, 0},
    .oven_state_num         = {255,   false, 0},
    .oven_state_text        = "Initialisierung",
    .oven_state_text_lastSent = 0,
    .burn_cycles            = {0,     false, 0},
    .heating_error_count    = {0,     false, 0},
    .controller_version     = {0,     false, 0},
    .oven_heated            = {false, false, 0},
    .heating_error          = {false, false, 0},
    .ember_bed              = {false, false, 0},
    .critical_temperature   = {false, false, 0},
    .can_bus_error          = {false, false, 0},
    .is_online              = {false, false, 0},
    .lastUpdateTimestamp    = 0
};