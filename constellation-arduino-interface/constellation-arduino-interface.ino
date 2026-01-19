#include <SPI.h>
#include <LoRa.h>
#include <time.h>
#include <stdint.h>
#include <LiquidCrystal.h>

#define ARDUINO_BUILD 1

#if ARDUINO_BUILD == 0
#include <avr/wdt.h>
#else
#include <hardware/watchdog.h>
#endif

#define RELAY_PIN 12
#define CONN_PIN 13

bool successful_connection = false;
bool rocket_primed = false;
bool launched = false;
bool fuse_off = true;
bool waitingOnLoraHandling = false;
bool wdt_enabled = false;

int32_t fuse_time;

int32_t expectedNextMessageSize;

unsigned long last_successful_ping = 0;
unsigned long previous = 0;
unsigned long turn_off_fuse_time = 0;
unsigned long launch_time = 0;

char message_packet[48];
char identificationPacket[32];

unsigned long loraHandlingTestingLastTime = 0;
int32_t loraHandlingTestingAverageTime = 0;
int32_t loraHandlingTestingTotalN = 0;
int32_t loraHandlingTestingLongestTime = 0;
int32_t loraHandlingTestingShortestTime = 1000000000;

float altitudeTestingStartZ;
int32_t altitudeTestingTotalN;
float altitudeTestingAccAvg;
float positionTestingStartX;
float positionTestingStartY;

float currentX;
float currentY;
float currentZ;

String activeTestingMode = "T_NA";

LiquidCrystal textDisplay(12, 11, 5, 4, 3, 2);

float CharStringToFloat(const char* charString, int idx) 
{
  float cpy_flt;
  memcpy(&cpy_flt, charString, 4);
  return cpy_flt;
}


void setup() {
  watchdogDisable();
  Serial.begin(115200);
  while (!Serial);

  if (!LoRa.begin(915E6)) {
    Serial.write("FAILED LORA");
    while (true);
  }

  textDisplay.begin(16,2);
  textDisplay.setCursor(0, 0);
  textDisplay.print("RSSI (dBm)")

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONN_PIN, OUTPUT);
}

void loop() {
  //Run when serial data is available
  if (Serial.available())
  {
    handleComputerSerialData();
  }

  //Send connection confirmation check
  if ((millis() - previous >= 1000) && !(Serial.available() > 0))
  {
    previous = millis();
    sendMessage("C_SC", { });
  }

  //Check for LoRa Packets and handle them if one is recieved
  int packetSize = LoRa.parsePacket();

  if (waitingOnLoraHandling) 
  {
    handleLoRaPacket(packetSize, expectedNextMessageSize);
  }
  else
  {
    expectedNextMessageSize = checkForLoRaPacket(packetSize);
  }
  
  //Self explanatory
  TurnOffFuseIfExpected();

  //Self explanatory
  LaunchRocketIfExpected();
  
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
  if (successful_connection && !wdt_enabled) {
    watchdogEnable();
    wdt_enabled = true;
  }

  //Signal strength for direction tuning
  int loraStrength = LoRa.rssi();
  textDisplay.setCursor(0, 1);
  textDisplay.print(loraStrength);

  //DEBUGGING PINS
  digitalWrite(CONN_PIN, successful_connection);
  digitalWrite(RELAY_PIN, !fuse_off);
}

