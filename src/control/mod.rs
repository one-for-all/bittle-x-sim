use std::{cell::RefCell, collections::VecDeque, rc::Rc};

use esp32rs::{
    esp32::run_instruction::Xtensa,
    esp32::{CPU_FREQUENCY, CPU_SLOWDOWN_FACTOR, ESP32},
    i2c::device::I2cDevice,
    servo::petoi_p1s::PetoiP1S,
    symbols::Symbols,
};
use gibbon_electronics::imu::icm42670::ICM42670;
use gorilla_physics::{
    GRAVITY, PI,
    control::servo,
    hybrid::{Rigid, articulated::Articulated, control::ArticulatedController},
    joint::Joint,
    types::Float,
};

use nalgebra::{DVector, Vector3, dvector};

pub mod servo_control;

pub struct BittleXEsp32Controller {
    pub esp32: ESP32,

    pub servos: [PetoiP1S; 9],

    /// The ICM-42670 on the BiBoard V1, AP_AD0 high (0x69), on `Wire`
    /// (SDA 21, SCL 22). Shared: the chip's bus reads its registers, and this
    /// controller tells it how the body moves.
    imu: Rc<RefCell<ICM42670>>,
    /// How the IMU moved at the previous step. `None` until there is one.
    imu_track: Option<ImuTrack>,

    uart_payload: VecDeque<u8>, // data pending to be fed into esp32 uart0
}

/// The IMU as the chip's I2C bus sees it.
struct ImuOnBus(Rc<RefCell<ICM42670>>);

impl I2cDevice for ImuOnBus {
    fn address(&self) -> u8 {
        self.0.borrow().address()
    }
    fn start(&mut self, read: bool) {
        self.0.borrow_mut().i2c_start(read);
    }
    fn write(&mut self, byte: u8) {
        self.0.borrow_mut().i2c_write(byte);
    }
    fn read(&mut self) -> u8 {
        self.0.borrow_mut().i2c_read()
    }
    fn stop(&mut self) {
        self.0.borrow_mut().i2c_stop();
    }
}

/// A body-frame vector in the IMU's axes. The chassis frame of the URDF has
/// the head along -y and the left side along +x (the head joint sits at
/// y = -0.044, the left legs at x = +0.042), while the firmware reads the chip
/// as x toward the tail, y right, z up. Its ICM42670 driver takes pitch as
/// asin(x / g), so this is the frame in which the head going up reads as
/// negative pitch, as the skills expect: sit holds -30, str and buttUp lean
/// forward at +20 and +15, and `getImuException` calls a pitch under -50
/// lifted. Read the other way round, the posture balance pushes a sitting
/// robot further off its pose until it trembles. Roll's sign doesn't matter
/// to the exceptions: the flip test takes |roll| > 85 with z below -0.1 g,
/// and the side test |y|.
fn body_to_imu(v: Vector3<Float>) -> [Float; 3] {
    [v.y, -v.x, v.z]
}

/// The corner of the low-pass that stands in for the robot's give between a
/// foot striking and the board feeling it, in Hz.
///
/// The solver's contacts are rigid: a foot landing or a servo snapping to a
/// pose changes the body's velocity within one step, which differenced reads
/// as several g for one step. The chip's own low-pass is quicker than that
/// and passes most of it, so `getImuException` would see shoves -- a jump of
/// half a g on x or y is all it takes while holding a posture -- where nothing
/// pushed. A real robot's legs, plastic and gear lash spread that over tens of
/// milliseconds; a push by a hand lasts longer still, and gets through.
const MOUNT_BANDWIDTH: Float = 10.;

/// How the IMU moved at a step, world frame.
#[derive(Clone, Copy)]
struct ImuTrack {
    /// Its velocity, from which the next step's acceleration is differenced.
    velocity: Vector3<Float>,
    /// Its acceleration, smoothed by the mount.
    acceleration: Vector3<Float>,
}

