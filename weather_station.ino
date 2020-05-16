#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <TaskScheduler.h>
#include <stdlib.h>
#include <FS.h>
#include <RtcDS3231.h>
#include <EepromAT24C32.h>
#include <ctype.h>

#include "eeprom.h"
#include "wmath.h"
#include "history.h"
#include "settings.h"

ESP8266WebServer server(80);
Adafruit_BME280 bmp;


void handleRoot();
void handleStaticFiles();
void handleJson();
void handleNotFound();
void handleSaveSettings();
void handleGetSettings();
void handleResetRecords();
void handleResetHistory();

void sensorUpdate();
void hourlyUpdate();
void remoteUpdate();
void formatTime(long t, char*);
void flashIP();
char* getPressureRange(int pressure);
char* getTrend();
char* getFrostRisk();


Task sensorTask(10000, TASK_FOREVER, &sensorUpdate);
Task hourlyTask(1000 * 60 * 60, TASK_FOREVER, &hourlyUpdate);
Task remoteTask(1000 * 60 * 10, TASK_FOREVER, &remoteUpdate);
Scheduler runner;
RtcDS3231<TwoWire> Rtc(Wire);
EepromAt24c32<TwoWire> RtcEeprom(Wire);

float currentTemperature;
int currentPressure;
float currentHumidity;

int hour = 0;