void handleComputerSerialData()s
{
  char command_buffer[5];
  while (Serial.available() != 4) {}
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
    return;
  }

  //Launch the rocket
  if (header == "C_LR")
  {
    if (rocket_primed && successful_connection)
    {
      StartRocketLaunch();
    }
    return;
  }

  //This is ok to be, and should be, blocking because this is during initialization. Theoretically should only run once at a commanded time.
  //Should this lead to issues, this should be turned into a flag based system, like the upstream telemetry handler.
  if (header == "C_ST") 
  {
    while (Serial.available() != 4) {}

    int32_t message_size;
    Serial.readBytes(reinterpret_cast<char*>(&message_size), 4);

    unsigned char* initialize_data_packet = new unsigned char[message_size];

    Serial.readBytes(initialize_data_packet, message_size);

    PrimeRocket(initialize_data_packet);
    rocket_primed = true;
    return;
  }

  //Handle Testing Declaration Cases
  if (header.charAt(0) == 'T')
  {
    activeTestingMode = header;

    char testingModeCharArray[5];
    header.toCharArray(testingModeCharArray, 4);
    unsigned char* testingModeCharArrayData = (unsigned char*)testingModeCharArray;
    LoRa.beginPacket();
    LoRa.write(testingModeCharArrayData, sizeof(testingModeCharArrayData));
    LoRa.endPacket();

    loraHandlingTestingLastTime = millis();
    loraHandlingTestingTotalN = 0;
    loraHandlingTestingLongestTime = 0;
    loraHandlingTestingShortestTime = 1000000000;
    loraHandlingTestingAverageTime = 0;

    altitudeTestingStartZ = currentZ;
    altitudeTestingTotalN = 0;
    altitudeTestingAccAvg = 0;
    positionTestingStartX = currentX;
    positionTestingStartY = currentY;
    return;
  }
}

void handleLoRaPacket(int packetSize, uint32_t message_size)
{
  if (packetSize == message_size)
  {
    char* serial_send = new char[message_size + 1];
    for(int i = 0; i < sizeof(serial_send); i++) {
      serial_send[i] = (char)LoRa.read();
    }

    String serialSendString = serial_send;
    sendMessage("C_UT", serialSendString);
    handleTestingData(serialSendString);
    
    waitingOnLoraHandling = false;
  }
}

void handleTestingData(String serialData) {

  currentX = CharStringToFloat(serialData.c_str(), 12);
  currentY = CharStringToFloat(serialData.c_str(), 16);
  currentZ = CharStringToFloat(serialData.c_str(), 20);
  float currentA = CharStringToFloat(serialData.c_str(), 4);

  if (activeTestingMode == "T_1B" || activeTestingMode == "T_1P" || activeTestingMode == "T_2P") 
  {
    loraHandlingTestingTotalN += 1;
    int timeSinceLast = loraHandlingTestingLastTime - millis();
    loraHandlingTestingLastTime = millis();

    loraHandlingTestingAverageTime = loraHandlingTestingAverageTime + ((timeSinceLast - loraHandlingTestingAverageTime) / loraHandlingTestingTotalN);
    if (timeSinceLast < loraHandlingTestingShortestTime) loraHandlingTestingShortestTime = timeSinceLast;
    if (timeSinceLast > loraHandlingTestingLongestTime) loraHandlingTestingLongestTime = timeSinceLast;

    String serial_send = "";

    if (activeTestingMode == "T_1B")
    {
      serial_send += "Time Since Last Byte = ";
      serial_send += timeSinceLast;
      serial_send += "\nAverage Time Between Bytes = ";
      serial_send += loraHandlingTestingAverageTime;
      serial_send += "\nLongest Time Between Bytes = ";
      serial_send += loraHandlingTestingLongestTime;
      serial_send += "\nShortest Time Between Bytes = ";
      serial_send += loraHandlingTestingShortestTime;
      serial_send += "\nAverage Bytes/s = ";
      serial_send += 1000 / loraHandlingTestingAverageTime;
    }
    else if (activeTestingMode == "T_1P")
    {
      serial_send += "Time Since Last Packet = ";
      serial_send += timeSinceLast;
      serial_send += "\nAverage Time Between Packets = ";
      serial_send += loraHandlingTestingAverageTime;
      serial_send += "\nLongest Time Between Packets = ";
      serial_send += loraHandlingTestingLongestTime;
      serial_send += "\nShortest Time Between Packets = ";
      serial_send += loraHandlingTestingShortestTime;
      serial_send += "\nAverage Packets/s = ";
      serial_send += 1000 / loraHandlingTestingAverageTime;
    }
    else if (activeTestingMode == "T_2P")
    {
      serial_send += "Time Since Last Packet = ";
      serial_send += timeSinceLast;
      serial_send += "\nAverage Time Between Packets = ";
      serial_send += loraHandlingTestingAverageTime;
      serial_send += "\nLongest Time Between Packets = ";
      serial_send += loraHandlingTestingLongestTime;
      serial_send += "\nShortest Time Between Packets = ";
      serial_send += loraHandlingTestingShortestTime;
      serial_send += "\nAverage Packets/s = ";
      serial_send += 1000 / loraHandlingTestingAverageTime;

      String responseString = "T_2P_RES";
      const unsigned char* response = reinterpret_cast<const unsigned char*>(responseString.c_str());
      LoRa.beginPacket();
      LoRa.write(response, sizeof(response));
      LoRa.endPacket();
    }

    sendMessage("T_DP", serial_send); 
  }
  else if (activeTestingMode == "T_AA") 
  {
    altitudeTestingTotalN += 1;
    float dz = currentZ - altitudeTestingStartZ;

    altitudeTestingAccAvg = altitudeTestingAccAvg + ((dz - altitudeTestingAccAvg) / altitudeTestingTotalN);

    String serial_send = "";

    serial_send += "Dz = ";
    serial_send += dz;
    serial_send += "\nAverage a = ";
    serial_send += altitudeTestingAccAvg;

    sendMessage("T_DP", serial_send); 
  }
  else if (activeTestingMode == "T_PA") 
  {
    float dx = currentX - positionTestingStartX;
    float dy = currentY - positionTestingStartY;

    String serial_send = "";

    serial_send += "Dx = ";
    serial_send += dx;
    serial_send += "\nDy = ";
    serial_send += dy;

    sendMessage("T_DP", serial_send); 
  }
}

