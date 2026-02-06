# 🦈 XVC++ | Xbox Virtual Controller en C++ 

**XVC++** es un potente emulador de mando de Xbox 360 desarrollado por **Sharkmami** 🦈. Esta herramienta permite transformar periféricos convencionales (teclado y mouse) en entradas de un mando virtual con alta precisión.

### ⚙️ El Salto Técnico: De Rust a C++
Esta versión representa una **actualización crítica** del motor antiguo diseñado en Rust. Al migrar a **C++ nativo**, el sistema ha logrado una integración mucho más profunda con el hardware:
* **Captura de Raw Input:** Acceso directo a la Win32 API para reducir la latencia al mínimo posible.
* **Gestión de Registros:** Un manejo de estructuras de datos optimizado para los reportes de `XUSB`.
* **Motor Determinístico:** Al eliminar el overhead de otros lenguajes, cada ciclo de CPU se dedica exclusivamente a procesar tu movimiento en tiempo real.

---

## 🛠 Comunicación con ViGEmBus

El software no solo emula teclas, sino que se comunica directamente con el kernel de Windows a través del driver **ViGEmBus**:

1.  **Instancia del Bus:** El código utiliza `vigem_alloc()` para establecer un puente con el bus de dispositivos virtuales.
2.  **Abstracción de Hardware:** Mediante `vigem_target_x360_alloc()`, el sistema operativo reconoce el software como un mando de Xbox 360 físico.
3.  **Lógica de Inyección:** Los datos se procesan y se envían mediante una estructura `XUSB_REPORT` cada 2ms. El driver recibe este reporte y lo inyecta en el sistema.

---

## 🕹 Diccionario de Mapeo del Control (Virtual)

Estos son los nombres exactos que utiliza el motor para identificar las funciones del mando (según `Config.hpp`):

| Identificador | Función del Mando |
| :--- | :--- |
| `A`, `B`, `X`, `Y` | Botones frontales |
| `LB`, `RB` | Bumpers (L1 / R1) |
| `LT`, `RT` | Gatillos analógicos (L2 / R2) |
| `START`, `BACK` | Botones de menú |
| `LS_CLICK`, `RS_CLICK` | Clicks de los sticks (L3 / R3) |
| `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT` | Cruceta / D-Pad |
| `LS_X`, `LS_Y` | Ejes del Stick Izquierdo |
| `LS_UP`, `LS_DOWN`, `LS_LEFT`, `LS_RIGHT` | Direcciones del Stick Izquierdo |
| `RS_X`, `RS_Y` | Ejes del Stick Derecho |
| `RS_UP`, `RS_DOWN`, `RS_LEFT`, `RS_RIGHT` | Direcciones del Stick Derecho |

---

## ⌨️ Diccionario de Entradas (Teclado y Mouse)

Utiliza estos nombres en tu configuración para asignar las teclas físicas (`Key.hpp`):

* **Mouse:** `MOUSE_X`, `MOUSE_Y`, `LBUTTON`, `RBUTTON`, `MBUTTON`.
* **Teclas Especiales:** `SPACE`, `LSHIFT`, `RSHIFT`, `LCONTROL`, `LALT`, `RETURN`, `ESCAPE`, `TAB`, `BACKSPACE`.
* **Movimiento:** `UP`, `DOWN`, `LEFT`, `RIGHT`.
* **Nulo:** `NONE`.

---

## 💡 Lógica Analógica y Smoothing Configurable

La verdadera potencia de este motor reside en cómo procesa los datos antes de enviarlos al driver:

### 1. Gatillos por Eje Vertical
Si asignas `LT` o `RT` a `MOUSE_Y`, el motor activa una lógica de posición:
* Al desplazar el mouse hacia la **parte superior** de la pantalla, se genera una señal progresiva para **RT** (aceleración).
* Al desplazarlo hacia la **parte inferior**, se genera una señal progresiva para **LT** (frenado).

### 2. Algoritmo de Suavizado Personalizable
Para evitar movimientos bruscos, el motor integra una función de **Interpolación Lineal (Lerp)**. Este comportamiento es totalmente **configurable**:

* **Ajuste Dinámico:** Puedes cambiar el `smoothing_factor` (0.01 a 1.0) desde el panel de control o en el `Mapping.ini`.
* **Control de Respuesta:** Un valor bajo ofrece fluidez, mientras que un valor de 1.0 ofrece respuesta instantánea.

---

## 📦 Instalación

1.  Instala el driver oficial [ViGEmBus](https://github.com/ViGEm/ViGEmBus/releases).
2.  Ejecuta `XVC++.exe`.
3.  ¡Configura tu mapeo y domina el juego! 🦈🚀

---

**Licencia MIT** | Desarrollado por **Sharkmami** 🦈
