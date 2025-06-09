#include <SPI.h>
#include <Arduino.h>
#include <math.h>
#include <sphericalShapes.h>

#define SYNC 10
#define LDAC 9
#define CLR  8
#define PI 3.1415926

void sendSPICommand(uint8_t cmd, uint16_t data) {
  digitalWrite(SYNC, LOW);
  SPI.transfer(cmd);
  SPI.transfer16(data); // Send 16-bit data
  digitalWrite(SYNC, HIGH);
}

// params
// Define maximum number of points (adjust depending on inc/range)
#define MAX_POINTS 20000
#define MAX_ANGLE 0.376 // Maximum angle in radians (e.g., ±20°)
int delay_time = 10000;
Point coords[MAX_POINTS]; // Array to hold generated points
uint16_t points;

struct DACcmd {
  uint16_t A;
  uint16_t B; 
};
DACcmd dacCmds[MAX_POINTS]; // Array to hold DAC commands

float X;
float Y;
float Z;
uint16_t A;
uint16_t B;

void setup0() {
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


  sendSPICommand(0b00011111, (int)65535/2.0); // Set Dac A+B to 0V
  delay(3000); // Wait for DAC to settle

  points = cartesianCircle(122, 80, 0.1, coords);
  if (points == MAX_POINTS + 1) {
    Serial.println("Too many points generated!");
    return;
  }

  for (int p = 0; p < points; p++) {
    X = coords[p].x;
    Y = coords[p].y;
    Z = coords[p].z;
    SphericalCoord sph = toSpherical(X, Y, Z);

    dacCmds[p].A =  toDacCmd(sph.theta);
    dacCmds[p].B = toDacCmd(sph.phi);
  }
}


int p = 0;
void loop0() {
  p = (p + 1) % points; // Loop through points
  A = dacCmds[p].A;
  B = dacCmds[p].B;

  sendSPICommand(0b00000000, A); // Write DAC A
  sendSPICommand(0b00000001, B); // Write DAC B

  sendSPICommand(0b00001111, 0b0000000000000000); // Update Dac A+B

  delayMicroseconds(delay_time);
}

// Why we see distortion:

// The current XYZ -> (theta, phi) conversion assumes that r will stay constant, which is of course bullshit.