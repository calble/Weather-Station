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
void sensorUpdate();
void hourlyUpdate();
void generateTable();
char* findTrend();
char* generateArray(float arr[]);
char* generateArray(int arr[]);
void flashIP();
char* findPressureLevel();
char * findPressureTrend();
String trendImage();

Task sensorTask(10000, TASK_FOREVER, &sensorUpdate);
Task hourlyTask(1000 * 60 * 60, TASK_FOREVER, &hourlyUpdate);
Scheduler runner;

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED1, OUTPUT); // Initialize the BUILTIN_LED1 pin as an output
  pinMode(BUILTIN_LED2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output
  pinMode(CLEAR_MEMORY, INPUT);

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
  MDNS.addService("http","tcp",80);
  MDNS.addService("weather","tcp",80);
  Wire.begin();
  delay(30);
  if (!bmp.begin(0x76, &Wire)) {
    Serial.println("BME280 failed to connect.");
     while(true){
      digitalWrite(BUILTIN_LED2, LOW);  
      delay(500);
      digitalWrite(BUILTIN_LED2, HIGH);
      delay(500);
    }   
  }

  //Setup RTC
  Rtc.Begin();
  if(!Rtc.IsDateTimeValid()){
    Serial.println("RTC IsDateTimeValid False!");
  }
  Rtc.SetIsRunning(true);
  //Setup EEPROM
  RtcEeprom.Begin();

  //  eeprom_test();
//  resetHighLow();
//  debug();
//  eeprom_rw_test(RtcEeprom);
  
  //Clear EEPROM if the pin is high 3 times in a row
  if(digitalRead(CLEAR_MEMORY) == LOW){
    delay(50);
    if(digitalRead(CLEAR_MEMORY) == LOW){
      delay(50);
      if(digitalRead(CLEAR_MEMORY) == LOW){
          clearEEPROM();
      }
    }
  }else{
    Serial.println("CLEAR PIN was HIGH, not reseting memory.");
  }
  
  //Setup server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJson);
  server.onNotFound(handleStaticFiles);
  server.on("/save_settings", HTTP_POST, handleSaveSettings);
  server.on("/get_settings",HTTP_POST, handleGetSettings);
  
  server.begin();

  //Schedule tasks
  runner.init();
  runner.addTask(sensorTask);
  runner.addTask(hourlyTask);
  sensorTask.enable();
  hourlyTask.enable();

  //Setup flash memory file system
  SPIFFS.begin();
  
  restore();
  flashIP();
}

void loop() {
  server.handleClient();
  runner.execute();
  MDNS.update();
  //  Serial.println("Someone connnected. Reading Sensor");
  //  float pressure = bmp.readPressure();
  //  float temp = bmp.readTemperature();
  //  Serial.print("Pressure: ");
  //  Serial.println(pressure);
  //
  //  Serial.print("Temperature: ");
  //  Serial.println(temp);
  if(digitalRead(CLEAR_MEMORY) == LOW){
    debug();
  }
}

