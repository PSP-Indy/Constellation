#include <SPI.h>
#include <LoRa.h>
#include <time.h>

#define RELAY_PIN 24
#define CONN_PIN 23

bool successful_connection = false;
bool rocket_primed = false;
bool launched = false;
bool launch_started = false;
bool fuse_off = false;
bool waiting_for_lora_packet = false;

int fuse_time;
int message_size;

unsigned long last_successful_ping = 0;
unsigned long previous = 0;
unsigned long turn_off_fuse_time = 0;

char message_packet[48];

void setup() {
  Serial.begin(9600);
  while (!Serial);  
  LoRa.begin(915E6);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONN_PIN, OUTPUT);
  pinMode(8, OUTPUT);
}

void loop() {
  //Run when serial data is 
  if (Serial.available() == 9)
  {
    handleDownstreamData();
  }

  //Send connection confirmation check
  if ((millis() - previous >= 1000) && !(Serial.available() > 0))
  {
    previous = millis();
    sendMesage("C_SC", {});
  }

  //Check for LoRa Packets and handle them if one is recieved
  int packetSize = LoRa.parsePacket();

  if (waiting_for_lora_packet) 
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

void handleDownstreamData()
{
  char command_buffer[9];
  Serial.readBytes(command_buffer, 9);
  String header;
  int message_size;    
  for (int i = 0; i < 4; i++)
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
}

void handleLoRaPacket(int packetSize)
{
  if (packetSize == message_size)
  {
    char* serial_send = new char[message_size + 1];
    for(int i = 0; i < sizeof(serial_send); i++) {
      serial_send[i] = (char)LoRa.read();
    }

    sendMesage("C_UT", serial_send);
    waiting_for_lora_packet = false;
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

    waiting_for_lora_packet = true;
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
      sendMesage("C_FO", {});
    }
  }
}

void sendMesage(String header, String message)
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
  sendMesage("C_FI", {});
  launched = true;
}

void PrimeRocket(const unsigned char* initialize_data_packet) {
  delay(100);
  memcpy(&fuse_time, initialize_data_packet, 4);

  LoRa.beginPacket();
  LoRa.write(initialize_data_packet, sizeof(initialize_data_packet));
  LoRa.endPacket();

  sendMesage("C_TS", {});
}