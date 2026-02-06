#pragma once
#include <algorithm>

class Processor {
public:
    // Normaliza valores (ej. mouse) a rango -1.0 a 1.0
    static float Normalize(int value, int min, int max) {
        float norm = 2.0f * ((float)(value - min) / (float)(max - min)) - 1.0f;
        return (norm < -1.0f) ? -1.0f : (norm > 1.0f ? 1.0f : norm);
    }

    // Suavizado de movimiento
    static float Lerp(float current, float target, float speed) {
        return current + speed * (target - current);
    }

    // Convierte a valor de Stick (-32768 a 32767)
    static short ToThumb(float value) {
        return (short)(value * 32767.0f);
    }

    // Convierte a valor de Gatillo (0 a 255)
    static unsigned char ToTrigger(float value) {
        return (unsigned char)(value * 255.0f);
    }
};