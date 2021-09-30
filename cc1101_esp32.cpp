#include "cc1101_esp32.h"
#include <SPI.h>

static uint8_t patable_315[] = {0x17, 0x1D, 0x26, 0x69, 0x51, 0x86, 0xCC, 0xC3};
static uint8_t patable_433[] = {0x6C, 0x1C, 0x06, 0x3A, 0x51, 0x85, 0xC8, 0xC0};
static uint8_t patable_868[] = {0x03, 0x17, 0x1D, 0x26, 0x50, 0x86, 0xCD, 0xC0};
static uint8_t patable_915[] = {0x0B, 0x1B, 0x6D, 0x67, 0x50, 0x85, 0xC9, 0xC1};

char *byte2hex(byte data)
{
    char *buffer = new char[5];

    byte nib1 = (data >> 4) & 0x0F;
    byte nib2 = (data >> 0) & 0x0F;
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[3] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
    buffer[4] = '\0';

    return buffer;
}

char *byte2hex(byte data[], unsigned int len)
{
    char *buffer = new char[len * 2];

    for (unsigned int i = 0; i < len; i++)
    {
        char *hex = byte2hex(data[i]);
        buffer[i * 2 + 0] = hex[2];
        buffer[i * 2 + 1] = hex[3];
    }

    buffer[len * 2] = '\0';

    return buffer;
}

uint8_t getIndexFromDbm(uint8_t dBm)
{
    if (dBm <= -30)
        return 0;
    else if (dBm <= -20)
        return 1;
    else if (dBm <= -15)
        return 2;
    else if (dBm <= -10)
        return 3;
    else if (dBm <= 0)
        return 4;
    else if (dBm <= 5)
        return 5;
    else if (dBm <= 7)
        return 6;
    else if (dBm <= 10)
        return 7;
}

CC1101_ESP32::CC1101_ESP32(uint8_t ssPin)
{
    SCK_PIN = SCK;
    MOSI_PIN = MOSI;
    MISO_PIN = MISO;
    SS_PIN = ssPin;
    initPins();
}

CC1101_ESP32::CC1101_ESP32(uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin, uint8_t ssPin)
{
    SCK_PIN = sckPin;
    MOSI_PIN = mosiPin;
    MISO_PIN = misoPin;
    SS_PIN = ssPin;
    initPins();
}

bool CC1101_ESP32::ready()
{
    return spiReadSingle(CC1101_VERSION) > 0;
}

bool CC1101_ESP32::init()
{
    if (ready())
    {
        digitalWrite(SS_PIN, HIGH);
        digitalWrite(SCK_PIN, HIGH);
        digitalWrite(MOSI_PIN, LOW);

        reset();
        enableCRC(true);
        enableStatusAppend(false);
        setFrequency(FOUR_THIRTY_THREE);
        setModulation(ASK);
        setPowerLevel(1);
        setOutputPowerLevel(1);
        setMode(IDLE);

        return true;
    }

    return false;
}

void CC1101_ESP32::reset()
{
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(SS_PIN, HIGH);
    delayMicroseconds(40);

    spiStrobe(CC1101_SRES);

    delay(1);
}

void CC1101_ESP32::enableCRC(bool enabled)
{
    byte pktctrl1 = spiReadBurst(CC1101_PKTCTRL1);

    if (enabled)
    {
        pktctrl1 |= 1UL << CC1101_CRC_AUTOFLUSH;
    } else {
        pktctrl1 &= ~(1UL << CC1101_CRC_AUTOFLUSH);
    }

    spiWriteBurst(CC1101_PKTCTRL1, pktctrl1);
}

void CC1101_ESP32::enableStatusAppend(bool enabled)
{
    byte pktctrl1 = spiReadBurst(CC1101_PKTCTRL1);

    if (enabled)
    {
        pktctrl1 |= 1UL << CC1101_CRC_APPEND_STATUS;
    } else {
        pktctrl1 &= ~(1UL << CC1101_CRC_APPEND_STATUS);
    }

    spiWriteBurst(CC1101_PKTCTRL1, pktctrl1);
}

Mode CC1101_ESP32::getMode()
{
    return _mode;
}

bool CC1101_ESP32::setMode(Mode mode)
{
    byte bitMode;

    switch (mode)
    {
    case TRANSMIT:
        bitMode = CC1101_STX;
        break;
    case RECEIVE:
        bitMode = CC1101_SRX;
        break;
    case IDLE:
        break;
    default:
        return false;
    }

    spiStrobe(CC1101_SIDLE);
    delayMicroseconds(100);
    
    if (mode != IDLE && bitMode)
    {
        spiStrobe(bitMode);
        delayMicroseconds(100);
    }

    _mode = mode;

    return true;
}

