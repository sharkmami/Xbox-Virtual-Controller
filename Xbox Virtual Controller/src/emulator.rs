use vigem_client::{Client, TargetId, XGamepad, Xbox360Wired};

pub struct Emulator {
    pub target: Xbox360Wired<Client>,
    pub gamepad: XGamepad,
}

impl Emulator {
    pub fn new() -> Self {
        let client = Client::connect().unwrap();
        let id = TargetId::XBOX360_WIRED;
        let mut target = Xbox360Wired::new(client, id);

        target.plugin().unwrap();
        target.wait_ready().unwrap();

        Emulator {
            target,
            gamepad: XGamepad::default(),
        }
    }

    pub fn emulate(&mut self) {
        self.target.update(&self.gamepad).unwrap();
    }
}
