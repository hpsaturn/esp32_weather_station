/********************************************************************
 * Weather Station with DHT22 and ESP32
 * 
 * @author Antonio Vanegas @hpsaturn
 * @date December 2018
 * @brief DHT22 sensor with a ESP32 with bluetooth BLE GATT server
 * @license GPL3
 * 
 * @url https://github.com/hpsaturn/esp32_weather_station
 *
 * rev000 20181128 initial values from DHTesp sample via serial
 * rev001 20181201 basic GATT server with temperature and humidity
 * rev002 20181202 suspend function for setup cycle
 * rev003 20181203 added blink led on BLE waiting connection state
 ********************************************************************/

#include "DHTesp.h"
#include "Ticker.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_sleep.h"

#ifndef ESP32
#pragma message(THIS CODE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

DHTesp dht;

void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;
/** Flag if task should run */
bool tasksEnabled = true;
/** Pin number for DHT22 data pin */
int dhtPin = 16;

// Bluetooth variables
BLEServer* pServer = NULL;
BLECharacteristic* pCharactDHT22 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
#define SERVICE_UUID        "c8d1d262-861f-5082-947e-f383a259aadd"
#define CHARAC_DHT_UUID    "b0f332a8-a5aa-4f3a-bb43-f99e8811ae01"

// ESP32 Deep Sleep time
#define DEEP_SLEEP_DURATION 15 // sleep x seconds and then wake up
#define GPIO_LED_GREEN 22 // Led on TTGO board (black) 
#define GPIO_SENSOR_ENABLE 17

/**
 * blinkOnboardLed
 * notify each data send package
 */

void blinkOnboardLed () { 
  digitalWrite (GPIO_LED_GREEN, LOW);
  delay(50); // fast blink for low power
  digitalWrite (GPIO_LED_GREEN, HIGH);
}

/**
 * initTemp
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initTemp() {
  // Initialize temperature sensor
	dht.setup(dhtPin, DHTesp::DHT22);
	Serial.println("-->[DHT] initiated");
  // Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,                       /* Function to implement the task */
			"tempTask ",                    /* Name of the task */
			4000,                           /* Stack size in words */
			NULL,                           /* Task input parameter */
			5,                              /* Priority of the task */
			&tempTaskHandle,                /* Task handle. */
			1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("--->[E] Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 10 seconds
    tempTicker.attach(10, triggerGetTemp);
  }
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
	   xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT22 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("-->[DHT] temp task loop started");
	while (1) // tempTask loop
  {
    if (tasksEnabled) {
      // Get temperature values
			getTemperature();
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}

/**
 * getTemperature
 * Reads temperature from DHT22 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature() {
	// Reading temperature for humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
	// Check if any reads failed and exit early (to try again).
	if (dht.getStatus() != 0) {
		Serial.println("--->[E] DHT22 error status: " + String(dht.getStatusString()));
		return false;
	}

	float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };

  Serial.println(
    "-->[DHT] T:" + String(newValues.temperature) + 
    " H:" + String(newValues.humidity) + 
    " I:" + String(heatIndex) + 
    " D:" + String(dewPoint) + 
    " " + comfortStatus
  );

  // notify changed value
  if (deviceConnected) {
    String output = String("") + String(newValues.temperature) + ";";
    output = output + String(newValues.humidity) + ";";
    output = output + String(dewPoint) + ";";
    output = output + String(heatIndex);
    pCharactDHT22->setValue(output.c_str());
    pCharactDHT22->notify();
  }

  blinkOnboardLed(); // notify via LED

  return true;
}

void enableSensor () {  // for show receive connections state
  digitalWrite (GPIO_SENSOR_ENABLE, HIGH);
}

void disableSensor () {  // for show receive connections state
  digitalWrite (GPIO_SENSOR_ENABLE, HIGH);
}

void gotToSuspend (){
  Serial.println("-->[ESP] suspending..");
  pServer->getAdvertising()->stop();
  disableSensor();
  delay(8); // waiting for writing msg on serial
  //esp_deep_sleep(1000000LL * DEEP_SLEEP_DURATION);
  esp_sleep_enable_timer_wakeup(1000000LL * DEEP_SLEEP_DURATION);
  esp_deep_sleep_start();
}

/******************************************************************************
*   B L U E T O O T H  M E T H O D S
******************************************************************************/
class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
      Serial.println("-->[BLE] onConnect");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("-->[BLE] onDisconnect");
      deviceConnected = false;
    };
}; // BLEServerCallbacks

void bleServerInit(){
  // Create the BLE Device
  BLEDevice::init("ESP32_DTH22");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic for PM 2.5
  pCharactDHT22 = pService->createCharacteristic(
                      CHARAC_DHT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  // Create a BLE Descriptor
  pCharactDHT22->addDescriptor(new BLE2902());
  // Start the service
  pService->start();
  // Start advertising
  Serial.println("-->[BLE] start advertising");
  pServer->getAdvertising()->start();
  Serial.println("-->[BLE] GATT server ready. (Waiting a client to notify)");
}

void bleLoop(){
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    // not devices, go to suspend cycle
    gotToSuspend();
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    oldDeviceConnected = deviceConnected;
    enableSensor();
    delay(1000);  // waiting for initial capture data
    initTemp();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("==>[DHT22 ESP32]<==");
  // GPIO setup
  pinMode(GPIO_LED_GREEN, OUTPUT);
  digitalWrite (GPIO_LED_GREEN, HIGH);
  pinMode(GPIO_SENSOR_ENABLE, OUTPUT);

  // waiting 1s for connections
  bleServerInit(); 
  delay(1000);

  // if any device, print sensor data via serial and go to suspend mode;  
  if(!deviceConnected){
    gotToSuspend();
  }
}

void loop() {
  bleLoop();
}
