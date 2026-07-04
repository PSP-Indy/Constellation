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
bool wdt_enabled = false;
bool lora_connected = false;
bool first_connection_ping = true;

unsigned long last_successful_ping = 0;
unsigned long previous = 0;

float charStringToFloat(uint8_t*, int);
void handleLoraPacket(int);
bool sendLoraPacket(uint8_t*, size_t);
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
    int packet_size = LoRa.parsePacket();

    if (packet_size > 0)
    {
      handleLoRaPacket(packet_size);
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

    if (sendLoraPacket(data_packet, 12))
    {
      sendMessage("C_TS", {});
    }

    return;
  }
}

void handleLoRaPacket(int message_size)
{
  uint8_t* serial_send = new uint8_t[message_size + 1];
  for(int i = 0; i < message_size; i++) {
    serial_send[i] = (uint8_t)LoRa.read();
  }

  String serialSendString(serial_send, message_size);
  sendMessage("C_UT", serialSendString);

  delete[] serial_send;
}

float charStringToFloat(const char* charString, int idx) 
{
  float cpy_flt;
  memcpy(&cpy_flt, charString + idx, 4);
  return cpy_flt;
}

bool sendLoraPacket(uint8_t* packet, size_t size)
{
  int success = LoRa.beginPacket(false);
  LoRa.write(packet, size);
  success += LoRa.endPacket();
  return success == 2;
}

void sendMessage(String header, String message)
{
  Serial.write(header.c_str(), 4);
  Serial.write(message.c_str(), message.length());
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