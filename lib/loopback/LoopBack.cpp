#include "LoopBack.hpp"

LoopBack::LoopBack(FileManager& fileManager) : fileManager(fileManager) {}

bool LoopBack::init() {
    if (!fileManager.isInitialised()) {
        Debug::msg("LOOPBACK: FileManager not initialised");
        return false;
    }

    expectedPoints = fileManager.getFileSize() / RECORD_SIZE;
    if (expectedPoints == 0) {
        Debug::msg("LOOPBACK: No points to capture");
        return false;
    }

    if (!fileManager.openLoopbackForWrite()) {
        return false;
    }

    initSPISlave();

    writeIdx = 0;
    rxByteIdx = 0;
    pointsCaptured = 0;
    totalRxBytes = 0;
    callCount = 0;
    lastDiagTime = 0;
    complete = false;
    initialised = true;

    Debug::msg("LOOPBACK: Initialised, expecting ");
    Debug::msg(expectedPoints);
    Debug::msg(" points");
    return true;
}

void LoopBack::initSPISlave() {
    // SPI1.begin() configures pin mux and clocking for LPSPI3
    SPI1.begin();

    // Force all three pin mux registers to the correct LPSPI3 ALT + SION.
    // Using assignment (not |=) so SPI1.begin()'s master-mode mux is overwritten.

    // Pin 1 = GPIO_AD_B0_03, ALT7 = LPSPI3_SDI (slave data in)
    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03 = 7 | 0x10;
    IOMUXC_LPSPI3_SDI_SELECT_INPUT = 0;

    // Pin 27 = GPIO_AD_B1_10, ALT2 = LPSPI3_SCK (slave clock in)
    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10 = 2 | 0x10;
    IOMUXC_LPSPI3_SCK_SELECT_INPUT = 1;

    // Pin 0 = GPIO_AD_B0_02, ALT7 = LPSPI3_PCS0 (slave chip-select in)
    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02 = 7 | 0x10;
    IOMUXC_LPSPI3_PCS0_SELECT_INPUT = 0;

    // Disable and reset LPSPI3
    IMXRT_LPSPI3_S.CR = 0;
    IMXRT_LPSPI3_S.CR = LPSPI_CR_RST;
    IMXRT_LPSPI3_S.CR = 0;

    // Slave mode (MASTER=0), NOSTALL to avoid blocking on FIFO overflow
    IMXRT_LPSPI3_S.CFGR1 = LPSPI_CFGR1_NOSTALL;

    // RX watermark = 0 (RDF flag set when ≥1 word in FIFO)
    IMXRT_LPSPI3_S.FCR = 0;

    // Clear all status flags
    IMXRT_LPSPI3_S.SR = 0x3F00;

    // Flush FIFOs (this also clears any stale TCR command words)
    IMXRT_LPSPI3_S.CR = LPSPI_CR_RRF | LPSPI_CR_RTF;

    // Enable module
    IMXRT_LPSPI3_S.CR = LPSPI_CR_MEN;

    // Write TCR AFTER enable so it isn't destroyed by the FIFO flush.
    // 8-bit frames, SPI mode 0 (CPOL=0, CPHA=0), MSB first.
    IMXRT_LPSPI3_S.TCR = LPSPI_TCR_FRAMESZ(7);

    // Seed TX FIFO with a dummy word so the slave doesn't underrun immediately
    IMXRT_LPSPI3_S.TDR = 0;

    Debug::msg("LOOPBACK: SPI1 slave configured - register dump:");
    Debug::msg("  CR=");
    Debug::msg(IMXRT_LPSPI3_S.CR);
    Debug::msg("  SR=");
    Debug::msg(IMXRT_LPSPI3_S.SR);
    Debug::msg("  CFGR1=");
    Debug::msg(IMXRT_LPSPI3_S.CFGR1);
    Debug::msg("  TCR=");
    Debug::msg(IMXRT_LPSPI3_S.TCR);
    Debug::msg("  FCR=");
    Debug::msg(IMXRT_LPSPI3_S.FCR);
    Debug::msg("  PARAM=");
    Debug::msg(IMXRT_LPSPI3_S.PARAM);
}

