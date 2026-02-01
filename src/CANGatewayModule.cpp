#include "CANGatewayModule.h"

// Die Indizes müssen mit deiner knx.xml übereinstimmen
// --- Hauptwerte ---
#define KO_COMBUSTION_TEMP       0  // DPT 9.001 (Temperatur °C)
#define KO_OVEN_STATE_NUM        1  // DPT 5.005 (Status Code)
#define KO_AIR_FLAP_ACT          2  // DPT 5.005 (Ist-Position %) damit ganze Zahlen angezeigt werden, wird als DPT 5.005 gesendet
#define KO_AIR_FLAP_TARGET       3  // DPT 5.005 (Soll-Position %) damit ganze Zahlen angezeigt werden, wird als DPT 5.005 gesendet
#define KO_CRITICAL_TEMP_ALARM   4  // DPT 1.005 (Alarm)

// --- Erweiterte Status-Werte ---
#define KO_OVEN_HEATED           5  // DPT 1.003 (Binär: Heizt/Heizt nicht)
#define KO_EMBER_BED             6  // DPT 1.002 (Binär: Grundglut ja/nein)
#define KO_HEATING_ERROR         7  // DPT 1.005 (Binär: Fehler ja/nein)
#define KO_TREND                 8  // DPT 6.001 (Trend-Indikator)

// --- Statistiken ---
#define KO_MAX_COMBUSTION_TEMP   9  // DPT 9.001 (°C)
#define KO_SMOLDERING_TEMP      10  // DPT 9.001 (°C)
#define KO_BURN_CYCLES          11  // DPT 7.001 (Zähler)
#define KO_HEATING_ERROR_COUNT  12  // DPT 7.001 (Zähler)

// --- Diagnose & Version ---
#define KO_CONTROLLER_VERSION   13  // DPT 5.010 (Version)
#define KO_IS_ONLINE            14  // DPT 1.011 (Status: Gateway erreichbar)
#define KO_CONNECTION_LOST      15  // DPT 1.005 (Alarm: Timeout zum Ofen)



CANGateway::CANGateway() {}

void CANGateway::setup()
{
    // Initialisierung der CAN-Hardware über unsere Abstraktionsschicht
    if (!CANInterface::begin()) {
        logErrorP("CAN Hardware konnte nicht gestartet werden!");
    } else {
        logInfoP("CAN Hardware erfolgreich initialisiert (125k).");
    }
}

void CANGateway::loop1() {
    // 1. Alle verfügbaren CAN-Nachrichten verarbeiten
    CANMessage msg;
    while (CANInterface::available())
    {
        CANInterface::getNextMessage(msg);

        // --- Debug Ausgaben ---
        if (msg.len == 8) 
        {
            Serial.println("");
            Serial.print("[CAN] Received CAN Message: ");
            Serial.print("  id: "); Serial.println(msg.id, HEX);
            Serial.print("  len: "); Serial.println(msg.len);
            Serial.print("  data: ");

            for(int x = 0; x < msg.len; x++) {
                Serial.print(msg.data[x], DEC); 
                Serial.print(" ");
            }
            Serial.println(""); // Zeilenumbruch nach den Daten
        }

        // --- Verarbeitung ---
        // Schicht B (Protokoll) füllt Schicht C (Datenmodell)
        LEDAProtocol::parseFrame(msg, _data);
        
        // Zeitstempel für "Online"-Check aktualisieren
        _data.lastUpdateTimestamp = millis();
    }

    // 2. Nach der Verarbeitung der CAN-Nachrichten prüfen, was an KNX muss
    syncDataToKNX();
}


