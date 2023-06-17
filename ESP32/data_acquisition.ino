#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ThingsBoard.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <PulseSensorPlayground.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define WIFI_AP "PFAWIFI"
#define WIFI_PASSWORD "cowhealth"

#define TOKEN "COWCARE"

// DHT
#define DHTPIN 2
#define DHTTYPE DHT11

char thingsboardServer[] = "thingsboard.cloud";

WiFiClient wifiClient;

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

ThingsBoard tb(wifiClient);

Adafruit_MPU6050 mpu;
PulseSensorPlayground pulseSensor;

SoftwareSerial ss(4, 5); // RX, TX
TinyGPSPlus gps;

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void setup()
{
  Serial.begin(115200);
  dht.begin();
  mpu.begin();
  pulseSensor.begin();
  ss.begin(9600);
  delay(10);
  InitWiFi();
  lastSend = 0;
}

void loop()
{
  if (!tb.connected())
  {
    reconnect();
  }

  if (millis() - lastSend > 1000) 
  {
    getAndSendSensorData();
    lastSend = millis();
  }

  tb.loop();
}

void getAndSendSensorData()
{
  // Read temperature and humidity data from DHT sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read acceleration data from MPU6050
  sensors_event_t accelEvent;
  mpu.getEvent(&accelEvent);
  float accelerationX = accelEvent.acceleration.x;
  float accelerationY = accelEvent.acceleration.y;
  float accelerationZ = accelEvent.acceleration.z;

  // Read heart rate data from Pulse Sensor
  int heartRate = pulseSensor.getBeatsPerMinute();

  // Read data from piezoelectric sensor
  int piezoValue = analogRead(A0);

  // Read GPS data
  while (ss.available() > 0)
  {
    gps.encode(ss.read());
  }
  float latitude = gps.location.lat();
  float longitude = gps.location.lng();

  Serial.println("Sending data to ThingsBoard:");

  // Print and send temperature and humidity data
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");
  tb.sendTelemetryFloat("temperature", temperature);
  tb.sendTelemetryFloat("humidity", humidity);

  // Print and send accelerometer data
  Serial.print("Acceleration X: ");
  Serial.print(accelerationX);
  Serial.print(" m/s^2\t");
  Serial.print("Acceleration Y: ");
  Serial.print(accelerationY);
  Serial.print(" m/s^2\t");
  Serial.print("Acceleration Z: ");
  Serial.print(accelerationZ);
  Serial.println(" m/s^2");
  tb.sendTelemetryFloat("acceleration_x", accelerationX);
  tb.sendTelemetryFloat("acceleration_y", accelerationY);
  tb.sendTelemetryFloat("acceleration_z", accelerationZ);

  // Print and send heart rate data
  Serial.print("Heart Rate: ");
  Serial.print(heartRate);
  Serial.println(" bpm");
  tb.sendTelemetryInt("heart_rate", heartRate);

  // Print and send piezoelectric sensor data
  Serial.print("Piezo Value: ");
  Serial.println(piezoValue);
  tb.sendTelemetryInt("piezo_value", piezoValue);

  // Print and send GPS data
  Serial.print("Latitude: ");
  Serial.print(latitude, 6);
  Serial.print("°\t");
  Serial.print("Longitude: ");
  Serial.print(longitude, 6);
  Serial.println("°");
  tb.sendTelemetryFloat("latitude", latitude);
  tb.sendTelemetryFloat("longitude", longitude);
}

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect()
{
  // Loop until we're reconnected
  while (!tb.connected())
  {
    status = WiFi.status();
    if (status != WL_CONNECTED)
    {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if (tb.connect(thingsboardServer, TOKEN))
    {
      Serial.println("[DONE]");
    }
    else
    {
      Serial.print("[FAILED]");
      Serial.println(" : retrying in 5 seconds]");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
