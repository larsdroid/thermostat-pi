#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define TRUE    1
#define FALSE   0

#define DATA_PIN                7
#define LATCH_PIN               0
#define CLOCK_PIN               3
#define NOT_OUTPUT_ENABLE_PIN   2
#define NOT_CLEAR_PIN           6

#define DELAY_MILLIS            100
#define CLOCK_DURATION          1

struct range_struct {
    int* indices;
    int length;
};

typedef struct range_struct range;

struct stage_struct {
    int repeatAmount;
    int isAutoReverse;
    int isCyclic;
    int bitSequenceCount;
    const int* bitSequences;
};

typedef struct stage_struct stage;

static const stage stages[] = {
    {4, TRUE, TRUE,
        4,
        (const int [])
        {1, 0, 0, 0, 0, 0, 0, 1,
         0, 1, 0, 0, 0, 0, 1, 0,
         0, 0, 1, 0, 0, 1, 0, 0,
         0, 0, 0, 1, 1, 0, 0, 0}
    },
    {5, FALSE, FALSE,
        1,
        (const int [])
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    {5, TRUE, TRUE,
        2,
        (const int [])
        {0, 1, 1, 0, 0, 0, 0, 0,
         0, 0, 1, 1, 0, 0, 0, 0}
    },
    {5, FALSE, FALSE,
        1,
        (const int [])
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    {5, TRUE, TRUE,
        2,
        (const int [])
        {0, 0, 0, 0, 0, 1, 1, 0,
         0, 0, 0, 0, 1, 1, 0, 0}
    },
    {5, FALSE, FALSE,
        1,
        (const int [])
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    {3, TRUE, TRUE,
        8,
        (const int [])
        {1, 0, 0, 0, 0, 0, 0, 0,
         0, 1, 0, 0, 0, 0, 0, 0,
         0, 0, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 1, 0, 0, 0, 0,
         0, 0, 0, 0, 1, 0, 0, 0,
         0, 0, 0, 0, 0, 1, 0, 0,
         0, 0, 0, 0, 0, 0, 1, 0,
         0, 0, 0, 0, 0, 0, 0, 1}
    },
    {1, FALSE, FALSE,
        1,
        (const int [])
        {0, 0, 0, 0, 0, 0, 0, 0}
    }
};

/*
Generates a list of indices that can be used to traverse the bitsequences.
There are three variants (example uses 4 bitsequences):
    1) Traverse each bitsequence once, in order:
        [0, 1, 2, 3]
    2) Traverse each bitsequence and then reverse back to the first one
        [0, 1, 2, 3, 2, 1, 0]
    3) Traverse each bitsequence and then reverse back, but don't include the first one.
       This is used in case an identical sequence follows and the transition to the next sequence should be smooth.
        [0, 1, 2, 3, 2, 1]
*/
range* cycleRangle(int amountOfBitSequences, int isAutoReverse, int isCyclic, int isLastCycle) {
    range* retval = (range*)malloc(sizeof(range));
    
    if (!isAutoReverse && !isCyclic) {
        // 0 .. amountOfBitSequences-1
        retval->length = amountOfBitSequences;
    }
    else if (isAutoReverse && (!isCyclic || (isCyclic && isLastCycle))) {
        // 0 .. amountOfBitSequences-1 .. 0
        retval->length = 2 * amountOfBitSequences - 1;
    }
    else if (isAutoReverse && isCyclic && !isLastCycle) {
        // 0 .. amountOfBitSequences-1 .. 1
        retval->length = 2 * (amountOfBitSequences - 1);
    }
    else {
        return NULL;
    }
    
    retval->indices = (int*)malloc(retval->length * sizeof(int));
    for (int i = 0; i < retval->length; i++) {
        if (i < amountOfBitSequences) {
            retval->indices[i] = i;
        }
        else {
            retval->indices[i] = 2 * (amountOfBitSequences - 1) - i;
        }
    }
    
    return retval;
}

void freeCycleRange(range* rp) {
    free(rp->indices);
    free(rp);
}

void initialise() {
    wiringPiSetup();
    
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(NOT_OUTPUT_ENABLE_PIN, OUTPUT);
    pinMode(NOT_CLEAR_PIN, OUTPUT);
    
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(LATCH_PIN, LOW);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(NOT_OUTPUT_ENABLE_PIN, LOW); // Pin is never used...
    
    // Clear the register
    digitalWrite(NOT_CLEAR_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    delay(CLOCK_DURATION);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(NOT_CLEAR_PIN, HIGH);
}

int main(void) {
    printf("Banana Pi shift register light show.\n");

    initialise();

    int size = sizeof(stages)/sizeof(stage);
    for (int i = 0; i < size; i++) {
        const stage* s = &stages[i];
        
        for (int j = 0; j < s->repeatAmount; j++) {
            int isLastCycle;
            if (j == s->repeatAmount - 1) {
                isLastCycle = TRUE;
            }
            else {
                isLastCycle = FALSE;
            }
            
            range* rp = cycleRangle(s->bitSequenceCount, s->isAutoReverse, s->isCyclic, isLastCycle);
            for (int k = 0; k < rp->length; k++) {
                int currentBitSequenceIndex = rp->indices[k];
                const int* currentBitSequence = &s->bitSequences[8 * currentBitSequenceIndex];
                
                for (int l = 0; l < 8; l++) {
                    int bit = currentBitSequence[l];
                    if (bit) {
                        digitalWrite(DATA_PIN, HIGH);
                    }
                    else {
                        digitalWrite(DATA_PIN, LOW);
                    }
                    digitalWrite(CLOCK_PIN, HIGH);
                    delay(CLOCK_DURATION);
                    digitalWrite(CLOCK_PIN, LOW);
                    digitalWrite(DATA_PIN, LOW);
                }
                
                digitalWrite(LATCH_PIN, HIGH);
                delay(CLOCK_DURATION);
                digitalWrite(LATCH_PIN, LOW);
                
                delay(DELAY_MILLIS);
            }
            freeCycleRange(rp);
        }
    }

    return 0;
}