void CANGateway::syncDataToKNX() {
    // 1. Prüfung auf Heartbeat / Zyklisches Senden (alle 5 Min)
    bool forceSend = (millis() - _lastCyclicSend > CYCLIC_SEND_INTERVAL);
    
    // 2. Prüfung auf Verbindungsverlust (Timeout nach 60 Sek)
    // Wir nehmen an, dass _data.lastUpdateTimestamp in der loop() gesetzt wird
    bool isOnline = (millis() - _data.lastUpdateTimestamp < 60000);
    
    if (_data.is_online.value != isOnline || forceSend) {
        _data.is_online.set(isOnline);
        knx.getGroupObject(KO_IS_ONLINE).value(isOnline, Dpt(1, 11));
        knx.getGroupObject(KO_CONNECTION_LOST).value(!isOnline, Dpt(1, 5));
    }

    // --- Hauptwerte ---
    if (_data.combustion_temp.updated || forceSend) {
        knx.getGroupObject(KO_COMBUSTION_TEMP).value((float)_data.combustion_temp.value, Dpt(9, 1));
        _data.combustion_temp.updated = false;
    }

    if (_data.oven_state_num.updated || forceSend) {
        knx.getGroupObject(KO_OVEN_STATE_NUM).value(_data.oven_state_num.value, Dpt(5, 5));
        _data.oven_state_num.updated = false;
    }

    if (_data.air_flap_act.updated || forceSend) {
        knx.getGroupObject(KO_AIR_FLAP_ACT).value(_data.air_flap_act.value, Dpt(5, 5));
        _data.air_flap_act.updated = false;
    }

    if (_data.air_flap_target.updated || forceSend) {
        knx.getGroupObject(KO_AIR_FLAP_TARGET).value(_data.air_flap_target.value, Dpt(5, 5));
        _data.air_flap_target.updated = false;
    }

    if (_data.critical_temperature.updated || forceSend) {
        knx.getGroupObject(KO_CRITICAL_TEMP_ALARM).value(_data.critical_temperature.value, Dpt(1, 5));
        _data.critical_temperature.updated = false;
    }

    // --- Erweiterte Status-Werte ---
    if (_data.oven_heated.updated || forceSend) {
        knx.getGroupObject(KO_OVEN_HEATED).value(_data.oven_heated.value, Dpt(1, 3));
        _data.oven_heated.updated = false;
    }

    if (_data.ember_bed.updated || forceSend) {
        knx.getGroupObject(KO_EMBER_BED).value(_data.ember_bed.value, Dpt(1, 2));
        _data.ember_bed.updated = false;
    }

    if (_data.heating_error.updated || forceSend) {
        knx.getGroupObject(KO_HEATING_ERROR).value(_data.heating_error.value, Dpt(1, 5));
        _data.heating_error.updated = false;
    }

    if (_data.trend.updated || forceSend) {
        knx.getGroupObject(KO_TREND).value(_data.trend.value, Dpt(6, 1));
        _data.trend.updated = false;
    }

    // --- Statistiken ---
    if (_data.max_combustion_temp.updated || forceSend) {
        knx.getGroupObject(KO_MAX_COMBUSTION_TEMP).value((float)_data.max_combustion_temp.value, Dpt(9, 1));
        _data.max_combustion_temp.updated = false;
    }

    if (_data.smoldering_temp.updated || forceSend) {
        knx.getGroupObject(KO_SMOLDERING_TEMP).value((float)_data.smoldering_temp.value, Dpt(9, 1));
        _data.smoldering_temp.updated = false;
    }

    if (_data.burn_cycles.updated || forceSend) {
        knx.getGroupObject(KO_BURN_CYCLES).value(_data.burn_cycles.value, Dpt(7, 1));
        _data.burn_cycles.updated = false;
    }

    if (_data.heating_error_count.updated || forceSend) {
        knx.getGroupObject(KO_HEATING_ERROR_COUNT).value(_data.heating_error_count.value, Dpt(7, 1));
        _data.heating_error_count.updated = false;
    }

    // --- Diagnose ---
    if (_data.controller_version.updated || forceSend) {
        knx.getGroupObject(KO_CONTROLLER_VERSION).value(_data.controller_version.value, Dpt(5, 10));
        _data.controller_version.updated = false;
    }

    // Zeitstempel für zyklisches Senden aktualisieren
    if (forceSend) {
        _lastCyclicSend = millis();
    }
}