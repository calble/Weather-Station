#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <EepromAT24C32.h>

#include "history.h"
#include "eeprom.h"
#include "settings.h"

void clearEEPROM(EepromAt24c32<TwoWire> RtcEeprom){
  
  Serial.println("Clearing EEPROM");
  byte clear[32];
  for(int i=0; i < 32; i++){
    clear[i] = 0;
  }

  digitalWrite(BUILTIN_LED1, HIGH);
  for(int i=0; i < 256; i++){
    Serial.printf("Clearing page: %d\n", i);
    digitalWrite(BUILTIN_LED2, LOW);
    RtcEeprom.SetMemory(i * 32, clear, 32);
    delay(50);
    digitalWrite(BUILTIN_LED2, HIGH);
    delay(50);
    yield();
  }
  
  //Put sane defaults in for the highs and lows
  resetHighLow(RtcEeprom);
  
  Serial.printf("EEPROM Cleared! Set pin %d to HIGH and restart.", CLEAR_MEMORY);
  
  //infinite loop to signal memory is cleared
  for(int i=0; i < 20; i++){
     delay(500);
     digitalWrite(BUILTIN_LED2, HIGH);
     delay(500);
     digitalWrite(BUILTIN_LED2, LOW);
     yield();
  }
}

void saveHistory(EepromAt24c32<TwoWire> RtcEeprom, DataPoint *history){
  Serial.println("Saving History");
  byte buffer[sizeof(DataPoint) * 24];
  historyToBytes(history, buffer);
  writeBytesToEeprom(RtcEeprom, ADDR_HISTORY, buffer, sizeof(DataPoint) * 24);
}

void resetHighLow(EepromAt24c32<TwoWire> RtcEeprom) {
  Serial.println("Reseting Records");
  Record r;
  r.maxTemperature = -201;
  r.minTemperature = 202;
  r.maxPressure = -200003;
  r.minPressure = 200004;
  r.maxHumidity = -205;
  r.minHumidity = 206;

  byte buffer[sizeof(Record)];
  
  recordToBytes(r, buffer); 
  writeBytesToEeprom(RtcEeprom, ADDR_RECORD, buffer, sizeof(Record));
}

void saveRecords(EepromAt24c32<TwoWire> RtcEeprom, Record record){
  Serial.println("Saving Records");
  byte buffer[sizeof(Record)];
  recordToBytes(record, buffer);
  Serial.println("Bytes Written from record memory:");
  printBytes(buffer, sizeof(Record));
  writeBytesToEeprom(RtcEeprom, ADDR_RECORD, buffer, sizeof(Record));
}

void restoreRecords(EepromAt24c32<TwoWire> RtcEeprom, Record* record){
  byte buffer[sizeof(Record)];
  Serial.println("Restoring Records");
  readBytesFromEeprom(RtcEeprom, ADDR_RECORD, buffer, sizeof(Record));
  Serial.println("Record Bytes Read From Memory:");
  printBytes(buffer, sizeof(Record));
  bytesToRecord(buffer, record);
  Serial.println("Bytes copied to record");
  Serial.printf("EEPROM Record: TM: %0.2f, Tm: %0.2f, HM: %0.2f, Hm: %0.2f, PM: %d, Pm: %d\n", record->maxTemperature, record->minTemperature, record->maxHumidity, record->minHumidity, record->maxPressure, record->minPressure);
}

void resetHistory(EepromAt24c32<TwoWire> RtcEeprom){
  byte buffer[sizeof(DataPoint) * 24];
  DataPoint history[24];
  for (int i = 0; i < 24; i++) {
    history[i].time = 0;
    history[i].temperature = 0;
    history[i].humidity = 0;
    history[i].pressure = 0;
  }
  writeBytesToEeprom(RtcEeprom, ADDR_HISTORY, buffer, sizeof(DataPoint) * 24);
}

void restoreHistory(EepromAt24c32<TwoWire> RtcEeprom, DataPoint* history){
  byte buffer[sizeof(DataPoint) * 24];
  readBytesFromEeprom(RtcEeprom, ADDR_HISTORY, buffer, sizeof(DataPoint) * 24);
  bytesToHistory(buffer, history);
}

void restoreSettings(EepromAt24c32<TwoWire> RtcEeprom, Setting* setting){
  byte buffer[sizeof(Setting)];
  readBytesFromEeprom(RtcEeprom, ADDR_SETTING, buffer, sizeof(Setting));
  bytesToSetting(buffer, setting);
}

void saveSettings(EepromAt24c32<TwoWire> RtcEeprom, Setting setting){
  Serial.println("Saving Settings");
  byte buffer[sizeof(Setting)];
  settingToBytes(setting, buffer);
  writeBytesToEeprom(RtcEeprom, ADDR_SETTING, buffer, sizeof(Setting));
}
