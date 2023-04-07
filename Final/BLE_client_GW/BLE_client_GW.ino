#include "BLEDevice.h"
//#include "BLEScan.h"

/************************ Sense Node Serial Configurations ************************/
// Define the pins for the ESP32-Arduino Serial communication
#define RX_PIN 16
#define TX_PIN 17
#define CommSerial Serial2

const char* unlock_response = "\x55\x02\x0a\x5e";         // Door successfully unlocked
const char* senseNode_notification = "\x55\x02\xa0\x5e";  // Presence detected by senseNode

void fnCheckSensNodeSerial();       // Check if data is recieved from SenseNode

/************************ BLE data configurations ************************/
#define THLDeviceID     0xAA
#define THLPacketSize   9

// sesnor data packet structure
typedef struct SENSOR_DATA {
  float temperature;
  float humidity;
  float light;
  float pressure; 
}__attribute__((aligned))sensor_data_t;

sensor_data_t g_sensor_data;

/************************ BLE connection configurations ************************/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("cfc1cd76-cb65-11ed-afa1-0242ac120002");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cfc1c916-cb65-11ed-afa1-0242ac120002");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

/************************ BLE functions and callbacks ************************/

void initBLE();   // scan for BLE server if required
bool connectToServer();       // returns true if connection to server succeeds
bool isServerConnected();     // returns true if server is connected
void startScanifRequired();   // scan for BLE server if required

// BLE notification recieved from BLE Nano
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
  
      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
      } 
    } 
}; 

/************************ ESP32 Gateway Setup and Loop ************************/

void setup() {
  // setup UART 2 for serial communication with senseNode
  CommSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // setup Serial Communication for Debugging
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  // initialize BLE connection parameters
  initBLE();
}

void loop() {
  // check if serial data is available
  fnCheckSensNodeSerial();
  
  if(isServerConnected()){     // checks if BLE server is advertising, if yes then attempt to connect to server
    Serial.println("We are now connected to the BLE Server.");
  }
  
  startScanifRequired();  
  delay(2000);                  // Delay a second between loops.
}



/************************ Function Definitions ************************/
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
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

void initBLE(){
  // Initializa BLE connection parameters
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
}

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

bool isServerConnected(){
  if (doConnect == true)
    return connectToServer()? true:false;
  return false;
}

void startScanifRequired(){
  if(!connected && doScan)
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
}

void fnCheckSensNodeSerial(){
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

  // check if senseNode notification is recieved
  if (msg == senseNode_notification){
    Serial.println("Someone is at the door!!!");
    // todo remove
    byte message[] = {0x55, 0x01, 0x0A, 0x5E};
    CommSerial.write(message, sizeof(message));
  }
}