bool CC1101_ESP32::setModulation(Modulation modulation)
{
    _modulation = modulation;
    byte modByte;
    switch (modulation)
    {
    case FSK2:
        modByte = 0;
        break; // 2-FSK
    case GFSK:
        modByte = 1;
        break; // GFSK
    case ASK:
        modByte = 3;
        break; // ASK
    case FSK4:
        modByte = 4;
        break; // 4-FSK
    case MSK:
        modByte = 7;
        break; // MSK
    default:
        return false;
    }

    uint8_t data;
    data = spiReadBurst(CC1101_MDMCFG2);
    data = (data & 0x8F) | (((modByte) << 4) & 0x70);
    spiWriteBurst(CC1101_MDMCFG2, data);

    return true;
}

bool CC1101_ESP32::setOutputPowerLevel(uint8_t dBm)
{
    byte frend0 = 0xC0;

    switch (getIndexFromDbm(dBm))
    {
    case 0:
        frend0 = 0x00;
        break;
    case 1:
        frend0 = 0x01;
        break;
    case 2:
        frend0 = 0x02;
        break;
    case 3:
        frend0 = 0x03;
        break;
    case 4:
        frend0 = 0x04;
        break;
    case 5:
        frend0 = 0x05;
        break;
    case 6:
        frend0 = 0x06;
        break;
    case 7:
        frend0 = 0x07;
        break;
    }

    spiWriteBurst(CC1101_FREND0, frend0);

    return frend0 != 0xC0;
}

void CC1101_ESP32::setPowerLevel(int dBm)
{
    byte *paTable = new byte[CC1101_NUM_PATABLE];
    byte attr;

    spiReadBurst(PATABLE_BURST, paTable, CC1101_NUM_PATABLE);

    uint8_t powerIndex = getIndexFromDbm(dBm);

    switch (_frequency)
    {
    case THREE_FIFTEEN:
        attr = patable_315[powerIndex];
        break;
    case FOUR_THIRTY_THREE:
        attr = patable_433[powerIndex];
        break;
    case EIGTH_SIXTY_EIGTH:
    default:
        attr = patable_868[powerIndex];
        break;
    case NINE_FIFTEEN:
        attr = patable_915[powerIndex];
        break;
    }

    switch (_modulation)
    {
    case ASK:
        paTable[0] = 0;
        paTable[1] = attr;
        break;
    default:
        paTable[0] = attr;
        paTable[1] = 0;
        break;
    }

    spiWriteBurst(CC1101_PATABLE, paTable, CC1101_NUM_PATABLE);
    delayMicroseconds(10);
}

void CC1101_ESP32::printChipInfo()
{
    byte *status = new byte[2];

    startSpiTransaction();

    PRINT(F("Version: "));
    PRINT_LN(byte2hex(spiReadSingle(CC1101_VERSION)));

    PRINT(F("Chip ID: "));
    PRINT_LN(byte2hex(spiReadSingle(CC1101_PARTNUM)));

    spiReadBurst(0, status, 1);

    byte chipStatus = (*status) >> 7;
    byte chipState = (*status >> 4) & 0b111;
    byte bytesAvailable = *status & 0b1111;

    PRINT(F("Chip Status: "));
    PRINT_LN(byte2hex(chipStatus));

    PRINT(F("State: "));
    PRINT_LN(byte2hex(chipState));

    PRINT(F("Has Data: "));
    PRINT(bytesAvailable);
    PRINT_LN(F(" bytes"));

    endSpiTransaction();
}

void CC1101_ESP32::showRegisters()
{
    byte config[CC1101_NUM_REGISTERS];
    byte paTable[8];

    spiReadBurst(READ_BURST, config, CC1101_NUM_REGISTERS);   //reads all 47 config register
    spiReadBurst(PATABLE_BURST, paTable, CC1101_NUM_PATABLE); //reads output power settings

    Serial.println(F("Registers:"));

    for (uint8_t i = 0; i < CC1101_NUM_REGISTERS; i++)
    {
        PRINT(byte2hex(config[i]));
        PRINT(F(" "));

        if ((i + 1) % 9 == 0)
        {
            PRINT_LN(F(""));
        }
    }
    PRINT_LN(F(""));
    PRINT_LN(F(""));
    Serial.println(F("Power Table:"));

    for (uint8_t i = 0; i < CC1101_NUM_PATABLE; i++) //showes rx_buffer for debug
    {
        PRINT(byte2hex(paTable[i]));
        PRINT(F(" "));
    }
    PRINT_LN(F(" "));
}

