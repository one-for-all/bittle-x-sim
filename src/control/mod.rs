use std::collections::VecDeque;

use esp32rs::{
    esp32::{CPU_FREQUENCY, CPU_SLOWDOWN_FACTOR, ESP32},
    servo::petoi_p1s::PetoiP1S,
    symbols::Symbols,
};
use gorilla_physics::{
    control::servo,
    hybrid::{articulated::Articulated, control::ArticulatedController},
    joint::Joint,
    types::Float,
};
use nalgebra::{DVector, dvector};

pub struct BittleXEsp32Controller {
    pub esp32: ESP32,

    pub servos: [PetoiP1S; 2],

    uart_payload: VecDeque<u8>, // data pending to be fed into esp32 uart0
}

impl BittleXEsp32Controller {
    pub async fn new() -> Self {
        let mut symbols = Symbols::new(); // symbols for printing

        let rom1_data: Vec<u8>;
        let rom0_data: Vec<u8>;
        let bootloader_data: Vec<u8>;
        let partition_table_data: Vec<u8>;
        let app_data: Vec<u8>;

        #[cfg(not(target_arch = "wasm32"))]
        {
            use std::fs;

            use gorilla_physics::util::read_file;

            rom1_data = fs::read("rom/wokwi/rom1.bin").unwrap();
            rom0_data = fs::read("rom/wokwi/rom0.bin").unwrap();
            symbols.add(&read_file("rom/symbols.txt"));

            bootloader_data =
                fs::read("OpenCatEsp32/build/OpenCatEsp32.ino.bootloader.bin").unwrap();
            partition_table_data =
                fs::read("OpenCatEsp32/build/OpenCatEsp32.ino.partitions.bin").unwrap();
            app_data = fs::read("OpenCatEsp32/build/OpenCatEsp32.ino.bin").unwrap();
            symbols.add(&read_file("OpenCatEsp32/build/symbols.txt"));
            symbols.add(&read_file("OpenCatEsp32/bootloader_symbols.txt"));
        }

        #[cfg(target_arch = "wasm32")]
        {
            use gorilla_physics::interface::util::read_web_file;
            use gorilla_physics::interface::util::read_web_file_bytes;

            rom1_data = read_web_file_bytes("rom/wokwi/rom1.bin").await;
            rom0_data = read_web_file_bytes("rom/wokwi/rom0.bin").await;
            symbols.add(&read_web_file("rom/symbols.txt").await);

            bootloader_data =
                read_web_file_bytes("OpenCatEsp32/build/OpenCatEsp32.ino.bootloader.bin").await;
            partition_table_data =
                read_web_file_bytes("OpenCatEsp32/build/OpenCatEsp32.ino.partitions.bin").await;
            app_data = read_web_file_bytes("OpenCatEsp32/build/OpenCatEsp32.ino.bin").await;
            symbols.add(&read_web_file("OpenCatEsp32/build/symbols.txt").await);
            symbols.add(&read_web_file("OpenCatEsp32/bootloader_symbols.txt").await);
        }

        let esp32 = ESP32::new(
            rom1_data,
            rom0_data,
            bootloader_data,
            partition_table_data,
            app_data,
            symbols,
        );

        Self {
            esp32,
            servos: [PetoiP1S::new(); 2],
            uart_payload: VecDeque::new(),
        }
    }
}

impl ArticulatedController for BittleXEsp32Controller {
    fn step(&mut self, dt: Float, articulated: &Articulated) {
        // let Hz = (240_000_000 / CPU_SLOWDOWN_FACTOR) as Float; // 240 Mhz
        let Hz = ((CPU_FREQUENCY * 1000_000) / CPU_SLOWDOWN_FACTOR) as Float;
        let n_steps = (dt * Hz) as usize;

        let pins = [23, 19];

        let mut count = 0;
        let max_count = 10; // 100
        for _ in 0..n_steps {
            self.esp32.step();
            if let Some(data) = self.uart_payload.pop_front() {
                self.esp32.feed_uart(data);
            }

            count += 1;
            if count == max_count {
                count = 0;
                for i in 0..self.servos.len() {
                    if let Some(pin) = self.esp32.read_pin(pins[i]) {
                        self.servos[i].step(max_count as Float / Hz, pin);
                    }
                }
            }
        }
    }

    fn control(&mut self, articulated: &Articulated, input: &Vec<Float>) -> DVector<Float> {
        let mut torques = vec![];
        let body_dof = if let Joint::FloatingJoint(_) = articulated.joints[0] {
            6
        } else {
            0
        };
        for _ in 0..body_dof {
            torques.push(0.);
        }

        let qs = articulated.q();
        let vs = articulated.v();
        for i in 0..self.servos.len() {
            let q;
            let v;
            if body_dof == 0 {
                q = qs[i];
                v = vs[i];
            } else {
                q = qs[body_dof + 1 + i];
                v = vs[body_dof + i];
            }

            self.servos[i].angle = q;
            self.servos[i].vel = v;
            // Note: artificially scale down servo torque.
            // TODO: fix servo torque constant?
            let torque = self.servos[i].torque();
            torques.push(torque);
        }

        DVector::from_vec(torques)
    }

    fn debug(&mut self) {
        self.esp32.print_uart();
        for servo in self.servos.iter() {
            if let Some(command_angle) = servo.command_angle {
                println!("servo command angle: {}", command_angle.to_degrees());
            } else {
                println!("no servo command angle",);
            }
        }
    }
}
