# esp8266-Environmental-Monitoring
These are arduino codes for 3 esp8266 modules. Module n°1 and n°2 are each connected to DHT22 temperature and humidity sensor, as well as an MQ-135 air quality sensor. Module n°3 have a Geiger counter detector and air quality sensor attached to it for ionizing radiation and air quality monitoring. 
Each module collectes and send periodically its data to a remote server (Raspberry Pi) through a Wifi network. MQTT protocol is used as the communication protocol between the module and the server.

