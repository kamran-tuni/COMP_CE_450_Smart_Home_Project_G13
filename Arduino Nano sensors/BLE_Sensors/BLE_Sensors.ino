#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <ArduinoBLE.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LDR_PIN A0

BLEService envService("181A");
BLEFloatCharacteristic tempCharacteristic("2A6E", BLERead | BLENotify);
BLEFloatCharacteristic humCharacteristic("2A6F", BLERead | BLENotify); 
BLEFloatCharacteristic lumCharacteristic("2A76", BLERead | BLENotify);
BLEFloatCharacteristic preCharacteristic("2724 ", BLERead | BLENotify);

void setup() {
    delay(2000);  
  Serial.begin(9600);

  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(LDR_PIN, INPUT);

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }


  // Set the local name and advertising interval
  BLE.setDeviceName("BLE Nano Sensors");
  BLE.setAdvertisedService(envService);
  BLE.setAdvertisingInterval(500);

  // Add characteristics to the service
  envService.addCharacteristic(tempCharacteristic);
  envService.addCharacteristic(humCharacteristic);
  envService.addCharacteristic(lumCharacteristic);
  envService.addCharacteristic(preCharacteristic);


  // Start advertising the service
  BLE.advertise();
}

void loop() {
  float temperature = HTS.readTemperature();
  float humidity = HTS.readHumidity();
  float pressure = BARO.readPressure();  
  int ldr_value = analogRead(LDR_PIN);
  float luminosity = ldr_value * (3.3 / 1023.0);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("T: ");
  display.print(temperature);
  display.println("C");
  display.print("H: ");
  display.print(humidity);
  display.println("%");
  display.print("L: ");
  display.print(luminosity);
  display.println(" V");
  display.print("P: ");
  display.print(pressure);
  display.println("kPa");
  display.display();

  // Update the characteristics and notify any connected devices
  tempCharacteristic.writeValue(temperature);
  humCharacteristic.writeValue(humidity);
  lumCharacteristic.writeValue(luminosity);
  preCharacteristic.writeValue(pressure);
  BLE.poll();

}