/// What the IMU on `body` feels -- specific force in m/s^2 and angular
/// velocity in rad/s, in the chip's axes -- along with how it moved, which the
/// next step carries on from.
///
/// The chip is taken to sit at the body frame's origin, on the main board; a
/// few centimetres either way only changes the rotational part of what it
/// feels, which is small next to gravity at the rates this robot turns. Its
/// acceleration is the step-to-step change in its velocity -- that is all a
/// velocity-level solver has -- passed through the mount's low-pass. That is
/// done in the world frame, before gravity is added: tilting still turns
/// gravity across the chip's axes at once, so the flip and lift tests do not
/// lag.
fn imu_motion(
    body: &Rigid,
    last: Option<ImuTrack>,
    dt: Float,
) -> ([Float; 3], [Float; 3], ImuTrack) {
    let rotation = body.pose.rotation;
    let position = body.pose.translation;
    // The twist's linear part is the velocity of the point at the world
    // origin; carry it over to the body.
    let velocity = body.twist.linear + body.twist.angular.cross(&position);

    let acceleration = match last {
        Some(last) if dt > 0. => {
            let raw = (velocity - last.velocity) / dt;
            // Exact discretisation of a first-order low-pass.
            let alpha = 1. - (-2. * PI * MOUNT_BANDWIDTH * dt).exp();
            last.acceleration + (raw - last.acceleration) * alpha
        }
        Some(last) => last.acceleration,
        None => Vector3::zeros(),
    };

    // What an accelerometer senses is the acceleration minus gravity: +1 g up
    // when nothing is moving.
    let specific_force = acceleration + Vector3::new(0., 0., GRAVITY);
    let specific_force = rotation.inverse_transform_vector(&specific_force);
    let angular_velocity = rotation.inverse_transform_vector(&body.twist.angular);

    (
        body_to_imu(specific_force),
        body_to_imu(angular_velocity),
        ImuTrack {
            velocity,
            acceleration,
        },
    )
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

        let mut esp32 = ESP32::new(
            rom1_data,
            rom0_data,
            bootloader_data,
            partition_table_data,
            app_data,
            symbols,
        );
        let imu = Rc::new(RefCell::new(ICM42670::new(true)));
        esp32.attach_i2c_device(Box::new(ImuOnBus(imu.clone())));

        Self {
            esp32,
            servos: [PetoiP1S::new(); 9],
            imu,
            imu_track: None,
            uart_payload: VecDeque::new(),
        }
    }

    /// Tell the IMU how the body is moving, and let it sample.
    fn sense_motion(&mut self, dt: Float, articulated: &Articulated) {
        let (specific_force, angular_velocity, track) =
            imu_motion(&articulated.bodies[0], self.imu_track, dt);
        self.imu_track = Some(track);

        let mut imu = self.imu.borrow_mut();
        imu.measure(specific_force, angular_velocity);
        imu.step(dt);
    }
}

impl ArticulatedController for BittleXEsp32Controller {
    fn step(&mut self, dt: Float, articulated: &Articulated) {
        // let Hz = (240_000_000 / CPU_SLOWDOWN_FACTOR) as Float; // 240 Mhz
        let Hz = ((CPU_FREQUENCY * 1000_000) / CPU_SLOWDOWN_FACTOR) as Float;
        let n_steps = (dt * Hz) as usize;

        let pins = [18, 23, 19, 4, 15, 12, 13, 33, 32];

        self.sense_motion(dt, articulated);

        let mut count = 0;
        let max_count = 10; // 100
        for _ in 0..n_steps {
            self.esp32.step();
            // Hold pending uart data until the firmware serial driver is up,
            // e.g. a command sent while the esp32 is still booting.
            if !self.uart_payload.is_empty() && self.esp32.uart_ready() {
                if let Some(data) = self.uart_payload.pop_front() {
                    self.esp32.feed_uart(data);
                }
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
            let mut torque = self.servos[i].torque();
            torque -= v * 1e-4; // model joint friction

            torques.push(torque);
        }

        DVector::from_vec(torques)
    }

    fn reboot_esp32(&mut self, app_bin: Vec<u8>, new_symbols: &str) {
        // TODO: add reboot function to ESP32
        let mut symbols = self.esp32.symbols.clone();
        symbols.add(new_symbols);

        let rom1_data: Vec<u8> = self.esp32.rom1_data.clone();
        let rom0_data: Vec<u8> = self.esp32.rom0_data.clone();
        let bootloader_data: Vec<u8> = self.esp32.bootloader_data.clone();
        let partition_table_data: Vec<u8> = self.esp32.partition_table_data.clone();
        let app_data: Vec<u8> = app_bin;

        let mut esp32 = ESP32::new(
            rom1_data,
            rom0_data,
            bootloader_data,
            partition_table_data,
            app_data,
            symbols,
        );
        // The same part, on the same bus: only the chip was replaced.
        esp32.attach_i2c_device(Box::new(ImuOnBus(self.imu.clone())));

        self.esp32 = esp32;
        for servo in self.servos.iter_mut() {
            servo.reset();
        }
        self.imu_track = None;
    }

    fn reboot(&mut self, _code: &str) {
        self.esp32.reboot(); // keeps the IMU attached
        for servo in self.servos.iter_mut() {
            servo.reset();
        }
        self.imu_track = None;
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

    /// Return the content in UART
    fn get_uart(&self) -> String {
        String::from_utf8(self.esp32.tx_FIFO.clone()).unwrap()
    }

    /// Send UART data to esp32
    fn send_uart(&mut self, payload: &str) {
        self.uart_payload.extend(String::from(payload).into_bytes());
    }
}
