#pragma once
#include <windows.h>
#include <string>
#include <algorithm>

class Key {
public:
    // Función estática para que Config.hpp pueda usarla sin crear un objeto Key
    static int StringToVK(std::string keyName) {
        if (keyName.empty() || keyName == "NONE") return 0;

        // Convertir a mayúsculas para evitar errores
        std::transform(keyName.begin(), keyName.end(), keyName.begin(), ::toupper);

        // Mapeo manual de teclas comunes
        if (keyName == "MOUSE_X")     return 901;
        if (keyName == "MOUSE_Y")     return 902;
        if (keyName == "LBUTTON")     return VK_LBUTTON;
        if (keyName == "RBUTTON")     return VK_RBUTTON;
        if (keyName == "MBUTTON")     return VK_MBUTTON;
        if (keyName == "SPACE")       return VK_SPACE;
        if (keyName == "LSHIFT")      return VK_LSHIFT;
        if (keyName == "RSHIFT")      return VK_RSHIFT;
        if (keyName == "LCONTROL")    return VK_LCONTROL;
        if (keyName == "LALT")        return VK_LMENU;
        if (keyName == "RETURN")      return VK_RETURN;
        if (keyName == "ESCAPE")      return VK_ESCAPE;
        if (keyName == "TAB")         return VK_TAB;
        if (keyName == "UP")          return VK_UP;
        if (keyName == "DOWN")        return VK_DOWN;
        if (keyName == "LEFT")        return VK_LEFT;
        if (keyName == "RIGHT")       return VK_RIGHT;
        if (keyName == "BACKSPACE")   return VK_BACK;

        // Si es una sola letra o número (A-Z, 0-9)
        if (keyName.length() == 1) {
            return (int)keyName[0];
        }

        return 0;
    }

    static bool isPressed(int vKey) {
        if (vKey == 0) return false;
        return (GetAsyncKeyState(vKey) & 0x8000);
    }
};