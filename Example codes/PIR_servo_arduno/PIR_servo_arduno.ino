#include <Servo.h>
#include <SoftwareSerial.h>

/************************ Sense Node Configurations ************************/
// PIR sensor for motion detection
#define PIRSensorPin  4
// Ultrasonics sensor for proximity detection         
#define trigPin       12       // generates a 10 us pulse
#define echoPin       11       // reads the pulse duration to calculate distance between the object and obstacle
#define MAX_DISTANCE  50       // in cm
// servo motor controlling the lock
Servo myservo;
#define servoPin      9

// PIR sensor variables
#define MAX_NUM_DETECTIONS 5000
int detect_counter;

/********************* Sense Node Serial Configurations ********************/
// Define the pins for the SoftwareSerial communication
SoftwareSerial softSerial(2, 3); // RX, TX
const char* cmd_unlock_door = "\x55\x01\xa0\x5e";   // lock command
void fnCheckGatewaySerial();    // Check if data is recieved from gateway

// todo : remove
const char* unlock_door = "unlock";

/********** SenseNode Presence and Proximity detection functions ***********/
boolean fnIsMostionDetected();    // detects presence through PIR sensor 
float fnCalculateDistance();      // when a person is detected, calculate distance of the person
void fnUnlockDoor();              // perform unlock operation

void setup() {
  // put your setup code here, to run once:
  pinMode(PIRSensorPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); 
  Serial.begin(9600);
  softSerial.begin(9600);
}

void loop() {
  // PIR Sensor Motion detection
  if(fnIsMostionDetected()){
    detect_counter++;
    if (detect_counter == MAX_NUM_DETECTIONS){
      Serial.println("Motion detected");
      float distance = fnCalculateDistance();
      if (distance < MAX_DISTANCE){
        // send notification to ESP32
        Serial.println("Person detected"); 
        byte message[] = {0x55, 0x02, 0xA0, 0x5E};
        softSerial.write(message, sizeof(message)); 
      }
    }
  }          
  else
    detect_counter = 0;

  delay(1000);
}

boolean fnIsMostionDetected()
{
  int sensorValue = digitalRead(PIRSensorPin);
  return (sensorValue == HIGH)?true:false;
}

float fnCalculateDistance(){
  /* travel time of ultrasonic wave (us) = pulse duration
     speed of ultrasonic wave = 340 m/s = 0.034 cm/us
     distance travelled by ultrasonic wave (cm) = speed * travel time = 0.034 cm/us * pulse duration
     distance btw sensor and obstacle = distance travelled by ultrasonic wave/2
                                      = (0.034 * pulse duration)/2.0 
                                      = 0.017 * pulse duration
  */
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // measure duration of pulse from ECHO pin
  float duration_us = pulseIn(echoPin, HIGH);
  // calculate the distance
  float distance_cm = 0.017 * duration_us;
  
  return distance_cm;
}

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

void fnCheckGatewaySerial(){
  String msg = "";
  while(softSerial.available()) {
    // todo uncomment this
    msg += char(softSerial.read());
    //msg = softSerial.readString();
    delay(50);
  }

//  // todo remove
//  if (msg == "unlock"){
//    Serial.println(msg);
//    fnUnlockDoor();
//  }
  
  // check if lock command is recieved
  if (msg == cmd_unlock_door){
    Serial.println("Unlock command recieved");
    // send acknowledgment to esp32 that door is unlocked
    byte message[] = {0x55, 0x02, 0x0A, 0x5E};
    softSerial.write(message, sizeof(message));
    fnUnlockDoor();
  }
}
