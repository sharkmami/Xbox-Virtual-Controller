mod config;
mod emulator;
mod key;
mod processor;

use config::Config;
use device_query::{DeviceQuery, DeviceState, Keycode};
use emulator::Emulator;
use processor::Processor;
use std::{thread, time::Duration, str::FromStr, io::{self, Write}};
use vigem_client::XButtons;

#[cfg(target_os = "windows")]
use winapi::um::winuser::{GetForegroundWindow, GetWindowThreadProcessId};
#[cfg(target_os = "windows")]
use winapi::um::processthreadsapi::GetCurrentProcessId;

const MARGIN: &str = "          ";

fn es_ventana_activa() -> bool {
    #[cfg(target_os = "windows")]
    unsafe {
        let hwnd = GetForegroundWindow();
        if hwnd.is_null() { return false; }
        let mut process_id = 0;
        GetWindowThreadProcessId(hwnd, &mut process_id);
        return process_id == GetCurrentProcessId();
    }
    #[cfg(not(target_os = "windows"))]
    true
}

fn clear_console() {
    if cfg!(target_os = "windows") {
        let _ = std::process::Command::new("cmd").args(&["/c", "cls"]).status();
    } else {
        print!("{}[2J{}[1;1H", 27 as char, 27 as char);
    }
    let _ = io::stdout().flush();
}

fn es_input_valido(s: &str) -> bool {
    let s_up = s.to_uppercase();
    let especiales = ["MOUSEX", "MOUSEY", "MOUSE1", "MOUSE2", "MOUSE3", "MOUSE4", "MOUSE5", ""];
    if especiales.contains(&s_up.as_str()) { return true; }
    Keycode::from_str(s).is_ok()
}

fn mostrar_mapeo_actual(config: &Config) {
    clear_console();
    let info = |m: &Option<String>| -> String {
        m.as_deref().unwrap_or("---").to_string()
    };
    
    println!("\n{}╔════════════════════════════════════════════════════╗", MARGIN);
    println!("{}║           ESTADO ACTUAL DEL MAPEADO                ║", MARGIN);
    println!("{}╠════════════════════════════════════════════════════╣", MARGIN);
    println!("{}  STICKS:                                           ", MARGIN);
    println!("{}    LS: X: {:<12} Y: {:<12} Click: {:<5}", MARGIN, info(&config.sticks.ls_x), info(&config.sticks.ls_y), info(&config.buttons.ls_click));
    println!("{}    RS: X: {:<12} Y: {:<12} Click: {:<5}", MARGIN, info(&config.sticks.rs_x), info(&config.sticks.rs_y), info(&config.buttons.rs_click));
    println!("{}  ──────────────────────────────────────────────────  ", MARGIN);
    println!("{}  GATILLOS/BUMPERS:                                 ", MARGIN);
    println!("{}    LT: {:<12} RT: {:<12}      ", MARGIN, info(&config.triggers.left_trigger), info(&config.triggers.right_trigger));
    println!("{}    LB: {:<12} RB: {:<12}      ", MARGIN, info(&config.buttons.lb), info(&config.buttons.rb));
    println!("{}  ──────────────────────────────────────────────────  ", MARGIN);
    println!("{}  BOTONES CENTRALES:                                ", MARGIN);
    println!("{}    Start: {:<8} Back: {:<8} Xbox: {:<8} ", MARGIN, info(&config.buttons.start), info(&config.buttons.back), info(&config.buttons.xbox));
    println!("{}  ──────────────────────────────────────────────────  ", MARGIN);
    println!("{}  BOTONES ACCIÓN:                                   ", MARGIN);
    println!("{}    A: {:<6} B: {:<6} X: {:<6} Y: {:<6} ", MARGIN, info(&config.buttons.a), info(&config.buttons.b), info(&config.buttons.x), info(&config.buttons.y));
    println!("{}╚════════════════════════════════════════════════════╝", MARGIN);
    println!("\n{} Presione [ENTER] para volver...", MARGIN);
    
    let mut _pausa = String::new();
    io::stdin().read_line(&mut _pausa).ok();
}

fn pedir_dato(label: &str, actual: &mut Option<String>) {
    loop {
        print!("\n{}>> {}: ", MARGIN, label);
        io::stdout().flush().unwrap();
        let mut input = String::new();
        io::stdin().read_line(&mut input).unwrap();
        let valor = input.trim().to_string();

        if es_input_valido(&valor) {
            *actual = if valor.is_empty() { None } else { Some(valor) };
            break;
        } else {
            println!("{}!! No reconozco '{}'.", MARGIN, valor);
        }
    }
}

