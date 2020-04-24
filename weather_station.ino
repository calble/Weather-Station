#include<ESP8266WiFi.h>
#include<Adafruit_BMP085.h>
#include<Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <TaskScheduler.h>
#include <stdlib.h>
#include <FS.h>

const short int BUILTIN_LED1 = 2; //GPIO2
const short int BUILTIN_LED2 = 16;//GPIO16

#define ALTITUDE 198 //Meter above sea level of station

ESP8266WebServer server(80);
Adafruit_BMP085 bmp;

char buffer[4000];
char table[2000];
float temp[24];
int pressure[24];

float currentTemp = 0;
int currentPressure = 0;
int hour = 0;
float highTemp = 0;
float lowTemp = 0;
int highPressure = 0;
int lowPressure = 0;

void handleRoot();
void handleStaticFiles();
void handleJson();
void handleNotFound();
void handleReset();
void sensorUpdate();
void hourlyUpdate();
void resetHighLow();
void generateTable();
char* findTrend();
int range(float arr[]);
int smallest(float arr[]);
int largest(float arr[]);
char* generateArray(float arr[]);
char* generateArray(int arr[]);
void flashIP();

String trendImage();
float cToF(float);
float minValue(float[], int);
float maxValue(float[], int);
int minValue(int[], int);
int maxValue(int[], int);

  

Task sensorTask(10000, TASK_FOREVER, &sensorUpdate);
Task hourlyTask(1000 * 60 * 60, TASK_FOREVER, &hourlyUpdate);
Scheduler runner;

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED1, OUTPUT); // Initialize the BUILTIN_LED1 pin as an output
  pinMode(BUILTIN_LED2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output

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
  if (!bmp.begin()) {
    Serial.println("BMP180 failed to connect.");
     while(true){
      digitalWrite(BUILTIN_LED2, LOW);  
      delay(500);
      digitalWrite(BUILTIN_LED2, HIGH);
      delay(500);
    }   
  }
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJson);
  server.onNotFound(handleStaticFiles);
  server.on("/reset", HTTP_POST, handleReset);
//  server.on("/main.css", HTTP_GET, handleMainCss);
//  server.on("/graph.js", HTTP_GET, handleGraphJs);
//  
  server.begin();

  runner.init();
  runner.addTask(sensorTask);
  runner.addTask(hourlyTask);
  sensorTask.enable();
  hourlyTask.enable();

  SPIFFS.begin();
  
  resetHighLow();
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
  Serial.println("pressure Min");
  int  pressureMin = minValue(pressure, offset, count);
  Serial.println("Pressure max");
  int pressureMax = maxValue(pressure, offset, count);

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
    <form method=\"post\" action=\"/reset\">\
      <input type=\"submit\" value=\"Reset High/Lows\"/>\
    </form>\
    <br/>\
    <h2>Current Weather</h2>\
    <div class=\"current\">\
      <div class=\"temp\">%.1f&deg;</div>\
      <div class=\"barometer\">\
        <div class=\"pressure\">%d<span>Pa</span></div>\
        <div class=\"trend\">%s</div>\
      </div>\
      <img src=\"%s\"/>\
    </div>\
    <h2>Historic Data</h2>\
    <div id=\"ts\"></div>\
    <div class=\"half\">Temperature (%.2f-%.2f)</div>\
    <div class=\"half\">Pressure (%d-%d)</div>\
    <ul>\
      <li><span>Record Low Temperature:</span> <span>%.2f&deg;F</span></li>\
      <li><span>Record High Temperature:</span> <span>%.2f&deg;F</span></li>\
      <li><span>Record Low Pressure:</span> <span>%dPa</span></li>\
      <li><span>Record High Pressure:</span> <span>%dPa</span></li>\
    </ul>\
    %s\
    </div>\
  </body>\
  </html>", t, p, tempF, currentPressure, trend, image.c_str(), tempMin, tempMax, pressureMin, pressureMax, tempFLow, tempFHigh, lowPressure, highPressure, table);
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
  float tempF = (currentTemp * 9.0 / 5.0) + 32;
  float tempFLow = (lowTemp * 9.0 / 5.0) + 32;
  float tempFHigh = (highTemp * 9.0 / 5.0) + 32;
  sprintf(buffer, "{\"station\":\"Ambrosse\",\n\"temperature_f\":%.2f,\n \"temperature_c\":%.2f,\n \"pressure_pa\":%d,\n \"high_temperature_f\":%.2f, \n \"high_temperature_c\":%.2f,\n \"low_temperature_f\":%.2f,\n \"high_tempurature_c\":%.2f,\n \"low_pressure\":%d,\n \"high_pressure\": %d, \"time_series\":[\n",
          tempF, currentTemp, currentPressure, tempFHigh, highTemp, tempFLow, lowTemp, lowPressure, highPressure);

  for(int i=0; i < 24; i++){
    char c[200];
    sprintf(c, "{\"hour\":%d,\"temperature_c\":%.2f,\"pressure\":%d},\n", i, temp[i], pressure[i]);
    strcat(buffer, c);
  }
  int len = strlen(buffer);
  buffer[len-2] = '\0';
  strcat(buffer, "]\n}\n");
  server.send(200, "text/json", buffer);
}

