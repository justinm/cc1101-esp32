#ifndef __CC1101_ESP32_H
#define __CC1101_ESP32_H

#include <Arduino.h>
#include "cc1101_esp32_registers.h"

#ifndef PRINT
#define PRINT(x) Serial.print(x)
#endif

#ifndef PRINT_LN
#define PRINT_LN(x) Serial.println(x)
#endif

char *byte2hex(byte data);
char *byte2hex(byte data[], unsigned int len);

enum Mode {
    RECEIVE,
    TRANSMIT,
    IDLE
};

enum Frequency {
    THREE_FIFTEEN,
    FOUR_THIRTY_THREE,
    EIGTH_SIXTY_EIGTH,
    NINE_FIFTEEN
};

enum Modulation {
    ASK,
    GFSK,
    FSK2,
    FSK4,
    MSK
};

class CC1101_ESP32 {
    protected:
        Mode _mode;
        Modulation _modulation;
        Frequency _frequency;
        byte _frend0;
        bool spiTransactionStarted = false;
        uint8_t SS_PIN;
        uint8_t MOSI_PIN;
        uint8_t MISO_PIN;
        uint8_t SCK_PIN;
        
        void initPins();
        void endSpiTransaction();
        void flushQueues();
        void reset();
        byte spiRead(byte data);
        void spiRead(byte data, byte *buffer, int length);
        byte spiReadBurst(byte addr);
        void spiReadBurst(byte addr, byte *results, int length);
        byte spiReadSingle(byte addr);
        void spiStrobe(byte data);
        void spiWrite(byte addr, byte buffer);
        void spiWrite(byte addr, byte *data, int length);
        void spiWriteBurst(byte addr, byte buffer);
        void spiWriteBurst(byte addr, byte *results, int length);
        byte spiWriteSingle(byte addr, byte data);
        void startSpiTransaction();

    public:
        CC1101_ESP32(uint8_t ssPin);
        CC1101_ESP32(uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin, uint8_t ssPin);
        void setFrequency(Frequency);
        void setFrequency(uint8_t freq0, uint8_t freq1, uint8_t freq2);
        void printChipInfo();
        uint8_t hasData(int timeWaitForData);
        bool setOutputPowerLevel(uint8_t powerLevel);
        bool setModulation(Modulation modulation);
        bool setMode(Mode mode);
        void setPowerLevel(int powerLevel);
        void showRegisters();
        void enableCRC(bool enabled);
        void enableStatusAppend(bool enabled);
        Mode getMode();
        int readData(byte *buffer);
        bool ready();
        bool init();
};

#endif
