/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"

// BLE Server name
#define bleServerName "BLE_Nano"

// BLE Service
static BLEUUID EnvironmentalSensingServiceUUID("cfc1cd76-cb65-11ed-afa1-0242ac120002");

// BLE Characteristics
static BLEUUID TemperatureCharacteristicUUID("cfc1c916-cb65-11ed-afa1-0242ac120002");
static BLEUUID HumidityCharacteristicUUID("cfc1d2d0-cb65-11ed-afa1-0242ac120002");
static BLEUUID LightCharacteristicUUID("cfc1cc18-cb65-11ed-afa1-0242ac120002");

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress);

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};

// Characteristicd that we want to read
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* humidityCharacteristic;
static BLERemoteCharacteristic* lightCharacteristic;

// Flags to check whether new temperature, humidity and light readings are available
boolean newTemperature = false;
boolean newHumidity = false;
boolean newLight = false;

// Variables to store temperature, humidity and light
char* temperatureChar;
char* humidityChar;
char* lightChar;

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress);

//create global BLE client to make the connection stable by adding reconnection support
BLEClient* pClient = BLEDevice::createClient();
 
// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  
  //Init BLE device
  BLEDevice::init("");
  /*
    Retrieve a Scanner and set the callback we want to use to be informed when we
    have detected a new device.  Specify that we want active scanning and start the
    scan to run for 30 seconds.
  */
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.

  // todo additional code to aid reconnection
  bool status = pClient->isConnected(); // check BLE connection
  if (!status) {
    doConnect = true;
    temperatureChar[0] = 0; humidityChar[0] = 0; lightChar[0] = 0;        // purge stale data
    Serial.println("Reconnecting BLE…");
    printReadings(); // redraw display so it does not display stale data
  }

  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      // Activate the Notify property of each Characteristic
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      humidityCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      lightCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } 
    else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  
  //if new temperature readings are available, print in the OLED
  if (newTemperature && newHumidity && newLight){
    newTemperature = false;
    newHumidity = false;
    newLight = false;
    printReadings();
  }
  delay(5000); // Delay a second between loops.
}

bool connectToServer(BLEAddress pAddress) {
  // BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(EnvironmentalSensingServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(EnvironmentalSensingServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  temperatureCharacteristic = pRemoteService->getCharacteristic(TemperatureCharacteristicUUID);
  humidityCharacteristic = pRemoteService->getCharacteristic(HumidityCharacteristicUUID);
  lightCharacteristic = pRemoteService->getCharacteristic(LightCharacteristicUUID);
  
  if (temperatureCharacteristic == nullptr || humidityCharacteristic == nullptr || lightCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  humidityCharacteristic->registerForNotify(humidityNotifyCallback);
  lightCharacteristic->registerForNotify(humidityNotifyCallback);
  return true;
}

// When the BLE Server sends a new temperature reading with the notify property
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  temperatureChar = (char*)pData;
  newTemperature = true;
  Serial.println(temperatureChar);
}

// When the BLE Server sends a new humidity reading with the notify property
static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  humidityChar = (char*)pData;
  newHumidity = true;
  Serial.println(humidityChar);
}

// When the BLE Server sends a new humidity reading with the notify property
static void lightNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  lightChar = (char*)pData;
  newLight = true;
  Serial.println(lightChar);
}

void printReadings(){
   Serial.print("Temperature: ");
   Serial.print(temperatureChar);
   Serial.println(" C");

   Serial.print("Humidity: ");
   Serial.print(humidityChar);
   Serial.println(" %");

   Serial.print("Light: ");
   Serial.print(lightChar);
   Serial.println(" L");
}
