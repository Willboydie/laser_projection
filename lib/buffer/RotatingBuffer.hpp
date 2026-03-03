


struct RotatingBuffer {
    
};

void Projector::updateValuesFromBuffer(uint32_t* offset, uint16_t* valA, uint16_t* valB, uint8_t* laserState) {
    *offset = (activePos << 2) + activePos; // *5 without multiplication
    *valA = (activeBuffer[*offset] << 8) | activeBuffer[*offset + 1];
    *valB = (activeBuffer[*offset + 2] << 8) | activeBuffer[*offset + 3];
    *laserState = activeBuffer[*offset + 4];
}