Record record;
Setting setting;
DataPoint history[24];

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED1, OUTPUT); // Initialize the BUILTIN_LED1 pin as an output
  pinMode(BUILTIN_LED2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output
  pinMode(CLEAR_MEMORY, INPUT);

  Serial.printf("DataPoint Size: %d\n", sizeof(DataPoint));
  Serial.printf("Setting Setting: %d\n", sizeof(Setting));
  Serial.printf("Record Record: %d\n", sizeof(Record));
  
  //TODO: add check to see if eeprom ssid is null, then enter setup mode.
  WiFi.mode(WIFI_STA);
  WiFi.begin("NohaNet-C", "B3B6A7AF71D34DF28A88F25BFD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected.");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  MDNS.begin("weather");
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("weather", "tcp", 80);
  Wire.begin();
  delay(30);
  if (!bmp.begin(0x76, &Wire)) {
    Serial.println("BME280 failed to connect.");
    while (true) {
      digitalWrite(BUILTIN_LED2, LOW);
      delay(500);
      digitalWrite(BUILTIN_LED2, HIGH);
      delay(500);
    }
  }

  //Setup RTC
  Rtc.Begin();
  if (!Rtc.IsDateTimeValid()) {
    Serial.println("RTC IsDateTimeValid False!");
  }
  Rtc.SetIsRunning(true);
  
  //Setup EEPROM
  RtcEeprom.Begin();

  //Testing EEPROM functions
//  byte data[64];
//  for(int i=0; i < 64; i++){
//    data[i] = i+1;
//  }
//  writeBytesToEeprom(RtcEeprom, 22, data, 64);
//  delay(10);
//  for(int i=0; i < 64; i++){
//    byte b = RtcEeprom.GetMemory(22+i);
//    if(data[i] != b){
//      Serial.printf("Write Failure at %d, %d != %d\n", i+22, data[i], b);
//    }else{
//      Serial.printf("Write OKAY    at %d, %d != %d\n", i+22, data[i], b);
//    }
//  }
//  byte data2[16];
//  readBytesFromEeprom(RtcEeprom, 22, data2, 64);
//  
//  for(int i=0; i < 64; i++){
//    if(data[i] != data2[i]){
//      Serial.printf("Failure at %d, %d != %d\n", i, data[i], data2[i]);
//    }
//  }
//    Record a;
//    a.minTemperature = -5;
//    a.maxTemperature = 5;
//    a.minHumidity = 7;
//    a.maxHumidity = 100;
//    a.minPressure = 100000;
//    a.maxPressure = 200000;
//    byte c[sizeof(Record)];
//    recordToBytes(a, c);
//    writeBytesToEeprom(RtcEeprom, ADDR_RECORD, c, sizeof(Record));
//    delay(10);
//    Record b;
//    byte d [sizeof(Record)];
//    readBytesFromEeprom(RtcEeprom, ADDR_RECORD, d, sizeof(Record));
//    bytesToRecord(d, &b);
//    Serial.printf("TEST RECORD: %f, %f, %f, %f, %d, %d\n", b.minTemperature, b.maxTemperature, b.minHumidity, b.maxHumidity, b.minPressure, b.maxPressure);
//  for (int i = 0; i < 24; i++) {
//    history[i].time = i;
//    history[i].temperature = i;
//    history[i].humidity = i;
//    history[i].pressure = i;
//  }
//  byte a[sizeof(DataPoint)*24];
//  historyToBytes(history, a);
//  writeBytesToEeprom(RtcEeprom, ADDR_HISTORY, a, sizeof(DataPoint) * 24);
//  delay(10);
//  DataPoint b[24];
//  byte c[sizeof(DataPoint) * 24];
//  readBytesFromEeprom(RtcEeprom, ADDR_HISTORY, c, sizeof(DataPoint) * 24);
//  bytesToHistory(c, b);
//
//  for (int i = 0; i < 24; i++) {
//    Serial.printf("HISTORY TEST #%d: %ld, %f, %f, %d\n", i, b[i].time, b[i].temperature, b[i].humidity, b[i].pressure);
//  }
  
  //Clear EEPROM if the pin is high 3 times in a row
//   clearEEPROM(RtcEeprom);
  if (digitalRead(CLEAR_MEMORY) == LOW) {
    delay(50);
    if (digitalRead(CLEAR_MEMORY) == LOW) {
      delay(50);
      if (digitalRead(CLEAR_MEMORY) == LOW) {
        clearEEPROM(RtcEeprom);
      }
    }
  } else {
    Serial.println("CLEAR PIN was HIGH, not reseting memory.");
  }

  //Setup server
  server.onNotFound(handleStaticFiles);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJson);
  server.on("/save_settings", HTTP_POST, handleSaveSettings);
  server.on("/get_settings", HTTP_POST, handleGetSettings);
  server.on("/reset_records", HTTP_POST, handleResetRecords);
  server.on("/reset_history", HTTP_POST, handleResetHistory);
  server.begin();

  //Schedule tasks
  runner.init();
  runner.addTask(sensorTask);
  runner.addTask(hourlyTask);
  runner.addTask(remoteTask);
  sensorTask.enable();
  hourlyTask.enable();
  remoteTask.enable();
  //Setup flash memory file system
  SPIFFS.begin();

  //Clear out all data
  currentTemperature = 0;
  currentPressure = 0;
  currentHumidity = 0;

  hour = 0;

  record.maxTemperature = -201;
  record.minTemperature = 202;
  record.maxPressure = -200003;
  record.minPressure = 200004;
  record.maxHumidity = -205;
  record.minHumidity = 206;

  setting.ssid[0] = '\0';
  setting.password[0] = '\0';
  setting.altitude = -100;
  setting.station[0] = 'S';
  setting.station[1] = '\0';
  setting.remote[0] = 'R';
  setting.remote[1] = '\0';


  for (int i = 0; i < 24; i++) {
    history[i].time = 0;
    history[i].temperature = 0;
    history[i].humidity = 0;
    history[i].pressure = 0;
  }
  
  restoreHistory(RtcEeprom, history);
  //You must restore hour to the proper value or no historical data will be sent via JSON
  for(int i=23; i >=0; i--){
    if(history[i].time != 0){
      hour++;
    }else{
      break;
    }
  }
  Serial.printf("Current HOUR: %d\n", hour);
  restoreSettings(RtcEeprom, &setting);
  restoreRecords(RtcEeprom, &record);
  yield();
  Serial.println("HISTORY DATA:");
  for (int i = 0; i < 24; i++) {
    Serial.printf("#%d: %ld, %f, %f, %d\n",history[i].time, history[i].temperature, history[i].humidity, history[i].pressure);
  }
  Serial.println("Settings:");
  Serial.printf("SSID: %s, PASSWORD: %s, REMOTE: %s, STATION: %s, ALTITUDE: %f\n", setting.ssid, setting.password, setting.remote, setting.station, setting.altitude);
  Serial.println("Records:");
  Serial.printf("MAX T: %f, MIN T: %f, MAX H: %f, MIN H: %f, MAX P: %d, MIN P: %d\n", record.maxTemperature, record.minTemperature, record.maxHumidity, record.minHumidity, record.maxPressure, record.minPressure);
  
  flashIP();
}

