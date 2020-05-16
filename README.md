# Weather Station
Weather Station is powered by an ESP8266, a BME280, and a DS3231 RTC with a AT25C32 CHIP.
The BME280 and the RTC are connected as follows:
	-- SCL is connected to D1 
	-- SDA is connected to D2.

Upon powerup the board will flash the flash both leds to indicated the last octet of its IP address.
Upon failure to connect to the BME280, led2 will flash in an infinte loop.
Upon completed boot LED1 will be on and LED2 will be off.

## Resetting EEPROM
EEPROM is reset by holding GPIO 3 LOW (Ground).  A message will be printed to the serial monitor about 
resetting progress.  During reset LED1 will be off and LED2 will flash at each of the 256 page
clears. This pattern looks like the LED is mostly on, with short blinks off. Upon completion LED1 will be off and LED2 will flash on and off at a rate of 2Hz for 20 times and then reboot into wifi setup mode.

## URL Enpoints
* / --> GET The main html page for the application
* /get_settings --> POST returns a JSON object with the station, altitude, and remote.
* /save_setttings --> POST saves setting information.  If ssid or password are sent the system will reboot into wifi setup mode.
* /reset_records --> POST resets the high and low records
* /reset_history --> POST resets the historical data
* /json --> GET the JSON representation of what is on /.

## File System
The jar file in the resources directory must be put in the proper Arduino directory to upload
the files.

Make sure you use one of the supported versions of Arduino IDE and have ESP8266 core installed.
Download the tool archive from releases page.
In your Arduino sketchbook directory, create tools directory if it doesn't exist yet. You can find the location of your sketchbook directory in the Arduino IDE at File > Preferences > Sketchbook location.
Unpack the tool into tools directory (the path will look like <sketchbook directory>/tools/ESP8266FS/tool/esp8266fs.jar).
Restart Arduino IDE.