 # ESP Haier
 
 
 ESP Haier is a project to use a ESP8266 (I did not test with ESP32) to control Haier Air Conditioner (firmware version R_1.0.00/e_2.5.14) with wifi module supoort with ESPHome and Home Assisant.
 

 You only need configure esphaier.yml, put Haier.h in the same directory and flash. 
 
 Home Assisant will recognize as climate device.
 
 For now, ESPHome climate component don't support fan speed or swing, so, ESP Haier neither. When esphome will have support, I will add it.
 
 This project is based in the works of Instalator and his project [Haier Wifi](https://github.com/instalator/Haier_WiFi/)
 
 
 I use a Wemos D1 Mini with a "hacked" USB cable. I cut the USB cable and connect directly to Wemos:
 
 - Red -> 5V 
 - Black -> GND 
 - Green -> RX 
 - White -> TX
 
  # Known issues
 
- The current temperature is not well reported. It always report 28ºC

- When setting the mode to Cool through the remote control it is getting as it is in auto instead of cool.
