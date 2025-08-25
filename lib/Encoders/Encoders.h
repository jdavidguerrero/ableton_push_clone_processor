#pragma once

#include <Encoder.h>
#include "shared/Config.h"

class Encoders {
public:
    void begin();
    void read();
private:
    Encoder* encoders[NUM_ENCODERS];
    long oldPositions[NUM_ENCODERS];
};