int CC1101_ESP32::readData(byte *buffer)
{
    int length = hasData(0);
    
    if (!length)
    {
        return 0x00;
    }

    buffer = new byte[length];

    spiReadBurst(CC1101_RXFIFO, buffer, length);
    spiReadBurst(CC1101_RXFIFO);
    spiReadBurst(CC1101_RXFIFO);

    spiStrobe(CC1101_SFRX); // Clear the RX queue
    spiStrobe(CC1101_SRX);  // Restart the RX queue

    return length;
}

uint8_t CC1101_ESP32::hasData(int timeWaitForData)
{
    if (spiReadBurst(CC1101_RXBYTES) & BYTES_IN_RXFIFO) {
        if (timeWaitForData > 0)
        {
            delay(timeWaitForData);
        }

        return true;
    }

    return false;
}

void CC1101_ESP32::initPins()
{
#ifdef ESP32
    pinMode(SCK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
#endif
    pinMode(SS_PIN, OUTPUT);
}

byte CC1101_ESP32::spiRead(byte data)
{
    byte *results = new byte[1];

    spiRead(data, results, 1);

    return results[0];
}

void CC1101_ESP32::spiRead(byte data, byte *buffer, int length)
{
    startSpiTransaction();

    SPI.transfer(data);

    for (int i = 0; i < length; i++)
    {
        buffer[i] = SPI.transfer(0);
    }

    endSpiTransaction();
}

void CC1101_ESP32::spiReadBurst(byte addr, byte *results, int length)
{
    spiRead(addr | READ_BURST, results, length);
}

byte CC1101_ESP32::spiReadBurst(byte addr)
{
    return spiRead(addr | READ_BURST);
}

byte CC1101_ESP32::spiReadSingle(byte addr)
{
    return spiRead(addr | READ_SINGLE);
}

void CC1101_ESP32::spiStrobe(byte data)
{
    startSpiTransaction();

    SPI.transfer(data);

    endSpiTransaction();
}

void CC1101_ESP32::spiWrite(byte addr, byte buffer)
{
    startSpiTransaction();

    SPI.transfer(addr);
    SPI.transfer(buffer);

    endSpiTransaction();
}

void CC1101_ESP32::spiWrite(byte addr, byte *buffer, int length)
{
    startSpiTransaction();

    SPI.transfer(addr);

    for (int i = 0; i < length; i++)
    {
        SPI.transfer(buffer[i]);
    }

    endSpiTransaction();
}

void CC1101_ESP32::spiWriteBurst(byte addr, byte *buffer, int length)
{
    spiWrite(addr | WRITE_BURST, buffer, length);
}

void CC1101_ESP32::spiWriteBurst(byte addr, byte buffer)
{
    spiWrite(addr | WRITE_BURST, buffer);
}

void CC1101_ESP32::spiWriteSingle(byte addr, byte data)
{
    spiWrite(addr | WRITE_SINGLE, data);
}

void CC1101_ESP32::startSpiTransaction()
{
    if (spiTransactionStarted)
    {
        return;
    }
#ifdef ESP32
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
#else
    SPI.begin();
#endif
    digitalWrite(SS_PIN, LOW);

    while (digitalRead(MISO_PIN))
        ;

    spiTransactionStarted = true;
}

void CC1101_ESP32::endSpiTransaction()
{
    if (!spiTransactionStarted)
    {
        return;
    }

    digitalWrite(SS_PIN, HIGH);
    SPI.endTransaction();
    SPI.end();
    spiTransactionStarted = false;
}

void CC1101_ESP32::setFrequency(Frequency freq)
{
    _frequency = freq;

    switch (freq)
    {
    case THREE_FIFTEEN:
        return setFrequency(0x89, 0x1D, 0x0C);
    case FOUR_THIRTY_THREE:
        return setFrequency(0x71, 0xB0, 0x10);
    case EIGTH_SIXTY_EIGTH:
        return setFrequency(0x6A, 0x65, 0x21);
    case NINE_FIFTEEN:
        return setFrequency(0x3B, 0x31, 0x23);
    }
}

void CC1101_ESP32::setFrequency(byte freq0, byte freq1, byte freq2)
{
    spiWriteBurst(CC1101_FREQ2, freq2);
    spiWriteBurst(CC1101_FREQ1, freq1);
    spiWriteBurst(CC1101_FREQ0, freq0);
}
