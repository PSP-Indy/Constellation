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

unsigned long last_successful_ping = 0;
unsigned long previous = 0;
unsigned long turn_off_fuse_time = 0;

char lora_recieved_packet[36];

void setup() {
  Serial.begin(9600);
  while (!Serial);  
  LoRa.begin(915E6);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONN_PIN, OUTPUT);
  pinMode(8, OUTPUT);

}

void loop() {
  if (Serial.available() == 5)
  {
    char command_buffer[5];
    Serial.readBytes(command_buffer, 5);
    String command = String(command_buffer);

    if (command == "C_SS") 
    {
      last_successful_ping = millis();
      successful_connection = true;
    }

    if (command == "C_LR")
    {
      if (rocket_primed && successful_connection)
      {
        LaunchRocket();
      }
    }

    if (command == "C_ST") 
    {
      while (Serial.available() != 32) {}

      char initialize_data_packet[32];

      Serial.readBytes(initialize_data_packet, 32);

      PrimeRocket(initialize_data_packet);
      rocket_primed = true;
    }
  }

  if ((millis() - previous >= 1000) && !(Serial.available() > 0))
  {
    previous = millis();
    strcpy(header, "C_SC");
    Serial.print(header);
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
      delay(100);
      digitalWrite(RELAY_PIN, LOW);
      fuse_off = true;
      strcpy(header, "C_FO");
      Serial.print(header);
    }
  }

  if ((millis() - last_successful_ping) >= 5000)
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
  delay(100);
  memcpy(&fuse_time, initialize_data_packet, 4);

  // LoRa.beginPacket();

  uint8_t packet_in_bytes[32];
  memcpy(packet_in_bytes, initialize_data_packet, 32); 
  // LoRa.write(packet_in_bytes, 32);
  // LoRa.endPacket();

  strcpy(header, "C_TS");
  Serial.print(header);
}