void handleNotFound() {
  Serial.println("Handling Not Found");
  server.send(404, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>404 File Not Found!</h2></body></html>");
}

void handleReset() {
  resetHighLow();
  server.send(200, "text/html", "<!DOCTYPE html>\
  <html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\">\
    <title>Weather Station Reset</title>\
  </head>\
  <body>\
    <h1>Weather Station</h1>\
    <p>Reset of low and high measurements is complete</p>\
    <a href=\"/\">Return Home</a>\
  </body>\
  </html>");
}

void handleStaticFiles(){
  if(SPIFFS.exists(server.uri())){
    String contentType;
    if(server.uri().endsWith(".htm")){
      contentType = "text/html";
    }else if(server.uri().endsWith(".css")){
      contentType = "text/css";
    }else if(server.uri().endsWith(".js")){
      contentType = "text/js";
    }else if(server.uri().endsWith(".png")){
      contentType = "image/png";
    }else if(server.uri().endsWith(".ico")){
      contentType = "image/png";
    }else{
      //We don't server any other type of file
      server.send(401, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>401 Unauthorized!</h2></body></html>");
      return;    
    }
    File file = SPIFFS.open(server.uri(), "r");
    server.streamFile(file, "");
    file.close();
  }else{
    //File does not exist
    server.send(404, "text/html", "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Weather Station</title></head><body><h1>Weather Station</h1><h2>404 File Not Found!</h2></body></html>");
  }
}

void resetHighLow() {
  lowPressure = highPressure = bmp.readSealevelPressure(ALTITUDE);
  lowTemp = highTemp = bmp.readTemperature();
}

void sensorUpdate() {
  Serial.println("Sensor Update");
  currentTemp = bmp.readTemperature();
  currentPressure = bmp.readSealevelPressure(ALTITUDE);

  if (currentTemp > highTemp) {
    highTemp = currentTemp;
  }
  if (currentTemp < lowTemp) {
    lowTemp = currentTemp;
  }
  if (currentPressure > highPressure) {
    highPressure = currentPressure;
  }
  if (currentPressure < lowPressure) {
    lowPressure = currentPressure;
  }
}

void hourlyUpdate() {
  Serial.println("Hourly Update");
  //Shift old pressures down and show most recent data on the right.
  for(int i=0; i < 23; i++){
    temp[i] = temp[i+1];
    pressure[i] = pressure[i+1];
  }
  
  temp[23] = currentTemp;
  pressure[23] = currentPressure;
  hour++;
}

void generateTable() {
  char strNum[20];
  //reset the table buffer
  
  Serial.println("Generating Table");
  sprintf(table, "<table><thead><th>Hour</th><th>Temperature</th><th>Pressure</th></thead><tbody>");
  for (int i = 23; i >= 0; i--) {
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
    strcat(table, "Pa</td></tr>");
  }
  int s = strlen(table);
  Serial.printf("Table Complete: %d\n", s);
}

char* findTrend(){
  char* trend = (char*)malloc(sizeof(char) * 50);
  trend[0] = '\0';

  if(hour < 10){
    strcat(trend, "More data needed");
    return trend;
  }
  
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

  int small = smallest(arr);
  int r = range(arr);

  Serial.printf("Generate Array-- Smallest: %d, Range: %d\nValues: ", small, r);
  int end = 1;
  int offset = (hour < 24)?24-hour:0;
  for(int i=0; i < 24; i++){
    char b[5];

    if(i < offset){
      strcat(tempArray, "0,");
      end += 2;
    }else{
      int scaledNumber = (int)(((arr[i] - small) / r) * 50);
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

int range(float arr[]){
  return abs(largest(arr) - smallest(arr));
}

int smallest(float arr[]){
  int smallest = (int)arr[0];
  for(int i=1; i < 24; i++){
    if(arr[i] < smallest){
      smallest = (int)arr[i];
    }
  }
  return smallest;
}

int largest(float arr[]){
  int largest = (int)arr[0];
  for(int i=1; i < 24; i++){
    if(arr[i] > largest){
      largest = (int)arr[i];
    }
  }
  return largest;
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
