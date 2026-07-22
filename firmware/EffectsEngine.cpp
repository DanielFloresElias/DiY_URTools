#include "EffectsEngine.h"

// Definició de la taula estàtica amb 16 colors RGB



EffectsEngine::EffectsEngine(uint8_t num_leds, uint8_t pin)
  : strip(num_leds, pin, NEO_GRB + NEO_KHZ800), numLeds(num_leds) {
  currentEffect = NO_EFFECT;
  timer = 0;
  interval = 30;
  index = 0;
  brightness = 0;
  Color_1 = 0;
  Color_2 = 0;
  dir = 1;
  SinusCounter = 0;
}


void EffectsEngine::begin() {
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'

  //setEffect(NO_EFFECT);
}

void EffectsEngine::setEffect(EffectID id) {
  currentEffect = id;
}

void EffectsEngine::setColor_1(uint8_t c) {
  Color_1 = c;
}

void EffectsEngine::setColor_2(uint8_t c) {
  Color_2 = c;
}

void EffectsEngine::setInterval(uint8_t i) {
  interval = i;
}

void EffectsEngine::setBrightness(uint8_t i) {
  brightness = i;
}


void EffectsEngine::update() {

  if (millis() - timer >= interval) {
    timer = millis();

    switch (currentEffect) {
      case NO_EFFECT: effect_no_effect(); break;
      case EFFECT_SOLID: effect_solid(); break;
      case EFFECT_BREATHING: effect_breathing(); break;
      case EFFECT_KNIGHT_RIDER: effect_knight_rider(); break;
      case EFFECT_POLICE: effect_police(); break;
      case EFFECT_FLASH: effect_flash(); break;
      case EFFECT_RAINBOW: effect_rainbow(); break;
      case EFFECT_ROLL_RING: effect_roll_ring(); break;
    }
  }
}

// definició taula de colors
const uint8_t EffectsEngine::Color[16][3] = {
  { 255, 0, 0 },      // 0: Vermell
  { 0, 255, 0 },      // 1: Verd
  { 0, 0, 255 },      // 2: Blau
  { 255, 255, 0 },    // 3: Groc
  { 255, 0, 255 },    // 4: Magenta
  { 0, 255, 255 },    // 5: Cian
  { 255, 128, 0 },    // 6: Taronja
  { 128, 0, 255 },    // 7: Violeta
  { 255, 255, 255 },  // 8: Blanc
  { 100, 100, 100 },  // 9: Gris
  { 50, 0, 0 },       // 10: Vermell fosc
  { 0, 50, 0 },       // 11: Verd fosc
  { 0, 0, 50 },       // 12: Blau fosc
  { 255, 105, 180 },  // 13: Rosa
  { 139, 69, 19 },    // 14: Marró
  { 0, 0, 0 }         // 15: Apagat (Negre)
};

//   E F F E C T S  //

void EffectsEngine::effect_no_effect() {
  strip.clear();
  strip.show();
}

void EffectsEngine::effect_solid() {
  if (Color_1 >= 0 && Color_1 < 16) {
    uint8_t r = Color[Color_1][0];
    uint8_t g = Color[Color_1][1];
    uint8_t b = Color[Color_1][2];

    for (int i = 0; i < numLeds; i++) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.setBrightness(brightness);
    strip.show();
  }
}

void EffectsEngine::effect_breathing() {
  strip.setBrightness(sine_table[SinusCounter]);
  
  for (int i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, strip.Color(0, 0, 255));

  strip.show();
  SinusCounter++;
  if (SinusCounter>=100) SinusCounter=0;

  
  /*
  brightness += dir;
  if (brightness < 10) brightness = 10;

  if (brightness >= 150 || brightness <= 10)
    dir = -dir;
  */


}

void EffectsEngine::effect_knight_rider() {
  // 1. Efecte d'esvaïment (fade) per crear la cua
  // Això fa que els LEDs no s'apaguin de cop, sinó que perdin intensitat
  for (int i = 0; i < strip.numPixels(); i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = c;
    // Reduïm el color vermell un 40% a cada cicle per crear el rastre
    strip.setPixelColor(i, strip.Color(r * 0.6, g * 0.6, b * 0.6));
  }

  // 2. Dibuixem el "cap" de l'escàner
  if (Color_1 >= 0 && Color_1 < 16) {
    uint8_t r = Color[Color_1][0];
    uint8_t g = Color[Color_1][1];
    uint8_t b = Color[Color_1][2];
    strip.setPixelColor(index, strip.Color(r, g, b));
  }

  strip.show();

  // 3. Actualitzem la posició per a la propera crida
  index++;

  // 4. Inicialitzaem índex en acabar l'anell
  if (index >= (strip.numPixels())) {
    index = 0;
  }
}

void EffectsEngine::effect_police() {
    uint8_t r1 = Color[Color_1][0], g1 = Color[Color_1][1], b1 = Color[Color_1][2];
    uint8_t r2 = Color[Color_2][0], g2 = Color[Color_2][1], b2 = Color[Color_2][2];

    for (int i = 0; i < numLeds; i++) {
        // Determinem en quina meitat està el LED segons la posició de l'índex
        if ((i + index) % numLeds < (numLeds / 2)) {
            strip.setPixelColor(i, strip.Color(r1, g1, b1));
        } else {
            strip.setPixelColor(i, strip.Color(r2, g2, b2));
        }
    }
    strip.setBrightness(brightness);
    strip.show();
    index = (index + 1) % numLeds; // Fa girar el patró
}

void EffectsEngine::effect_flash() {
    if (index % 2 == 0) {
        // Encenem tot l'anell amb Color_1
        uint8_t r = Color[Color_1][0], g = Color[Color_1][1], b = Color[Color_1][2];
        for (int i = 0; i < numLeds; i++) strip.setPixelColor(i, strip.Color(r, g, b));
    } else {
        // Apaguem les llums per crear l'efecte parpelleig
        strip.clear();
    }
    strip.setBrightness(brightness);
    strip.show();
    index++; 
}

void EffectsEngine::effect_rainbow() {
    // 1. Recorrem tots els LEDs
    for (int i = 0; i < strip.numPixels(); i++) {
        // 2. Calculem el color (Hue) de cada LED. 
        // L'ús d''index' aquí és el que permet que el color canviï de posició.
        uint32_t pixelHue = index + (i * 65536L / strip.numPixels());
        
        // 3. Assignem el color amb correcció gamma per a millor qualitat visual
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    
    // 4. Mostrem els canvis a l'anell
    strip.setBrightness(brightness);
    strip.show();
    
    // 5. INCREMENT CRÍTIC: Sense això, l'arc de Sant Martí no gira
    // Afegim un valor a 'index' per desplaçar els colors en el proper 'update'
    index += 256; 
}

void EffectsEngine::effect_roll_ring() {
    uint8_t r = Color[Color_1][0], g = Color[Color_1][1], b = Color[Color_1][2];
    strip.clear(); // Comencem amb tot apagat per dibuixar només els actius

    for (int i = 0; i < numLeds; i++) {
        // Si la posició relativa a l'índex és múltiple de 4, l'encenem
        if ((i + index) % 4 == 0) {
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
    }
    strip.setBrightness(brightness);
    strip.show();
    index = (index + 1) % numLeds; // Velocitat de gir segons la variable interval
}