# 🎮 Xbox Virtual Controller (Shark Edition)

Una herramienta de alto rendimiento desarrollada en **Rust** que emula un mando de Xbox 360 utilizando el teclado y el ratón. Diseñada para ofrecer latencia cero, estabilidad total y una personalización profunda mediante archivos de configuración.

---

## 🇪🇸 Versión en Español

### 🛠️ Instalación y Requisitos
Para que Windows reconozca el mando virtual, es **estrictamente obligatorio** instalar el driver de bus:
1. **Descarga el Driver**: [ViGEmBus Runtime](https://github.com/nefarius/ViGEmBus/releases/latest).
2. **Ejecución**: Abre `Xbox-Virtual-Controller.exe`. El programa generará automáticamente un archivo `Config.toml` si no existe.

### 🔍 Análisis: Pros y Contras
* **PROS**: 
    * **Latencia Cero**: Al usar Rust, el procesamiento de teclas es casi instantáneo.
    * **Seguridad de Foco**: El menú de configuración (`C`, `V`) solo se activa si la ventana negra está al frente, evitando que cambies teclas por error mientras juegas.
    * **Suavizado de Movimiento**: El `smoothing_factor` permite que el ratón no se sienta brusco, emulando la inercia de un stick real.
* **CONTRAS**:
    * **Doble Presencia**: Windows detectará tanto tu teclado real como el mando virtual. Algunos juegos pueden mostrar iconos parpadeantes (cambiando entre teclas y botones de mando). Es un efecto visual normal; el mando siempre tiene prioridad.

### ⌨️ Guía de Mapeo (Config.toml)
Debes usar los nombres exactos. **Nota:** Las letras siempre van en MAYÚSCULAS.

| Categoría | Nombre en el archivo | Ejemplo |
| :--- | :--- | :--- |
| **Letras** | `A` - `Z` | `a = "W"`, `b = "E"` |
| **Ratón (Ejes)** | `MouseX`, `MouseY` | `ls_x = "MouseX"` |
| **Ratón (Clicks)** | `Mouse1` (Izq), `Mouse2` (Der) | `rt = "Mouse1"` |
| **Especiales** | `LControl`, `LShift`, `LAlt`, `Space`, `Enter` | `lb = "LShift"` |
| **Flechas** | `Up`, `Down`, `Left`, `Right` | `dpad_up = "Up"` |

---

## 🇺🇸 English Version

### 🛠️ Setup & Requirements
For Windows to recognize the virtual controller, you **must** install the bus driver:
1. **Download Driver**: [ViGEmBus Runtime](https://github.com/nefarius/ViGEmBus/releases/latest).
2. **Run**: Launch `Xbox-Virtual-Controller.exe`. A `Config.toml` will be created automatically.

### 🔍 Pros & Cons
* **PROS**: 
    * **Zero Lag**: Built with Rust for microsecond-level input processing.
    * **Focus Security**: Configuration commands (`C`, `V`) only work when the console window is active.
    * **Input Smoothing**: The `smoothing_factor` adds physics-based inertia to mouse movements for a realistic feel.
* **CONS**:
    * **Double Presence**: Windows sees both your keyboard and the controller. Some games might flicker UI icons between keyboard prompts and controller buttons. This is normal; the controller input remains stable.

### ⌨️ Mapping Guide (Config.toml)
Values are case-sensitive. **Note:** All letters must be UPPERCASE.

| Category | Name in file | Example |
| :--- | :--- | :--- |
| **Letters** | `A` - `Z` | `x = "R"`, `y = "F"` |
| **Mouse (Axis)** | `MouseX`, `MouseY` | `rs_y = "MouseY"` |
| **Mouse (Clicks)** | `Mouse1` (Left), `Mouse2` (Right) | `rb = "Mouse2"` |
| **Modifiers** | `LControl`, `LShift`, `LAlt`, `Space`, `Enter` | `back = "Escape"` |
| **Navigation** | `Up`, `Down`, `Left`, `Right`, `Tab` | `dpad_down = "Down"` |

---

## 🦈 Shark Edition
*Developed for performance. Optimized for the community.*
