#ifndef SETTINGS
#define SETTINGS

#define ADDR_SETUP           0   //  1 byte, if zero run setup, otherwise run like normal
#define ADDR_RECORD         32   // 24 bytes
#define ADDR_SETTING        64   //204 bytes
#define ADDR_HISTORY        288  //384 bytes

#define BUILTIN_LED1 2 //GPIO2
#define BUILTIN_LED2 16//GPIO16
#define CLEAR_MEMORY D6

struct DataPoint {
  //Time is the number of seconds since 2000.
  unsigned int time;
  float temperature;
  float humidity;
  int pressure;
};

struct Setting {
  char ssid[33];
  char password [65];
  float altitude;
  char station[33];
  char remote[65];
};

struct Record {
  float maxTemperature;
  float minTemperature;

  int maxPressure;
  int minPressure;

  float maxHumidity;
  float minHumidity;
};

#endif
