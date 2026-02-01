pub struct Processor;

impl Processor {
    pub fn normalize(value: i32, min: i32, max: i32) -> f32 {
        let norm = 2.0 * ((value as f32 - min as f32) / (max as f32 - min as f32)) - 1.0;
        norm.min(1.0).max(-1.0)
    }

    pub fn lerp(current: f32, target: f32, speed: f32) -> f32 {
        current + speed * (target - current)
    }

    pub fn to_thumb_val(value: f32) -> i16 {
        (value * i16::MAX as f32) as i16
    }

    pub fn to_trigger_val(value: f32) -> u8 {
        (value * 255.0) as u8
    }
}