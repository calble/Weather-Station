#include <Arduino.h>
#include "wmath.h"

float cToF(float c){
  return (c * 9.0 / 5.0) + 32;
}

float minValue(float arr[], int offset, int count){
  float v = arr[offset];
  for(int i=offset; i < (offset + count); i++){
    v = min(v, arr[i]);
  }
  return v;
}

float maxValue(float arr[], int offset, int count){
  float v = arr[offset];
  for(int i=offset; i < (offset + count); i++){
    v = max(v, arr[i]);
  }
  return v;
}

int minValue(int arr[], int offset, int count){
  int v = arr[offset];
  for(int i=offset; i < (offset + count); i++){
    v = min(v, arr[i]);
  }
  return v;
}

int maxValue(int arr[], int offset, int count){
  int v = arr[offset];
  for(int i=offset; i < (offset + count); i++){
    v = max(v, arr[i]);
  }
  return v;
}


float range(float arr[], int hour){
  float difference = largest(arr, hour) - smallest(arr, hour);
  return (difference < 0)? difference * -1:difference;
}

float smallest(float arr[], int hour){
  int offset = (hour < 24)?24-hour:0;
  float smallest = arr[offset];
  for(int i=offset+1; i < 24; i++){
    if(arr[i] < smallest){
      smallest = arr[i];
    }
  }
  return smallest;
}

float largest(float arr[], int hour){
  int offset = (hour < 24)?24-hour:0;
  float largest = arr[offset];
  for(int i=offset + 1; i < 24; i++){
    if(arr[i] > largest){
      largest = arr[i];
    }
  }
  return largest;
}
