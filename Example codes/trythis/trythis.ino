// Define the pins for the ESP32-Arduino Serial communication
#define RX_PIN 16
#define TX_PIN 17
#define LED_PIN 2
// this might help aswell: https://icircuit.net/arduino-esp32-hardware-serial2-example/3181#:~:text=ESP32%20has%20three%20hardware%20UART,%2C%20baud%20rate%20etc.).

#define CommSerial Serial2

const char* unlock_response = "\x55\x02\x0a\x5e";

void setup() {
  // Initialize the HardwareSerial communication
  pinMode(LED_PIN,OUTPUT);
  Serial.begin(115200);
  CommSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
//  if (CommSerial.available()) {
//    // Read the incoming data from the Arduino
//    char c = CommSerial.read();
//    if (c == 'f'){
//      digitalWrite(LED_PIN,HIGH);
//      delay(1000);
//      digitalWrite(LED_PIN,LOW);
//    }
//    // Print the incoming data to the Serial monitor for debugging
//    DebugSerial.print(c);
//  }
//
//  // Send data to the Arduino every 1 second
//  CommSerial.write("Hello, Arduino!");
//  delay(1000);
  // check if serial data is available

  byte message[] = {0x55, 0x01, 0x0A, 0x5E};
  CommSerial.write(message, sizeof(message));
  
  String msg = "";
  while(CommSerial.available()) {
    // todo uncomment this
    msg += char(CommSerial.read());
    // msg = CommSerial.readString();
    delay(50);
    // Serial.println(msg);
  }

  // check if lock command is recieved
  if (msg == unlock_response){
    Serial.println("Door unlocked");
  }
    
  delay(20000);

}
