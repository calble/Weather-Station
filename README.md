# Weather Station
Weather Station is powered by an ESP8266 and a BMP180.
The BMP180 SCL is connected to D1 and SDA is connected to D2.

Upon powerup the board will flash the flash both leds to indicated the last octet of its IP address.
Upon failure to connect to the BMP180, led2 will flash in an infinte loop.
Upon completed boot LED1 will be on and LED2 will be off.

# URL Enpoints
* / --> GET The main html page for the application
* /reset --> POST to this endpoint to reset the high/low pressure and temperature readings.
* /json --> GET the JSON representation of what is on /.
* /main.css --> GET the CSS for the / page.
* /graph.js --> GET the javascript code for rendering the graph on the root page.