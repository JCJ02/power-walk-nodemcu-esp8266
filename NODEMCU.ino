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

MFRC522 mfrc522(SS_PIN, RST_PIN); // CREATE MFRC522 INSTANCE.
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

int battery = 0; // VARIABLE TO STORE THE SOUND VALUE
float previousVoltage = 0; 

const int relay = 16;
const char* serverName = "https://192.168.100.5/powerwalksystem/server.php";
unsigned long timerStart = 0; // TO STORE THE START TIME OF THE TIMER
unsigned long timerDuration = 10000; // TIMER DURATION IN MILLISECONDS (E.G., 10000 MS = 10 SECONDS)
bool timerStarted = false; // FLAG TO CHECK IF THE TIMER WAS STARTED
unsigned long elapsedTime = 0; // VARIABLE TO STORE ELAPSED TIME

// const int batteryPin = A0; // ADC PIN CONNECTED TO BATTERY VOLTAGE
// const float fullBatteryVoltage = 12.8; // EXAMPLE FOR A LI-ION BATTERY

const int batteryPin = A0; // ADC PIN CONNECTED TO BATTERY VOLTAGE
const float voltageDividerRatio = 4.78; // ADJUST THIS BASED ON YOUR VOLTAGE DIVIDER CIRCUIT
const float referenceVoltage = 3.3; // NodeMCU ADC REFERENCE VOLTAGE

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
    // VOLTAGE TO CAPACITY MAPPING FOR a 12V LEAD-ACID BATTERY
    const float voltagePoints[] = {10.5, 11.31, 11.58, 11.75, 11.9, 12.06, 12.2, 12.32, 12.42, 12.5, 12.6};
    const float capacityPoints[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // ENSURE THE VOLTAGE IS WITHIN THE VALID RANGE
    if (voltage <= voltagePoints[0]) return 0; // BBELOW MINIMUM VOLTAGE
    if (voltage >= voltagePoints[10]) return 100; // ABOVE MAXIMUM VOLTAGE

    // FIND THE INTERVAL WHERE THE VOLTAGE LIES
    int i = 0;
    while (voltage > voltagePoints[i + 1]) {
        i++;
    }

    // LINEAR INTERPOLATION
    float percentage = capacityPoints[i] + (voltage - voltagePoints[i]) * (capacityPoints[i + 1] - capacityPoints[i]) / (voltagePoints[i + 1] - voltagePoints[i]);

    return percentage;
}

// ADD THE AVERAGING FUNCTION HERE
int getAverageADCReading(int pin, int samples = 10) {
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(1000); // SHORT DELAY BETWEEN READINGS
    }
    return sum / samples;
}

void setup() 
{
  Serial.begin(115200); // INITIATE A SERIAL COMMUNICATION
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
  SPI.begin(); // INITIATE SPI BUS
  mfrc522.PCD_Init(); // INITIATE MFRC522
}

void loop() 
{

  if(WiFi.status()== WL_CONNECTED){
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
  
  
    delay(1000);
    HTTPClient https;
    https.begin(*client, serverName);

    // SPECIFY CONTENT-TYPE HEADER
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

    // CALCULATE ELECTRICITY GENERATED (IF VOLTAGE INCREASED)
    float electricityGenerated = 0.0;
    if (batteryVoltage > previousVoltage) {
      electricityGenerated = batteryVoltage - previousVoltage; // VOLTAGE INCREASE (E.G., 0.1V)
    }
    previousVoltage = batteryVoltage; // UPDATE PREVIOUS VOLTAGE

    // int adcReading = analogRead(batteryPin); 
    // float batteryVoltage = (adcReading * 12.0) / 1023; // ASSUMING 3.3V REFERENCE VOLTAGE
    // float batteryPercentage = (batteryVoltage / fullBatteryVoltage) * 100;

    // LOOK FOR NEW CARDS
    if ( ! mfrc522.PICC_IsNewCardPresent()) 
    {
      return;
    }
    // SELECT ONE OF THE CARDS
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
    content.toUpperCase(); // ENSURE CASE CONSISTENCY (HEX IS CASE-INSENSITIVE)

    // String elect = String(raw_adc);
    // String elect = String(adcReading);
    String bv = String(batteryVoltage);
    String batt = String(batteryPercentage);
    String eg = String(electricityGenerated);
    // String co2 = String(content);
    String co2 = String(decimalContent);

    // PREPARE YOUR HTTP POST REQUEST DATA
    String httpRequestData = "&battery=" + batt + "&uid=" + co2 + "&battVol=" + bv + "&electGen=" + eg;

    // YOU CAN COMMENT THE httpRequestData VARIABLE ABOVE
    // THEN, USE THE httpRequestData VARIABLE BELOW (FOR TESTING PURPOSES WITHOUT THE BME280 SENSOR)
    //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&location=Office&value1=24.75&value2=49.54&value3=1005.14";
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = https.POST(httpRequestData);

    String payload = https.getString();
 
    if (httpResponseCode > 0) {

    } else {
      
    }

    https.end();

    // CHECK IF THE UID IS "123456"
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

