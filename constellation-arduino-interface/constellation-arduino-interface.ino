#include <SPI.h>
#include <LoRa.h>

#define RELAY_PIN 0

void setup() {
  Serial.begin(9600);
  while (!Serial);  

  LoRa.begin(915E6);

  pinMode(RELAY_PIN, OUTPUT);
}

bool launched = false;
bool fuse_off = false;
char initialize_data_packet[32];
char lora_recieved_packet[36];
int turn_off_fuse_time;

void loop() {
  if (Serial.available() == 32 && !launched) {
    for(int i = 0; i < 32; i++) {
      initialize_data_packet[i] = Serial.read();
    } 

    launch_rocket(initialize_data_packet);
  }  

  int packetSize = LoRa.parsePacket();
  if (packetSize == sizeof(lora_recieved_packet)) {
    for(int i = 0; i < sizeof(lora_recieved_packet); i++) {
      lora_recieved_packet[i] = (char)LoRa.read();
    }
    Serial.print(lora_recieved_packet);
  }

  if (millis() >= turn_off_fuse_time && !fuse_off) {
    digitalWrite(RELAY_PIN, LOW);
    fuse_off = true;
  }
  
}

void launch_rocket(const char* initialize_data_packet) {
  LoRa.beginPacket();
  uint8_t packet_in_bytes[32];
  memcpy(packet_in_bytes, initialize_data_packet, 32); 
  LoRa.write(packet_in_bytes, 32);
  LoRa.endPacket();

  int fuse_time;
  memcpy(&fuse_time, initialize_data_packet, 4);
  
  turn_off_fuse_time = millis() + (fuse_time * 1000);

  digitalWrite(RELAY_PIN, HIGH);
}