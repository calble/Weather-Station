#include<ESP8266WiFi.h>
#include<Adafruit_BMP085.h>
#include<Wire.h>
#include <ESP8266WebServer.h>
#include <TaskScheduler.h>
#include <stdlib.h>

const short int BUILTIN_LED1 = 2; //GPIO2
const short int BUILTIN_LED2 = 16;//GPIO16

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
void findTrend(char *);

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
  digitalWrite(BUILTIN_LED1, LOW); // Turn the LED ON by making the voltage LOW digitalWrite(BUILTIN_LED2, HIGH); // Turn the LED off by making the voltage HIGH delay(1000); // Wait for a second

  Wire.begin();
  delay(30);
  if (!bmp.begin()) {
    Serial.println("BMP180 failed to connect.");
  } else {
    digitalWrite(BUILTIN_LED2, LOW);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJson);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/main.css", HTTP_GET, handleMainCss);
  server.onNotFound(handleNotFound);
  server.begin();

  runner.init();
  runner.addTask(sensorTask);
  runner.addTask(hourlyTask);
  sensorTask.enable();
  hourlyTask.enable();

  resetHighLow();
}

void loop() {
  server.handleClient();
  runner.execute();
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
  char trend[5];
  findTrend(trend);
  
  sprintf(buffer, "<!DOCTYPE html>\
  <html lang=\"en\">\
    <head>\
    <meta charset=\"UTF-8\">\
    <title>Weather Station</title>\
    <link rel=\"stylesheet\" href=\"/main.css\"/>\
    </head>\
  <body>\
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
    <ul>\
      <li><span>Low Temperature:</span> <span>%.2f&deg;F</span> <span>(%.2f&deg;C)</span></li>\
      <li><span>High Temperature:</span> <span>%.2f&deg;F</span> <span>(%.2f&deg;C)</span></li>\
      <li><span>Low Pressure:</span> <span>%dPa</span></li>\
      <li><span>High Pressure:</span> <span>%dPa</span></li>\
    </ul>\
    %s\
    </div>\
  </body>\
  </html>", tempF, currentTemp, currentPressure, trend, tempFLow, lowTemp, tempFHigh, highTemp, lowPressure, highPressure, table);
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
  char css[2000];
  
  sprintf(css, "body{background-color: lightblue; font-family: sans-serif;}\
  body>div{background-color: white; padding: 5px; margin: 10px auto; width: 90%%; max-width: 500px; border-radius: 10px;}\
  input[type=submit]{font-weight: bold; background-color: orange; padding: 5px; margin: 10px; float: right; color: white; border-radius: 10px}\
  body>div>h1{float: left; margin: 10px; padding: 5px;}\
  body>div>h2{clear: both; margin: 10px; padding: 5px;}\
  thead{background-color: #BBB;}\
  tbody tr:nth-child(even){background-color: #DDD;}\
  table {width: 100%%; border-radius: 0px 10px 10px 0px;}\
  td,th {margin: 2px;}\
  .r {background-color: red; height: 10px}\
  .b {background-color: blue; height: 10px}\
  .up {background-color: green}\
  .down {background-color: red}\
  .even {background-color: yellow}");

  for(int i=1; i <= 50; i++){
    char g[50];
    sprintf(g, ".s%d{width:%dpx}", i, i);
    strcat(css, g);
  }

  //Serial.printf("CSS:\n%s\n", css);
  server.send(200, "text/css", css); 
}

void resetHighLow() {
  lowPressure = highPressure = bmp.readPressure();
  lowTemp = highTemp = bmp.readTemperature();
}

void sensorUpdate() {
  Serial.println("Sensor Update");
  currentTemp = bmp.readTemperature();
  currentPressure = bmp.readPressure();

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
  int position = hour % 24;
  temp[position] = currentTemp;
  pressure[position] = currentPressure;
  hour++;
}

void generateTable() {
  char strNum[20];
  //reset the table buffer
  table[0] = '\0';
  Serial.println("Generating Table");
  strcat(table, "<table><thead><th>Hour</th><th>Temperature</th><th>Pressure</th></thead><tbody>");
  for (int i = 0; i < 24; i++) {
    //Hour column
    strcat(table, "<tr><th>");
    itoa(i, strNum, 10);
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

void findTrend(char *trend){
  int past = 0;
  int present = 0;
  //TODO: use the hour%24 to determine oldest 12 and newest 12 hours.
  int currentHour = hour % 24;
  int pos = currentHour;
  for(int i=0; i < 12; i++){
    if(pos > 23){
      pos = 0;
    }
    present += pressure[pos];
  }
  present /= 12;
  for(int i=0; i < 12; i++){
    if(pos > 23){
      pos = 0;
    }
    past += pressure[pos];
  }
  past /= 12;

  if(past == 0 || present == 0){
    sprintf(trend, "none");
    return;
  }
  if(abs(present-past) < 34){
    sprintf(trend, "even");
    return;
  }
  if(past < present){
    sprintf(trend, "up");
    return;
  }
  if(past > present){
    sprintf(trend, "down");
    return;
  }
}
