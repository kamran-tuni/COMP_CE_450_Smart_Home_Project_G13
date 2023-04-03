// Define the pins for the HardwareSerial communication
#define RX_PIN 16
#define TX_PIN 17

void setup() {
  // Initialize the HardwareSerial communication
  Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  if (Serial.available()) {
    // Read the incoming data from the Arduino
    char c = Serial.read();
    // Print the incoming data to the Serial monitor for debugging
    Serial.print(c);
  }
  
  // Send data to the Arduino every 1 second
  Serial.println("Hello, Arduino!");
  delay(1000);
}
