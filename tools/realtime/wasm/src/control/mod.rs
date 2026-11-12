// The controller itself, plus a constructor taking the images as bytes: in
// wasm `new` fetches them, which Node has no page to fetch from.
include!("../../../../../src/control/mod.rs");

impl BittleXEsp32Controller {
    pub fn from_images(
        rom1: Vec<u8>,
        rom0: Vec<u8>,
        boot: Vec<u8>,
        part: Vec<u8>,
        app: Vec<u8>,
        symbols: Symbols,
    ) -> Self {
        Self {
            esp32: ESP32::new(rom1, rom0, boot, part, app, symbols),
            servos: [PetoiP1S::new(); 9],
            uart_payload: VecDeque::new(),
        }
    }
}