int32_t checkForLoRaPacket(int packetSize)
{
  if (packetSize == 4) 
  {
    int32_t message_size;
    char message_size_string[4];
    for (int i = 0; i < 4; i++)
    {
      message_size_string[i] = (char)LoRa.read();
    }
    memcpy(&message_size, message_size_string, 4);

    waitingOnLoraHandling = true;
    return message_size;
  }
  return 0;
}

void TurnOffFuseIfExpected()
{
  if (launched)
  {
    if (millis() >= turn_off_fuse_time && !fuse_off) {
      delay(100);
      digitalWrite(RELAY_PIN, LOW);
      fuse_off = true;
      sendMessage("C_FO", { });
    }
  }
}

void sendMessage(String header, String message)
{
  uint8_t packet[8];

  memcpy(packet, header.c_str(), 4);

  uint32_t message_length = message.length();
  memcpy(packet + 4, &message_length, sizeof(uint32_t));

  Serial.write(packet, 8);
  Serial.write(message.c_str(), message_length);
}

void StartRocketLaunch()
{
  Serial.flush();
  launch_time = millis() + 5000;
}

void LaunchRocketIfExpected()
{
  if (launch_time != 0 && millis() >= launch_time && readyForLaunch()) 
  {
    launch_time = 0;
    fuse_off = false;
    turn_off_fuse_time = millis() + (fuse_time * 1000);

    sendMessage("C_FI", {});
    launched = true;
  }
}

void PrimeRocket(const unsigned char* initialize_data_packet) {
  delay(100);
  memcpy(&fuse_time, initialize_data_packet, 4);

  LoRa.beginPacket();
  LoRa.write(initialize_data_packet, sizeof(initialize_data_packet));
  LoRa.endPacket();

  sendMessage("C_TS", {});
}

bool readyForLaunch()
{
  bool ready = true;

  ready &= rocket_primed;
  ready &= wdt_enabled;
  ready &= successful_connection;
  ready &= fuse_off;
  ready &= (fuse_time > 0);

  return ready;
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