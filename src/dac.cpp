#include <SPI.h>
#include <Arduino.h>
#include <math.h>
#include <test_points.h>
#include <square.h>

#define SYNC 10
#define LDAC 9
#define CLR  8


void sendSPICommand(uint8_t cmd, uint16_t data) {
  digitalWrite(SYNC, LOW);
  SPI.transfer(cmd);
  SPI.transfer16(data); // Send 16-bit data
  digitalWrite(SYNC, HIGH);
}

// params
int delay_time = 10; // 8.3us for 120kHz update rate
// int points = 2000;


void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize pins
  pinMode(SYNC, OUTPUT);
  pinMode(LDAC, OUTPUT);
  pinMode(CLR, OUTPUT);
  digitalWrite(SYNC, HIGH);
  digitalWrite(LDAC, HIGH);
  digitalWrite(CLR, HIGH); // Disable reset

  // Start SPI (MODE0!)
  SPI.begin();
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  Serial.println("SPI initialized");

  // Power down DAC A and B
  sendSPICommand(0b00100000, 0b00000000000100011); // Command 100 (power down), data=0
  delay(100); // Wait for power down to complete 

  // Power up DAC A and B
  sendSPICommand(0b00100000, 0b0000000000000011); // Command 100 (power up), data=3
  delay(100); // Wait for power up to complete
  Serial.println("DAC A and B powered up");

// Software reset (optional)
  sendSPICommand(0b00101000, 0b0000000000000001); // Command 101 (reset), data=1
  delay(100); // Wait for reset to complete
  Serial.println("Software reset complete");

  // Enable internal reference (2.5V) + gain=2 (0-5V range)
  sendSPICommand(0b00111000, 0b0000000000000001); // Command 111 (enable ref), data=1
  delay(10); // Wait for reference to stabilize
  Serial.println("Internal reference enabled");


  sendSPICommand(0b00011111, (int)65535/1.0); // Set Dac A+B to 0V
  delay(3000); // Wait for DAC to settle

  generateSquare();

}


int t = 0;
int i = 0;
int x;
int y;


int32_t timestamp = millis();
void loop() {

  t++; // Increment t, wrapping around at points
  if (t == 65535) {
    int32_t wavelength = millis() - timestamp;
    Serial.println(wavelength);
    timestamp = millis();
    t = 0;
  }

  i = (i + 1) % NUM_POINTS;
  

  // x = (int) 24575 * cos((t/65535.0) * 2 * 3.14159) + 32767;
  // y = (int) 24575 * sin((t/65535.0) * 2 * 3.14159) + 32767;

  // x = (int) 32767 * sin((t/65535.0) * 2 * 3.14159) + 32767;
  // y = (int) 32767 * sin((t/65535.0) * 2 * 3.14159) + 32767;

  x = squareCoords[i][0];
  y = squareCoords[i][1];


  sendSPICommand(0b00000000, x); // Write DAC A
  sendSPICommand(0b00000001, y); // Write DAC B

  sendSPICommand(0b00001111, 0b0000000000000000); // Update Dac A+B
  
  delayMicroseconds(delay_time);
}
