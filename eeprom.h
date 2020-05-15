#ifndef N_EEPROM
#define N_EEPROM

#include "settings.h"

void longToBytes(long number, byte bytes[]);
void intToBytes(int number, byte bytes[]);
void floatToBytes(float number, byte bytes[]);

void bytesToLong(byte bytes[], long* n);
void bytesToInt(byte bytes[], int* n);
void bytesToFloat(byte bytes[], float* n);

void eeprom_test();
void eeprom_rw_test(EepromAt24c32<TwoWire> w);

void writeBytesToEeprom(EepromAt24c32<TwoWire> w, int address, byte* data, int count);
void readBytesFromEeprom(EepromAt24c32<TwoWire> w, int address, byte* data, int count);

void settingToBytes(Setting setting, byte* data);
void recordToBytes(Record record, byte* data);
void historyToBytes(DataPoint* datapoints, byte* data);


void bytesToSetting(byte* data, Setting* setting);
void bytesToRecord(byte* data, Record* record);
void bytesToHistory(byte* data, DataPoint* datapoints);

boolean shouldRunSetup(EepromAt24c32<TwoWire> w);
#endif
