#include <windows.h>
#include <thread>
#include <iostream>
#include <conio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <chrono>
#include "Processor.hpp"
#include "Emulator.hpp"
#include "Config.hpp"
#include "Key.hpp"

// Globales
Config configGlobal;
bool emulacionActiva = false;

// Variables para el contador de inactividad
POINT ultimaPosMouse = { 0, 0 };
ULONGLONG ultimoMovimientoMs = 0;
const int TIEMPO_RESET_IDLE = 3000; // Tiempo en milisegundos para resetear (3 segundos)

template<class T>
constexpr const T& mi_clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// --- UTILIDADES ---
void limpiarPantalla() { system("cls"); }
void pausa() { std::cout << "\nPresione cualquier tecla para continuar..."; _getch(); }

std::string aMayusculas(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string vkToName(int vk) {
    if (vk == 901) return "MOUSE_X";
    if (vk == 902) return "MOUSE_Y";
    if (vk == 0) return "NONE (NULO)";
    char name[64];
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, name, 64)) return std::string(name);
    return "CODE_" + std::to_string(vk);
}

// --- LÓGICA DE POSICIÓN ABSOLUTA DEL MOUSE ---
float obtenerPosicionMouse(int axis) {
    POINT p;
    GetCursorPos(&p);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    float valorNormalizado = 0.0f;

    if (axis == 901) { // EJE X
        valorNormalizado = ((float)p.x / (float)screenW) * 2.0f - 1.0f;
    }
    else if (axis == 902) { // EJE Y
        valorNormalizado = -(((float)p.y / (float)screenH) * 2.0f - 1.0f);
    }

    return mi_clamp(valorNormalizado, -1.0f, 1.0f);
}

// Función auxiliar para calcular el valor del gatillo (Lógica de subir/bajar eje)
unsigned char calcularGatillo(std::string action) {
    int vk = configGlobal.mapping[action];
    if (vk == 0) return 0;

    if (vk == 901 || vk == 902) {
        float pos = obtenerPosicionMouse(vk);
        float finalVal = 0.0f;

        if (action == "RT") {
            if (pos > 0) finalVal = pos;
        }
        else if (action == "LT") {
            if (pos < 0) finalVal = -pos;
        }

        return (unsigned char)(mi_clamp(finalVal, 0.0f, 1.0f) * 255.0f);
    }

    return configGlobal.isTriggered(action) ? 255 : 0;
}