fn menu_configuracion(config: &mut Config) {
    loop {
        clear_console();
        println!("\n{}┌────────────────────────────────────────────────────┐", MARGIN);
        println!("{}│             PANEL DE CONFIGURACIÓN                 │", MARGIN);
        println!("{}├────────────────────────────────────────────────────┤", MARGIN);
        println!("{}│  [1] Editar STICKS            [3] Editar GATILLOS  │", MARGIN);
        println!("{}│  [2] Editar BOTONES           [4] Editar CRUCETA   │", MARGIN);
        println!("{}│                                                    │", MARGIN);
        println!("{}│  [G] GUARDAR Y SALIR          [C] CANCELAR         │", MARGIN);
        println!("{}└────────────────────────────────────────────────────┘", MARGIN);
        
        print!("\n{}Selección: ", MARGIN);
        io::stdout().flush().unwrap();
        
        let mut op = String::new();
        io::stdin().read_line(&mut op).unwrap();
        let seleccion = op.trim().to_uppercase();

        match seleccion.as_str() {
            "1" => {
                clear_console();
                pedir_dato("LS X", &mut config.sticks.ls_x);
                pedir_dato("LS Y", &mut config.sticks.ls_y);
                pedir_dato("LS Click (L3)", &mut config.buttons.ls_click);
                pedir_dato("RS X", &mut config.sticks.rs_x);
                pedir_dato("RS Y", &mut config.sticks.rs_y);
                pedir_dato("RS Click (R3)", &mut config.buttons.rs_click);
            },
            "2" => {
                clear_console();
                pedir_dato("Botón A", &mut config.buttons.a);
                pedir_dato("Botón B", &mut config.buttons.b);
                pedir_dato("Botón X", &mut config.buttons.x);
                pedir_dato("Botón Y", &mut config.buttons.y);
                pedir_dato("Start", &mut config.buttons.start);
                pedir_dato("Back", &mut config.buttons.back);
                pedir_dato("Botón Xbox", &mut config.buttons.xbox);
            },
            "3" => {
                clear_console();
                pedir_dato("Gatillo RT", &mut config.triggers.right_trigger);
                pedir_dato("Gatillo LT", &mut config.triggers.left_trigger);
                pedir_dato("Bumper RB", &mut config.buttons.rb);
                pedir_dato("Bumper LB", &mut config.buttons.lb);
            },
            "4" => {
                clear_console();
                pedir_dato("D-Pad Up", &mut config.buttons.dpad_up);
                pedir_dato("D-Pad Down", &mut config.buttons.dpad_down);
                pedir_dato("D-Pad Left", &mut config.buttons.dpad_left);
                pedir_dato("D-Pad Right", &mut config.buttons.dpad_right);
            },
            "G" => { config.save(); break; },
            "C" => break,
            _ => continue,
        }
    }
}

fn imprimir_inicio_limpio() {
    clear_console();
    println!("\n{}╔════════════════════════════════════════╗", MARGIN);
    println!("{}║        XBOX VIRTUAL CONTROLLER         ║", MARGIN);
    println!("{}╠════════════════════════════════════════╣", MARGIN);
    println!("{}║  By: Shark                             ║", MARGIN);
    println!("{}╠════════════════════════════════════════╣", MARGIN);
    println!("{}║  [C]   Configurar Mapeo                ║", MARGIN);
    println!("{}║  [V]   Ver Mapeo Actual                ║", MARGIN);
    println!("{}║  [F10] Pausar / Reanudar               ║", MARGIN);
    println!("{}║  [F12] Salir completamente             ║", MARGIN);
    println!("{}╚════════════════════════════════════════╝", MARGIN);
}