void LoopBack::readNSave() {
    if (complete || !initialised) return;

    callCount++;

    // Periodic diagnostic every ~2 seconds
    uint32_t now = millis();
    if (now - lastDiagTime >= 2000) {
        lastDiagTime = now;
        Debug::msg("LOOPBACK DIAG: calls=");
        Debug::msg(callCount);
        Debug::msg(" rxBytes=");
        Debug::msg(totalRxBytes);
        Debug::msg(" points=");
        Debug::msg(pointsCaptured);
        Debug::msg(" rxByteIdx=");
        Debug::msg(rxByteIdx);
        Debug::msg(" writeIdx=");
        Debug::msg(writeIdx);
        Debug::msg(" SR=0x");
        Debug::msg(IMXRT_LPSPI3_S.SR);
        Debug::msg(" FSR=0x");
        Debug::msg(IMXRT_LPSPI3_S.FSR);
    }

    // Drain all available bytes from the SPI slave receive FIFO
    while (IMXRT_LPSPI3_S.SR & LPSPI_SR_RDF) {
        uint32_t data = IMXRT_LPSPI3_S.RDR;
        rxFrame[rxByteIdx++] = data & 0xFF;
        totalRxBytes++;

        // Log first few raw bytes for verification
        if (totalRxBytes <= 12) {
            Debug::msg("LOOPBACK RX byte #");
            Debug::msg(totalRxBytes);
            Debug::msg(" = 0x");
            Debug::msg(data & 0xFF);
        }

        if (rxByteIdx == 6) {
            writeBuffer[writeIdx++] = rxFrame[1];
            writeBuffer[writeIdx++] = rxFrame[2];
            writeBuffer[writeIdx++] = rxFrame[4];
            writeBuffer[writeIdx++] = rxFrame[5];

            pointsCaptured++;
            rxByteIdx = 0;

            if (writeIdx >= LOOPBACK_BUFFER_SIZE * LOOPBACK_RECORD_SIZE) {
                flushBuffer();
            }
        }
    }

    if (pointsCaptured >= expectedPoints) {
        flushBuffer();
        fileManager.closeLoopback();
        complete = true;
        Debug::msg("LOOPBACK: Capture complete - ");
        Debug::msg(pointsCaptured);
        Debug::msg(" points saved to loopback.bin");
        sendSerial();
        fileManager.wipeLoopback();
    }
}

void LoopBack::flushBuffer() {
    if (writeIdx == 0) return;
    bool ok = fileManager.writeLoopback(writeBuffer, writeIdx);
    fileManager.flushLoopback();
    Debug::msg("LOOPBACK FLUSH: requested=");
    Debug::msg(writeIdx);
    Debug::msg(" ok=");
    Debug::msg(ok ? "yes" : "no");
    Debug::msg(" fileSize=");
    Debug::msg(fileManager.loopbackSize());
    writeIdx = 0;
}

void LoopBack::sendSerial() {
    uint8_t header[5];
    header[0] = LOOPBACK_DATA_CMD;
    header[1] = (uint8_t)(pointsCaptured & 0xFF);
    header[2] = (uint8_t)((pointsCaptured >> 8) & 0xFF);
    header[3] = (uint8_t)((pointsCaptured >> 16) & 0xFF);
    header[4] = (uint8_t)((pointsCaptured >> 24) & 0xFF);
    Serial.write(header, 5);

    if (!fileManager.openLoopbackForRead()) {
        Debug::msg("LOOPBACK: Failed to reopen file for serial send");
        return;
    }

    size_t totalBytes = pointsCaptured * LOOPBACK_RECORD_SIZE;
    size_t bytesSent = 0;

    while (bytesSent < totalBytes) {
        size_t chunkSize = min(sizeof(writeBuffer), totalBytes - bytesSent);
        int bytesRead = fileManager.readLoopback(writeBuffer, chunkSize);
        if (bytesRead <= 0) break;
        Serial.write(writeBuffer, bytesRead);
        Serial.flush();
        bytesSent += bytesRead;
    }

    fileManager.closeLoopback();
    Debug::msg("LOOPBACK: Sent ");
    Debug::msg((uint32_t)bytesSent);
    Debug::msg(" bytes via serial");
}

void LoopBack::toggle() {
    if (on) {
        on = false;
        initialised = false;
        complete = false;
        fileManager.closeLoopback();
        Debug::msg("LOOPBACK: Disabled");
        return;
    }
    else {
        if (!init()) {
            Debug::msg("LOOPBACK: Enable failed");
            return;
        }
        on = true;
        Debug::msg("LOOPBACK: Enabled");
        Debug::msg("LOOPBACK: Expected points: ");
        Debug::msg(expectedPoints);
        return;
    }
}