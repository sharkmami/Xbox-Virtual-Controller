#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include "Key.hpp"

class Config {
private:
    const char* iniPath = ".\\Mapping.ini";

    void WriteIniString(const char* section, const char* key, std::string value) {
        WritePrivateProfileStringA(section, key, value.c_str(), iniPath);
    }

public:
    float smoothing_factor = 0.15f;
    int screen_width, screen_height;
    std::map<std::string, int> mapping;

    const std::vector<std::string> BOTONES_VALIDOS = {
        "LS_X", "LS_Y", "LS_UP", "LS_DOWN", "LS_LEFT", "LS_RIGHT",
        "RS_X", "RS_Y", "RS_UP", "RS_DOWN", "RS_LEFT", "RS_RIGHT",
        "A", "B", "X", "Y", "LB", "RB", "START", "BACK", "LS_CLICK", "RS_CLICK",
        "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "LT", "RT"
    };

    Config() {
        screen_width = GetSystemMetrics(SM_CXSCREEN);
        screen_height = GetSystemMetrics(SM_CYSCREEN);
        LoadConfig();
    }

    void LoadConfig() {
        char sBuf[10];
        // Leemos el valor de suavizado. Si no existe, por defecto es 0.15
        GetPrivateProfileStringA("Settings", "Smoothing", "0.15", sBuf, 10, iniPath);
        smoothing_factor = std::stof(sBuf);

        for (const std::string& a : BOTONES_VALIDOS) {
            const char* section = "Buttons";
            if (a.find("LS_") == 0 || a.find("RS_") == 0) section = "Sticks";
            else if (a.find("DPAD_") == 0) section = "DPAD";
            else if (a == "LT" || a == "RT") section = "Triggers";

            char buffer[255];
            GetPrivateProfileStringA(section, a.c_str(), "NONE", buffer, 255, iniPath);
            mapping[a] = Key::StringToVK(buffer);
        }
    }

    bool EsBotonValido(std::string boton) {
        return std::find(BOTONES_VALIDOS.begin(), BOTONES_VALIDOS.end(), boton) != BOTONES_VALIDOS.end();
    }

    void UpdateMapping(std::string action, std::string newKeyName) {
        if (!EsBotonValido(action)) return;

        const char* section = "Buttons";
        if (action.find("LS_") == 0 || action.find("RS_") == 0) section = "Sticks";
        else if (action.find("DPAD_") == 0) section = "DPAD";
        else if (action == "LT" || action == "RT") section = "Triggers";

        WriteIniString(section, action.c_str(), newKeyName);
        mapping[action] = Key::StringToVK(newKeyName);
    }

    //Actualiza el suavizado en el archivo y en memoria
    void UpdateSmoothing(float nuevoValor) {
        if (nuevoValor < 0.01f) nuevoValor = 0.01f; // Evitar valores negativos o cero absoluto
        if (nuevoValor > 1.0f) nuevoValor = 1.0f;   // 1.0 es movimiento instantáneo (sin lag)

        smoothing_factor = nuevoValor;
        WriteIniString("Settings", "Smoothing", std::to_string(nuevoValor));
    }

    bool isTriggered(std::string action) {
        if (mapping.count(action)) {
            int vk = mapping[action];
            if (vk == 0) return false;
            return (GetAsyncKeyState(vk) & 0x8000);
        }
        return false;
    }

    float getAxis(std::string pos, std::string neg) {
        float val = 0;
        if (isTriggered(pos)) val += 1.0f;
        if (isTriggered(neg)) val -= 1.0f;
        return val;
    }
};