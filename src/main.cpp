#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Adafruit_MPU6050.h>
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11
#define BMP_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP3XX bmp;
Adafruit_MPU6050 mpu;
#define I2C_SDA 21
#define I2C_SCL 22
TwoWire I2CMPU = TwoWire(0);



//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 23
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels



//packet counter
int readingID = 0;

int counter = 0;
String LoRaMessage = "";

float temperature = 0;
float pressure = 0;
float altitude = 0;
float acc_x = 0;
float acc_y = 0;
float acc_z = 0;
float rotation_x = 0;
float rotation_y = 0;
float rotation_z = 0;


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Initialize OLED display
void startOLED(){
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

//Initialize LoRa module
void startLoRA(){
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    readingID++;
    Serial.println("Starting LoRa failed!"); 
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

void startBMP(){
  I2CMPU.begin(I2C_SDA, I2C_SCL, 10000);
  if (!bmp.begin_I2C(0x77)) {   
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }
  Serial.println("BMP Found!");
 
}
void startMPU(){
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }
 
}
void getReadings(){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  temperature = (bmp.temperature * 1.8) + 32;
  pressure = bmp.pressure / 100.0;
  altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA) + 8.5;
  acc_x = a.acceleration.x;
  acc_y = a.acceleration.y;
  acc_z = a.acceleration.z;
  rotation_x = a.gyro.x;
  rotation_y = a.gyro.y;
  rotation_z = a.gyro.z;
}

void sendReadings() {
  LoRaMessage = String(readingID) + "/" + String(temperature) + "&" + String(pressure) + "#" + String(altitude) + "*" + String(acc_x) + "^"+ String(acc_y) + "%" + String(acc_z);
  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.print("LoRa packet sent!");
  display.setCursor(0,20);
  display.print("Temperature:");
  display.setCursor(72,20);
  display.print(temperature);
  display.setCursor(0,30);
  display.print("Pressure:");
  display.setCursor(54,30);
  display.print(pressure);
  display.setCursor(0,40);
  display.print("Altitude:");
  display.setCursor(54,40);
  display.print(altitude);
  display.setCursor(0,50);
  display.print("Acceleration X:");
  display.setCursor(66,50);
  display.print(acc_x);
  display.display();
  Serial.print("Acceleration Y: ");
  Serial.println(acc_y);
  readingID++;
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  startOLED();
  startBMP();
  startMPU();
  startLoRA();
  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}
void loop() {
  getReadings();
  sendReadings();
  delay(1000);
}