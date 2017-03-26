#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <wiringPi.h>

#define DATA_PIN    3
#define LATCH_PIN   0
#define CLOCK_PIN   7

#define A_SEGMENT   10
#define B_SEGMENT   6
#define C_SEGMENT   3
#define D_SEGMENT   1
#define E_SEGMENT   0
#define F_SEGMENT   9
#define G_SEGMENT   4
#define DP_PIN      2
#define DIGIT_1     11
#define DIGIT_2     8
#define DIGIT_3     7
#define UNUSED_PIN  5

#define DELAY_MILLIS 200

void insertBits(int* bits, int length) {
    for (int i = length - 1; i >= 0; i--) {
        if (bits[i]) {
            digitalWrite(DATA_PIN, HIGH);
        }
        else {
            digitalWrite(DATA_PIN, LOW);
        }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
    }
    
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
}

/*
For easy wiring we're not clearing using the CLEAR pin, but instead pumping 16 zeroes into the registers.
*/
void clear() {
    int seqClear[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    insertBits(seqClear, 16);
}

void initialise() {
    wiringPiSetup();
    
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(LATCH_PIN, LOW);
    digitalWrite(CLOCK_PIN, LOW);
    
    clear();
}

int* generateBitSequence(int time, int digitNumber) {
    int* sequence = malloc(12 * sizeof(int));
    
    int digit = time % (int)pow(10, 4 - digitNumber) / (int)pow(10, 3 - digitNumber);

    sequence[A_SEGMENT]  = !(digit == 1 || digit == 4);
    sequence[B_SEGMENT]  = !(digit == 5 || digit == 6);
    sequence[C_SEGMENT]  = !(digit == 2);
    sequence[D_SEGMENT]  = !(digit == 1 || digit == 4 || digit ==  7);
    sequence[E_SEGMENT]  = (digit == 0 || digit == 2 || digit ==  6 || digit ==  8);
    sequence[F_SEGMENT]  = !(digit == 1 || digit == 2 || digit ==  3 || digit ==  7);
    sequence[G_SEGMENT]  = !(digit == 0 || digit ==  1 || digit ==  7);

    sequence[DP_PIN]     = 0;

    sequence[DIGIT_1]    = (digitNumber != 1);
    sequence[DIGIT_2]    = (digitNumber != 2);
    sequence[DIGIT_3]    = (digitNumber != 3);

    sequence[UNUSED_PIN] = 0;
    
    return sequence;
}

double getTimeMillis() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

int main(int argc, const char* argv[]) {
    printf("Banana Pi two shift registers and three digit LED display.\n");

    initialise();
    
    if (argc == 1) {
        double previousTimeMillis = getTimeMillis();
        double timeMillis;
        int number = 999;

        /*
        Digit 1 is the most significant digit.
        */
        int* digit1Sequence;
        int* digit2Sequence;
        int* digit3Sequence;

        while (number >= 0) {
            digit1Sequence = generateBitSequence(number, 1);
            digit2Sequence = generateBitSequence(number, 2);
            digit3Sequence = generateBitSequence(number, 3);

            do {
                insertBits(digit1Sequence, 12);
                insertBits(digit2Sequence, 12);
                insertBits(digit3Sequence, 12);
                
                timeMillis = getTimeMillis();
            } while (timeMillis - previousTimeMillis < DELAY_MILLIS);
            previousTimeMillis = timeMillis;

            free(digit1Sequence);
            free(digit2Sequence);
            free(digit3Sequence);
            
            number--;
        }
    }

    return 0;
}

