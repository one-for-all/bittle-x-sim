//! The scene `createBittleX` builds, as a wasm module Node can drive: the same
//! release build and wasm-opt pass the page gets, so what `run.mjs` measures is
//! what the browser's step costs. See ../README.md.
#![allow(dead_code, unused_imports)]
#[path = "../../../../src/can.rs"]
mod can;
#[path = "../../../../src/builder.rs"]
mod builder;
mod control;

use esp32rs::symbols::Symbols;
use gorilla_physics::{
    collision::halfspace::HalfSpace, hybrid::Hybrid, hybrid::mesh::URDFMeshes, na::Vector3,
    na::vector, types::Float,
};
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct Bench {
    state: Hybrid,
    input: Vec<Float>,
}

#[wasm_bindgen]
impl Bench {
    #[wasm_bindgen(constructor)]
    pub fn new(
        urdf: &str,
        rom1: Vec<u8>,
        rom0: Vec<u8>,
        rom_symbols: &str,
        boot: Vec<u8>,
        part: Vec<u8>,
        app: Vec<u8>,
        symbols: &str,
        boot_symbols: &str,
    ) -> Bench {
        let urdf = urdf_rs::read_from_string(urdf).unwrap();
        let mut meshes = URDFMeshes::empty();
        let mut state = builder::build_bittle_x(&mut meshes, &urdf);
        let mut syms = Symbols::new();
        syms.add(rom_symbols);
        syms.add(symbols);
        syms.add(boot_symbols);
        let controller = control::BittleXEsp32Controller::from_images(rom1, rom0, boot, part, app, syms);
        state.set_controller(0, controller);
        state.add_articulated(can::build_beverage_can(&vector![0.2, 0.1, 0.]));
        state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.));
        Bench { state, input: vec![0.; 3] }
    }

    pub fn step(&mut self, n: usize) {
        let dt: Float = 1. / 60. / 60.;
        for _ in 0..n {
            self.state.step(dt, &self.input);
        }
    }

    pub fn send_uart(&mut self, s: &str) {
        self.state.controllers[0].send_uart(s);
    }

    /// Milliseconds per phase since the last call, then the step count.
    pub fn profile(&mut self) -> Vec<f64> {
        let (totals, n) = self.state.profile.take();
        let mut v = totals.to_vec();
        v.push(n as f64);
        v
    }

    pub fn fingerprint(&self) -> String {
        let b = &self.state.articulated[0].bodies[0];
        let c = &self.state.articulated[1].bodies[0];
        let uart = self.state.controllers[0].get_uart();
        let mut h: u64 = 0xcbf29ce484222325;
        for x in uart.bytes() {
            h ^= x as u64;
            h = h.wrapping_mul(0x100000001b3);
        }
        format!(
            "pose t=[{:.12}, {:.12}, {:.12}] r=[{:.12}, {:.12}, {:.12}, {:.12}]\ncan t=[{:.12}, {:.12}, {:.12}]\nuart len {} hash {:016x}",
            b.pose.translation.x, b.pose.translation.y, b.pose.translation.z,
            b.pose.rotation.i, b.pose.rotation.j, b.pose.rotation.k, b.pose.rotation.w,
            c.pose.translation.x, c.pose.translation.y, c.pose.translation.z,
            uart.len(), h
        )
    }
}