fn main() {
    print!("\x1B]0;Xbox Virtual Controller\x07");
    let _ = io::stdout().flush();
    let mut emulator = Emulator::new();
    let device_state = DeviceState::new();
    let mut config = Config::new();
    let mut is_paused = false;
    let mut last_c = false;
    let mut last_v = false;
    let mut last_f10 = false;

    let (mut c_lsx, mut c_lsy, mut c_rsx, mut c_rsy, mut c_lt, mut c_rt) = (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    imprimir_inicio_limpio();

    loop {
        let keys = device_state.get_keys();
        let mouse = device_state.get_mouse();
        let enfocada = es_ventana_activa();

        if keys.contains(&Keycode::F12) && enfocada { break; }

        let key_c = keys.contains(&Keycode::C) && enfocada;
        if key_c && !last_c {
            menu_configuracion(&mut config);
            imprimir_inicio_limpio();
        }
        last_c = key_c;

        let key_v = keys.contains(&Keycode::V) && enfocada;
        if key_v && !last_v {
            mostrar_mapeo_actual(&config);
            imprimir_inicio_limpio();
        }
        last_v = key_v;

        let f10 = keys.contains(&Keycode::F10) && enfocada;
        if f10 && !last_f10 { 
            is_paused = !is_paused; 
            println!("{}ESTADO: {}", MARGIN, if is_paused {"PAUSADO"} else {"EJECUTANDO"});
        }
        last_f10 = f10;

        if is_paused { 
            thread::sleep(Duration::from_millis(100)); 
            continue; 
        }

        let get_v = |m: &Option<String>| -> f32 {
            if let Some(s) = m {
                match s.to_uppercase().as_str() {
                    "MOUSEX" => Processor::normalize(mouse.coords.0, 0, config.screen_width),
                    "MOUSEY" => Processor::normalize(mouse.coords.1, 0, config.screen_height),
                    "MOUSE1" => if mouse.button_pressed[1] { 1.0 } else { 0.0 },
                    "MOUSE2" => if mouse.button_pressed[2] { 1.0 } else { 0.0 },
                    _ => if let Ok(k) = Keycode::from_str(s) { if keys.contains(&k) { 1.0 } else { 0.0 } } else { 0.0 }
                }
            } else { 0.0 }
        };

        c_lsx = Processor::lerp(c_lsx, get_v(&config.sticks.ls_x), config.smoothing_factor);
        c_lsy = Processor::lerp(c_lsy, get_v(&config.sticks.ls_y), config.smoothing_factor);
        c_rsx = Processor::lerp(c_rsx, get_v(&config.sticks.rs_x), config.smoothing_factor);
        c_rsy = Processor::lerp(c_rsy, get_v(&config.sticks.rs_y), config.smoothing_factor);

        emulator.gamepad.thumb_lx = Processor::to_thumb_val(c_lsx);
        emulator.gamepad.thumb_ly = Processor::to_thumb_val(c_lsy * -1.0);
        emulator.gamepad.thumb_rx = Processor::to_thumb_val(c_rsx);
        emulator.gamepad.thumb_ry = Processor::to_thumb_val(c_rsy * -1.0);

        let mut b: u16 = 0;
        let ck = |m: &Option<String>| get_v(m) > 0.5;
        if ck(&config.buttons.a) { b |= XButtons::A; }
        if ck(&config.buttons.b) { b |= XButtons::B; }
        if ck(&config.buttons.x) { b |= XButtons::X; }
        if ck(&config.buttons.y) { b |= XButtons::Y; }
        if ck(&config.buttons.lb) { b |= XButtons::LB; }
        if ck(&config.buttons.rb) { b |= XButtons::RB; }
        if ck(&config.buttons.start) { b |= XButtons::START; }
        if ck(&config.buttons.back) { b |= XButtons::BACK; }
        if ck(&config.buttons.xbox) { b |= XButtons::GUIDE; }
        if ck(&config.buttons.dpad_up) { b |= XButtons::UP; }
        if ck(&config.buttons.dpad_down) { b |= XButtons::DOWN; }
        if ck(&config.buttons.dpad_left) { b |= XButtons::LEFT; }
        if ck(&config.buttons.dpad_right) { b |= XButtons::RIGHT; }
        if ck(&config.buttons.ls_click) { b |= XButtons::LTHUMB; }
        if ck(&config.buttons.rs_click) { b |= XButtons::RTHUMB; }
        emulator.gamepad.buttons = vigem_client::XButtons(b);

        c_rt = Processor::lerp(c_rt, get_v(&config.triggers.right_trigger), config.smoothing_factor);
        c_lt = Processor::lerp(c_lt, get_v(&config.triggers.left_trigger), config.smoothing_factor);
        emulator.gamepad.right_trigger = Processor::to_trigger_val(c_rt.abs());
        emulator.gamepad.left_trigger = Processor::to_trigger_val(c_lt.abs());

        let _ = emulator.target.update(&emulator.gamepad);
        thread::sleep(Duration::from_millis(2));
    }
}