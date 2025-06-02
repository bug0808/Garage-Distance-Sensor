# ESP32 Garage Door Manager
WebServer for garage door management. 
Given direction from law enforcement NOT to have a garage door opener in your car it is convenient to have a way to open a close your garage door from your driveway.
Chances are your wifi extends out to your driveway!
This simple little project uses an ESP32 to host a webpage and to run a relay that is connected to your garage door opener.
By connecting to the webserver from your phone you can pulse the door to either open or close.
The webserver provides a list of the last 10 garage door opens and closes and a simple button to trigger the door.

**New Features:**
- **Ultrasonic distance sensor:** Measures how close your car is to the sensor.
- **LED ring (NeoPixel):** Changes color (green/red/blue) to indicate if you should stop or keep moving your car, based on the measured distance.
- **Magnetic (reed) switch:** Detects if the garage door is open or closed. The LED ring only lights up when the door is open.

I put a cheap I2C oled to put a screen for debugging purposes.  (Can easily comment out this code if not necessary)
I use a fixed IP (192.168.0.9) for the web server so that in case of a router Reboot it will always have the same IP, you will likely need to adjust for your network.
The ESP32 uses NTP to lookup current time, it is set to Toronto time, you will want to adjust for your purposes.
In the event of not being able to connect to WIFI the ESP32 will reboot and attempt to reconnect.  (Will do that indefinitely)

To build this you need:
ESP32
Single Pole 3V Relay suitable for the ESP32
I2C OLED 
Ultrasonic distance sensor (e.g., HC-SR04)
NeoPixel/WS2812 LED ring
Magnetic (reed) switch for door state detection

## Code Pin Assignments

The following GPIO pins are used in the default code. You can change these in the `.ino` file if needed to match your hardware setup:

- **Relay control:** GPIO 27 (`output27`)
- **Ultrasonic sensor:**
  - **TRIG:** GPIO 13 (`TRIG_PIN`)
  - **ECHO:** GPIO 12 (`ECHO_PIN`)
- **NeoPixel LED ring:** GPIO 17 (`LED_PIN`)
- **Magnetic (reed) switch:** GPIO 26 (`DOOR_SENSOR_PIN`)
- **I2C OLED display:** Uses default I2C pins (SDA/SCL) for your ESP32 board

**Note:**  
- All pin assignments are defined at the top of the code using `#define` or `const int`.
- If you use different pins, update the corresponding definitions in your code.

Wiring notes:
- Wire the relay Normally Open (NO) and Common (CO) in parallel with whatever existing garage door button you have.
- Connect the ultrasonic sensor TRIG and ECHO pins to the ESP32 as defined in the code.
- Connect the LED ring DIN to the ESP32 as defined in the code.
- Connect one side of the magnetic switch to the ESP32 GPIO (see code, e.g., GPIO 26), and the other side to GND. Use `INPUT_PULLUP` in code.


