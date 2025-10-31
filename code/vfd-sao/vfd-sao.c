#include "ch32fun.h"
#include "i2c-slave.h"
#include <stdio.h>

// Programming one-liner:
//  make; python -m mpremote cp vfd-sao.bin :.; python -m mpremote eval "programSAO(1)"

// The I2C slave library uses a one byte address so you can extend the size of this array up to 256 registers
// note that the register set is modified by interrupts, to prevent the compiler from accidently optimizing stuff
// away make sure to declare the register array volatile

// reg 128 - mode
// reg 129 - delay time
volatile uint8_t i2c_registers[255] = {0x00};

void onWrite(uint8_t reg, uint8_t length) {
    funDigitalWrite(PA2, i2c_registers[0] & 1);
}

volatile uint16_t vfd_reg;

// SysTick is CLK/8, or about 6MHz
uint32_t millis()
{
    return SysTick->CNT / DELAY_MS_TIME;
}

#define SEG_C  (1 << 0)
#define SEG_DP (1 << 1)
#define SEG_B  (1 << 2)
#define SEG_A  (1 << 3)
#define SEG_H  (1 << 4)
#define SEG_G  (1 << 5)
#define SEG_F  (1 << 6)
#define SEG_E  (1 << 7)
#define SEG_D  (1 << 8)
#define SEG_I  (1 << 9)

#define HV_DAT PA1
#define HV_CLK PA2
#define HV_LATCH PC4

// shifts LSB-first
void writeVFD(uint16_t pattern)
{
    for (int i=0; i<10; ++i)
    {
        funDigitalWrite(HV_DAT, pattern & 1);

        pattern >>= 1;
        funDigitalWrite(HV_CLK, FUN_LOW);
        funDigitalWrite(HV_CLK, FUN_HIGH);
    }

    funDigitalWrite(HV_LATCH, FUN_HIGH);
    funDigitalWrite(HV_LATCH, FUN_LOW);
}

int main() {
    SystemInit();
    funGpioInitAll();

    // Initialize I2C slave
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
    SetupI2CSlave(0x9, i2c_registers, sizeof(i2c_registers), onWrite, NULL, false);

    funPinMode(HV_DAT, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(HV_CLK, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(HV_LATCH, GPIO_CFGLR_OUT_10Mhz_PP);
    
    funDigitalWrite(HV_LATCH, FUN_LOW);
    funDigitalWrite(HV_CLK, FUN_HIGH);

    uint32_t lastTime = 0;
    uint32_t currentTime = millis();

//#define SEVEN_SEG

    uint32_t numbers[] = {
#ifdef SEVEN_SEG
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
                SEG_B | SEG_C,
        SEG_A | SEG_B |         SEG_D | SEG_E |         SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D |                 SEG_G,
                SEG_B | SEG_C |                 SEG_F | SEG_G,
        SEG_A |         SEG_C | SEG_D |         SEG_F | SEG_G,
        SEG_A |         SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D |         SEG_F | SEG_G
#else
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
                SEG_B | SEG_C |                                 SEG_H,
        SEG_A | SEG_B |         SEG_D | SEG_E |         SEG_G,
        SEG_A |         SEG_C | SEG_D |                 SEG_G | SEG_H,
                SEG_B | SEG_C |                 SEG_F | SEG_G,
        SEG_A |         SEG_C | SEG_D |         SEG_F | SEG_G,
        SEG_A |         SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A |                                                 SEG_H | SEG_I,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D |         SEG_F | SEG_G
#endif // SEVEN_SEG
    };

    int i = 0;
    while (1)
    {
        currentTime = millis();
        if ((uint32_t) (currentTime - lastTime) > 4)
        {
            lastTime = currentTime;

            if (i == 0)
                writeVFD(numbers[8]);
            else
                writeVFD(0);

            i = (i == 3) ? 0 : i + 1;
        }
//        if ((uint32_t) (currentTime - lastTime) > 500)
//        {
//            lastTime = currentTime;
//
//            writeVFD(numbers[i] | ((i % 2) ? 0 : SEG_DP));
//
//            i = (i == 9) ? 0 : i + 1;
//        }
    }
}
