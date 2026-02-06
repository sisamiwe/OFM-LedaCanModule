#include "CANGatewayModule.h"

// --- Hauptwerte ---
#define KO_COMBUSTION_TEMP       0  // DPT 9.001 (Temperatur °C)
#define KO_OVEN_STATE_NUM        1  // DPT 5.005 (Status Code)
#define KO_AIRFLAP_ACT           2  // DPT 5.005 (Ist-Position %) damit ganze Zahlen angezeigt werden, wird als DPT 5.005 gesendet
#define KO_AIRFLAP_TARGET        3  // DPT 5.005 (Soll-Position %) damit ganze Zahlen angezeigt werden, wird als DPT 5.005 gesendet
#define KO_CRITICAL_TEMPERATURE  4  // DPT 1.005 (Alarm)

// --- Erweiterte Status-Werte ---
#define KO_OVEN_HEATED           5  // DPT 1.003 (Binär: Heizt/Heizt nicht)
#define KO_EMBER_BED             6  // DPT 1.002 (Binär: Grundglut ja/nein)
#define KO_HEATING_ERROR         7  // DPT 1.005 (Binär: Fehler ja/nein)
#define KO_TREND                 8  // DPT 6.001 (Trend-Indikator)
#define KO_OVEN_STATE_TXT        9  // DPT 13.001 (Text)

// --- Statistiken ---
#define KO_MAX_COMBUSTION_TEMP  10  // DPT 9.001 (°C)
#define KO_SMOLDERING_TEMP      11  // DPT 9.001 (°C)
#define KO_BURN_CYCLES          12  // DPT 7.001 (Zähler)
#define KO_HEATING_ERROR_COUNT  13  // DPT 7.001 (Zähler)

// --- Diagnose & Version ---
#define KO_CONTROLLER_VERSION   14  // DPT 5.010 (Version)
#define KO_IS_ONLINE            15  // DPT 1.011 (Status: Gateway erreichbar)
#define KO_CONNECTION_LOST      16  // DPT 1.005 (Alarm: Timeout zum Ofen)

