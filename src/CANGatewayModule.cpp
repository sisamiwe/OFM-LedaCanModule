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

uint32_t nun = millis();

    // Hilfsfunktion/Lambda für Zyklus-Check (Minuten in ms)
    auto checkCycle = [&](uint32_t lastMs, uint32_t intervalMinutes) {
        if (intervalMinutes == 0) return false; // 0 = "Nicht senden"
        return (nun - lastMs >= (intervalMinutes * 60000));
    };

    // --- 2. COMBUSTION_TEMP (Mit Hysterese & Zyklus) ---
    float cTemp = (float)_data.combustion_temp.value;
    if ((Param_CombTempSendChg_1 && abs(cTemp - _lastSentCombTemp) >= Param_CombTempAmount_1) || 
         checkCycle(_lastCombTempCycle, Param_CombTempCycle_1)) {
        knx.getGroupObject(KO_COMBUSTION_TEMP).value(cTemp, Dpt(9, 1));
        _lastSentCombTemp = cTemp;
        _lastCombTempCycle = nun;
    }

    // --- 3. MAX_COMBUSTION_TEMP (Mit Hysterese & Zyklus) ---
    float mTemp = (float)_data.max_combustion_temp.value;
    if ((Param_MaxCombTempSendChg_1 && abs(mTemp - _lastSentMaxTemp) >= Param_MaxCombTempAmount_1) || 
         checkCycle(_lastMaxTempCycle, Param_MaxCombTempCycle_1)) {
        knx.getGroupObject(KO_MAX_COMBUSTION_TEMP).value(mTemp, Dpt(9, 1));
        _lastSentMaxTemp = mTemp;
        _lastMaxTempCycle = nun;
    }

    // --- 4. SMOLDERING_TEMP (Mit Hysterese & Zyklus) ---
    float sTemp = (float)_data.smoldering_temp.value;
    if ((Param_SmoldTempSendChg_1 && abs(sTemp - _lastSentSmoldTemp) >= Param_SmoldTempAmount_1) || 
         checkCycle(_lastSmoldTempCycle, Param_SmoldTempCycle_1)) {
        knx.getGroupObject(KO_SMOLDERING_TEMP).value(sTemp, Dpt(9, 1));
        _lastSentSmoldTemp = sTemp;
        _lastSmoldTempCycle = nun;
    }

    // --- 5. AIRFLAP_ACT (DPT 5.005, Mit Hysterese & Zyklus) ---
    uint8_t fAct = _data.air_flap_act.value;
    if ((Param_AirActSendChg_1 && abs((int)fAct - _lastSentAirAct) >= Param_AirActAmount_1) || 
         checkCycle(_lastAirActCycle, Param_AirActCycle_1)) {
        knx.getGroupObject(KO_AIR_FLAP_ACT).value(fAct, Dpt(5, 5));
        _lastSentAirAct = fAct;
        _lastAirActCycle = nun;
    }

    // --- 6. AIRFLAP_TARGET (DPT 5.005, Mit Hysterese & Zyklus) ---
    uint8_t fTrg = _data.air_flap_target.value;
    if ((Param_AirTrgSendChg_1 && abs((int)fTrg - _lastSentAirTrg) >= Param_AirTrgAmount_1) || 
         checkCycle(_lastAirTrgCycle, Param_AirTrgCycle_1)) {
        knx.getGroupObject(KO_AIR_FLAP_TARGET).value(fTrg, Dpt(5, 5));
        _lastSentAirTrg = fTrg;
        _lastAirTrgCycle = nun;
    }

    // --- 7. OVEN_STATE_NUM (Nur Zyklus) ---
    if (checkCycle(_lastStateNumCycle, Param_StateNumCycle_1)) {
        knx.getGroupObject(KO_OVEN_STATE_NUM).value(_data.oven_state_num.value, Dpt(5, 5));
        _lastStateNumCycle = nun;
    }

    // --- 8. OVEN_STATE_TXT (Nur Zyklus) ---
    if (checkCycle(_lastStateTxtCycle, Param_StateTxtCycle_1)) {
        knx.getGroupObject(KO_OVEN_STATE_TXT).value(_data.oven_state_text.value, Dpt(16, 0));
        _lastStateTxtCycle = nun;
    }

    // --- 9. TREND (Hysterese & Zyklus) ---
    uint8_t trend = _data.trend.value;
    if (abs((int)trend - _lastSentTrend) >= Param_TrendAmount_1 || checkCycle(_lastTrendCycle, Param_TrendCycle_1)) {
        knx.getGroupObject(KO_TREND).value(trend, Dpt(5, 5));
        _lastSentTrend = trend;
        _lastTrendCycle = nun;
    }

    // --- 10. OVEN_HEATED (Nur Zyklus) ---
    if (checkCycle(_lastHeatedCycle, Param_HeatedCycle_1)) {
        knx.getGroupObject(KO_OVEN_HEATED).value(_data.oven_heated.value, Dpt(1, 1));
        _lastHeatedCycle = nun;
    }

    // --- 11. HEATING_ERROR (Event-basiert bei Update) ---
    if (_data.heating_error.updated) {
        knx.getGroupObject(KO_HEATING_ERROR).value(_data.heating_error.value, Dpt(1, 1));
        _data.heating_error.updated = false;
    }

    // --- 12. BURN_CYCLES (Event-basiert bei Update) ---
    if (_data.burn_cycles.updated) {
        knx.getGroupObject(KO_BURN_CYCLES).value(_data.burn_cycles.value, Dpt(7, 1));
        _data.burn_cycles.updated = false;
    }

    // --- 13. HEATING_ERROR_COUNT (Event-basiert bei Update) ---
    if (_data.heating_error_count.updated) {
        knx.getGroupObject(KO_HEATING_ERROR_COUNT).value(_data.heating_error_count.value, Dpt(7, 1));
        _data.heating_error_count.updated = false;
    }

    // --- 14. CONTROLLER_VERSION (Event-basiert bei Update) ---
    if (_data.controller_version.updated) {
        knx.getGroupObject(KO_CONTROLLER_VERSION).value(_data.controller_version.value, Dpt(5, 5));
        _data.controller_version.updated = false;
    }

    // --- 15. EMBER_BED (Event-basiert) ---
    if (_data.ember_bed.updated) {
        knx.getGroupObject(KO_EMBER_BED).value(_data.ember_bed.value, Dpt(1, 1));
        _data.ember_bed.updated = false;
    }

    // --- 16. CRITICAL_TEMPERATURE (Event-basiert) ---
    if (_data.critical_temperature.updated) {
        knx.getGroupObject(KO_CRITICAL_TEMP_ALARM).value(_data.critical_temperature.value, Dpt(1, 1));
        _data.critical_temperature.updated = false;
    }

    // --- 17. CAN_BUS_ERROR (Event-basiert) ---
    if (_data.can_bus_error.updated) {
        knx.getGroupObject(KO_CAN_BUS_ERROR).value(_data.can_bus_error.value, Dpt(1, 1));
        _data.can_bus_error.updated = false;
    }

    // Zeitstempel für zyklisches Senden aktualisieren
    if (forceSend) {
        _lastCyclicSend = millis();
    }
}