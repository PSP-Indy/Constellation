#include <LoRa.h>
#include <math.h>
#include <WiFi.h>
#include <SPI.h>
#include <SimplePgSQL.h>
#define FLASH_CS 5                      // physical pin 7
#define LORA_G0 1                       // physical pin 2
#define LORA_RST 6                      // physical pin 9
#define MISO 16                         // physical pin 21
#define MOSI 19                         // physical pin 23
#define CLK 18                          // physical pin 24

IPAddress PGIP(192,168,1,5);            // your PostgreSQL server IP

const char ssid[] = "network_ssid";     // your network SSID (name)
const char pass[] = "network_pass";     // your network password

const char user[] = "constellation";    // your database user
const char password[] = "pswd";         // your database password
const char dbname[] = "flightdata";     // your database name

WiFiClient client;

static PROGMEM const char insert_query[] = "INSERT INTO runs (run_id, a_value, v_value, x_value, y_value,\
                z_value, x_rot_value, y_rot_value, z_rot_value, rssi)\
                VALUES (%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\
                ON CONFLICT (run_id, timestamp)\
                DO UPDATE SET\
                a_value = EXCLUDED.a_value, v_value = EXCLUDED.v_value, x_value = EXCLUDED.x_value,\
                y_value = EXCLUDED.y_value, z_value = EXCLUDED.z_value, x_rot_value = EXCLUDED.x_rot_value,\
                y_rot_value = EXCLUDED.y_rot_value, z_rot_value = EXCLUDED.z_rot_value, rssi = EXCLUDED.rssi\
                WHERE EXCLUDED.rssi > logs.rssi;";

static PROGMEM const char run_id_query[] = "SELECT run_id FROM runs ORDER BY id DESC LIMIT 1;";

static PROGMEM const char create_uuid_query[] = "INSER INTO antennas uuid VALUES (%d);";

static PROGMEM const char update_rssi_query[] = "INSERT INTO your_table (uuid, rssi) VALUES (%d, %f) ON CONFLICT (uuid) DO UPDATE SET rssi = EXCLUDED.rssi;";

int run_id;
int uuid;
char buffer[1024];
char query[1024];
uint8_t* message[36];
PGconnection conn(&client, 0, 1024, buffer);

void setup() 
{
  WiFi.begin((char *)ssid, pass);

  SPI.setRX(MISO);
  SPI.setTX(MOSI);
  SPI.setSCK(CLK);
  SPI.begin();

  delay(500);

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_G0);

  LoRa.begin(915E6)

  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(500E3);

  delay(500);

  conn.setDbLogin(PGIP, user, password, dbname, "utf8");

  randomSeed(analogRead(A0)); 
  uuid = random(0, 2147483647); 

  conn.execute(run_id_query);
  run_id = String(conn.getValue(0)).toInt();
  conn.clearStatus(); 

  snprintf(query, sizeof(query), create_uuid_query, uuid, 0.0f);
  conn.execture(query);

  LoRa.onReceive(handleLoRaPacket);
}

void loop() 
{
  delay(50);
}

void handleLoRaPacket()
{
  int message_size = LoRa.parsePacket();

  if (message_size != 36) return;

  for(int i = 0; i < message_size; i++) {
    message[i] = (uint8_t)LoRa.read();
  }

  float time = StringToFloat(message, 0);
  float a_value = StringToFloat(message, 4);
  float v_value = StringToFloat(message, 8);
  float x_value = StringToFloat(message, 12);
  float y_value = StringToFloat(message, 16);
  float z_value = StringToFloat(message, 20);
  float x_rot_value = StringToFloat(message, 24);
  float y_rot_value = StringToFloat(message, 28);
  float z_rot_value = StringToFloat(message, 32);
  float rssi = LoRa.packetRssi();

  snprintf(query, sizeof(query), insert_query, run_id, time, a_value, v_value, x_value, y_value, z_value, x_rot_value, y_rot_value, z_rot_value, rssi);
  conn.execture(query);

  snprintf(query, sizeof(query), update_rssi_query, uuid, rssi);
  conn.execture(query);
}
