#include <wiringPi.h>
#include <pcf8574.h>

// Part of WiringPi dev
#include "lcd.h"

#include "ini_file.h"

#define AF_BASE 100

#define AF_RS   (AF_BASE + 0)
#define AF_RW   (AF_BASE + 1)
#define AF_E    (AF_BASE + 2)

#define AF_BL   (AF_BASE + 3)

#define AF_D1   (AF_BASE + 4)
#define AF_D2   (AF_BASE + 5)
#define AF_D3   (AF_BASE + 6)
#define AF_D4   (AF_BASE + 7)

unsigned char degrees[8] = { 0b00100, 0b01010, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};

int initalize() {
    wiringPiSetupSys();

    pcf8574Setup(AF_BASE, i2cAddress);

    int screen = lcdInit(4, 20, 4,
        AF_RS, AF_E,
        AF_D1, AF_D2, AF_D3, AF_D4, 0, 0, 0, 0);

    /*int offset = 4;
    int screen = lcdInit(4, 20, 8,
        AF_RS, AF_E,
        AF_BASE + offset + 0,
        AF_BASE + offset + 1,
        AF_BASE + offset + 2,
        AF_BASE + offset + 3,
        AF_BASE + offset + 4,
        AF_BASE + offset + 5,
        AF_BASE + offset + 6,
        AF_BASE + offset + 7);*/

    // Backlight pin is an output pin
    pinMode(AF_BL, OUTPUT);

    //Set LCD into write mode.
    pinMode(AF_RW, OUTPUT);
    digitalWrite(AF_RW, 0);

    return screen;
}

void light() {
    digitalWrite(AF_BL, 1);
}

void dark() {
    digitalWrite(AF_BL, 0);
}

int main() {
    int screen = initalize();

    lcdCharDef(screen, 0, degrees);

    lcdClear(screen);
    lcdPosition(screen, 0, 0);
    lcdPuts(screen, "Hello World!");
    lcdPosition(screen, 0, 1);
    lcdPuts(screen, "Lars was here!");
    lcdPosition(screen, 0, 2);
    lcdPuts(screen, "ABCDEFGHIJKLMNOPQRST");
    lcdPosition(screen, 0, 3);
    lcdPuts(screen, ">> 0 << 50* >> 9.7*");

    lcdPosition(screen, 10, 3);
    lcdPutchar(screen, 0);
    lcdPosition(screen, 18, 3);
    lcdPutchar(screen, 0);

    light();

    lcdPosition(screen, 3, 3);
    int j = 0;
    for (int i = 0; i < 100; i++) {
        unsigned char c = (unsigned char)(48 + i % 10);
        lcdPosition(screen, 3, 3);
        lcdPutchar(screen, c);

        if (j == 10) {
            dark();
        }
        else if (j == 20) {
            light();
            j = 0;
        }
        j++;

        delay(250);
    }

    return 0;
}
