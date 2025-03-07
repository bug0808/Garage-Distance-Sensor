# ESP32 Garage Door Manager
WebServer for garage door management. 
Given direction from law enforcement NOT to have a garage door opener in your car it is convenient to have a way to open a close your garage door from your driveway.
Chances are your wifi extends out to your driveway!
This simple little project uses an ESP32 to host a webpage and to run a relay that is connected to your garage door opener.
By connecting to the webserver from your phone you can pulse the door to either open or close.
The webserver provides a list of the last 10 garage door opens and closes and a simple button to trigger the door.

I put a cheap I2C oled to put a screen for debugging purposes.  (Can easily comment out this code if not necessary)
I use a fixed IP (192.168.0.9) for the web server so that in case of a router Reboot it will always have the same IP, you will likely need to adjust for your network.
The ESP32 uses NTP to lookup current time, it is set to Toronto time, you will want to adjust for your purposes.
In the event of not being able to connect to WIFI the ESP32 will reboot and attempt to reconnect.  (Will do that indefinitely)

To build this you need:
ESP32
Single Pole 3V Relay suitable for the ESP32
I2C OLED 

Wire the relay Normally Open (NO) and Common (CO) in parallel with whatever existing garage door button you have. 


