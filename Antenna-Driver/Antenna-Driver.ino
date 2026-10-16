#include <LoRa.h>
#include <WiFi.h>
#include <SPI.h>
#include <SimplePgSQL.h>

#define PACKET_SIZE 36
#define PACKET_BUFFER_SIZE 12

#define LORA_CS 5                       // physical pin 7
#define LORA_G0 1                       // physical pin 2
#define LORA_RST 6                      // physical pin 9
#define MISO 16                         // physical pin 21
#define MOSI 19                         // physical pin 23
#define CLK 18                          // physical pin 24

const char ssid[] = "network_ssid";     // your network SSID (name)
const char pass[] = "network_pass";     // your network password

const char user[] = "constellation";    // your database user
const char password[] = "pswd";         // your database password
const char dbname[] = "flightdata";     // your database name

struct sample {
  float data[9];
  float rssi;
};

static PROGMEM const char insert_query[] = "INSERT INTO logs (run_id, timestamp, a_value, v_value, x_value, y_value,\
                z_value, x_rot_value, y_rot_value, z_rot_value, rssi)\
                VALUES (%d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\
                ON CONFLICT (run_id, timestamp)\
                DO UPDATE SET\
                a_value = EXCLUDED.a_value, v_value = EXCLUDED.v_value, x_value = EXCLUDED.x_value,\
                y_value = EXCLUDED.y_value, z_value = EXCLUDED.z_value, x_rot_value = EXCLUDED.x_rot_value,\
                y_rot_value = EXCLUDED.y_rot_value, z_rot_value = EXCLUDED.z_rot_value, rssi = EXCLUDED.rssi\
                WHERE EXCLUDED.rssi > logs.rssi;";

static PROGMEM const char run_id_query[] = "SELECT run_id FROM runs ORDER BY run_id DESC LIMIT 1;";

int run_id = -1;
char buffer[1024];
char query_buffer[4096];
uint8_t message[36];
bool run_id_queried = false;
static sample sample_buffer[PACKET_BUFFER_SIZE];
volatile int buffer_end;
WiFiClient client;

enum PgState { PG_IDLE, PG_WAIT_RUN_ID, PG_WAIT_INSERT };
static PgState pg_state = PG_IDLE;
static uint32_t last_run_query = 0;

PGconnection conn(&client, 0, 1024, buffer);
IPAddress PGIP(192,168,1,5);

void runSQL();
void handleLoRaPacket(int);
float charStringToFloat(const uint8_t*, int);
void runQuery(PGconnection*, const char*, ...);
static void resetPg();
static void appendf(size_t, const char, ...);

void setup() 
{
  WiFi.begin((char *)ssid, pass);

  SPI.setRX(MISO);
  SPI.setTX(MOSI);
  SPI.setSCK(CLK);
  SPI.begin();

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_G0);

  LoRa.begin(915E6);

  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(500E3);

  delay(500);

  LoRa.onReceive(handleLoRaPacket);
  LoRa.receive();
}

void loop() 
{
  switch (conn.status()) {
    case CONNECTION_NEEDED:
      if (WiFi.status() == WL_CONNECTED) {
        conn.setDbLogin(PGIP, user, password, dbname, "utf8");
      }
      break;
    case CONNECTION_OK:
      runSQL();      
      break;
    case CONNECTION_BAD:
      conn.close();
      delay(1000);
      ESP.restart();
      break;
    default:
      break;
  }
}

static void appendf(size_t &bits, const char *format, ...) 
{
  if (bits >= sizeof(query_buffer) - 1) return;

  va_list va_list; 
  va_start(va_list, format);
  int written_bits = vsnprintf(query_buffer + bits, sizeof(query_buffer) - bits, format, va_list);
  va_end(va_list);

  if (written_bits > 0) 
  {
    bits = min(bits + (size_t)written_bits, sizeof(query_buffer) - 1);
  }
}

void runSQL()
{
  if (pg_state != PG_IDLE) 
  {
    int rc = conn.getData();

    if (rc < 0) return;

    if (pg_state == PG_WAIT_RUN_ID && (rc & PG_RSTAT_HAVE_ROW)) 
    {
      const char *v = conn.getValue(0);
      if (v && *v) run_id = atoi(v);
    }

    if (rc & PG_RSTAT_READY)
    {
      pg_state = PG_IDLE;
    }

    return;
  }

  if (run_id < 0) 
  {
    if (millis() - last_run_query < 1000) return;

    last_run_query = millis();
    conn.execute(run_id_query);
    pg_state = PG_WAIT_RUN_ID;

    return;
  }

  if (buffer_end == 0) return;

  sample batch[PACKET_BUFFER_SIZE];
  noInterrupts();
  int count = buffer_end;
  memcpy(batch, sample_buffer, count * sizeof(sample));
  buffer_end = 0;
  interrupts();

  size_t n = 0;
  query_buffer[0] = 0;
  appendf(n, "INSERT INTO logs (run_id, timestamp, a_value, v_value, x_value, y_value, "
             "z_value, x_rot_value, y_rot_value, z_rot_value, rssi) VALUES ");
  
  int rows = 0;
  for (int i = 0; i < count; i++) 
  {
    const sample &s = batch[i];
    appendf(n, "%s(%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f)", rows++ ? "," : "", run_id,
            s.data[0], s.data[1], s.data[2], s.data[3], s.data[4],
            s.data[5], s.data[6], s.data[7], s.data[8], s.rssi);
  }

  appendf(n, " ON CONFLICT (run_id, timestamp) DO UPDATE SET "
             "a_value = EXCLUDED.a_value, v_value = EXCLUDED.v_value, x_value = EXCLUDED.x_value, "
             "y_value = EXCLUDED.y_value, z_value = EXCLUDED.z_value, x_rot_value = EXCLUDED.x_rot_value, "
             "y_rot_value = EXCLUDED.y_rot_value, z_rot_value = EXCLUDED.z_rot_value, rssi = EXCLUDED.rssi "
             "WHERE EXCLUDED.rssi > logs.rssi;");

  conn.execute(query_buffer);
  pg_state = PG_WAIT_INSERT;
}

void handleLoRaPacket(int message_size)
{
  if (message_size != PACKET_SIZE || buffer_end >= PACKET_BUFFER_SIZE) return;

  uint8_t* destination = (uint8_t*)sample_buffer[buffer_end].data;
  for (int i = 0; i < PACKET_SIZE; i++) {
    destination[i] = (uint8_t)LoRa.read();
  }
  sample_buffer[buffer_end].rssi = (float)LoRa.packetRssi();
  buffer_end++;
}

float charStringToFloat(const uint8_t* charString, int idx) 
{
  float cpy_flt;
  memcpy(&cpy_flt, charString + idx, 4);
  return cpy_flt;
}

void runQuery(PGconnection* conn, const char* query, ...)
{
  va_list args;
  va_start(args, query);
  vsnprintf(query_buffer, sizeof(query_buffer), query, args);
  va_end(args);
  conn->execute(query_buffer);
}