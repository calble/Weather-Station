#include <Arduino.h>
#include<Wire.h>
#include <RtcDS3231.h>
#include <EepromAT24C32.h>

#include "history.h"
#include "eeprom.h"
#include "settings.h"

void validateMemory(){
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

void debug(){
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

void clearEEPROM(){
  Serial.println("Validating EEPROM");
  validateMemory();
  
  Serial.println("Clearing EEPROM");
  byte clear[32];
  for(int i=0; i < 32; i++){
    clear[i] = 0;
  }

//  digitalWrite(BUILTIN_LED1, HIGH);
//  for(int i=0; i < 256; i++){
//    Serial.printf("Clearing page: %d\n", i);
//    digitalWrite(BUILTIN_LED2, LOW);
//    RtcEeprom.SetMemory(i * 32, clear, 32);
//    delay(50);
//    digitalWrite(BUILTIN_LED2, HIGH);
//    delay(50);
//  }
  
  //Put sane defaults in for the highs and lows
  resetHighLow();
  
  Serial.printf("EEPROM Cleared! Set pin %d to HIGH and restart.", CLEAR_MEMORY);
  debug();
  //infinite loop to signal memory is cleared
  while(true){
     delay(500);
     digitalWrite(BUILTIN_LED2, HIGH);
     delay(500);
     digitalWrite(BUILTIN_LED2, LOW);
  }
}

void restore(){
  byte b[8];

  RtcEeprom.GetMemory(ADDR_HIGH_TEMP, b, 4);
  bytesToFloat(b, &highTemp);
  RtcEeprom.GetMemory(ADDR_LOW_TEMP, b, 4);
  bytesToFloat(b, &lowTemp);

  RtcEeprom.GetMemory(ADDR_HIGH_HUMIDITY, b, 4);
  bytesToFloat(b, &highHumidity);
  RtcEeprom.GetMemory(ADDR_LOW_HUMIDITY, b, 4);
  bytesToFloat(b, &lowHumidity);

  RtcEeprom.GetMemory(ADDR_HIGH_PRESSURE, b, 4);
  bytesToInt(b, &highPressure);
  RtcEeprom.GetMemory(ADDR_LOW_PRESSURE, b, 4);
  bytesToInt(b, &lowPressure);

  RtcEeprom.GetMemory(ADDR_STATION, (uint8_t*)station, 31);
  station[31] = '\0';
  RtcEeprom.GetMemory(ADDR_REMOTE, (uint8_t*)remote, 31);
  station[32] = '\0';

  int offsetMeasure = 0;
  int offsetTime = 0;
  long tBuffer;
  for(int i=0; i < 24; i++){
    RtcEeprom.GetMemory(ADDR_PRESSURE + offsetMeasure, b, 4);
    bytesToInt(b, &pressure[i]);
    RtcEeprom.GetMemory(ADDR_HUMIDITY + offsetMeasure, b, 4);
    bytesToFloat(b, &humidity[i]);
    RtcEeprom.GetMemory(ADDR_TEMP + offsetMeasure, b, 4);
    bytesToFloat(b, &temp[i]);
    RtcEeprom.GetMemory(ADDR_TIME + offsetTime, b, 8);
    bytesToLong(b, &tBuffer);
    dates[i] = tBuffer;
    
    offsetTime += 8;
    offsetMeasure += 4;
  }

  //restore the hour variable
  for(int i=23; i >=0; i++){
    if(dates[i] == 0){
      break;
    }
    hour++;
  }
}

void save(){
  int offsetMeasure = 0;
  int offsetTime = 0;
  long tBuffer;
  byte b[8];
  
  for(int i=0; i < 24; i++){
    intToBytes(pressure[i], b);
    RtcEeprom.SetMemory(ADDR_PRESSURE + offsetMeasure, b, 4);
    delay(10);
    floatToBytes(humidity[i], b);
    RtcEeprom.SetMemory(ADDR_HUMIDITY + offsetMeasure, b, 4);
    delay(10);
    floatToBytes(temp[i], b);
    RtcEeprom.SetMemory(ADDR_TEMP + offsetMeasure, b, 4);
    delay(10);
    longToBytes(dates[i], b);
    RtcEeprom.SetMemory(ADDR_TIME + offsetTime, b, 8);
    delay(10);
    offsetTime += 8;
    offsetMeasure += 4;
    yield();
  }
}

void resetHighLow() {
  //Set min and max measurements to sane defaults
  byte buffer[4];
  
  float tempMax = 1000;
  float tempMin = -1000;
  float humidityMax = 200;
  float humidityMin = -200;
  int pressureMax = 200000;
  int pressureMin = -200000;

  floatToBytes(tempMin, buffer);
  Serial.printf("RESET BUFFER TEMP MAX: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_HIGH_TEMP,  (const uint8_t*)buffer, 4);
  delay(20);
  floatToBytes(tempMax, buffer);
  Serial.printf("RESET BUFFER TEMP MIN: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_LOW_TEMP,  (const uint8_t*)buffer, 4);
  delay(20);
  
  intToBytes(pressureMin, buffer);
  Serial.printf("RESET BUFFER PRESSURE MIN: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_HIGH_PRESSURE,  (const uint8_t*)buffer, 4);
  delay(20);
  intToBytes(pressureMax, buffer);
  Serial.printf("RESET BUFFER PRESSURE MAX: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_LOW_PRESSURE,  (const uint8_t*)buffer, 4);
  delay(20);
  
  floatToBytes(humidityMin, buffer);
  Serial.printf("RESET BUFFER HUMIDITY MAX: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_HIGH_HUMIDITY,  (const uint8_t*)buffer, 4);
  delay(20);
  floatToBytes(humidityMax, buffer);
  Serial.printf("RESET BUFFER HUMIDITY MIN: %d,%d,%d,%d\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  RtcEeprom.SetMemory(ADDR_LOW_HUMIDITY,  (const uint8_t*)buffer, 4);
  delay(20);

  for(int i=0; i < 24; i++){
    temp[i] = 0;
    humidity[i] = 0;
    pressure[i] = 0;
    dates[i] = 0;
  }
}
