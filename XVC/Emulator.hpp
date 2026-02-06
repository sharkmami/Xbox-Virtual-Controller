#pragma once
#include <windows.h>
#include <ViGEm/Client.h>
#include <iostream>

#pragma comment(lib, "setupapi.lib")

class Emulator {
public:
    PVIGEM_CLIENT client;
    PVIGEM_TARGET pad;
    XUSB_REPORT report;

    Emulator() {
        client = vigem_alloc();
        vigem_connect(client);
        pad = vigem_target_x360_alloc();
        vigem_target_add(client, pad);
        XUSB_REPORT_INIT(&report);
    }

    // El destructor se encarga de la limpieza automática al cerrar
    ~Emulator() {
        if (client) {
            vigem_target_remove(client, pad);
            vigem_target_free(pad);
            vigem_free(client);
        }
    }

    void SendUpdate() {
        vigem_target_x360_update(client, pad, report);
    }
};