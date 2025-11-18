#include <SPI.h>
#include <LoRa.h>

#define RELAY_PIN 24
#define CONN_PIN 23

char header[40];

bool successful_connection = false;
bool launched = false;
bool launch_started = false;
bool fuse_off = false;

char successful_connecton_packet[4];
char initialize_data_packet[32];
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
  if (!successful_connection) {
    digitalWrite(CONN_PIN, HIGH);
    if (Serial.available()) {
      Serial.readBytes(successful_connecton_packet, 4);

      if (successful_connecton_packet[0] == 'C' && successful_connecton_packet[1] == '_' && successful_connecton_packet[2] == 'S' && successful_connecton_packet[3] == 'S')
      {
        successful_connection = true;
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
  }
  else 
  {
    digitalWrite(CONN_PIN, LOW);
  }

  if (Serial.available() == 32 && !launch_started && successful_connection) {
    for(int i = 0; i < 32; i++) {
      initialize_data_packet[i] = Serial.read();
    } 

    launch_rocket(initialize_data_packet);
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
}

void launch_rocket(const char* initialize_data_packet) {
  launch_started = true;
  // LoRa.beginPacket();

  uint8_t packet_in_bytes[32];
  memcpy(packet_in_bytes, initialize_data_packet, 32); 
  // LoRa.write(packet_in_bytes, 32);
  // LoRa.endPacket();

  strcpy(header, "C_TS");
  Serial.print(header);

  delay(5000);

  int fuse_time;
  memcpy(&fuse_time, initialize_data_packet, 4);
  
  turn_off_fuse_time = millis() + (fuse_time * 1000);

  digitalWrite(RELAY_PIN, HIGH);
  strcpy(header, "C_FI");
  Serial.print(header);
  launched = true;
}