// --- MOTOR DE EMULACIÓN ---
void motorEmulacion() {
    Emulator emu;
    float curLSX = 0, curLSY = 0;
    float curRSX = 0, curRSY = 0;

    while (true) {
        if (emulacionActiva) {
            // Lógica de detección de inactividad del Mouse
            POINT posActual;
            GetCursorPos(&posActual);

            if (posActual.x != ultimaPosMouse.x || posActual.y != ultimaPosMouse.y) {
                ultimaPosMouse = posActual;
                ultimoMovimientoMs = GetTickCount64();
            }

            bool estaEnIdle = (GetTickCount64() - ultimoMovimientoMs) > TIEMPO_RESET_IDLE;

            // --- STICK IZQUIERDO (LS) ---
            float tLSX = 0, tLSY = 0;
            if (configGlobal.isTriggered("LS_UP"))    tLSY += 1.0f;
            if (configGlobal.isTriggered("LS_DOWN"))  tLSY -= 1.0f;
            if (configGlobal.isTriggered("LS_LEFT"))  tLSX -= 1.0f;
            if (configGlobal.isTriggered("LS_RIGHT")) tLSX += 1.0f;

            // Solo aplicar posición del mouse si se está moviendo (No Idle)
            if (!estaEnIdle) {
                if (configGlobal.mapping["LS_X"] == 901) tLSX = obtenerPosicionMouse(901);
                if (configGlobal.mapping["LS_Y"] == 902) tLSY = obtenerPosicionMouse(902);
            }

            curLSX = Processor::Lerp(curLSX, tLSX, configGlobal.smoothing_factor);
            curLSY = Processor::Lerp(curLSY, tLSY, configGlobal.smoothing_factor);
            emu.report.sThumbLX = Processor::ToThumb(curLSX);
            emu.report.sThumbLY = Processor::ToThumb(curLSY);

            // --- STICK DERECHO (RS) ---
            float tRSX = 0, tRSY = 0;
            if (configGlobal.isTriggered("RS_UP"))    tRSY += 1.0f;
            if (configGlobal.isTriggered("RS_DOWN"))  tRSY -= 1.0f;
            if (configGlobal.isTriggered("RS_LEFT"))  tRSX -= 1.0f;
            if (configGlobal.isTriggered("RS_RIGHT")) tRSX += 1.0f;

            if (!estaEnIdle) {
                if (configGlobal.mapping["RS_X"] == 901) tRSX = obtenerPosicionMouse(901);
                if (configGlobal.mapping["RS_Y"] == 902) tRSY = obtenerPosicionMouse(902);
            }

            curRSX = Processor::Lerp(curRSX, tRSX, configGlobal.smoothing_factor);
            curRSY = Processor::Lerp(curRSY, tRSY, configGlobal.smoothing_factor);
            emu.report.sThumbRX = Processor::ToThumb(curRSX);
            emu.report.sThumbRY = Processor::ToThumb(curRSY);

            // --- BOTONES ---
            emu.report.wButtons = 0;
            if (configGlobal.isTriggered("A"))          emu.report.wButtons |= XUSB_GAMEPAD_A;
            if (configGlobal.isTriggered("B"))          emu.report.wButtons |= XUSB_GAMEPAD_B;
            if (configGlobal.isTriggered("X"))          emu.report.wButtons |= XUSB_GAMEPAD_X;
            if (configGlobal.isTriggered("Y"))          emu.report.wButtons |= XUSB_GAMEPAD_Y;
            if (configGlobal.isTriggered("LB"))         emu.report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (configGlobal.isTriggered("RB"))         emu.report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (configGlobal.isTriggered("START"))      emu.report.wButtons |= XUSB_GAMEPAD_START;
            if (configGlobal.isTriggered("BACK"))       emu.report.wButtons |= XUSB_GAMEPAD_BACK;
            if (configGlobal.isTriggered("LS_CLICK"))   emu.report.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
            if (configGlobal.isTriggered("RS_CLICK"))   emu.report.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;
            if (configGlobal.isTriggered("DPAD_UP"))    emu.report.wButtons |= XUSB_GAMEPAD_DPAD_UP;
            if (configGlobal.isTriggered("DPAD_DOWN"))  emu.report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
            if (configGlobal.isTriggered("DPAD_LEFT"))  emu.report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
            if (configGlobal.isTriggered("DPAD_RIGHT")) emu.report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

            // --- GATILLOS ---
            emu.report.bLeftTrigger = calcularGatillo("LT");
            emu.report.bRightTrigger = calcularGatillo("RT");

            emu.SendUpdate();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

// --- MENÚS ---
void mostrarMapeosActuales(Config& cfg) {
    limpiarPantalla();
    std::cout << "=== CONFIGURACION ACTUAL ===\n" << std::endl;
    for (const auto& btn : cfg.BOTONES_VALIDOS) {
        std::cout << btn << " -> " << vkToName(cfg.mapping[btn]) << std::endl;
    }
    pausa();
}

void mapearControl(Config& cfg) {
    std::string boton, nuevaTecla;
    while (true) {
        limpiarPantalla();
        std::cout << "=== CONFIGURADOR CONTINUO (Escriba 'SALIR' para volver al menu) ===\n" << std::endl;
        std::cout << "Boton del mando (ej: RS_X, A, LT): ";

        std::cin >> boton;
        boton = aMayusculas(boton);
        if (boton == "SALIR") return;

        if (!cfg.EsBotonValido(boton)) {
            std::cout << "[!] Boton invalido." << std::endl;
            pausa(); continue;
        }

        std::cout << "Asignar a (Presione ENTER para nulo, o escriba tecla/MOUSE_X/MOUSE_Y): ";

        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::getline(std::cin, nuevaTecla);

        nuevaTecla = aMayusculas(nuevaTecla);

        if (nuevaTecla.empty()) {
            nuevaTecla = "NONE";
        }

        if (nuevaTecla == "SALIR") return;

        if (Key::StringToVK(nuevaTecla) != 0 || nuevaTecla == "NONE") {
            cfg.UpdateMapping(boton, nuevaTecla);
            std::cout << "\n[OK] " << boton << " mapeado a " << nuevaTecla << std::endl;
            std::cout << "Presione una tecla para continuar mapeando otro boton...";
            _getch();
        }
        else {
            std::cout << "[!] Tecla no reconocida." << std::endl;
            pausa();
        }
    }
}

void ajustarSuavizado(Config& cfg) {
    std::string input;
    while (true) {
        limpiarPantalla();
        std::cout << "=== AJUSTAR SMOOTHING (Actual: " << cfg.smoothing_factor << ") ===" << std::endl;
        std::cout << "Valor (0.01 - 1.0) o 'SALIR': "; std::cin >> input;
        if (aMayusculas(input) == "SALIR") break;
        try {
            float n = std::stof(input);
            cfg.UpdateSmoothing(n);
            break;
        }
        catch (...) { std::cout << "[!] Error de entrada." << std::endl; pausa(); }
    }
}

int main() {
    SetConsoleTitleA("XVC++");
    std::thread hilo(motorEmulacion);
    hilo.detach();

    std::string menuInput;
    int seleccion = 0;

    while (seleccion != 5) {
        limpiarPantalla();
        std::cout << "========================================" << std::endl;
        std::cout << "         XVC++ PANEL DE CONTROL         " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << " MOTOR: " << (emulacionActiva ? "[ENCENDIDO]" : "[APAGADO]") << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << " 1. Alternar Motor (ON/OFF)" << std::endl;
        std::cout << " 2. Ver Configuracion" << std::endl;
        std::cout << " 3. Mapear Control" << std::endl;
        std::cout << " 4. Ajustar Suavizado" << std::endl;
        std::cout << " 5. Salir" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Seleccione: ";

        std::cin >> menuInput;
        try { seleccion = std::stoi(menuInput); }
        catch (...) { seleccion = 0; }

        switch (seleccion) {
        case 1: emulacionActiva = !emulacionActiva; break;
        case 2: mostrarMapeosActuales(configGlobal); break;
        case 3: mapearControl(configGlobal); break;
        case 4: ajustarSuavizado(configGlobal); break;
        }
    }
    return 0;
}