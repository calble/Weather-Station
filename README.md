# Weather Station
Weather Station is powered by an ESP8266 and a BMP180.
The BMP180 SCL is connected to D1 and SDA is connected to D2.

# URL Enpoints
* / --> GET The main html page for the application
* /reset --> POST to this endpoint to reset the high/low pressure and temperature readings.
* /json --> GET the JSON representation of what is on /.
* /main.css --> GET the CSS for the / page.
* /graph.js --> GET the javascript code for rendering the graph on the root page.