void handleRoot() {
  Serial.println("Handling Root");
  //  float pressure = bmp.readPressure();
  //  float temp = bmp.readTemperature();
  float tempF = cToF(currentTemp);
  float tempFLow = cToF(lowTemp);
  float tempFHigh = cToF(highTemp);
  //Image for trend

  Serial.println("Trend Image");
  String image = trendImage();
  //24 hour max and min temps and pressure
  int offset = (hour < 23)?(24-hour):0;
  int count = (hour < 24)?hour:24;
  
  Serial.println("tempMin");
  float tempMin = cToF(minValue(temp, offset, count));
  Serial.println("tempMax");
  float tempMax = cToF(maxValue(temp, offset, count));
  float tempRange = tempMax - tempMin;;
  
  Serial.println("pressure Min");
  int  pressureMin = minValue(pressure, offset, count);
  Serial.println("Pressure max");
  int pressureMax = maxValue(pressure, offset, count);
  int pressureRange = pressureMax - pressureMin;
  
  Serial.println("Generate Table");
  generateTable();
//  table[0] = '\0';

  Serial.println("Find Trend");
  char* trend =  findTrend();
//  char* trend = (char*)malloc(sizeof(char) * 50);
//  sprintf(trend, "XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX");
  //Not the trend causing the reboot

  //Could the problem be the generating of the graph?
  //tt is used to test the generateArray method.
//  float tt[] = {68.2,69,70.12,70,71.55, 75, 77.3, 78, 72, 71, 70, 72.3,68.2,69,70.12,70,71.55, 75, 77.3, 78, 72, 71, 70, 72.3};
  Serial.println("Generate Array temp");
  char* t = generateArray(temp);
  Serial.println("Generate Array pressure");
  char* p = generateArray(pressure);
//  char* t = "[10,20,30,40,50,25,22,44,15,20,10,20,30,40,50,25,22,44,15,20,30,13,17,50]";
//  char* p = "[25,22,44,15,20,10,20,30,25,22,44,15,20,10,20,30,25,22,44,15,20,10,20,30,31]";
  Serial.println("SPRINTF");
  sprintf(buffer, "<!DOCTYPE html>\
  <html lang=\"en\">\
    <head>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <title>Weather Station</title>\
    <link rel=\"stylesheet\" href=\"/main.css\"/>\
    <script src=\"/graph.js\"></script>\
    <script>\
      var t = %s;\
      var p = %s;\
    </script>\
    </head>\
  <body onload=\"f()\">\
    <div>\
    <h1>Weather Station</h1>\
    <hr/>\
    <h2>Current Weather</h2>\
    <div class=\"small\">As of %d/%d/%d at %d:%02d</div>\
    <div class=\"current\">\
      <div class=\"temp\">%.1f&deg;</div>\
      <div class=\"barometer\">\
        <div class=\"pressure\">%d<span>Pa</span><img src=\"%s\"/></div>\
        <div class=\"trend\">%s</div>\
      </div>\
    </div>\
    <h2>Historic Data</h2>\
    <div id=\"ts\"></div>\
    <div class=\"half\">Temperature (%.2f-%.2f) Range: %.2f</div>\
    <div class=\"half\">Pressure (%d-%d) Range: %d</div>\
    <h3>Record Highs and Lows</h3>\
    <ul>\
      <li><span>Record Low Temperature:</span> <span>%.2f&deg;F</span></li>\
      <li><span>Record High Temperature:</span> <span>%.2f&deg;F</span></li>\
      <li><span>Record Low Pressure:</span> <span>%dPa</span></li>\
      <li><span>Record High Pressure:</span> <span>%dPa</span></li>\
    </ul>\
    <h3>Hourly Data</h3>\
    %s\
    <form class=\"i\" method=\"get\" action=\"/settings.htm\">\
      <input type=\"submit\" value=\"Settings\"/>\
    </form>\
    </div>\
  </body>\
  </html>", t, p, 
  Rtc.GetDateTime().Month(),
  Rtc.GetDateTime().Day(),
  Rtc.GetDateTime().Year(),
  Rtc.GetDateTime().Hour(),
  Rtc.GetDateTime().Minute(),
  tempF, currentPressure, image.c_str(), trend,  tempMin, tempMax, tempRange, pressureMin, pressureMax, pressureRange, tempFLow, tempFHigh, lowPressure, highPressure, table);
  free(t);
  free(p);
  free(trend);
  Serial.printf("Root content length: %d\n", strlen(buffer));
  server.send(200, "text/html", buffer);
}

