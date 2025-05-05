# Power Walk System
# NODEMCU.ino File
- This C++ code is written for a NodeMCU (ESP8266) microcontroller. It integrates multiple components, including an RFID reader (MFRC522), an OLED display (SSD1306), a relay, and a battery monitoring system. It also connects to a WiFi network and communicates with a remote server via HTTP.

### How It Works:
- Connects to WiFi and initializes hardware components.
- Reads the battery voltage and calculates battery percentage.
- Scans an RFID card to get its unique ID.
- Sends battery data & RFID UID to the server via an HTTP POST request.
- Receives a response: If "Yes", access is granted. Otherwise, print the response.
