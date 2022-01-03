# esp8266-Environmental-Monitoring
These are arduino codes for 3 esp8266 modules. Module n°1 and n°2 are each connected to DHT22 temperature and humidity sensor, as well as an MQ-135 air quality sensor. Module n°3 have a Geiger counter detector and air quality sensor attached to it for ionizing radiation and air quality monitoring. 
Each module collectes and send periodically its data to a remote server (Raspberry Pi) through a Wifi network. MQTT protocol is used as the communication protocol between the module and the server.

Here are photos the 3 modules:
![Screenshot_20210928-183110_Gallery](https://user-images.githubusercontent.com/23704606/147923003-289a969f-ff5f-40e1-830e-e65fab18e129.jpg)


