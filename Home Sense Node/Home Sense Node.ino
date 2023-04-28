#include <Wire.h>
#include <ArduinoBLE.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <arduino-timer.h>

#include <nrf_power.h>
#include <nrf_sdh.h>
#include <nrf_sdh_soc.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define LDR_PIN A0
#define BAUD_RATE 9600
#define BLE_ADVERSTING_INTERVAL 250
#define DEVICE_NAME "IoT Sensors"
#define DEVICE_ID 0xAA
#define BLE_NUM_OF_BYTES 9
#define SENSOR_READ_INTERVAL 60000
#define BLE_ADVERTISEMENT_TIMEOUT 60000
#define BLE_CONNECTION_TIMEOUT 60000

typedef struct RAW_SENSOR_DATA {
  uint8_t device_id; 
  uint16_t temperature; 
  uint16_t humidity; 
  uint16_t light; 
  uint16_t pressure;  
}__attribute__((aligned))ble_sensor_t;

typedef struct SENSOR_DATA {
  float temperature; 
  float humidity; 
  float light; 
  float pressure;  
}__attribute__((aligned))display_sensor_t;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* g_service_uuid = "cfc1cd76-cb65-11ed-afa1-0242ac120002";
const char* g_tx_char_uuid = "cfc1c916-cb65-11ed-afa1-0242ac120002";
BLEService envService(g_service_uuid);
BLECharacteristic THLCharacteristic(g_tx_char_uuid, BLERead | BLENotify,BLE_NUM_OF_BYTES,false);

display_sensor_t g_display_data;
uint8_t g_ble_byte_array[BLE_NUM_OF_BYTES];
auto timer = timer_create_default(); // create a timer with default settings
static volatile bool g_read_flag = false;
static volatile bool g_disconnect_flag = true;
static volatile bool g_connect_flag = false;
BLEDevice central_device;

bool timer_isr(void *) {
  Serial.println("Read timer triggered.");
  g_read_flag = true;
  return true; // repeat? true
}

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  g_connect_flag = true;
  central_device = central;
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  g_disconnect_flag = false;
  g_connect_flag =false; 
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void ble_init()
{
    // Initialize BLE
    if (!BLE.begin()) {
      Serial.println("Failed to initialize BLE!");
      while (1);
    }
    // Set the local name and advertising interval
    BLE.setDeviceName(DEVICE_NAME);
    BLE.setAdvertisedService(envService); // Advertise the custom BLE service
    BLE.setAdvertisingInterval(BLE_ADVERSTING_INTERVAL);

    // assign event handlers for connected, disconnected to peripheral
    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

    // Add characteristics to the service
    envService.addCharacteristic(THLCharacteristic);
    BLE.addService(envService);
}

void sensors_init()
{
    pinMode(LDR_PIN, INPUT);

    if (!HTS.begin()) {
      Serial.println("Failed to initialize humidity temperature sensor!");
      while (1);
    }

    if (!BARO.begin()) {
      Serial.println("Failed to initialize pressure sensor!");
      while (1);
    }
}

void sensors_read()
{
    g_display_data.temperature = HTS.readTemperature();
    g_display_data.humidity = HTS.readHumidity();
    g_display_data.light = analogRead(LDR_PIN) * (3.3 / 1023.0);
    g_display_data.pressure = BARO.readPressure(); 

    ble_sensor_t g_sensor_data;
    g_sensor_data.device_id   = DEVICE_ID;
    g_sensor_data.temperature = g_display_data.temperature * 100;
    g_sensor_data.humidity    = g_display_data.humidity * 100;
    g_sensor_data.light       = g_display_data.light * 100;
    g_sensor_data.pressure    = g_display_data.pressure * 100;

      
// memcpy(g_ble_byte_array,&g_sensor_data, sizeof(g_sensor_data)); 

    g_ble_byte_array[0] = g_sensor_data.device_id;
    g_ble_byte_array[1] = (g_sensor_data.temperature >> 8) & 0xFF; // MSB of temperature
    g_ble_byte_array[2] = g_sensor_data.temperature & 0xFF; // LSB of temperature
    g_ble_byte_array[3] = (g_sensor_data.humidity >> 8) & 0xFF; // MSB of humidity
    g_ble_byte_array[4] = g_sensor_data.humidity & 0xFF; // LSB of humidity
    g_ble_byte_array[5] = (g_sensor_data.light >> 8) & 0xFF; // MSB of light
    g_ble_byte_array[6] = g_sensor_data.light & 0xFF; // LSB of light
    g_ble_byte_array[7] = (g_sensor_data.pressure >> 8) & 0xFF; // MSB of pressure
    g_ble_byte_array[8] = g_sensor_data.pressure & 0xFF; // LSB of pressure    
}

void display_oled()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("T: ");
    display.print(g_display_data.temperature);
    display.println("C");
    display.print("H: ");
    display.print(g_display_data.humidity);
    display.println("%");
    display.print("L: ");
    display.print(g_display_data.light);
    display.println(" V");
    display.print("P: ");
    display.print(g_display_data.pressure);
    display.println("kPa");
    display.display();
} 

void setup() 
{
    delay(2000);  
    Serial.begin(BAUD_RATE);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();

    sensors_init();
    ble_init();
    BLE.advertise(); // Start advertising the BLE service

    // call the toggle_led function every 60000 millis (1 minute)
    timer.every(SENSOR_READ_INTERVAL, timer_isr);
}

void loop()
{
  timer.tick();
  int ret =0;  
     
  if (g_read_flag)
  {
    sensors_read();
    display_oled();
  
    if (g_connect_flag)
    {
     ret = THLCharacteristic.writeValue(g_ble_byte_array,sizeof(g_ble_byte_array),true);
    }
    
    if (ret) 
    {      
      g_read_flag = false;
    }   
  }
  
   BLE.poll();
}