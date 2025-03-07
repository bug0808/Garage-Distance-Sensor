/*********
Replacement for Garage Door Remote
V7
- Added redirect page
v6
- Removed sub-page, control is done from the main page directly
v5
- Added display of the last 10 pulse times with Toronto time in 12-hour format
- Added display of the last restart time in 12-hour format, captured after NTP call
V4
- Prevented the Pulse Door button from executing unintentionally by switching to GET method and using query parameters
- Added redirect to remove query parameter after pulse to prevent triggering on page refresh
- Fixed issue with conflicting HTTP headers during redirect

*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp32-hal.h> // Required for rebooting the ESP32
#include <time.h> // Library for time functions

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Network credentials
const char* ssid = "YOUR WIFI";
const char* password = "YOUR PASSWORD";

// Optionally use a fixed IP address
bool useFixedIP = true;
IPAddress local_IP(192, 168, 0, 9);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // Optional
IPAddress secondaryDNS(8, 8, 4, 4); // Optional

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Assign output variables to GPIO pins
const int output27 = 27;

// Define pulse duration in milliseconds (example: 2000ms = 2s)
const long pulseDuration = 2000;

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// Define the time interval for checking the internet connection (10 seconds)
const long checkInterval = 10000;
unsigned long lastCheckTime = 0;

// Array to store the last 10 pulse timestamps
String pulseTimes[10];
int pulseIndex = 0;

// Variable to store the last restart time
String lastRestartTime;

// Timer for inactivity-based redirect
unsigned long lastActivityTime = millis();
const unsigned long inactivityTimeout = 60000; // 1 minute inactivity timeout

void setup() {
  Serial.begin(115200);

  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer.
  display.clearDisplay();

  // Initialize the output variables as outputs
  pinMode(output27, OUTPUT);
  // Set output to HIGH
  digitalWrite(output27, HIGH);

  // Set Wi-Fi to maximum power
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  if (useFixedIP) {
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }
  }
  WiFi.begin(ssid, password);

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,28);
  display.println(WiFi.localIP());
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  // Print local IP address and start web server
  display.clearDisplay();
  display.setCursor(0,6);
  display.setTextSize(3);
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
  display.clearDisplay();

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // Configure time for NTP using Toronto time (Eastern Time)
  configTime(-5 * 3600, 3600, "pool.ntp.org", "time.nist.gov"); // -5 hours for EST, 3600 seconds for DST

  // Wait for time to be set
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { // Wait until time is set
    delay(500);
    now = time(nullptr);
  }

  // Capture the last restart time
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char timeStr[30];
  strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p %B %d, %Y", &timeinfo);
  lastRestartTime = String(timeStr);
}

void handleLast10Page(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\"></head>");
  client.println("<body><h1>Last 10 Pulses</h1>");
  client.println("<ul>");
  for (int i = 0; i < 10; i++) {
    int index = (pulseIndex + i) % 10;
    if (pulseTimes[index] != "") {
      client.println("<li>" + pulseTimes[index] + "</li>");
    }
  }
  client.println("</ul>");
  client.println("</body></html>");
}

void loop() {
  unsigned long currentTime = millis();

  // Check if it's time to check the internet connection
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;

    // Check for a valid internet connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("No internet connection. Rebooting ESP32...");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0,28);
      display.println("WIFI FAIL");
      display.display();
      delay(2000);
      ESP.restart(); // Reboot the ESP32
    }
  }

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    unsigned long previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    bool triggerPulse = false;              // Flag to trigger the pulse action
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Check if the pulse button was pressed
            if (header.indexOf("GET /?pulse=1") >= 0) {
              triggerPulse = true;
            }

            if (header.indexOf("GET /last10") >= 0) {
              handleLast10Page(client);
              break;
            }

            if (triggerPulse) {
              Serial.println("Pulsing GPIO 27");
              digitalWrite(output27, LOW);

              // Get the current time
              time_t now;
              struct tm timeinfo;
              time(&now);
              localtime_r(&now, &timeinfo);
              
              // Format the time as a string in 12-hour format with AM/PM and store it
              char timeStr[30];
              strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p %B %d, %Y", &timeinfo);
              pulseTimes[pulseIndex] = String(timeStr);
              pulseIndex = (pulseIndex + 1) % 10; // Keep the last 10 pulses

              // Display Text
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0,28);
              display.println("Pulse Door");
              display.display();
              delay(pulseDuration);
              digitalWrite(output27, HIGH);
              display.clearDisplay();
              display.setCursor(0,12);
              display.setTextSize(3);
              display.println(WiFi.localIP());
              display.display();

              // Send redirect to the last 10 pulses page
              client.println("HTTP/1.1 302 Found");
              client.println("Location: /last10");
              client.println("Connection: close");
              client.println();
              break;
            } else {
              // Display the HTML web page
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the pulse button 
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println("h1 { color: #333; }");
              client.println("p { font-size: 1.2rem; }");
              client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555;}</style></head>");
              
              // Web Page Heading
              client.println("<body><h1>ESP32 Web Server</h1>");
              client.println("<p>Connected to: " + String(ssid) + "</p>");
              
              // Display last restart time
              client.println("<p>Last Restart Time: " + lastRestartTime + "</p>");
              
              // Pulse button for Garage Door (GPIO 27)
              client.println("<p>Garage Door - GPIO 27</p>");
              client.println("<form action=\"/\" method=\"GET\">");
              client.println("<input type=\"hidden\" name=\"pulse\" value=\"1\">");
              client.println("<input type=\"submit\" class=\"button\" value=\"PULSE\">");
              client.println("</form>");
              
              // Countdown timer for inactivity
              client.println("<h2>Redirecting in <span id='countdown'>30</span> seconds...</h2>");
              client.println("<script>");
              client.println("let timeLeft = 30;");
              client.println("const countdownElement = document.getElementById('countdown');");
              client.println("const timer = setInterval(() => {");
              client.println("  timeLeft--;");
              client.println("  countdownElement.textContent = timeLeft;");
              client.println("  if (timeLeft <= 0) {");
              client.println("    clearInterval(timer);");
              client.println("    window.location.href = '/last10';");
              client.println("  }");
              client.println("}, 1000);");
              client.println("</script>");
              
              // Display the last 10 pulse times
              client.println("<h2>Last 10 Pulses</h2>");
              client.println("<ul>");
              for (int i = 0; i < 10; i++) {
                int index = (pulseIndex + i) % 10;
                if (pulseTimes[index] != "") {
                  client.println("<li>" + pulseTimes[index] + "</li>");
                }
              }
              client.println("</ul>");
              
              client.println("</body></html>");
              
              // The HTTP response ends with another blank line
              client.println();
            }

            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
