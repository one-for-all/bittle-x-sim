use std::time::Instant;

use bittle_x::{
    builder::build_bittle_x,
    control::{BittleXEsp32Controller, servo_control::BittleXServoController},
};
use esp32rs::{plot::plot, util::read_file};
use gorilla_physics::{
    hybrid::{control::NullArticulatedController, mesh::URDFMeshes},
    types::Float,
};

#[tokio::main]
async fn main() {
    let mut meshes = URDFMeshes::empty();
    let urdf_path = "onshape/robot.urdf";
    let urdf_file = read_file(urdf_path);
    let urdf_robot = urdf_rs::read_from_string(&urdf_file).unwrap();
    let mut state = build_bittle_x(&mut meshes, &urdf_robot);

    // let controller = NullArticulatedController {};
    let controller = BittleXEsp32Controller::new().await;
    // let controller = BittleXServoController::new();
    state.set_controller(0, controller);

    let mut data = vec![];
    let mut data2: Vec<Float> = vec![];

    let dt = 1. / 60. / 100.;
    let t_final = 2.0;
    let num_steps = (t_final / dt) as usize;

    let start = Instant::now();

    for s in 0..num_steps {
        state.step(dt, &vec![]);
        data.push(state.articulated[0].q()[1]);
        // data2.push(state.controllers[0].debug_data());
    }

    let duration = start.elapsed();

    // println!("{}", state.controllers[0].get_uart());
    // // plot(&data2, dt, "pin");

    // state.controllers[0].debug();
    println!("Time taken: {:?}", duration);

    println!("angle: {}", state.articulated[0].q()[1].to_degrees());
    plot(&data, dt, "BittleXServoController");
}
