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

Identificadores exactos para tu `Mapping.ini` (definidos en `Config.hpp`):

| Identificador | Función del Mando |
| :--- | :--- |
| `A`, `B`, `X`, `Y` | Botones de Acción Principales |
| `LB`, `RB` | Bumpers Superiores |
| `LT`, `RT` | Gatillos Analógicos |
| `LS_X`, `LS_Y` | Ejes del Stick Izquierdo |
| `RS_X`, `RS_Y` | Ejes del Stick Derecho |
| `DPAD_UP`, `DPAD_DOWN` | Cruceta Direccional |

---

## ⌨️ Diccionario de Entradas (Hardware)

Asigna estas teclas físicas en tu configuración (`Key.hpp`):

* **Mouse:** `MOUSE_X`, `MOUSE_Y`, `LBUTTON`, `RBUTTON`.
* **Especiales:** `SPACE`, `LSHIFT`, `LCONTROL`, `RETURN`, `ESCAPE`.
* **Alfanuméricos:** Teclas estándar de la `A-Z` y del `0-9`.
* **Nulo:** `NONE` (Desactiva la entrada).

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