void loop() {
  server.handleClient();
  runner.execute();
  MDNS.update();
}

void handleRoot() {
  File file = SPIFFS.open("/index.htm", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleJson() {
  server.setContentLength(4000);
  int len = 0;
  
  server.send(200, "text/json", "");
  Serial.println("Handling JSON");
  float p = currentPressure / 100.0;
  char *jsonStart = (char*)"{'station':'%s',\n'current':{\n\t'temperature':%0.2f,\n\t'humidity':%0.2f,\n\t'barometer':%0.2f,\n\t'pressure':'%s',\n\t'trend':'%s',\n\t'time':'%s',\n\t'frost':%s\n},\n'records':{\n\t'temperature':{'high':%0.2f,'low':%0.2f},\n\t'humidity':{'high':%0.2f,'low':%0.2f},\n\t'barometer':{'high':%0.2f,'low':%0.2f}\n},\n'history':[\n";
  char *h = (char*)"\t{\n\t\t'time':'%s',\n\t\t'temperature':%0.2f,\n\t\t'humidity':%0.2f,\n\t\t'barometer':%0.2f\n\t}";
  char *jsonEnd = (char*)"\n]\n}";
  
  replaceStr(jsonStart, '\'', '\"');
  replaceStr(h, '\'', '\"');
  replaceStr(jsonEnd, '\'', '\"');
  
  char buffer[2000];
  buffer[0] = '\0';

  //JSON Start
  RtcDateTime t = Rtc.GetDateTime();
  char time[18];
  sprintf(time, "%04d-%02d-%02d %02d:%02d", t.Year(), t.Month(), t.Day(), t.Hour(), t.Minute());
  sprintf(buffer, jsonStart,  setting.station, cToF(currentTemperature), currentHumidity, currentPressure / 100.0, getPressureRange(currentPressure), getTrend(), time, getFrostRisk(), cToF(record.maxTemperature), cToF(record.minTemperature), record.maxHumidity, record.minHumidity, record.maxPressure / 100.0, record.minPressure / 100.0);
  len += strlen(buffer);
  server.sendContent(buffer, strlen(buffer));

  //JSON History
  int count = (hour < 24) ? 24 - hour : 0;
  for (int i = count; i < 24; i++) {
    char b[200];
    char t[18];
    formatTime(history[i].time, t);
    Serial.printf("History: #%d(Time %s, Temp: %d, Hum: %d, Pres: %d)\n", i, t, cToF(history[i].temperature), history[i].humidity, history[i].pressure);
    sprintf(b, h, t, cToF(history[i].temperature), history[i].humidity, history[i].pressure / 100.0);
    if(i != 23){
      strcat(b, ",\n");
    }
    int hLen = strlen(b);
    server.sendContent(b, hLen);
    len += hLen; 
  }

  //JSON End
  int endLen = strlen(jsonEnd);
  len += endLen;
  server.sendContent(jsonEnd, endLen);

  char blank[100];
  for(int i=0; i < 100; i++){
    blank[i] = ' ';
  }
  
  for(int i=len; i < 4000; i+=100){
    server.sendContent(blank, 100);
//    Serial.printf("%d...\n", i);
  }
  if(len % 100 != 0){
    server.sendContent(blank, len % 100);
  }
}

void handleStaticFiles() {
  if (SPIFFS.exists(server.uri())) {
    String contentType;
    if (server.uri().endsWith(".htm")) {
      contentType = "text/html";
    } else if (server.uri().endsWith(".css")) {
      contentType = "text/css";
    } else if (server.uri().endsWith(".js")) {
      contentType = "text/javascript";
    } else if (server.uri().endsWith(".png")) {
      contentType = "image/png";
    } else if (server.uri().endsWith(".ico")) {
      contentType = "image/png";
    } else if (server.uri().endsWith(".gif")) {
      contentType = "image/gif";
    } else {
      //We don't server any other type of file
      server.send(401, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>401 Unauthorized!</h2></body></html>");
      return;
    }
    File file = SPIFFS.open(server.uri(), "r");
    server.streamFile(file, contentType);
    file.close();
  } else {
    //File does not exist
    server.send(404, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>404 File Not Found!</h2></body></html>");
  }
}

void handleSaveSettings() {
  String message = "success";
  
  if(server.hasArg("action") && server.arg("action") == "save"){
    boolean shouldRestart = false;
    
    if(server.hasArg("ssid") && server.arg("ssid") != ""){
      String ssid = server.arg("ssid");
      if(ssid.length() > 32){
        ssid = ssid.substring(0, 32);
      }
      strcpy(setting.ssid, ssid.c_str());
      shouldRestart = true;  
    }
    if(server.hasArg("password") && server.arg("password") != ""){
      String password = server.arg("password");
      if(password.length() > 64){
        password = password.substring(0, 64);
      }
      strcpy(setting.password, password.c_str());
      shouldRestart = true;  
    }

    if(server.hasArg("remote") && server.hasArg("station") && server.hasArg("altitude") && server.hasArg("date") && server.hasArg("time")){
      String remote = server.arg("remote");
      String station = server.arg("station");

      if(station.length() > 32){
        station = station.substring(0,32);
      }
      strcpy(setting.station, station.c_str());
      
      if(remote.length() > 64){
        remote = remote.substring(0,64);
      }
      strcpy(setting.remote, remote.c_str());
      
      float altitude = atof(server.arg("altitude").c_str());
      setting.altitude = altitude;

      String nDate = server.arg("date");
      String nTime = server.arg("time");
      
      int y = atoi(nDate.substring(0,4).c_str());
      int m = atoi(nDate.substring(5,7).c_str());
      int d = atoi(nDate.substring(8,10).c_str());
      int h = atoi(nTime.substring(0,2).c_str());
      int M = atoi(nTime.substring(3,5).c_str());
      
      RtcDateTime newDateTime(y,m,d,h,M,0);
      Rtc.SetDateTime(newDateTime);
     
    }else{
      message = "missing fields";
    }
    saveSettings(RtcEeprom, setting);
    
    if(shouldRestart){
      Serial.println("SSID or Password have changed, restarting.");
      server.send(200,"text/json","{\"response\":\"rebooting\"}");
      server.client().stop();
      ESP.restart();
      return;
    }
    server.send(200,"text/json","{\"response\":\""+message+"\"}");
    return;
  }
  server.send(400, "text/json", "{\"response\":\"Bad Request\"}");
}

void handleResetRecords() {
  if(server.hasArg("action") && server.arg("action") == "reset"){
    record.maxTemperature = -201;
    record.minTemperature = 202;
    record.maxPressure = -200003;
    record.minPressure = 200004;
    record.maxHumidity = -205;
    record.minHumidity = 206;

    resetHighLow(RtcEeprom);

    server.send(200, "text/json", "{\"response\":\"success\"}");
    return;
  }
  server.send(400, "text/json", "{\"response\":\"bad request\"}");
}

void handleResetHistory() {
  if(server.hasArg("action") && server.arg("action") == "reset"){
    for (int i = 0; i < 24; i++) {
      history[i].time = 0;
      history[i].temperature = 0;
      history[i].humidity = 0;
      history[i].pressure = 0;
    }
    hour = 0;
    resetHistory(RtcEeprom);
    hourlyUpdate();
    server.send(200, "text/json", "{\"response\":\"success\"}");
    return;
  }
   server.send(400, "text/json", "{\"response\":\"bad request\"}");
}

void sensorUpdate() {
  currentTemperature = bmp.readTemperature();
  currentHumidity = bmp.readHumidity();
  currentPressure = pressureAtSealevel();

  Serial.printf("Sensor Update: %0.2fc, %0.2f%%, %0.2fmBar\n", currentTemperature, currentHumidity, currentPressure / 100.0);
  boolean flag = false;
  if (currentTemperature > record.maxTemperature) {
    record.maxTemperature = currentTemperature;
    flag = true;
  }
  if (currentTemperature < record.minTemperature) {
    record.minTemperature = currentTemperature;
    flag = true;
  }


  if (currentPressure > record.maxPressure) {
    record.maxPressure = currentPressure;
    flag = true;
  }
  if (currentPressure < record.minPressure) {
    record.minPressure = currentPressure;
    flag = true;
  }


  if (currentHumidity > record.maxHumidity) {
    record.maxHumidity = currentHumidity;
    flag = true;
  }
  if (currentHumidity < record.minHumidity) {
    record.minHumidity = currentHumidity;
    flag = true;
  }

  if (flag) {
    saveRecords(RtcEeprom, record);
  }
}

void hourlyUpdate() {
  Serial.println("Hourly Update");
  for (int i = 1; i < 24; i++) {
    history[i-1].temperature = history[i].temperature;
    history[i-1].pressure = history[i].pressure;
    history[i-1].humidity = history[i].humidity;
    history[i-1].time = history[i].time;
  }

  history[23].temperature = currentTemperature;
  history[23].pressure = currentPressure;
  history[23].humidity = currentHumidity;
  history[23].time = Rtc.GetDateTime().TotalSeconds();
  hour++;
  saveHistory(RtcEeprom, history);
}

void flashIP() {
  Serial.printf("LAST: %d\n", WiFi.localIP()[3]);
  int ip = WiFi.localIP()[3];
  int digits[3] = {ip / 100, ip / 10 % 10, ip % 10};

  digitalWrite(BUILTIN_LED1, HIGH);
  digitalWrite(BUILTIN_LED2, HIGH);
  delay(4000);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < digits[i]; j++) {
      digitalWrite(BUILTIN_LED1, LOW);
      digitalWrite(BUILTIN_LED2, LOW);
      delay(300 * 3);
      digitalWrite(BUILTIN_LED1, HIGH);
      digitalWrite(BUILTIN_LED2, HIGH);
      delay(300);

    }
    delay(2500);
  }

  digitalWrite(BUILTIN_LED1, LOW);
  digitalWrite(BUILTIN_LED2, HIGH);
}

int pressureAtSealevel() {
  int pressure = bmp.readPressure();
  return pressure / pow(1 - setting.altitude / 44330.0, 5.255);
}

void handleGetSettings() {
  char* tpl = (char*)"{\"station\":\"%s\",\n\"remote\":\"%s\",\n\"altitude\":%0.2f,\n\"time\":\"%04d-%02d-%02d %02d:%02d\"}";
  char buffer[250];
  RtcDateTime d = Rtc.GetDateTime();
  sprintf(buffer, tpl, setting.station, setting.remote, setting.altitude, d.Year(), d.Month(), d.Day(), d.Hour(), d.Minute());
  server.send(200,"text/json", buffer);
}

void remoteUpdate() {
  if (strlen(setting.remote) != 0) {
    //TODO: Do the remote stuff
  }
}

char* getPressureRange(int pressure) {
  if (pressure > 102268) {
    return (char*)"High Pressure";
  } else if (pressure > 100914) {
    return (char*)"Normal Pressure";
  } else {
    return (char*)"Low Pressure";
  }
}

char* getTrend() {
  if (hour < 10) {
    return (char*)"Unknown";
  }

  int change = currentPressure  - (history[17].pressure + history[18].pressure + history[19].pressure) / 3;

  if (change >= 600) {
    return (char*)"very rapidly rising";
  } else if (change >= 360) {
    return (char*)"rapidly rising";
  } else if (change >= 160) {
    return (char*)"rising";
  } else if ( change >= 15) {
    return (char*)"slowly rising";
  } else if (change < 15 && change > -15) {
    return (char*)"steady";
  } else if (change < -600) {
    return (char*)"very rapidly falling";
  } else if (change <= -360) {
    return (char*)"rapidly falling";
  } else if (change <= -160) {
    return (char*)"falling";
  } else {
    return (char*)"slowly falling";
  }
}

char* getFrostRisk() {
  if (currentTemperature < 1 || (hour >= 3 && history[23].temperature < 1 && history[22].temperature < 1 && history[21].temperature < 1)) {
    return (char*)"true";
  }
  return (char*)"false";
}

void formatTime(unsigned int t, char* sTime) {
  RtcDateTime d(t);
  Serial.printf("Format: %04d-%02d-%02d %02d:%02d", d.Year(), d.Month(), d.Day(), d.Hour(), d.Minute());
  sprintf(sTime, "%04d-%02d-%02d %02d:%02d", d.Year(), d.Month(), d.Day(), d.Hour(), d.Minute());
}
