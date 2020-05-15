#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <EepromAT24C32.h>

#include "history.h"
#include "eeprom.h"
#include "settings.h"

void validateMemory(EepromAt24c32<TwoWire> RtcEeprom){
  Serial.println("Validating Memory");
  byte c[32];

  for(int i=0; i < 32; i++){
    c[i] = 255;
  }

  for(int i=0; i < 256; i++){
    RtcEeprom.SetMemory(i*32, c, 32);
    Serial.printf("Page: %d\n", i);
    delay(10);
    yield();
  }
  
  for(int i=0; i < 256; i++){
    RtcEeprom.GetMemory(i * 32, c, 32);
    for(int j=0; j < 32; j++){
      if(c[j] != 255){
        Serial.printf("Failed Location: %d, value: %d\n", i*32+j, c[j]);
      }
      c[j] = 0;
    }
    yield();
  }
}

void debug(EepromAt24c32<TwoWire> RtcEeprom){
  byte b[8];
  float f;
  int d;

  Serial.println("\nDEBUG:");
  RtcEeprom.GetMemory(ADDR_HIGH_TEMP, b, 4);
  bytesToFloat(b, &f);
  Serial.printf("High Temp: %.2f [%d,%d,%d,%d]\n", f, b[0], b[1], b[2], b[3]);
  
  RtcEeprom.GetMemory(ADDR_LOW_TEMP, b, 4);
  bytesToFloat(b, &f);
  Serial.printf("Low Temp: %.2f [%d,%d,%d,%d]\n", f, b[0], b[1], b[2], b[3]);
  
  RtcEeprom.GetMemory(ADDR_HIGH_HUMIDITY, b, 4);
  bytesToFloat(b, &f);
  Serial.printf("High Humididty: %.2f [%d,%d,%d,%d]\n", f, b[0], b[1], b[2], b[3]);
  
  RtcEeprom.GetMemory(ADDR_LOW_HUMIDITY, b, 4);
  bytesToFloat(b, &f);
  Serial.printf("Low Humidity: %.2f [%d,%d,%d,%d]\n", f, b[0], b[1], b[2], b[3]);
  
  RtcEeprom.GetMemory(ADDR_HIGH_PRESSURE, b, 4);
  bytesToInt(b, &d);
  Serial.printf("High Pressure: %d [%d,%d,%d,%d]\n", d, b[0], b[1], b[2], b[3]);
  
  RtcEeprom.GetMemory(ADDR_LOW_PRESSURE, b, 4);
  bytesToInt(b, &d);
  Serial.printf("Low Pressure: %.2d [%d,%d,%d,%d]\n", d, b[0], b[1], b[2], b[3]);

  char xx[32];
  RtcEeprom.GetMemory(ADDR_STATION, (uint8_t*)xx, 31);
  xx[31] = '\0';
  Serial.printf("Station: %s\n", xx);
  
  RtcEeprom.GetMemory(ADDR_REMOTE, (uint8_t*)xx, 31);
  xx[31] = '\0';
  Serial.printf("Remote: %s\n", xx);
  
  int offsetMeasure = 0;
  int offsetTime = 0;
  long tBuffer;
  int p;
  float t,h;
  long tt;
  
  for(int i=0; i < 24; i++){
    RtcEeprom.GetMemory(ADDR_PRESSURE + offsetMeasure, b, 4);
    bytesToInt(b, &p);
    Serial.println(p);
    RtcEeprom.GetMemory(ADDR_HUMIDITY + offsetMeasure, b, 4);
    bytesToFloat(b, &h);
    RtcEeprom.GetMemory(ADDR_TEMP + offsetMeasure, b, 4);
    bytesToFloat(b, &t);
    RtcEeprom.GetMemory(ADDR_TIME + offsetTime, b, 8);
    bytesToLong(b, &tt);

    Serial.printf("IDX: %d, Pressure: %d, Temp: %.2f, Humidity: %.2f, Time: %ld\n", i, p, t, h, tt);
    offsetTime += 8;
    offsetMeasure += 4;
  } 
}

void clearEEPROM(EepromAt24c32<TwoWire> RtcEeprom){
  Serial.println("Validating EEPROM");
  validateMemory(RtcEeprom);
  
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
  }
  
  //Put sane defaults in for the highs and lows
  resetHighLow(RtcEeprom);
  
  Serial.printf("EEPROM Cleared! Set pin %d to HIGH and restart.", CLEAR_MEMORY);
  debug(RtcEeprom);
  //infinite loop to signal memory is cleared
  while(true){
     delay(500);
     digitalWrite(BUILTIN_LED2, HIGH);
     delay(500);
     digitalWrite(BUILTIN_LED2, LOW);
  }
}

void restore(EepromAt24c32<TwoWire> RtcEeprom){
  
}

void save(EepromAt24c32<TwoWire> RtcEeprom, DataPoint *history){
  
}

void resetHighLow(EepromAt24c32<TwoWire> RtcEeprom) {
 
}

void saveRecords(EepromAt24c32<TwoWire> RtcEeprom, Record record[]){
  
}

void restoreRecords(EepromAt24c32<TwoWire> RtcEeprom, Record *record){
  
}

void resetHistory(EepromAt24c32<TwoWire> RtcEeprom){
  
}
