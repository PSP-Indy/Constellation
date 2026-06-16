#include <SPI.h>
#include <LoRa.h>
#include <time.h>
#include <stdint.h>

#define LORA_WAIT_TIMEOUT_MS 500
#define ARDUINO_BUILD 1

#if ARDUINO_BUILD == 0
#include <avr/wdt.h>
#else
#include <hardware/watchdog.h>
#endif

#define RELAY_PIN 28

bool successful_connection = false;
bool waiting_on_lora_handling = false;
bool wdt_enabled = false;
bool lora_connected = false;
bool first_connection_ping = true;

int16_t expected_next_message_size;

unsigned long last_successful_ping = 0;
unsigned long previous = 0;
unsigned long lora_wait_start_time = 0;

float charStringToFloat(const char*, int);
int16_t checkForLoRaPacket(int);
bool sendLoraPacket(unsigned char*, size_t)
void sendMessage(String, String);
void watchdogEnable();
void watchdogDisable();
void watchdogReset();

void setup() {
  watchdogDisable();
  Serial.begin(115200);

  LoRa.setPins(7, 6, 1);
  if (LoRa.begin(915E6)) {
    lora_connected = true;
  }
  else lora_connected = false;

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  //Run when serial data is available
  if (Serial.available() >= 4)
  {
    handleComputerSerialData();
  }

  //Send connection confirmation check
  if ((millis() - previous >= 1000))
  {
    if (!lora_connected && !first_connection_ping && LoRa.begin(915E6)) {
      lora_connected = true;
      sendMessage("C_SC", "C_LC");
    }
    else
    {
      if (lora_connected && first_connection_ping) 
      {
        sendMessage("C_SC", "C_LC");
      }
      else
      {
        sendMessage("C_SC", {});
      }
    }
    
    previous = millis();
    
  }

  //Check for LoRa Packets and handle them if one is recieved
  if (lora_connected && !first_connection_ping)
  {
    int packetSize = LoRa.parsePacket();

    if (waiting_on_lora_handling) 
    {
      handleLoRaPacket(packetSize, expected_next_message_size);
    }
    else
    {
      expected_next_message_size = checkForLoRaPacket(packetSize);
    }
  }
  
  //Turn off connection successfulness after 5 seconds of silence
  if ((millis() - last_successful_ping) >= 5000)
  {
    successful_connection = false;
    wdt_enabled = false;
  }
  else
  {
    successful_connection = true;
  }

  //Set up watchdog when connection is scuccessful
  if (successful_connection && !wdt_enabled && millis() > 6000) {
    //watchdogEnable();
    wdt_enabled = true;
  }

  digitalWrite(LED_BUILTIN, lora_connected);  
}

void handleComputerSerialData()
{
  uint8_t command_buffer[5];

  Serial.readBytes(command_buffer, 4);
  String header;

  for (int32_t i = 0; i < 4; i++)
  {
    header += command_buffer[i];
  }

  //Respond to connection check requests
  if (header == "C_SS") 
  {
    watchdogReset();
    last_successful_ping = millis();

    if (first_connection_ping)
    {
      first_connection_ping = false;
    }

    return;
  }

  if (header == "C_SD") 
  {
    while (Serial.available() < 12);

    uint8_t data_packet[12];

    Serial.readBytes(data_packet, 12);

    if (sendLoraPacket(data_packet))
    {
      sendMessage("C_TS", {});
    }

    return;
  }
}

void handleLoRaPacket(int packetSize, uint16_t message_size)
{
  if (millis() - lora_wait_start_time > LORA_WAIT_TIMEOUT_MS) {
    while (LoRa.available()) {
      LoRa.read();
    }
    waiting_on_lora_handling = false;
    return;
  }
  
  if (packetSize == message_size && packetSize > 0)
  {
    char* serial_send = new char[message_size + 1];
    for(int i = 0; i < message_size; i++) {
      serial_send[i] = (char)LoRa.read();
    }

    String serialSendString(serial_send, message_size);
    delete[] serial_send;
    sendMessage("C_UT", serialSendString);
    handleTestingData(serialSendString);
  }
  
  waiting_on_lora_handling = false;
}

float charStringToFloat(const char* charString, int idx) 
{
  float cpy_flt;
  memcpy(&cpy_flt, charString + idx, 4);
  return cpy_flt;
}

int16_t checkForLoRaPacket(int packetSize)
{
  waiting_on_lora_handling = true;
  lora_wait_start_time = millis();

  if (packetSize == 2) 
  {
    int16_t message_size;
    uint8_t message_size_string[2];
    for (int i = 0; i < 2; i++)
    {
      message_size_string[i] = LoRa.read();
    }
    memcpy(&message_size, message_size_string, 2);

    waiting_on_lora_handling = true;
    return message_size;
  }
  return 0;
}

bool sendLoraPacket(unsigned char* packet, size_t size)
{
  int success = LoRa.beginPacket();
  LoRa.write(packet, size);
  success += LoRa.endPacket();
  return success == 2;
}

void sendMessage(String header, String message)
{
  uint8_t packet[6];

  memcpy(packet, header.c_str(), 4);

  uint16_t message_length = message.length();
  memcpy(packet + 4, &message_length, sizeof(uint16_t));

  Serial.write(packet, 6);
  Serial.write(message.c_str(), message_length);
}

void watchdogEnable() 
{
  #if ARDUINO_BUILD == 0
  wdt_enable(WDTO_4S);
  #else
  watchdog_enable(4000, 1);
  #endif
}

void watchdogDisable() 
{
  #if ARDUINO_BUILD == 0
  wdt_disable();
  #else
  watchdog_disable();
  #endif
}

void watchdogReset() 
{
  #if ARDUINO_BUILD == 0
  wdt_reset();
  #else
  watchdog_update();
  #endif
}