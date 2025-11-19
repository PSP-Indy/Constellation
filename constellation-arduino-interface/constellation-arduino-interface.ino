#include <SPI.h>
#include <LoRa.h>
#include <time.h>

#define RELAY_PIN 24
#define CONN_PIN 23

char header[40];

bool successful_connection = false;
bool rocket_primed = false;
bool launched = false;
bool launch_started = false;
bool fuse_off = false;

int fuse_time;

time_t last_successful_ping;

char successful_connecton_packet[4];
char lora_recieved_packet[36];

int turn_off_fuse_time;

int previous = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);  
  LoRa.begin(915E6);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONN_PIN, OUTPUT);
}

void loop() {
  if (Serial.available() == 4)
  {
    String command;
    for (int i = 0; i < 4; i++) 
    {
      command += Serial.read();
    }

    if (command == "C_SS") 
    {
      last_successful_ping = time(NULL);
      successful_connection = true;
    }

    if (command == "C_LR")
    {
      if (rocket_primed && successful_connection)
      {
        LaunchRocket();
      }
    }
  }
  else if (Serial.available() == 32) 
  {
    if (successful_connection)
    {
      char initialize_data_packet[32];

      for(int i = 0; i < 32; i++) {
        initialize_data_packet[i] = Serial.read();
      } 

      PrimeRocket(initialize_data_packet);
      rocket_primed = true;
    }
  } 
  else 
  {
    if (millis() - previous >= 1000)
    {
      previous = millis();
      strcpy(header, "C_SC");
      Serial.print(header);
    }
  }

  if (launched)
  {
    // int packetSize = LoRa.parsePacket();
    // if (packetSize == sizeof(lora_recieved_packet)) {
    //   for(int i = 0; i < sizeof(lora_recieved_packet); i++) {
    //     lora_recieved_packet[i] = (char)LoRa.read();
    //   }
    //   char serial_send[40] = "C_UT";
    //   strcat(serial_send, lora_recieved_packet);
    //   Serial.print(serial_send);
    // }

    if (millis() >= turn_off_fuse_time && !fuse_off) {
      digitalWrite(RELAY_PIN, LOW);
      fuse_off = true;
      strcpy(header, "C_FO");
      Serial.print(header);
    }
  }

  if (abs(difftime(last_successful_ping, time(NULL))) > 5)
  {
    successful_connection = false;
  }

  digitalWrite(CONN_PIN, !successful_connection);  
}

void LaunchRocket()
{
  launch_started = true;
  Serial.flush();

  delay(5000);
  
  turn_off_fuse_time = millis() + (fuse_time * 1000);

  digitalWrite(RELAY_PIN, HIGH);
  strcpy(header, "C_FI");
  Serial.print(header);
  launched = true;
}

void PrimeRocket(const char* initialize_data_packet) {
  memcpy(&fuse_time, initialize_data_packet, 4);

  Serial.flush();
  // LoRa.beginPacket();

  uint8_t packet_in_bytes[32];
  memcpy(packet_in_bytes, initialize_data_packet, 32); 
  // LoRa.write(packet_in_bytes, 32);
  // LoRa.endPacket();

  strcpy(header, "C_TS");
  Serial.print(header);
}