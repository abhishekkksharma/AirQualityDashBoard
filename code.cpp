#include <DHT.h>
//this code is only for getting values of mq sensor and dht sensor, the co2 sensor is still awaited

#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ135_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// MH-Z19 UART
HardwareSerial co2Serial(2);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Start CO2 sensor serial (RX = 16, TX = 17)
  co2Serial.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("System Started...");
}

int readCO2() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0,0,0,0,0,0x79};
  byte response[9];

  co2Serial.write(cmd, 9);
  delay(10);

  if (co2Serial.available() >= 9) {
    co2Serial.readBytes(response, 9);
    int ppm = (256 * response[2]) + response[3];
    return ppm;
  }
  return -1;
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int mq135 = analogRead(MQ135_PIN);
  int co2 = readCO2();

  Serial.println("------");

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT22 Error!");
  } else {
    Serial.print("Temp: "); Serial.println(temp);
    Serial.print("Humidity: "); Serial.println(hum);
  }

  Serial.print("MQ135: "); Serial.println(mq135);

  if (co2 == -1) {
    Serial.println("CO2: Not available");
  } else {
    Serial.print("CO2: "); Serial.println(co2);
  }

  delay(3000);
}