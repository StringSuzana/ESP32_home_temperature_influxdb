#include <WiFi.h>
#include <WebServer.h>

#include "SFE_BMP180.h"
#include "Wire.h"

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// DEFINES
////INFLUX DB
#define INFLUXDB_URL "https://europe-west1-1.gcp.cloud2.influxdata.com"                                           // InfluxDB v2 server url (InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_TOKEN "INFLUXDB_TOKEN" // 
#define INFLUXDB_ORG "INFLUXDB_ORG@gmail.com"                                                            // InfluxDB v2 organization id ( InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_BUCKET "INFLUXDB_BUCKET"                                                                          // InfluxDB v2 bucket name ( InfluxDB UI ->  Data -> Buckets)
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define DEVICE "ESP32"

// VARIABLES
////WEB
const char *ssid = "SSID";
const char *password = "pass";
WebServer server(80);

////LED
uint8_t LED1pin = 23;
bool LED1status = LOW;

////SENSOR BMP180
SFE_BMP180 bmp180_sensor;
double air_pressure, air_temperature;

////INFLUXDB

InfluxDBClient influxdb_client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert); // InfluxDB client instance with preconfigured InfluxCloud certificate

// Data point
Point wifi_status_point("wifi_status");
Point weather_point("weather");

// FUNCTION PROTOTYPES
void handle_OnConnect();
void handle_led1on();
void handle_led1off();
void handle_NotFound();

double read_pressure();
double read_temperature();
void refresh_measurements();
void write_weather_to_influxdb();

String SendHTML(uint8_t led1stat);

void setup()
{
  Serial.begin(9600);
  delay(100);
  pinMode(LED1pin, OUTPUT);

  Serial.println("Connecting to " + String(ssid));
  WiFi.begin(ssid, password);

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  // server callbacks
  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.onNotFound(handle_NotFound);
  // start server
  server.begin();
  Serial.println("HTTP server started");

  // init sensor
  if (bmp180_sensor.begin())
    Serial.println("BMP180 connected.");
  else
    Serial.println("BMP180 not found.");

  // init influxdb
  wifi_status_point.addTag("device", DEVICE); // Add tags
  wifi_status_point.addTag("SSID", WiFi.SSID());

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov"); // Accurate time is necessary for certificate validation and writing in batches
                                                     // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
                                                     // Syncing progress and the time will be printed to Serial.

  if (influxdb_client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influxdb_client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influxdb_client.getLastErrorMessage());
  }
}
void loop()
{
  server.handleClient();
  if (LED1status)
  {
    digitalWrite(LED1pin, HIGH);
  }
  else
  {
    digitalWrite(LED1pin, LOW);
  }

  // sensor
  refresh_measurements();
  delay(100);
  
  //influxdb
  write_weather_to_influxdb();
  Serial.println("Wait 10s");
  delay(10000);
}
void refresh_measurements()
{
  air_pressure = read_pressure();
  Serial.println("Tlak zraka = " + String(air_pressure) + "hPa");

  air_temperature = read_temperature();
  Serial.print("Temp zraka = " + String(air_temperature) + "C");
  /*if (bmp180_sensor.getPressure(air_pressure, air_temperature))
  {
    Serial.print("Pressure: ");
    Serial.print(air_pressure);
    Serial.println(" hPa");
  }
  else
  {
    Serial.println("Failed to read pressure.");
  }

  if (bmp180_sensor.getTemperature(air_temperature))
  {
    Serial.print("Temperature: ");
    Serial.print(air_temperature);
    Serial.println(" C");
  }
  else
  {
    Serial.println("Failed to read temperature.");
  }*/
}
void write_weather_to_influxdb()
{
  // Use a point and add fields
  weather_point.addField("pressure", air_pressure);
  weather_point.addField("temperature", air_temperature);

  // Print what are we exactly writing
  Serial.println("Writing: ");
  Serial.println(weather_point.toLineProtocol());

  // Write point
  if (!influxdb_client.writePoint(weather_point))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(influxdb_client.getLastErrorMessage());
  }
}
void handle_OnConnect()
{
  LED1status = LOW;
  Serial.println("GPIO34 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status));
}

void handle_led1on()
{
  LED1status = HIGH;
  Serial.println("GPIO34 Status: ON");
  server.send(200, "text/html", SendHTML(true));
}

void handle_led1off()
{
  LED1status = LOW;
  Serial.println("GPIO34 Status: OFF");
  server.send(200, "text/html", SendHTML(false));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

String SendHTML(uint8_t led1stat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Home weather station</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 Weather station </h1>\n";
  ptr += "<h3>Using Station(STA) Mode</h3>\n";

  ptr += "<p>Temperature: ";
  ptr += air_temperature;

  ptr += char(176);
  ptr += "C</p>";
  ptr += "<p>Pressure: ";
  ptr += air_pressure;
  ptr += "hPa</p>";

  if (led1stat)
  {
    ptr += "<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
double read_pressure()
{
  char status;
  /*
   *  definiramo varijable:
   *  temp - air temperature
   *  Tlak - air pressure
   *  tlak0 - pressure on the sea level
   *  nadVisina - height above sea level
   */
  double temp, pressure, pressure_at_sea_level, height_asl;

  status = bmp180_sensor.startTemperature();
  if (status != 0)
  {
    delay(status);
    status = bmp180_sensor.getTemperature(temp);
    if (status != 0)
    {
      // 3 oversampling_settings
      status = bmp180_sensor.startPressure(3);
      if (status != 0)
      {
        delay(status);
        if (bmp180_sensor.getPressure(pressure, temp) != 0)
        {
          return (pressure);
        }
      }
    }
  }
  else
    return -1;
}

double read_temperature()
{
  char status;
  double temp;

  status = bmp180_sensor.startTemperature();
  if (status != 0)
  {
    delay(status);
    if (bmp180_sensor.getTemperature(temp) != 0)
    {
      return (temp);
    }
  }
  else
    return -1;
}