void handleJson() {
  Serial.println("Handling JSON");
  //  float pressure = bmp.readPressure();
  //  float temp = bmp.readTemperature();
  float tempF = cToF(currentTemp);
  float tempFLow = cToF(lowTemp * 9.0 / 5.0);
  float tempFHigh = cToF(highTemp * 9.0 / 5.0);
  char* pressureLevel = findPressureLevel();
  char* pressureTrend = findPressureTrend();

  sprintf(buffer, "{\"station\":\"%s\",\n\"temperature_f\":%.2f,\n \"temperature_c\":%.2f,\n\"humidity\":%.2f,\n \"pressure_pa\":%d,\n \"high_temperature_f\":%.2f, \n \"high_temperature_c\":%.2f,\n \"low_temperature_f\":%.2f,\n \"high_tempurature_c\":%.2f,\n \"low_pressure\":%d,\n \"high_pressure\": %d,\n\"low_humidity\":%.2f,\n\"high_humidity\":%.2f,\n\"pressure_level\":\"%s\",\n\"pressure_trend\":\"%s\",\n\"time_series\":[\n",
          station, tempF, currentTemp, currentHumidity, currentPressure, tempFHigh, highTemp, tempFLow, lowTemp, lowPressure, highPressure, lowHumidity, highHumidity, pressureLevel, pressureTrend);

  for(int i=23; i >= 0; i--){
    char c[200];

    if(dates[i] == NULL){
      continue;
    }
    RtcDateTime tmp;
    tmp.InitWithEpoch64Time(dates[i]);
    
    sprintf(c, "{\"date\":\"%04d-%02d-%02d %02d:%02d\",\"temperature_c\":%.2f,\"pressure\":%d, \"humidity\":%.2f},\n",
                tmp.Year(),
                tmp.Month(),
                tmp.Day(),
                
                tmp.Hour(),
                tmp.Minute(),
                temp[i],
                pressure[i],
                humidity[i]);
    strcat(buffer, c);
  }
  int len = strlen(buffer);
  buffer[len-2] = '\0';
  strcat(buffer, "]\n}\n");
  server.send(200, "text/json", buffer);
}

