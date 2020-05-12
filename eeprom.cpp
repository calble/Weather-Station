#include <Arduino.h>
#include <EepromAT24C32.h>
#include<Wire.h>

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
