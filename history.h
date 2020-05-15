#ifndef HISTORY
#define HISTORY

#include "settings.h"

void validateMemory(EepromAt24c32<TwoWire> RtcEeprom);
void debug(EepromAt24c32<TwoWire> RtcEeprom);
void clearEEPROM(EepromAt24c32<TwoWire> RtcEeprom);

void restore(EepromAt24c32<TwoWire> RtcEeprom);
void restoreRecords(EepromAt24c32<TwoWire> RtcEeprom, Record *record);

void save(EepromAt24c32<TwoWire> RtcEeprom, DataPoint dp[]);
void saveRecords(EepromAt24c32<TwoWire> RtcEeprom, Record *record);

void resetHistory(EepromAt24c32<TwoWire> RtcEeprom);
void resetHighLow(EepromAt24c32<TwoWire> RtcEeprom);

#endif
