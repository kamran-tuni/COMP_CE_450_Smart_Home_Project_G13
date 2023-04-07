#include <SoftwareSerial.h>
#include <Servo.h>

// servo motor controlling the lock
Servo myservo;
#define servoPin      9

const char* cmd_unlock_door = "\x55\x01\x0a\x5e";   // lock command

// Define the pins for the SoftwareSerial communication
SoftwareSerial softSerial(2, 3); // RX, TX

void fnUnlockDoor(){
    // unlock door for 5 seconds
    // Sweep from 0 to 180 degrees:
    myservo.attach(servoPin);
    for (int angle = 0; angle <= 180; angle += 1) {
      myservo.write(angle);
      delay(10);
    }
    delay(5000);
    // And back from 180 to 0 degrees:
    for (int angle = 180; angle >= 0; angle -= 1) {
      myservo.write(angle);
      delay(10);
    }
    myservo.detach();
}

void setup() {
  // Initialize the SoftwareSerial communication
  softSerial.begin(115200);
  // Initialize the Serial communication for debugging
  Serial.begin(115200);
}

void loop() {
//  if (softSerial.available()) {
//    // Read the incoming data from the ESP32
//    char c = softSerial.read();
//    // Print the incoming data to the Serial monitor for debugging
//    Serial.print(c);
//  }
//  
//  if (Serial.available()) {
//    // Read the incoming data from the Serial monitor
//    char c = Serial.read();
//    // Send the incoming data to the ESP32
//    softSerial.write(c);
//  }
//  // Send data to the Arduino every 1 second
//  softSerial.write("Hello, ESP!");
  String msg = "";
  String msg_s = "";
  while(softSerial.available()) {
    // todo uncomment this
    msg += char(softSerial.read());
    // msg_s = softSerial.readString();
    delay(50);
  }
//  Serial.println(msg_s);
  //Serial.println(msg);
  
  // todo remove
//  if (msg_s == "unlock"){
//    Serial.println(msg);
//    fnUnlockDoor();
//    byte message[] = {0x55, 0x02, 0x0A, 0x5E};
//    softSerial.write(message, sizeof(message));
//  }
  
  // check if lock command is recieved
  if (msg == cmd_unlock_door){
    Serial.println("Unlock command recieved");
    // send acknowledgment to esp32 that door is unlocked
    byte message[] = {0x55, 0x02, 0x0A, 0x5E};
    softSerial.write(message, sizeof(message));
    fnUnlockDoor();
  }
  delay(1000);
}
