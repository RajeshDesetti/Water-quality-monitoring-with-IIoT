#define BLYNK_TEMPLATE_ID "TMPL3X4ICfflG"
#define BLYNK_TEMPLATE_NAME "Water quality"
#define BLYNK_AUTH_TOKEN "hboqi2p41Ve5KeBj-9JtiJ76DlZmJ7L0"

#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Redmi Note 11 Pro+ 5G";
char pass[] = "87654321";

namespace pin {
  const byte tds_sensor = A0; // TDS Sensor
  const byte one_wire_bus = D5; // Dallas Temperature Sensor
}

namespace device {
  float aref = 3.3; // Vref, this is for 3.3v compatible controller boards, for Arduino use 5.0v.
}

namespace sensor {
  float ec = 0;
  unsigned int tds = 0;
  float ecCalibration = 1;
}

OneWire oneWire(pin::one_wire_bus);
DallasTemperature dallasTemperature(&oneWire);

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  dallasTemperature.begin();
}

void loop() {
  Blynk.run();
  double waterTemp = TempRead();
  waterTemp  = waterTemp * 0.0625; // conversion accuracy is 0.0625 / LSB

  float rawEc = analogRead(pin::tds_sensor) * device::aref / 1024.0;
  float temperatureCoefficient = 1.0 + 0.02 * (waterTemp - 25.0);

  sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration;
  sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;

  Serial.print(F("TDS:"));
  Serial.println(sensor::tds);
  Serial.println(F(" ppm"));

  if(sensor::tds < 900)
  {
    Blynk.logEvent("test_event", String("Good Quality TDS: ") + sensor::tds);
  }
  if(sensor::tds > 900)
  {
    Blynk.logEvent("test_eventt", String("Poor Quality TDS: ") + sensor::tds);
  }
  
  Serial.print(F("EC:"));
  Serial.println(sensor::ec, 2);
  Serial.println(F(" μS/cm"));

  Serial.print(F("Temperature:"));
  Serial.println(waterTemp, 2);
  Serial.println(F(" °C"));

  Blynk.virtualWrite(V1, sensor::tds);
  Blynk.virtualWrite(V2, sensor::ec);
  Blynk.virtualWrite(V3, waterTemp);

  delay(2000);
}

boolean DS18B20_Init() {
  pinMode(pin::one_wire_bus, OUTPUT);
  digitalWrite(pin::one_wire_bus, HIGH);
  delayMicroseconds(5);
  digitalWrite(pin::one_wire_bus, LOW);
  delayMicroseconds(750);
  digitalWrite(pin::one_wire_bus, HIGH);
  pinMode(pin::one_wire_bus, INPUT);
  int t = 0;
  while (digitalRead(pin::one_wire_bus)) {
    t++;
    if (t > 60) return false;
    delayMicroseconds(1);
  }
  t = 480 - t;
  pinMode(pin::one_wire_bus, OUTPUT);
  delayMicroseconds(t);
  digitalWrite(pin::one_wire_bus, HIGH);
  return true;
}

void DS18B20_Write(byte data) {
  pinMode(pin::one_wire_bus, OUTPUT);
  for (int i = 0; i < 8; i++) {
    digitalWrite(pin::one_wire_bus, LOW);
    delayMicroseconds(10);
    if (data & 1) digitalWrite(pin::one_wire_bus, HIGH);
    else digitalWrite(pin::one_wire_bus, LOW);
    data >>= 1;
    delayMicroseconds(50);
    digitalWrite(pin::one_wire_bus, HIGH);
  }
}

byte DS18B20_Read() {
  pinMode(pin::one_wire_bus, OUTPUT);
  digitalWrite(pin::one_wire_bus, HIGH);
  delayMicroseconds(2);
  byte data = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(pin::one_wire_bus, LOW);
    delayMicroseconds(1);
    digitalWrite(pin::one_wire_bus, HIGH);
    pinMode(pin::one_wire_bus, INPUT);
    delayMicroseconds(5);
    data >>= 1;
    if (digitalRead(pin::one_wire_bus)) data |= 0x80;
    delayMicroseconds(55);
    pinMode(pin::one_wire_bus, OUTPUT);
    digitalWrite(pin::one_wire_bus, HIGH);
  }
  return data;
}

int TempRead() {
  if (!DS18B20_Init()) return 0;
  DS18B20_Write (0xCC);
  DS18B20_Write (0x44);
  if (!DS18B20_Init()) return 0;
  DS18B20_Write (0xCC);
  DS18B20_Write (0xBE);
  int waterTemp = DS18B20_Read();
  waterTemp |= DS18B20_Read() << 8;
  return waterTemp;
}
