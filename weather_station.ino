#include<ESP8266WiFi.h>
#include<Adafruit_BMP085.h>
#include<Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <TaskScheduler.h>
#include <stdlib.h>

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
void handleJson();
void handleNotFound();
void handleReset();
void handleMainCss();
void sensorUpdate();
void hourlyUpdate();
void resetHighLow();
void generateTable();
char* findTrend();
void handleGraphJs();
int range(float arr[]);
int smallest(float arr[]);
int largest(float arr[]);
char* generateArray(float arr[]);
char* generateArray(int arr[]);
void flashIP();

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
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/main.css", HTTP_GET, handleMainCss);
  server.on("/graph.js", HTTP_GET, handleGraphJs);
  server.onNotFound(handleNotFound);
  server.begin();

  runner.init();
  runner.addTask(sensorTask);
  runner.addTask(hourlyTask);
  sensorTask.enable();
  hourlyTask.enable();

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
  float tempF = (currentTemp * 9.0 / 5.0) + 32;
  float tempFLow = (lowTemp * 9.0 / 5.0) + 32;
  float tempFHigh = (highTemp * 9.0 / 5.0) + 32;

  generateTable();
//  table[0] = '\0';
  
  char* trend =  findTrend();
//  char* trend = (char*)malloc(sizeof(char) * 50);
//  sprintf(trend, "XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX");
  //Not the trend causing the reboot

  //Could the problem be the generating of the graph?
  //tt is used to test the generateArray method.
//  float tt[] = {68.2,69,70.12,70,71.55, 75, 77.3, 78, 72, 71, 70, 72.3,68.2,69,70.12,70,71.55, 75, 77.3, 78, 72, 71, 70, 72.3};
  char* t = generateArray(temp);
  
  char* p = generateArray(pressure);
//  char* t = "[10,20,30,40,50,25,22,44,15,20,10,20,30,40,50,25,22,44,15,20,30,13,17,50]";
//  char* p = "[25,22,44,15,20,10,20,30,25,22,44,15,20,10,20,30,25,22,44,15,20,10,20,30,31]";
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
    <ul>\
      <li><span>Temperature:</span> %.2f&deg;F (%.2f&deg;C)</li>\
      <li><span>Pressure:</span> %dPa</li>\
      <li><span>Trend:</span> %s</li>\
    </ul>\
    <h2>Historic Data</h2>\
    <div id=\"ts\"></div>\
    <div class=\"half\">Temperature</div>\
    <div class=\"half\">Pressure</div>\
    <ul>\
      <li><span>Low Temperature:</span> <span>%.2f&deg;F</span> <span>(%.2f&deg;C)</span></li>\
      <li><span>High Temperature:</span> <span>%.2f&deg;F</span> <span>(%.2f&deg;C)</span></li>\
      <li><span>Low Pressure:</span> <span>%dPa</span></li>\
      <li><span>High Pressure:</span> <span>%dPa</span></li>\
    </ul>\
    %s\
    </div>\
  </body>\
  </html>", t, p, tempF, currentTemp, currentPressure, trend, tempFLow, lowTemp, tempFHigh, highTemp, lowPressure, highPressure, table);
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

void handleMainCss() {
  char css[1500];
  
  sprintf(css, "body{background-color: lightblue; font-family: sans-serif;}\
  body>div{background-color: white; padding: 5px; margin: 10px auto; width: 90%%; max-width: 500px; border-radius: 10px;}\
  input[type=submit]{font-weight: bold; background-color: orange; padding: 5px; margin: 10px; float: right; color: white; border-radius: 10px}\
  body>div>h1{float: left; margin: 10px; padding: 5px;}\
  body>div>h2{clear: both; margin: 10px; padding: 5px;}\
  thead{background-color: #BBB;}\
  tbody tr:nth-child(even){background-color: #DDD;}\
  table {width: 100%%; border-radius: 0px 10px 10px 0px;}\
  td,th {margin: 2px;}\
  #ts::after{ content:\"\"; clear:both; display: table}\
  .r {background-color: #f56642; width: 1.5%%; margin-right: 0.5%%; float: left}\
  .b {background-color: #4299f5; width: 1.5%%; margin-right: 0.5%%; float: left}\
  .up {background-color: green}\
  .down {background-color: red}\
  .even {background-color: yellow}\
  .half {width: 50%%; float: left; text-align: center; font-size: 12px; margin-bottom: 8px}");

//  for(int i=1; i <= 50; i++){
//    char g[50];
//    int marginTop = 50 - i;
//    sprintf(g, ".s%d{height:%dpx, margin-top: %dpx}", i, i, marginTop);
//    strcat(css, g);
//  }

//  Serial.printf("CSS:\n%s\n", css);
  server.send(200, "text/css", css); 
}

void handleGraphJs(){
  char* js = "function f(){\
  t.forEach(e=>{\
    var x = document.createElement(\"div\");\
    x.setAttribute(\"class\",\"r\");\
    x.setAttribute(\"style\",\"height: \" + (e+1) + \"px; margin-top: \" + (51-e) + \"px\");\
    document.querySelector(\"#ts\").appendChild(x);\
  });\
  p.forEach(e=>{\
    var x = document.createElement(\"div\");\
    x.setAttribute(\"class\",\"b\");\
    x.setAttribute(\"style\",\"height: \" + (e+1) + \"px; margin-top: \" + (51-e) + \"px\");\
    document.querySelector(\"#ts\").appendChild(x);\
  });\
  }";
  
  server.send(200, "text/javascript", js);  
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
    float fTemp = (temp[i] * 9.0 / 5.0) + 32;

    strcat(table, "<td>");
    sprintf(strNum, "%.2f", fTemp);
    strcat(table, strNum);
    strcat(table, "&deg;F ");
    sprintf(strNum, "%.2f", temp[i]);
    strcat(table, " (");
    strcat(table, strNum);
    strcat(table, "&deg;C)</td>");

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
  }else if(change <= -15){
    strcat(trend, "slowly falling");
  }else if(change <= -160){
    strcat(trend, "falling");
  }else if(change <= -360){
    strcat(trend, "rapidly falling");
  }else{
    strcat(trend, "very rapidly falling");
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
  for(int i=0; i < 24; i++){
    char b[5];
    int scaledNumber = (int)(((arr[i] - small) / r) * 50);
    Serial.printf("%d,", scaledNumber);
    sprintf(b, "%d,", scaledNumber);
    strcat(tempArray, b);
    end += strlen(b) + 1;
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
