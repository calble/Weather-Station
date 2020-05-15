#include <Arduino.h>
#include <EepromAT24C32.h>
#include<Wire.h>

#include "settings.h"

void longToBytes(long number, byte bytes[]){
  union {
    long n;
    byte b[8];
  } a;

  a.n = number;

  for(int i=0; i < 8; i++){
    bytes[i] = a.b[i];
  }
}


void intToBytes(int number, byte bytes[]){
   union {
    int n;
    byte b[4];
  } a;

  a.n = number;

  for(int i=0; i < 4; i++){
    bytes[i] = a.b[i];
  }
}

void floatToBytes(float number, byte bytes[]){
   union {
    float n;
    byte b[4];
  } a;

  a.n = number;

  for(int i=0; i < 4; i++){
    bytes[i] = a.b[i];
  }
}

void bytesToLong(byte bytes[], long* n){
  union {
    long n;
    byte b[8];
  } u;

  for(int i=0; i < 8; i++){
    bytes[i] = u.b[i];
  }

  *n = u.n;
}

void bytesToInt(byte bytes[], int* n){
  union {
    int n;
    byte b[4];
  } u;

  for(int i=0; i < 4; i++){
    bytes[i] = u.b[i];
  }

  *n = u.n;
}

void bytesToFloat(byte bytes[], float* n){
  union {
    float n;
    byte b[4];
  } u;

  for(int i=0; i < 4; i++){
    bytes[i] = u.b[i];
  }

  *n = u.n;
}

void eeprom_test(){
  float a = 89.23;
  float b = -77.92;
  int c = 234;
  int d = -235;
  long e = 200000;
  long f = -23023;
  byte buffer[8];

  float a1, b1;
  int c1, d1;
  long e1, f1;

  floatToBytes(a, buffer);
  bytesToFloat(buffer, &a1);

  floatToBytes(b, buffer);
  bytesToFloat(buffer, &b1);

  
  intToBytes(c, buffer);
  bytesToInt(buffer, &c1);

  intToBytes(d, buffer);
  bytesToInt(buffer, &d1);


  longToBytes(e, buffer);
  bytesToLong(buffer, &e1);

  longToBytes(f, buffer);
  bytesToLong(buffer, &f1);
  
  Serial.printf("\nA: 89.23 --> %f\n", a1);
  Serial.printf("B: -77.92 --> %f\n", b1);

  Serial.printf("C: 234 --> %d\n", c1);
  Serial.printf("D: -235 --> %d\n", d1);

  Serial.printf("E: 200000 --> %ld\n", e1);
  Serial.printf("F: -23023 --> %ld\n", f1);
}

void eeprom_rw_test(EepromAt24c32<TwoWire> RtcEeprom){
  float a = -23.21;
  byte b[4]; 
  floatToBytes(a, b);
  RtcEeprom.SetMemory(704, b, 4);
  delay(10);
  float c = 0;
  byte d[4];
  RtcEeprom.GetMemory(704, d, 4);
  bytesToFloat(d, &c);
  Serial.printf("\nEEPROM R/W TEST:\nORG: %f\nNEW:%f\n", a, c);
  
}

void settingToBytes(Setting setting, byte* data){
  union {
    Setting setting;
    byte data[sizeof(Setting)];
  } u;

  u.setting.altitude = setting.altitude;
  strcpy(u.setting.ssid, setting.ssid);
  strcpy(u.setting.password, setting.password);
  strcpy(u.setting.remote, setting.remote);
  strcpy(u.setting.station, setting.station);
  
  memcpy(data, u.data, sizeof(Setting));
}

void recordToBytes(Record record, byte* data){
  union {
    Record record;
    byte data[sizeof(Record)];
  } u;

  u.record.maxTemperature = record.maxTemperature;
  u.record.minTemperature = record.minTemperature;

  u.record.maxPressure = record.maxPressure;
  u.record.minPressure = record.minPressure;

  u.record.maxHumidity = record.maxHumidity;
  u.record.minHumidity = record.minHumidity;
  
  memcpy(data, u.data, sizeof(Record));
}

void historyToBytes(DataPoint* datapoints, byte* data){
  union {
    DataPoint datapoint;
    byte data[sizeof(DataPoint)];
  } u;

  for(int i=0; i < 24; i++){
    u.datapoint.time = datapoints[i].time;
    u.datapoint.temperature = datapoints[i].temperature;
    u.datapoint.humidity = datapoints[i].humidity;
    u.datapoint.pressure = datapoints[i].pressure;
    memcpy(data, u.data, sizeof(DataPoint));
    data += sizeof(DataPoint);
  }
  
}

void bytesToSetting(byte* data, Setting* setting){
  union {
    Setting setting;
    byte data[sizeof(Setting)];
  } u;
  memcpy(u.data, data, sizeof(Setting));

  setting->altitude = u.setting.altitude;
  strcpy(setting->ssid, u.setting.ssid);
  strcpy(setting->password, u.setting.password);
  strcpy(setting->remote, u.setting.remote);
  strcpy(setting->station, u.setting.station);
}

void bytesToRecord(byte* data, Record* record){
  union {
    Record record;
    byte data[sizeof(Record)];
  } u;
  memcpy(u.data, data, sizeof(Record));
  
  record->maxTemperature = u.record.maxTemperature;
  record->minTemperature = u.record.minTemperature;

  record->maxPressure = u.record.maxPressure;
  record->minPressure = u.record.minPressure;

  record->maxHumidity = u.record.maxHumidity;
  record->minHumidity = u.record.minHumidity;
  
}

void bytesToHistory(byte* data, DataPoint* datapoints){
  union {
    DataPoint datapoint;
    byte data[sizeof(DataPoint)];
  } u;

  for(int i=0; i < 24; i++){
    memcpy(u.data, data, sizeof(DataPoint));
    
    datapoints[i].time = u.datapoint.time;
    datapoints[i].temperature = u.datapoint.temperature;
    datapoints[i].humidity = u.datapoint.humidity;
    datapoints[i].pressure = u.datapoint.pressure;
        
    data += sizeof(DataPoint);
  }
}

void writeBytesToEeprom(EepromAt24c32<TwoWire> w, int address, byte data[], int count){
  //bites to write to get to a 32 byte offset
  int pre = 32 - address % 32;
  w.SetMemory(address, data, pre);
  
  //write chunks of 32bytes
  int remaining = count - pre;
  int chunks = remaining / 32;
  
  if(remaining > 0){  
    for(int i=0; i < chunks; i++){
      w.SetMemory(address + pre + i * 32, data, 32);
    }
  }

  //write remaining bytes
  int leftOver = remaining % 32;
  w.SetMemory(address + pre + chunks * 32, data, leftOver);
}

void readBytesFromEeprom(EepromAt24c32<TwoWire> w, int address, byte* data, int count){
  //bites to write to get to a 32 byte offset
  int pre = 32 - address % 32;
  w.GetMemory(address, data, pre);
  
  //write chunks of 32bytes
  int remaining = count - pre;
  int chunks = remaining / 32;
  
  if(remaining > 0){  
    for(int i=0; i < chunks; i++){
      w.GetMemory(address + pre + i * 32, data, 32);
    }
  }

  //write remaining bytes
  int leftOver = remaining % 32;
  w.GetMemory(address + pre + chunks * 32, data, leftOver);
}

boolean shouldRunSetup(EepromAt24c32<TwoWire> w){
  byte b = w.GetMemory(ADDR_SETUP);
  return b == 0;
}
