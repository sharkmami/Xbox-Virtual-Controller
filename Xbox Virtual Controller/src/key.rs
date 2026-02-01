pub struct Key {
    pub value: bool,
    locked: bool
}

impl Key {
    pub fn new() -> Self {
        Key {
            value: false,
            locked: false
        }
    }

    pub fn update(&mut self, pressing: bool) {
        if self.locked {
            if !pressing {
                self.locked = false;
            }

            return;
        }

        if pressing {
            self.locked = true;
            self.value = !self.value;
        }
    }
}