#define KO_CAN_BUS_ERROR        17  // DPT 1.005 (Alarm: CAN-Bus-Fehler)


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
    uint32_t nun = millis();

    // --- 2. VERBRENNUNGSTEMPERATUR (KO 1 - DPT 9.001) ---
    float cTemp = (float)_data.combustion_temp.value;
    float cLast = (float)_lastSentData.combustion_temp.value;
    if ((Param_CombTempSendChg_1 && abs(cTemp - cLast) >= Param_CombTempAmount_1) || 
         _data.combustion_temp.cycleElapsed(Param_CombTempCycle_1)) {
        
        knx.getGroupObject(KO_COMBUSTION_TEMP).value(cTemp, Dpt(9, 1));
        _lastSentData.combustion_temp.value = _data.combustion_temp.value;
        _data.combustion_temp.markAsSent();
    }

    // --- 3. MAX. VERBRENNUNGSTEMPERATUR (KO 2 - DPT 9.001) ---
    float mTemp = (float)_data.max_combustion_temp.value;
    float mLast = (float)_lastSentData.max_combustion_temp.value;
    if ((Param_MaxCombTempSendChg_1 && abs(mTemp - mLast) >= Param_MaxCombTempAmount_1) || 
         _data.max_combustion_temp.cycleElapsed(Param_MaxCombTempCycle_1)) {
        
        knx.getGroupObject(KO_MAX_COMBUSTION_TEMP).value(mTemp, Dpt(9, 1));
        _lastSentData.max_combustion_temp.value = _data.max_combustion_temp.value;
        _data.max_combustion_temp.markAsSent();
    }

    // --- 4. GLUTTEMPERATUR (KO 3 - DPT 9.001) ---
    float sTemp = (float)_data.smoldering_temp.value;
    float sLast = (float)_lastSentData.smoldering_temp.value;
    if ((Param_SmoldTempSendChg_1 && abs(sTemp - sLast) >= Param_SmoldTempAmount_1) || 
         _data.smoldering_temp.cycleElapsed(Param_SmoldTempCycle_1)) {
        
        knx.getGroupObject(KO_SMOLDERING_TEMP).value(sTemp, Dpt(9, 1));
        _lastSentData.smoldering_temp.value = _data.smoldering_temp.value;
        _data.smoldering_temp.markAsSent();
    }

    // --- 5. LUFTKLAPPE IST (KO 4 - DPT 5.005) ---
    uint8_t fAct = _data.air_flap_act.value;
    uint8_t fActLast = _lastSentData.air_flap_act.value;
    if ((Param_AirActSendChg_1 && abs((int)fAct - (int)fActLast) >= Param_AirActAmount_1) || 
         _data.air_flap_act.cycleElapsed(Param_AirActCycle_1)) {
        
        knx.getGroupObject(KO_AIRFLAP_ACT).value(fAct, Dpt(5, 5));
        _lastSentData.air_flap_act.value = fAct;
        _data.air_flap_act.markAsSent();
    }

    // --- 6. LUFTKLAPPE SOLL (KO 5 - DPT 5.005) ---
    uint8_t fTrg = _data.air_flap_target.value;
    uint8_t fTrgLast = _lastSentData.air_flap_target.value;
    if ((Param_AirTrgSendChg_1 && abs((int)fTrg - (int)fTrgLast) >= Param_AirTrgAmount_1) || 
         _data.air_flap_target.cycleElapsed(Param_AirTrgCycle_1)) {
        
        knx.getGroupObject(KO_AIRFLAP_TARGET).value(fTrg, Dpt(5, 5));
        _lastSentData.air_flap_target.value = fTrg;
        _data.air_flap_target.markAsSent();
    }

    // --- 7. OFEN STATUS CODE (KO 6 - DPT 5.005) ---
    if (_data.oven_state_num.cycleElapsed(Param_StateNumCycle_1)) {
        knx.getGroupObject(KO_OVEN_STATE_NUM).value(_data.oven_state_num.value, Dpt(5, 5));
        _data.oven_state_num.markAsSent();
    }

    // --- 8. OFEN STATUS TEXT (KO 7 - DPT 16.000) ---
    if (nun - _data.oven_state_text_lastSent >= (Param_StateTxtCycle_1 * 60000)) {
        knx.getGroupObject(KO_OVEN_STATE_TXT).value(_data.oven_state_text, Dpt(16, 0));
        _data.oven_state_text_lastSent = nun;
    }

    // --- 9. TREND (KO 8 - DPT 5.005) ---
    int8_t trd = _data.trend.value;
    int8_t trdLast = _lastSentData.trend.value;
    if (abs((int)trd - (int)trdLast) >= Param_TrendAmount_1 || _data.trend.cycleElapsed(Param_TrendCycle_1)) {
        knx.getGroupObject(KO_TREND).value((uint8_t)trd, Dpt(5, 5));
        _lastSentData.trend.value = trd;
        _data.trend.markAsSent();
    }

    // --- 10. OFEN GEHEIZT (KO 9 - DPT 1.001) ---
    if (_data.oven_heated.cycleElapsed(Param_HeatedCycle_1)) {
        knx.getGroupObject(KO_OVEN_HEATED).value(_data.oven_heated.value, Dpt(1, 1));
        _data.oven_heated.markAsSent();
    }

    // --- 11. HEIZFEHLER (KO 10 - DPT 1.001) ---
    if (_data.heating_error.updated) {
        knx.getGroupObject(KO_HEATING_ERROR).value(_data.heating_error.value, Dpt(1, 1));
        _data.heating_error.markAsSent();
    }

    // --- 12. ABBRANDZYKLEN (KO 11 - DPT 7.001) ---
    if (_data.burn_cycles.updated) {
        knx.getGroupObject(KO_BURN_CYCLES).value(_data.burn_cycles.value, Dpt(7, 1));
        _data.burn_cycles.markAsSent();
    }

    // --- 13. FEHLERZÄHLER (KO 12 - DPT 7.001) ---
    if (_data.heating_error_count.updated) {
        knx.getGroupObject(KO_HEATING_ERROR_COUNT).value(_data.heating_error_count.value, Dpt(7, 1));
        _data.heating_error_count.markAsSent();
    }

    // --- 14. VERSION (KO 13 - DPT 5.005) ---
    if (_data.controller_version.updated) {
        knx.getGroupObject(KO_CONTROLLER_VERSION).value(_data.controller_version.value, Dpt(5, 5));
        _data.controller_version.markAsSent();
    }

    // --- 15. GLUTBETT (KO 14 - DPT 1.001) ---
    if (_data.ember_bed.updated) {
        knx.getGroupObject(KO_EMBER_BED).value(_data.ember_bed.value, Dpt(1, 1));
        _data.ember_bed.markAsSent();
    }

    // --- 16. KRITISCHE TEMP ALARM (KO 15 - DPT 1.001) ---
    if (_data.critical_temperature.updated) {
        knx.getGroupObject(KO_CRITICAL_TEMPERATURE).value(_data.critical_temperature.value, Dpt(1, 1));
        _data.critical_temperature.markAsSent();
    }

    // --- 17. CAN BUS FEHLER (KO 16 - DPT 1.001) ---
    if (_data.can_bus_error.updated) {
        knx.getGroupObject(KO_CAN_BUS_ERROR).value(_data.can_bus_error.value, Dpt(1, 1));
        _data.can_bus_error.markAsSent();
    }
}