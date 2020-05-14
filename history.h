#ifndef HISTORY
#define HISTORY

#include "settings.h"

void validateMemory(EepromAt24c32<TwoWire> RtcEeprom);
void debug(EepromAt24c32<TwoWire> RtcEeprom);
void clearEEPROM(EepromAt24c32<TwoWire> RtcEeprom);
void restore(EepromAt24c32<TwoWire> RtcEeprom);
void save(EepromAt24c32<TwoWire> RtcEeprom, DataPoint dp[]);
void resetHighLow(EepromAt24c32<TwoWire> RtcEeprom);
void saveRecords(EepromAt24c32<TwoWire> RtcEeprom, Record *record);

#endif
