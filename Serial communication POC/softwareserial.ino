#include <SoftwareSerial.h>

// Define the pins for the SoftwareSerial communication
SoftwareSerial softSerial(2, 3); // RX, TX

void setup() {
  // Initialize the SoftwareSerial communication
  softSerial.begin(9600);
  // Initialize the Serial communication for debugging
  Serial.begin(9600);
}

void loop() {
  if (softSerial.available()) {
    // Read the incoming data from the ESP32
    char c = softSerial.read();
    // Print the incoming data to the Serial monitor for debugging
    Serial.print(c);
  }
  
  if (Serial.available()) {
    // Read the incoming data from the Serial monitor
    char c = Serial.read();
    // Send the incoming data to the ESP32
    softSerial.write(c);
  }
}
