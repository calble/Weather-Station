#include <Arduino.h>

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
