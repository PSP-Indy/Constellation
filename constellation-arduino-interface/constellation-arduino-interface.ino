#include <SPI.h>
#include <LoRa.h>
#include <time.h>
#include <cstdint>

#define RELAY_PIN 24
#define CONN_PIN 23

bool successful_connection = false;
bool rocket_primed = false;
bool launched = false;
bool launch_started = false;
bool fuse_off = false;
bool waitingOnLoraHandling = false;

int32_t fuse_time;
int32_t message_size;

unsigned long last_successful_ping = 0;
unsigned long previous = 0;
unsigned long turn_off_fuse_time = 0;

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

float CharStringToFloat(const char* charString, int idx) 
{
  float cpy_flt;
  memcpy(&cpy_flt, charString, 4);
  return cpy_flt;
}


void setup() {
  Serial.begin(9600);
  while (!Serial);
  identificationPacket[4] = (char)0x04;
  Serial.print(identificationPacket);  
  LoRa.begin(915E6);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONN_PIN, OUTPUT);
  pinMode(8, OUTPUT);
}

void loop() {
  //Run when serial data is available
  if (Serial.available() == 9)
  {
    handleComputerSerialData();
  }

  //Send connection confirmation check
  if ((millis() - previous >= 1000) && !(Serial.available() > 0))
  {
    previous = millis();
    sendMessage("C_SC", {});
  }

  //Check for LoRa Packets and handle them if one is recieved
  int packetSize = LoRa.parsePacket();

  if (waitingOnLoraHandling) 
  {
    handleLoRaPacket(packetSize);
  }
  else
  {
    checkForLoRaPacket(packetSize);
  }
  
  //Self explanatory
  turnOffFuseIfExpected();
  
  //Turn off connection successfulness after 5 seconds of silence
  if ((millis() - last_successful_ping) >= 5000)
  {
    successful_connection = false;
  }
  else
  {
    successful_connection = true;
  }

  digitalWrite(CONN_PIN, !successful_connection);
}

void handleComputerSerialData()
{
  char command_buffer[9];
  Serial.readBytes(command_buffer, 9);
  String header;
  int32_t message_size;    
  for (int32_t i = 0; i < 4; i++)
  {
    header += command_buffer[i];
  }
  memcpy(&message_size, command_buffer + 4, 4);

  //Respond to connection check requests
  if (header == "C_SS") 
  {
    last_successful_ping = millis();
  }

  //Launch the rocket
  if (header == "C_LR")
  {
    if (rocket_primed && successful_connection)
    {
      LaunchRocket();
    }
  }

  //This is ok to be, and should be, blocking because this is during initialization. Theoretically should only run once at a commanded time.
  //Should this lead to issues, this should be turned into a flag based system, like the upstream telemetry handler.
  if (header == "C_ST") 
  {
    while (Serial.available() != message_size) {}

    unsigned char* initialize_data_packet = new unsigned char[message_size];

    Serial.readBytes(initialize_data_packet, message_size);

    PrimeRocket(initialize_data_packet);
    rocket_primed = true;
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
  }
}

void handleLoRaPacket(int packetSize)
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

void checkForLoRaPacket(int packetSize)
{
  if (packetSize == 4) 
  {
    char message_size_string[4];
    for (int i = 0; i < 4; i++)
    {
      message_size_string[i] = (char)LoRa.read();
    }
    memcpy(&message_size, message_size_string, 4);

    waitingOnLoraHandling = true;
  }
}

void turnOffFuseIfExpected()
{
  if (launched)
  {
    if (millis() >= turn_off_fuse_time && !fuse_off) {
      delay(100);
      digitalWrite(RELAY_PIN, LOW);
      fuse_off = true;
      sendMessage("C_FO", {});
    }
  }
}

void sendMessage(String header, String message)
{
  char header_packet[9];
  char message_size[4];

  char header_value[5];
  strcpy(header_value, header.c_str());

  memcpy(header_packet, header_value, 4);

  int message_length = message.length();
  memcpy(header_packet + 4, &message_length, 4);

  Serial.print(header);
  Serial.print(message);
}

void LaunchRocket()
{
  launch_started = true;
  Serial.flush();

  delay(5000);
  
  turn_off_fuse_time = millis() + (fuse_time * 1000);

  digitalWrite(RELAY_PIN, HIGH);
  sendMessage("C_FI", {});
  launched = true;
}

void PrimeRocket(const unsigned char* initialize_data_packet) {
  delay(100);
  memcpy(&fuse_time, initialize_data_packet, 4);

  LoRa.beginPacket();
  LoRa.write(initialize_data_packet, sizeof(initialize_data_packet));
  LoRa.endPacket();

  sendMessage("C_TS", {});
}