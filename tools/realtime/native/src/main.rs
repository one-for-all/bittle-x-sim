//! Native stand-in for the browser run loop: the scene `createBittleX` builds
//! (robot, ESP32 board, can, ground), stepped at the substep
//! `simulator.run(60, 0)` uses (1/3600 s), with bittle-x-sim's own sources
//! compiled in through #[path]. See ../README.md.
//!
//! Usage: bittle-realtime [sim seconds] [time:command ...]
//!   e.g. bittle-realtime 8 4.0:kwkF
//! Env: QUIET=1 (totals only), UART=1 (print the console), NOCAN=1.
#![allow(dead_code, unused_imports)]

#[path = "../../../../src/can.rs"]
mod can;
#[path = "../../../../src/builder.rs"]
mod builder;
#[path = "../../../../src/control/mod.rs"]
mod control;

use std::time::Instant;

use gorilla_physics::{
    collision::halfspace::HalfSpace, hybrid::control::ArticulatedController,
    hybrid::mesh::URDFMeshes, hybrid::profile::PHASE_NAMES, na::Vector3, na::vector,
    types::Float,
};

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let t_final: Float = args.get(1).map(|s| s.parse().unwrap()).unwrap_or(6.0);
    // "t:cmd" pairs, e.g. 3.0:kwkF
    let cmds: Vec<(Float, String)> = args
        .iter()
        .skip(2)
        .map(|a| {
            let (t, c) = a.split_once(':').unwrap();
            (t.parse().unwrap(), c.to_string())
        })
        .collect();
    let quiet = std::env::var("QUIET").is_ok();

    // Everything is read from where the page fetches it.
    std::env::set_current_dir(concat!(env!("CARGO_MANIFEST_DIR"), "/../../../www/static")).unwrap();
    let urdf_file = std::fs::read_to_string("robot.urdf").unwrap();
    let urdf = urdf_rs::read_from_string(&urdf_file).unwrap();
    let mut meshes = URDFMeshes::empty();
    let mut state = builder::build_bittle_x(&mut meshes, &urdf);

    let controller =
        futures::executor::block_on(control::BittleXEsp32Controller::new());
    state.set_controller(0, controller);
    if std::env::var("NOCAN").is_err() {
        let can = can::build_beverage_can(&vector![0.2, 0.1, 0.]);
        state.add_articulated(can);
    }
    state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.));

    let dt: Float = 1. / 60. / 60.;
    let steps = (t_final / dt).round() as usize;
    let window = 1800; // 0.5 sim s
    let input = vec![0.; 3];

    let start = Instant::now();
    let mut wstart = Instant::now();
    let mut next_cmd = 0;
    for s in 0..steps {
        let t = s as Float * dt;
        while next_cmd < cmds.len() && t >= cmds[next_cmd].0 {
            state.controllers[0].send_uart(&cmds[next_cmd].1);
            next_cmd += 1;
        }
        state.step(dt, &input);
        if (s + 1) % window == 0 {
            let wall = wstart.elapsed().as_secs_f64();
            let (totals, n) = state.profile.take();
            if !quiet {
                let mut parts: Vec<(usize, Float)> = totals.iter().cloned().enumerate().collect();
                parts.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap());
                let top: Vec<String> = parts
                    .iter()
                    .take(5)
                    .map(|(i, ms)| format!("{} {:.0}us", PHASE_NAMES[*i], 1000. * ms / n as Float))
                    .collect();
                println!(
                    "t={:5.2} rt={:.2} step={:.0}us | {}",
                    (s + 1) as Float * dt,
                    (window as Float * dt) / wall,
                    1e6 * wall / window as f64,
                    top.join(", ")
                );
            }
            wstart = Instant::now();
        }
    }
    let wall = start.elapsed().as_secs_f64();
    println!(
        "TOTAL sim {:.2}s wall {:.2}s realtime {:.3}",
        t_final,
        wall,
        t_final / wall
    );

    // Fingerprint the outcome so an optimisation can be checked to leave the
    // trajectory and the firmware's output untouched.
    let body = &state.articulated[0].bodies[0];
    println!(
        "pose t=[{:.12}, {:.12}, {:.12}] r=[{:.12}, {:.12}, {:.12}, {:.12}]",
        body.pose.translation.x,
        body.pose.translation.y,
        body.pose.translation.z,
        body.pose.rotation.i,
        body.pose.rotation.j,
        body.pose.rotation.k,
        body.pose.rotation.w
    );
    let can_body = &state.articulated[state.articulated.len() - 1].bodies[0];
    println!(
        "can t=[{:.12}, {:.12}, {:.12}]",
        can_body.pose.translation.x, can_body.pose.translation.y, can_body.pose.translation.z
    );
    let uart = state.controllers[0].get_uart();
    let mut h: u64 = 0xcbf29ce484222325;
    for b in uart.bytes() {
        h ^= b as u64;
        h = h.wrapping_mul(0x100000001b3);
    }
    println!("uart len {} hash {:016x}", uart.len(), h);
    if std::env::var("UART").is_ok() {
        println!("{}", uart);
    }
}
