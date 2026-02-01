use serde::{Deserialize, Serialize};
use std::fs;

#[derive(Debug)]
pub struct Config {
    pub screen_width: i32,
    pub screen_height: i32,
    pub smoothing_factor: f32,
    pub sticks: StickConfig,
    pub buttons: ButtonMapping,
    pub triggers: TriggerMapping,
}

#[derive(Debug, Clone)]
pub struct StickConfig {
    pub ls_x: Option<String>, pub ls_y: Option<String>,
    pub rs_x: Option<String>, pub rs_y: Option<String>,
}

#[derive(Debug, Clone)]
pub struct ButtonMapping {
    pub a: Option<String>, pub b: Option<String>, pub x: Option<String>, pub y: Option<String>,
    pub lb: Option<String>, pub rb: Option<String>,
    pub start: Option<String>, pub back: Option<String>,
    pub xbox: Option<String>,
    pub dpad_up: Option<String>, pub dpad_down: Option<String>,
    pub dpad_left: Option<String>, pub dpad_right: Option<String>,
    pub ls_click: Option<String>, pub rs_click: Option<String>,
}

#[derive(Debug, Clone)]
pub struct TriggerMapping {
    pub left_trigger: Option<String>,
    pub right_trigger: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Default)]
pub struct ConfigToml {
    pub screen_width: Option<i32>,
    pub screen_height: Option<i32>,
    pub smoothing_factor: Option<f32>,
    pub sticks: Option<StickMappingToml>,
    pub buttons: Option<ButtonMappingToml>,
    pub triggers: Option<TriggerMappingToml>,
}

#[derive(Serialize, Deserialize, Debug, Default)] pub struct StickMappingToml { pub ls_x: Option<String>, pub ls_y: Option<String>, pub rs_x: Option<String>, pub rs_y: Option<String> }
#[derive(Serialize, Deserialize, Debug, Default)] 
pub struct ButtonMappingToml { 
    pub a: Option<String>, pub b: Option<String>, pub x: Option<String>, pub y: Option<String>, 
    pub lb: Option<String>, pub rb: Option<String>, pub start: Option<String>, pub back: Option<String>, 
    pub xbox: Option<String>,
    pub dpad_up: Option<String>, pub dpad_down: Option<String>, pub dpad_left: Option<String>, pub dpad_right: Option<String>, 
    pub ls_click: Option<String>, pub rs_click: Option<String> 
}
#[derive(Serialize, Deserialize, Debug, Default)] pub struct TriggerMappingToml { pub left_trigger: Option<String>, pub right_trigger: Option<String> }

impl Config {
    pub fn new() -> Self {
        let content = fs::read_to_string("./Config.toml").unwrap_or_default();
        let raw: ConfigToml = toml::from_str(&content).unwrap_or_default();
        let s = raw.sticks.unwrap_or_default();
        let b = raw.buttons.unwrap_or_default();
        let t = raw.triggers.unwrap_or_default();

        Config {
            screen_width: raw.screen_width.unwrap_or(1920),
            screen_height: raw.screen_height.unwrap_or(1080),
            smoothing_factor: raw.smoothing_factor.unwrap_or(0.15),
            sticks: StickConfig { ls_x: s.ls_x, ls_y: s.ls_y, rs_x: s.rs_x, rs_y: s.rs_y },
            buttons: ButtonMapping { 
                a: b.a, b: b.b, x: b.x, y: b.y, lb: b.lb, rb: b.rb, 
                start: b.start, back: b.back, xbox: b.xbox,
                dpad_up: b.dpad_up, dpad_down: b.dpad_down, dpad_left: b.dpad_left, dpad_right: b.dpad_right, 
                ls_click: b.ls_click, rs_click: b.rs_click 
            },
            triggers: TriggerMapping { left_trigger: t.left_trigger, right_trigger: t.right_trigger },
        }
    }

    pub fn save(&self) {
        let toml_string = format!(
r#"screen_width = {}
screen_height = {}
smoothing_factor = {}

[sticks]
ls_x = "{}"
ls_y = "{}"
rs_x = "{}"
rs_y = "{}"

[triggers]
left_trigger = "{}"
right_trigger = "{}"

[buttons]
a = "{}"
b = "{}"
x = "{}"
y = "{}"
lb = "{}"
rb = "{}"
start = "{}"
back = "{}"
xbox = "{}"
dpad_up = "{}"
dpad_down = "{}"
dpad_left = "{}"
dpad_right = "{}"
ls_click = "{}"
rs_click = "{}"
"#,
            self.screen_width, self.screen_height, self.smoothing_factor,
            self.sticks.ls_x.as_deref().unwrap_or(""), self.sticks.ls_y.as_deref().unwrap_or(""), 
            self.sticks.rs_x.as_deref().unwrap_or(""), self.sticks.rs_y.as_deref().unwrap_or(""),
            self.triggers.left_trigger.as_deref().unwrap_or(""), self.triggers.right_trigger.as_deref().unwrap_or(""),
            self.buttons.a.as_deref().unwrap_or(""), self.buttons.b.as_deref().unwrap_or(""), 
            self.buttons.x.as_deref().unwrap_or(""), self.buttons.y.as_deref().unwrap_or(""),
            self.buttons.lb.as_deref().unwrap_or(""), self.buttons.rb.as_deref().unwrap_or(""), 
            self.buttons.start.as_deref().unwrap_or(""), self.buttons.back.as_deref().unwrap_or(""),
            self.buttons.xbox.as_deref().unwrap_or(""),
            self.buttons.dpad_up.as_deref().unwrap_or(""), self.buttons.dpad_down.as_deref().unwrap_or(""), 
            self.buttons.dpad_left.as_deref().unwrap_or(""), self.buttons.dpad_right.as_deref().unwrap_or(""),
            self.buttons.ls_click.as_deref().unwrap_or(""), self.buttons.rs_click.as_deref().unwrap_or("")
        );
        fs::write("./Config.toml", toml_string).ok();
    }
}