# 🎮 Xbox Virtual Controller (by Shark)

Una herramienta potente y ligera escrita en **Rust** que transforma tu teclado y ratón en un mando virtual de Xbox 360 de alta precisión. Ideal para simuladores, juegos de carreras o cualquier título donde un teclado estándar no es suficiente.

---

## 🛠️ Requisitos previos (¡Lee esto primero!)

Para que el emulador funcione, tu sistema necesita "hablar" con el mando virtual. Debes instalar el siguiente driver:

1.  **ViGEmBus**: [Descargar última versión aquí](https://github.com/nefarius/ViGEmBus/releases) (Descarga el `.exe` o `.msi` e instálalo).
2.  **Windows**: Diseñado específicamente para sistemas Windows.

---

## 🚀 Cómo empezar

1. Descarga el ejecutable `Xbox-Virtual-Controller.exe` desde la sección de **Releases**.
2. Ejecuta el programa. Se creará automáticamente un archivo `Config.toml` en la misma carpeta.
3. El mando se conectará instantáneamente como un controlador de Xbox 360 oficial.

---

## ⌨️ Controles e Interfaz (Cosas no tan obvias)

El programa funciona en segundo plano, pero tiene un sistema de seguridad inteligente: **solo aceptará comandos de menú si la ventana de la consola está activa** (para no arruinar tu partida).

* **`[C]` - Menú de Configuración**: Abre el panel para remapear teclas. Puedes escribir letras (A, B, Z) o comandos especiales como `MOUSEX`, `MOUSEY` para los sticks, y `MOUSE1`, `MOUSE2` para disparar.
* **`[V]` - Ver Mapeo**: Muestra una tabla detallada de qué tecla controla qué botón del mando. Útil para verificar tu configuración antes de entrar a un juego.
* **`[F10]` - Pausa Global**: ¿Necesitas escribir en el chat del juego? Presiona F10 para pausar la emulación y que tus teclas vuelvan a la normalidad. Presiona de nuevo para retomar el control.
* **`[F12]` - Salida Segura**: Cierra el programa y desconecta el mando virtual limpiamente.

---

## ⚙️ Tips de Configuración
Si editas el `Config.toml` a mano, el valor `smoothing_factor` (por defecto `0.15`) controla qué tan "suave" se siente el movimiento.
- **Valores bajos (0.05)**: Movimiento muy lento y fluido (estilo simulación).
- **Valores altos (0.5)**: Movimiento instantáneo y agresivo.

---

## 🦈 Créditos
Este proyecto es una versión mejorada y personalizada.
**Desarrollado y mantenido por: Shark**

---
*Disclaimer: Requiere ViGEmBus para la emulación del bus de dispositivos.*