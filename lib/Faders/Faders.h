#pragma once

#include "shared/Config.h"

class Faders {
public:
    void begin();
    void read();
private:
    int oldValues[NUM_FADERS];
};
