/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
#include <SoftwareSerial.h>
//#include "BLEScan.h"

#define THLDeviceID     0xAA
#define THLPacketSize   9

typedef struct SENSOR_DATA {
  float temperature;
  float humidity;
  float light;
  float pressure; 
}__attribute__((aligned))sensor_data_t;

sensor_data_t g_sensor_data;

// The remote service we wish to connect to.
static BLEUUID serviceUUID("cfc1cd76-cb65-11ed-afa1-0242ac120002");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cfc1c916-cb65-11ed-afa1-0242ac120002");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// todo remove this code after testing
// time constants
unsigned long unlock_previous_time; 
unsigned long unlock_current_time; // millis() function returns unsigned long
unsigned long unlock_timeout = 10000;
const char* unlock_response = "\x55\x02\xa0\x5e";

// setting up software Serial
#define TXPIN 10
#define RXPIN 11
// Set up a new SoftwareSerial object with RX in digital pin 10 and TX in digital pin 11
SoftwareSerial mySerial(TXPIN, RXPIN);



static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
//    Serial.print("Notify callback for characteristic ");
//    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
//    Serial.print(" of data length ");
//    Serial.println(length);
//    Serial.print("data: ");
//    Serial.write(pData, length);
//    Serial.println();

    if(length == THLPacketSize && pData[0] == THLDeviceID){
      g_sensor_data.temperature = (((uint16_t)pData[1] << 8) | pData[2])/100.0;
      g_sensor_data.humidity = (((uint16_t)pData[3] << 8) | pData[4])/100.0;
      g_sensor_data.light = (((uint16_t)pData[5] << 8) | pData[6])/100.0;
      g_sensor_data.pressure = (((uint16_t)pData[7] << 8) | pData[8])/100.0;
      Serial.print("Temperature: ");
      Serial.println(g_sensor_data.temperature);
      Serial.print("Humidity: ");
      Serial.println(g_sensor_data.humidity);
      Serial.print("Light: ");
      Serial.println(g_sensor_data.light);
      Serial.print("Pressure: ");
      Serial.println(g_sensor_data.pressure);
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
 * Called for each advertising BLE server.
 */

void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  mySerial.begin(9600);
  
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);


  // todo remove this
  unlock_previous_time = millis();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // sending lock command once a minute
  // todo remove this part
  unlock_current_time = millis(); // update the timer every cycle
  if (unlock_current_time - unlock_previous_time >= unlock_timeout) {
    Serial.println("Sending unlock cmd"); // the event to trigger
    unlock_previous_time = unlock_current_time;  // reset the timer
    //byte message[] = {0x55, 0x01, 0x0A, 0x5E};
    //mySerial.write(message, sizeof(message));
    mySerial.write("unlock");
  }
  
  String msg = "";
  while(mySerial.available()) {
    // todo uncomment this
    msg += char(mySerial.read());
    delay(50);
  }

  if (msg == unlock_response){
    Serial.println("Door Unlocked by Sense Node");
  }
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000); // Delay a second between loops.
} // End of loop
