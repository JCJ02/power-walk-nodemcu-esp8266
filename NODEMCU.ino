/*
 *  Created by TheCircuit
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SS_PIN 0  //D2
#define RST_PIN 2 //D1

#include <SPI.h>
#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
int statuss = 0;
int out = 0;

#include <SPI.h>
#include <Wire.h>


#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

const char* ssid = "Jacobe Family";
const char* password = "b@w@lk0m0n3k";  


int battery = 0; // Variable to store the sound value
float previousVoltage = 0; 

const int relay = 16;
const char* serverName = "https://192.168.100.71/powerwalksystem/server.php";
unsigned long timerStart = 0; // To store the start time of the timer
unsigned long timerDuration = 10000; // Timer duration in milliseconds (e.g., 10000 ms = 10 seconds)
bool timerStarted = false; // Flag to check if the timer was started
unsigned long elapsedTime = 0; // Variable to store elapsed time

// const int batteryPin = A0; // ADC pin connected to battery voltage
// const float fullBatteryVoltage = 12.8; // Example for a Li-ion battery

const int batteryPin = A0; // ADC pin connected to battery voltage
const float voltageDividerRatio = 4.78; // Adjust this based on your voltage divider circuit
const float referenceVoltage = 3.3; // NodeMCU ADC reference voltage

// float getBatteryPercentage(float voltage) {
//     if (voltage >= 12.6) return 100;
//     else if (voltage >= 12.5) return 90;
//     else if (voltage >= 12.42) return 80;
//     else if (voltage >= 12.32) return 70;
//     else if (voltage >= 12.2) return 60;
//     else if (voltage >= 12.06) return 50;
//     else if (voltage >= 11.9) return 40;
//     else if (voltage >= 11.75) return 30;
//     else if (voltage >= 11.58) return 20;
//     else if (voltage >= 11.31) return 10;
//     else return 0;
// }

float getBatteryPercentage(float voltage) {
    // Voltage-to-capacity mapping for a 12V lead-acid battery
    const float voltagePoints[] = {10.5, 11.31, 11.58, 11.75, 11.9, 12.06, 12.2, 12.32, 12.42, 12.5, 12.6};
    const float capacityPoints[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // Ensure the voltage is within the valid range
    if (voltage <= voltagePoints[0]) return 0; // Below minimum voltage
    if (voltage >= voltagePoints[10]) return 100; // Above maximum voltage

    // Find the interval where the voltage lies
    int i = 0;
    while (voltage > voltagePoints[i + 1]) {
        i++;
    }

    // Linear interpolation
    float percentage = capacityPoints[i] + (voltage - voltagePoints[i]) * (capacityPoints[i + 1] - capacityPoints[i]) / (voltagePoints[i + 1] - voltagePoints[i]);

    return percentage;
}

// Add the averaging function here
int getAverageADCReading(int pin, int samples = 10) {
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(100); // Short delay between readings
    }
    return sum / samples;
}

void setup() 
{
  Serial.begin(115200);   // Initiate a serial communication
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  pinMode(relay, OUTPUT);

  digitalWrite(relay, HIGH);
  Serial.print("Connected to WiFi Network with IP Address: ");
  Serial.println(WiFi.localIP());

  delay(3000);
  SPI.begin(); // Initiate  SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522
}

void loop() 
{

  if(WiFi.status()== WL_CONNECTED){
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
  
  
    delay(1000);
    HTTPClient https;
    https.begin(*client, serverName);

    // Specify content-type header
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int adcReading = getAverageADCReading(batteryPin);
    // int adcReading = analogRead(batteryPin); 
    float batteryVoltage = (adcReading / 1023.0) * referenceVoltage * voltageDividerRatio;
    float batteryPercentage = getBatteryPercentage(batteryVoltage);

    // Serial.print("Battery Voltage: ");
    // Serial.print(batteryVoltage);
    // Serial.print("V, Battery Percentage: ");
    // Serial.print(batteryPercentage);
    // Serial.println("%");


    // Calculate electricity generated (if voltage increased)
    float electricityGenerated = 0.0;
    if (batteryVoltage > previousVoltage) {
      electricityGenerated = batteryVoltage - previousVoltage; // Voltage increase (e.g., 0.1V)
    }
    previousVoltage = batteryVoltage; // Update previous voltage


    // int adcReading = analogRead(batteryPin); 

    // float batteryVoltage = (adcReading * 12.0) / 1023; // Assuming 3.3V reference voltage

    // float batteryPercentage = (batteryVoltage / fullBatteryVoltage) * 100;




    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) 
    {
      return;
    }
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) 
    {
      return;
    }

    String content= "";
    byte letter;
    String decimalContent= "";
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
      decimalContent.concat(String(mfrc522.uid.uidByte[i], DEC));
      
    }
    content.toUpperCase(); // Ensure case consistency (HEX is case-insensitive)

    // String elect = String(raw_adc);
    // String elect = String(adcReading);
    String bv = String(batteryVoltage);
    String batt = String(batteryPercentage);
    String eg = String(electricityGenerated);
    // String co2 = String(content);
    String co2 = String(decimalContent);

    // Prepare your HTTP POST request data
    String httpRequestData = "&battery=" + batt + "&uid=" + co2 + "&battVol=" + bv + "&electGen=" + eg;

    // You can comment the httpRequestData variable above
    // then, use the httpRequestData variable below (for testing purposes without the BME280 sensor)
    //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&location=Office&value1=24.75&value2=49.54&value3=1005.14";
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = https.POST(httpRequestData);

    String payload = https.getString();
 
    if (httpResponseCode > 0) {

    } else {
      
    }

    https.end();

    // Check if the UID is "123456"
    if (payload=="Yes") {

      Serial.println("Yes");

    } else{
      Serial.println(payload);

      delay(1000);
      Serial.flush();
    }

  } else {

  }

} 

