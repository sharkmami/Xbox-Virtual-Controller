# 🦈 XVC++ | Xbox Virtual Controller en C++

**XVC++** es un potente emulador de mando de Xbox 360 de alto rendimiento desarrollado por **Sharkmami** 🦈. Esta herramienta transforma periféricos estándar (teclado y mouse) en entradas de un mando virtual con precisión de grado analógico.

### ⚙️ Evolución Técnica: Más allá de Rust
Esta versión representa una **actualización crítica** respecto al motor original diseñado en Rust. Al migrar a **C++ nativo**, el sistema ha logrado una integración mucho más profunda con el hardware:
* **Captura de Raw Input:** Acceso directo a la Win32 API para reducir la latencia de entrada al mínimo posible.
* **Flujo de Datos Optimizado:** Manejo de alta velocidad para las estructuras de reporte `XUSB`.
* **Motor Determinístico:** Sin el "overhead" de otros lenguajes, cada ciclo de CPU se dedica a procesar tu movimiento en tiempo real.

---

## 🛠 Comunicación a Nivel de Kernel

El software actúa como un puente entre tus acciones y el sistema operativo mediante el driver [ViGEmBus](https://github.com/ViGEm/ViGEmBus):

1.  **Instancia del Bus:** El código utiliza `vigem_alloc()` para establecer la conexión con el bus de dispositivos virtuales.
2.  **Abstracción de Hardware:** A través de `vigem_target_x360_alloc()`, Windows identifica el software como un mando de Xbox 360 físico.
3.  **Inyección de Reportes:** Los datos se empaquetan en un `XUSB_REPORT` y se envían cada 2ms para una respuesta fluida.



---

## 🕹 Diccionario de Mapeo (Control Virtual)

Identificadores exactos para usar en `Mapping.ini` (según `Config.hpp`):

| Categoría | Identificador | Función del Mando |
| :--- | :--- | :--- |
| **Acción** | `A`, `B`, `X`, `Y` | Botones frontales principales |
| **Gatillos** | `LT`, `RT` | Gatillos analógicos (L2 / R2) |
| **Bumpers** | `LB`, `RB` | Botones superiores (L1 / R1) |
| **Menú** | `START`, `BACK` | Botones centrales de sistema |
| **Clicks** | `LS_CLICK`, `RS_CLICK` | Presionar palancas (L3 / R3) |
| **Cruceta** | `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT` | D-Pad |
| **Stick L** | `LS_X`, `LS_Y`, `LS_UP`, `LS_DOWN`, `LS_LEFT`, `LS_RIGHT` | Stick Izquierdo (Ejes y Digital) |
| **Stick R** | `RS_X`, `RS_Y`, `RS_UP`, `RS_DOWN`, `RS_LEFT`, `RS_RIGHT` | Stick Derecho (Ejes y Digital) |
---

## ⌨️ Diccionario de Entradas (Teclado y Mouse)

Valores válidos para asignar a los botones anteriores (`Key.hpp`):

* **Ratón (Analógico):** `MOUSE_X` (Horizontal), `MOUSE_Y` (Vertical).
* **Ratón (Botones):** `LBUTTON` (Click Izquierdo), `RBUTTON` (Click Derecho), `MBUTTON` (Rueda).
* **Teclas de Sistema:** `SPACE`, `LSHIFT`, `RSHIFT`, `LCONTROL`, `LALT`, `RETURN` (Enter), `ESCAPE`, `TAB`, `BACKSPACE`.
* **Flechas:** `UP`, `DOWN`, `LEFT`, `RIGHT`.
* **Alfanuméricos:** Cualquier letra (`A` - `Z`) o número (`0` - `9`).
* **Desactivar:** `NONE` (Sin asignar).

---

## 💡 Lógica Analógica Universal y Smoothing

La característica principal del motor es su **Procesamiento de Entrada Híbrido**:

### 1. Mapeo Universal de Ejes
Puedes mapear **cualquier** acción del mando a `MOUSE_X` o `MOUSE_Y`. El motor calcula automáticamente la presión o inclinación basándose en la posición absoluta del cursor en la pantalla.

### 2. Suavizado Configurable (Lerp)
Para eliminar el ruido o "jitter" del mouse, el motor implementa un algoritmo de **Interpolación Lineal** que es **totalmente ajustable por el usuario**:
* **Factor Dinámico:** Cambia el `smoothing_factor` (0.01 a 1.0) en tiempo real desde el panel.
* **Ajuste de Respuesta:** Valores bajos ofrecen fluidez cinematográfica, mientras que 1.0 entrega una respuesta bruta e instantánea.

---

**Licencia MIT** | Desarrollado por **Sharkmami** 🦈


---


# 🦈 XVC++ | Xbox Virtual Controller in C++

**XVC++** is a high-performance Xbox 360 controller emulator developed by **Sharkmami** 🦈. This tool transforms standard peripherals into virtual controller inputs with true analog-grade precision.

### ⚙️ Technical Evolution: Beyond Rust
This version marks a **critical update** from the legacy engine. Migrating to **native C++** allowed for deeper hardware integration:
* **Raw Input Capture:** Direct Win32 API access to minimize input latency.
* **Optimized Data Flow:** High-speed handling of `XUSB` reporting structures.
* **Deterministic Engine:** Zero language overhead ensures every CPU cycle processes your movement in real-time.

---

## 🛠 Kernel-Level Communication

The software bridges your actions to the OS via the [ViGEmBus](https://github.com/ViGEm/ViGEmBus) driver:

1.  **Bus Allocation:** The code uses `vigem_alloc()` to establish a bridge with virtual device buses.
2.  **Hardware Abstraction:** Through `vigem_target_x360_alloc()`, the OS identifies the software as a physical Xbox 360 controller.
3.  **Report Injection:** Data is packed into an `XUSB_REPORT` and dispatched every 2ms for seamless response.


---

## 🕹 Virtual Controller Mapping Dictionary

Exact identifiers for your `Mapping.ini` (as defined in `Config.hpp`):

| Category | Identifier | Controller Function |
| :--- | :--- | :--- |
| **Action** | `A`, `B`, `X`, `Y` | Main face buttons |
| **Triggers** | `LT`, `RT` | Analog Triggers (L2 / R2) |
| **Bumpers** | `LB`, `RB` | Shoulder Buttons (L1 / R1) |
| **System** | `START`, `BACK` | Menu and Back buttons |
| **Stick Clicks**| `LS_CLICK`, `RS_CLICK` | L3 / R3 Thumbstick buttons |
| **D-Pad** | `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT` | Directional Pad |
| **Left Stick** | `LS_X`, `LS_Y`, `LS_UP`, `LS_DOWN`, `LS_LEFT`, `LS_RIGHT` | Axis and Digital directions |
| **Right Stick**| `RS_X`, `RS_Y`, `RS_UP`, `RS_DOWN`, `RS_LEFT`, `RS_RIGHT` | Axis and Digital directions |

---

## ⌨️ Input Dictionary (Hardware Keys)

Valid values to assign to the virtual buttons (`Key.hpp`):

* **Mouse (Analog):** `MOUSE_X` (Horizontal), `MOUSE_Y` (Vertical).
* **Mouse (Buttons):** `LBUTTON` (Left Click), `RBUTTON` (Right Click), `MBUTTON` (Middle Click).
* **Control Keys:** `SPACE`, `LSHIFT`, `RSHIFT`, `LCONTROL`, `LALT`, `RETURN` (Enter), `ESCAPE`, `TAB`, `BACKSPACE`.
* **Arrow Keys:** `UP`, `DOWN`, `LEFT`, `RIGHT`.
* **Alphanumeric:** Any letter (`A` - `Z`) or number (`0` - `9`).
* **Null:** `NONE` (Disables the input).

---

## 💡 Universal Analog Logic & Smoothing

The engine's core feature is its **Hybrid Input Processing**:

### 1. Universal Axis Mapping
You can map **any** controller action to `MOUSE_X` or `MOUSE_Y`. The engine automatically calculates the pressure or tilt based on the cursor's absolute position on the screen.

### 2. Configurable Smoothing (Lerp)
To eliminate raw mouse jitter, the engine implements a **Linear Interpolation** algorithm that is **fully user-adjustable**:
* **Dynamic Factor:** Change the `smoothing_factor` (0.01 to 1.0) in real-time.
* **Response Tuning:** Lower values offer cinematic fluidity, while 1.0 provides raw, instantaneous feedback.

---

**MIT License** | Developed by **Sharkmami** 🦈
