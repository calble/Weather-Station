#ifndef N_EEPROM
#define N_EEPROM

void longToBytes(long number, byte bytes[]);
void intToBytes(int number, byte bytes[]);
void floatToBytes(float number, byte bytes[]);

void bytesToLong(byte bytes[], long* n);
void bytesToInt(byte bytes[], int* n);
void bytesToFloat(byte bytes[], float* n);

void eeprom_test();
void eeprom_rw_test(EepromAt24c32<TwoWire> w);

#endif
