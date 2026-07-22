#ifndef EFFECTS_ENGINE_H
#define EFFECTS_ENGINE_H

#include <Adafruit_NeoPixel.h>

const uint8_t sine_table[100] = {
  127, 135, 143, 151, 159, 166, 174, 181, 188, 195,
  201, 207, 213, 219, 224, 229, 234, 238, 241, 245,
  247, 250, 251, 253, 254, 254, 254, 253, 251, 250,
  247, 245, 241, 238, 234, 229, 224, 219, 213, 207,
  201, 195, 188, 181, 174, 166, 159, 151, 143, 135,
  127, 119, 111, 103, 95,  88,  80,  73,  66,  59,
  53,  47,  41,  35,  30,  25,  20,  16,  13,  9,
  7,   4,   3,   1,   0,   0,   0,   1,   3,   4,
  7,   9,   13,  16,  20,  25,  30,  35,  41,  47,
  53,  59,  66,  73,  80,  88,  95,  103, 111, 119
};



enum EffectID
{
    NO_EFFECT,
    EFFECT_SOLID,
    EFFECT_BREATHING,
    EFFECT_KNIGHT_RIDER,
    EFFECT_POLICE,
    EFFECT_FLASH,
    EFFECT_RAINBOW,
    EFFECT_ROLL_RING,
};


class EffectsEngine
{
public:

    EffectsEngine(uint8_t num_leds, uint8_t pin);

    void setEffect(EffectID id);
    void update();
    void begin();
    void setColor_1(uint8_t c);
    void setColor_2(uint8_t c);
    void setInterval(uint8_t i);
    void setBrightness(uint8_t i);

private:

    Adafruit_NeoPixel strip;
    int numLeds;
    EffectID currentEffect;
    unsigned long timer;
    uint8_t interval;
    uint16_t index;
    uint8_t brightness;
    uint8_t Color_1;
    uint8_t Color_2;
    uint8_t SinusCounter;
    int dir;
    static const uint8_t Color[16][3]; // Declaració de la taula de colors 16x3
    void effect_no_effect();
    void effect_solid();
    void effect_breathing();
    void effect_knight_rider();
    void effect_police();
    void effect_flash();
    void effect_rainbow();
    void effect_roll_ring();
};

#endif