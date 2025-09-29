#include <DAC.h>

void fastSPITransfer24(uint32_t frame) {
  
  while (!(LPSPI1_SR & LPSPI_SR_TDF)) {}   // Wait for TX FIFO space
  // Send 24-bit frame MSB first (bits 23-0)
  LPSPI1_TDR = (frame & 0xFFFFFF);         // Load 24-bit frame
  while (!(LPSPI1_SR & LPSPI_SR_TCF)) {}   // Wait for transfer complete
  (void)LPSPI1_RDR;                        // Drain RX FIFO
  
}

void sendSPICommand(uint32_t frame) {
  digitalWriteFast(SYNC, LOW);
  fastSPITransfer24(frame);
  digitalWriteFast(SYNC, HIGH);
}

void setupLPSPI1() {
  // Enable LPSPI1 clock
  CCM_CCGR1 |= CCM_CCGR1_LPSPI1(CCM_CCGR_ON);

  // Disable before config
  LPSPI1_CR = 0;
  LPSPI1_CFGR1 = LPSPI_CFGR1_MASTER;  // Master mode

  // Configure for 24-bit frames, mode 0 (CPOL=0, CPHA=0), TX normal
  LPSPI1_TCR =
      LPSPI_TCR_FRAMESZ(23) |     // 24 bits (frame size = N+1)
      0 |                         // no TX mask
      0;                          // CPOL=0, CPHA=0

  // Set clock divider (2 -> 30 MHz, 8 -> 10 MHz)
  LPSPI1_CCR = LPSPI_CCR_SCKDIV(2);

  // Flush FIFOs
  LPSPI1_FCR = 0;

  // Enable module
  LPSPI1_CR = LPSPI_CR_MEN;
  
  Serial.println("LPSPI1 configured for 24-bit transfers at ~30 MHz");
}

void initDAC() {
    // Initialize pins
    pinMode(SYNC, OUTPUT);
    pinMode(LDAC, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(SYNC, HIGH);
    digitalWrite(LDAC, HIGH);
    digitalWrite(CLR, HIGH); // Disable reset

    // Configure LPSPI1 pins (MOSI, SCK, MISO)
    // These are the default SPI pins on Teensy 4.1
    pinMode(11, OUTPUT);  // MOSI
    pinMode(13, OUTPUT);  // SCK
    pinMode(12, INPUT);   // MISO (not used but good practice)

    setupLPSPI1();

    Serial.println("SPI initialized");

    // Power down DAC A and B
    sendSPICommand(0b001000000000000000100011); // Command 100 (power down), data=0x0023
    delay(100); // Wait for power down to complete 

    // Power up DAC A and B
    sendSPICommand(0b001000000000000000000011); // Command 100 (power up), data=0x0003
    delay(100); // Wait for power up to complete
    Serial.println("DAC A and B powered up");

    // Software reset (optional)
    sendSPICommand(0b001010000000000000000001); // Command 101 (reset), data=0x0001
    delay(100); // Wait for reset to complete
    Serial.println("Software reset complete");

    // Enable internal reference (2.5V) + gain=2 (0-5V range)
    sendSPICommand(0b001110000000000000000001); // Command 111 (enable ref), data=0x0001
    delay(10); // Wait for reference to stabilize
    Serial.println("Internal reference enabled");

    sendSPICommand(0b000110001000000000000000); // Set Dac A to 0V (32768 = mid-scale)
    delay(10);
    sendSPICommand(0b000110011000000000000000); // Set Dac B to 0V (32768 = mid-scale)
    delay(100); // Wait for DAC to settle
}

// Control functions using SPI.h library for debugging
void initSPIControl() {
    // Initialize SPI with standard library
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);  // ~10 MHz clock
    SPI.setDataMode(SPI_MODE0);           // CPOL=0, CPHA=0
    SPI.setBitOrder(MSBFIRST);            // Most significant bit first
    
    // Initialize control pins
    pinMode(SYNC, OUTPUT);
    pinMode(LDAC, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(SYNC, HIGH);
    digitalWrite(LDAC, HIGH);
    digitalWrite(CLR, HIGH);
    
    Serial.println("SPI Control initialized (using SPI.h library)");
}

void sendSPICommandControl(uint32_t frame) {
    digitalWrite(SYNC, LOW);
    
    // Send 24-bit frame as 3 bytes (MSB first)
    uint8_t byte1 = (frame >> 16) & 0xFF;  // Most significant byte
    uint8_t byte2 = (frame >> 8) & 0xFF;   // Middle byte
    uint8_t byte3 = frame & 0xFF;          // Least significant byte
    
    SPI.transfer(byte1);
    SPI.transfer(byte2);
    SPI.transfer(byte3);
    
    digitalWrite(SYNC, HIGH);
}

void testDACWiring() {
    Serial.println("=== Testing DAC Wiring with SPI.h Library ===");
    
    initSPIControl();
    delay(100);
    
    // Test 1: Power down command
    Serial.println("Test 1: Power down DAC A and B");
    sendSPICommandControl(0b001000000000000000100011);
    delay(100);
    
    // Test 2: Power up command
    Serial.println("Test 2: Power up DAC A and B");
    sendSPICommandControl(0b001000000000000000000011);
    delay(100);
    
    // Test 3: Software reset
    Serial.println("Test 3: Software reset");
    sendSPICommandControl(0b001010000000000000000001);
    delay(100);
    
    // Test 4: Enable internal reference
    Serial.println("Test 4: Enable internal reference");
    sendSPICommandControl(0b001110000000000000000001);
    delay(100);
    
    // Test 5: Set DAC to mid-scale (should output ~2.5V if working)
    Serial.println("Test 5: Set DAC to mid-scale (2.5V)");
    sendSPICommandControl(0b000111110000000000000000);
    delay(100);
    
    // Test 6: Set DAC to full scale (should output ~5V if working)
    Serial.println("Test 6: Set DAC to full scale (5V)");
    sendSPICommandControl(0b000111111111111111111111);
    delay(100);
    
    // Test 7: Set DAC to zero scale (should output ~0V if working)
    Serial.println("Test 7: Set DAC to zero scale (0V)");
    sendSPICommandControl(0b000111100000000000000000);
    delay(100);
    
    Serial.println("=== Wiring test complete ===");
    Serial.println("Check DAC output voltage with multimeter:");
    Serial.println("- Mid-scale should be ~2.5V");
    Serial.println("- Full-scale should be ~5V");
    Serial.println("- Zero-scale should be ~0V");
}

void compareSPIMethods() {
    Serial.println("=== Comparing SPI Methods ===");
    
    uint32_t testFrame = 0b000111110000000000000000; // Mid-scale command
    
    // Test with SPI.h library
    Serial.println("Testing with SPI.h library:");
    initSPIControl();
    sendSPICommandControl(testFrame);
    delay(100);
    
    // Test with direct LPSPI
    Serial.println("Testing with direct LPSPI:");
    initDAC();
    sendSPICommand(testFrame);
    delay(100);
    
    Serial.println("Both methods tested. Check DAC output voltage.");
}