void handleStaticFiles(){
  if(SPIFFS.exists(server.uri())){
    String contentType;
    if(server.uri().endsWith(".htm")){
      contentType = "text/html";
    }else if(server.uri().endsWith(".css")){
      contentType = "text/css";
    }else if(server.uri().endsWith(".js")){
      contentType = "text/javascript";
    }else if(server.uri().endsWith(".png")){
      contentType = "image/png";
    }else if(server.uri().endsWith(".ico")){
      contentType = "image/png";
    }else if(server.uri().endsWith(".gif")){
      contentType = "image/gif";
    }else{
      //We don't server any other type of file
      server.send(401, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>401 Unauthorized!</h2></body></html>");
      return;    
    }
    File file = SPIFFS.open(server.uri(), "r");
    server.streamFile(file, contentType);
    file.close();
  }else{
    //File does not exist
    server.send(404, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>404 File Not Found!</h2></body></html>");
  }
}

void handleSaveSettings(){
  if(server.hasArg("action") && server.arg("action") == "Save Settings" && server.hasArg("time") && server.hasArg("date") && server.hasArg("station") && server.hasArg("remote")){
    //Clear out old remote and station settings
    char clearStation[17];
    char clearRemote[32];
    for(int i=0; i < 17; i++){
      clearStation[i] = '\0';
    }
    for(int i=0; i < 32; i++){
      clearRemote[i] = '\0';
    }
    RtcEeprom.SetMemory(ADDR_STATION, (const uint8_t*)clearStation, 32);
    delay(50);
    RtcEeprom.SetMemory(ADDR_REMOTE, (const uint8_t*)clearRemote, 32);
    delay(50);
    
    String t = server.arg("time");
    String d = server.arg("date");

    int year = atoi(d.substring(0,4).c_str());
    int month = atoi(d.substring(5,7).c_str());
    int day = atoi(d.substring(8,10).c_str());

    int hour = atoi(t.substring(0,2).c_str());
    int minute = atoi(t.substring(3,5).c_str());

    RtcDateTime date(year, month, day, hour, minute, 0);
    Rtc.SetDateTime(date);

    //Station saving logic
    String station = server.arg("station");
    if(station.length() > 31){
      station = station.substring(0, 31);
    }
    Serial.printf("Station Length: %d::%d -- ", station.length(), (byte)station.length());
    RtcEeprom.SetMemory(ADDR_STATION, (const uint8_t*)station.c_str(), station.length());
    delay(50);
    
    //Remote saving logic
    String remote = server.arg("remote");
    if(remote.length() > 31){
      remote = remote.substring(0, 31);
    }

    Serial.printf("Remote Length: %d::%d\n", remote.length(), (byte)remote.length());
    RtcEeprom.SetMemory(ADDR_REMOTE, (const uint8_t*)remote.c_str(), remote.length());
    delay(50);
    
    File file = SPIFFS.open("/saved.htm", "r");
    server.streamFile(file, "text/html");
  }else if(server.hasArg("action") && server.arg("action") == "Reset Historical Data"){
    resetHighLow();
    File file = SPIFFS.open("/reset.htm", "r");
    server.streamFile(file, "text/html");
  }else{
    server.send(400, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station Error</title></head><body><h1>Weather Station Error</h1><h2>400 Bad Request!</h2><p>Settings not saved!</p></body></html>");
  }
}



void sensorUpdate() {
  currentTemp = bmp.readTemperature();
  currentPressure = pressureAtSealevel();
  currentHumidity = bmp.readHumidity();

  Serial.printf("Sensor Update: %.2fC, %.2f%%, %dPa\n", currentTemp, currentHumidity, currentPressure);
  byte buffer[4];
  
  if (currentTemp > highTemp) {
    highTemp = currentTemp;
    floatToBytes(highTemp, buffer);
    RtcEeprom.SetMemory(ADDR_HIGH_TEMP, buffer, 4);
    delay(10);
  }
  if (currentTemp < lowTemp) {
    lowTemp = currentTemp;
    floatToBytes(lowTemp, buffer);
    RtcEeprom.SetMemory(ADDR_LOW_TEMP, buffer, 4);
    delay(10);
  }
  
  if (currentPressure > highPressure) {
    highPressure = currentPressure;
    intToBytes(highPressure, buffer);
    RtcEeprom.SetMemory(ADDR_HIGH_PRESSURE, buffer, 4);
    delay(10);
  }
  if (currentPressure < lowPressure) {
    lowPressure = currentPressure;
    intToBytes(lowPressure, buffer);
    RtcEeprom.SetMemory(ADDR_LOW_PRESSURE, buffer, 4);
    delay(10);
  }

  if(currentHumidity > highHumidity){
    highHumidity = currentHumidity;
    floatToBytes(highHumidity, buffer);
    RtcEeprom.SetMemory(ADDR_HIGH_HUMIDITY, buffer, 4);
    delay(10);
  }
  if(currentHumidity < lowHumidity){
    lowHumidity = currentHumidity;
    floatToBytes(lowHumidity, buffer);
    RtcEeprom.SetMemory(ADDR_LOW_HUMIDITY, buffer, 4);
    delay(10);
  }
}

void hourlyUpdate() {
  Serial.println("Hourly Update");
  //Shift old pressures down and show most recent data on the right.
  for(int i=0; i < 23; i++){
    temp[i] = temp[i+1];
    pressure[i] = pressure[i+1];
    humidity[i] = humidity[i+1];
    dates[i] = dates[i+1];
  }
  
  temp[23] = currentTemp;
  pressure[23] = currentPressure;
  humidity[23] = currentHumidity;
  dates[23] = Rtc.GetDateTime();
  hour++;

  save();
}

void generateTable() {
  char strNum[20];
  //reset the table buffer
  
  Serial.println("Generating Table");
  sprintf(table, "<table><thead><th>Hour</th><th>Temperature</th><th>Pressure</th><th>Humidity</th></thead><tbody>");
  int stopValue = (hour < 24)?(24-hour):0;
  
  for (int i = 23; i >= stopValue; i--) {
    //Hour column
    strcat(table, "<tr><th>");
    itoa((23 - i), strNum, 10);
    strcat(table, strNum);
    strcat(table, "</th>");

    //Temperature Column
    float fTemp = cToF(temp[i]);

    strcat(table, "<td>");
    sprintf(strNum, "%.2f", fTemp);
    strcat(table, strNum);
    strcat(table, "&deg;F </td>");

    //Pressure Column
    strcat(table, "<td>");
    itoa(pressure[i], strNum, 10);
    strcat(table, strNum);
    strcat(table, "Pa</td>");

    //Humidity Column
    strcat(table,"<td>");
    sprintf(strNum, "%.2f", humidity[i]);
    strcat(table, strNum);
    strcat(table, "%</td></tr>");
  }
  strcat(table, "</tbody></table>");
  int s = strlen(table);
  Serial.printf("Table Complete: %d\n", s);
}

char* findPressureLevel(){
  //high pressure
  if(currentPressure > 102268){
   return (char*)"High Pressure";
  //Normal Pressure
  }else if(currentPressure > 100914){
    return (char*)"Normal Pressure";
  //Low Pressure
  }else{
    return (char*)"Low Pressure";
  }
}

char * findPressureTrend(){
  if(hour < 10){
    return (char*)"Unknown";
  }
  
  int change = currentPressure  - (pressure[17] + pressure[18] + pressure[19]) / 3;

  if(change >= 600){
    return (char*)"very rapidly rising";
  }else if(change >= 360){
    return (char*)"rapidly rising";
  }else if(change >= 160){
    return (char*)"rising";
  }else if( change >= 15){
    return (char*)"slowly rising";
  }else if(change < 15 && change > -15){
    return (char*)"steady";
  }else if(change < -600){
    return (char*)"very rapidly falling";
  }else if(change <= -360){
    return (char*)"rapidly falling";
  }else if(change <= -160){
    return (char*)"falling";
  }else{
    return (char*)"slowly falling";
  }
}
char* findTrend(){
  char* trend = (char*)malloc(sizeof(char) * 50);
  trend[0] = '\0';
  
  //high pressure
  if(currentPressure > 102268){
    strcat(trend, "High Pressure, ");
  //Normal Pressure
  }else if(currentPressure > 100914){
    strcat(trend, "Normal Pressure, ");
  //Low Pressure
  }else{
    strcat(trend, "Low Pressure, ");
  }

  if(hour < 10){
    strcat(trend, "More data needed");
    return trend;
  }
  
  int change = currentPressure  - (pressure[17] + pressure[18] + pressure[19]) / 3;

  if(change >= 600){
    strcat(trend, "very rapidly rising");
  }else if(change >= 360){
    strcat(trend, "rapidly rising");
  }else if(change >= 160){
    strcat(trend, "rising");
  }else if( change >= 15){
    strcat(trend, "slowly rising");
  }else if(change < 15 && change > -15){
    strcat(trend, "steady");
  }else if(change < -600){
    strcat(trend, "very rapidly falling");
  }else if(change <= -360){
    strcat(trend, "rapidly falling");
  }else if(change <= -160){
    strcat(trend, "falling");
  }else{
    strcat(trend, "slowly falling");
  }
  
  return trend;
}

char* generateArray(int arr[]){
    float fArr[24];
    for(int i=0; i < 24; i++){
      fArr[i] = (float)arr[i];
    }
    return generateArray(fArr);
}

char* generateArray(float arr[]){
  int size = 2 + (2 + 1) * 24 + 5;
  char* tempArray = (char*)malloc(sizeof(char) * size);
  tempArray[0] = '\0';
  strcat(tempArray, "[");

  float small = smallest(arr, hour);
  float r = range(arr, hour);

  Serial.printf("Generate Array-- Smallest: %f, Range: %f, Hour: %d", small, r, hour);
  Serial.printf("Org Data: [");
  for(int i=0; i < 24; i++){
    Serial.printf("%f,", arr[i]);
  }
  Serial.printf("\nJS Values: ");
  
  int end = 1;
  int offset = (hour < 24)?24-hour:0;
  for(int i=0; i < 24; i++){
    char b[5];

    if(i < offset){
      strcat(tempArray, "0,");
      end += 2;
    }else{
      int scaledNumber;
      if(r == 0){
        scaledNumber = 50;
      }else{
        scaledNumber = (int)(((arr[i] - small) / r) * 50);
      }
      Serial.printf("%d,", scaledNumber);
      sprintf(b, "%d,", scaledNumber);
      strcat(tempArray, b);
      end += strlen(b) + 1;
    }
  }
  
  strcat(tempArray, "]");
  Serial.printf("\nComplete: %s\n\n", tempArray);
  return tempArray;
}

void flashIP(){
  Serial.printf("LAST: %d\n",WiFi.localIP()[3]);
  int ip = WiFi.localIP()[3];
  int digits[3] = {ip/100, ip/10%10, ip%10};
  
  digitalWrite(BUILTIN_LED1, HIGH);
  digitalWrite(BUILTIN_LED2, HIGH);
  delay(4000);
  for(int i=0; i < 3; i++){
    for(int j=0; j < digits[i]; j++){
        digitalWrite(BUILTIN_LED1, LOW);
        digitalWrite(BUILTIN_LED2, LOW);
        delay(300*3);
        digitalWrite(BUILTIN_LED1, HIGH);
        digitalWrite(BUILTIN_LED2, HIGH);
        delay(300);
        
    }
    delay(2500);
  }

  digitalWrite(BUILTIN_LED1, LOW);
  digitalWrite(BUILTIN_LED2, HIGH);
}

String trendImage(){
  int change = currentPressure  - (pressure[17] + pressure[18] + pressure[19]) / 3;

  if(hour < 10){
    return "/unknown.png";
  }
  
  if(change >= 160){
    return "/big_up.png";
  }else if(change >= 15){
    return "/up.png";
  }else if(change < 15 && change > -15){
    return "/steady.png";
  }else if(change <= -160){
    return "/big_down.png";
  }else{
    return "/down.png";
  }
}

int pressureAtSealevel(){
  int pressure = bmp.readPressure();
  return pressure / pow(1-ALTITUDE/44330.0, 5.255);
}

void handleGetSettings(){
  int size = RtcEeprom.GetMemory(ADDR_STATION);
  Serial.printf("Station length: %d", size);
  //station name is capped at 31
  byte station[32];
  RtcEeprom.GetMemory(ADDR_STATION, station, 32);

  //Clean up input in case of garbage from eeprom
  for(int i=0; i < 32; i++){
    if(!isprint(station[i]) && station[i] != '\0'){
      station[i] = '.';
    }
  }
  station[31] = '\0';
  
  byte remote[32];
  RtcEeprom.GetMemory(ADDR_REMOTE, remote, 32);

  //Clean up input in case of garbage from eeprom
  for(int i=0; i < 30; i++){
    if(!isprint(remote[i]) && remote[i] != '\0'){
      remote[i] = '.';
    }
  }
  remote[31] = '\0';

  char buffer[500];
  sprintf(buffer, "{\"date\":\"%d-%02d-%02d %02d:%02d\",\"station\":\"%s\",\"remote\":\"%s\"}",  
  Rtc.GetDateTime().Year(),
  Rtc.GetDateTime().Month(),
  Rtc.GetDateTime().Day(),
  Rtc.GetDateTime().Hour(),
  Rtc.GetDateTime().Minute(), 
  (char*)station, 
  (char*)remote);
  
  server.send(200, "text/json", buffer);
}
