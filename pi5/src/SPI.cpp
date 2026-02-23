SPI::configureSPI() {
    const char *device = "/dev/spidev0.0";
    uint8_t mode = SPI_MODE_0;           // or whichever mode your DAC needs
    uint8_t bits = 8;                    // 8-bit word size is usual
    uint32_t speed = 50000000;           // try 50 MHz to start with
    int fd = open(device, O_RDWR);
    
    if (fd < 0) {
        perror("Can't open SPI device");
        return 1;
    }

    // Configure SPI mode
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Can't set SPI mode");
        return 1;
    }

    // Configure bits per word
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Can't set bits per word");
        return 1;
    }

    // Configure max speed
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Can't set max speed");
        return 1;
    }
}


SPI::transfer24(uint8_t cmd, uint16_t value) {
    // Pack into 3-byte buffer (big-endian)
    uint8_t tx[3];
    tx[0] = (value >> 8) & 0xFF;
    tx[1] = value & 0xFF;
    tx[2] = cmd;

    uint8_t rx[3] = {0, 0, 0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed,
        .bits_per_word = bits,
        .delay_usecs = 0,
        .cs_change = 0
    };

    // Perform SPI transfer
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("Can't send SPI message");
        return 1;
    }
}