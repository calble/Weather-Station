#ifndef SETTINGS
#define SETTINGS

#define ADDR_STATION        0   //One Page
#define ADDR_REMOTE         32  //One Page
#define ADDR_TIME           64  //Eight Pages
#define ADDR_TEMP           320 //Three Pages
#define ADDR_HUMIDITY       448 //Three Pages
#define ADDR_PRESSURE       576 //Three Pages
#define ADDR_HIGH_TEMP      704
#define ADDR_LOW_TEMP       708
#define ADDR_HIGH_PRESSURE  712
#define ADDR_LOW_PRESSURE   716
#define ADDR_HIGH_HUMIDITY  720
#define ADDR_LOW_HUMIDITY   724

#define BUILTIN_LED1 2 //GPIO2
#define BUILTIN_LED2 16//GPIO16
#define CLEAR_MEMORY 3

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
