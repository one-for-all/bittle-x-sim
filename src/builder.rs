use gorilla_physics::{
    WORLD_FRAME,
    hybrid::{
        Hybrid,
        articulated::Articulated,
        control::NullArticulatedController,
        mesh::URDFMeshes,
        rigid::helper::{build_joint, build_rigid},
    },
    interface::{hybrid::InterfaceHybrid, util::read_web_file},
    joint::Joint,
    na::Vector3,
    spatial::transform::Transform3D,
    types::Float,
};
use urdf_rs::Robot;
use wasm_bindgen::prelude::wasm_bindgen;

use crate::control::BittleXEsp32Controller;

pub fn build_bittle_x(meshes: &mut URDFMeshes, urdf: &Robot) -> Hybrid {
    let mut state = Hybrid::empty();

    #[rustfmt::skip]
    let zero_position_angles = [
        135, 225, 135, 135,
        190, 80, 190, 80,
        190, 80, 80, 190
    ];

    let body_frame = "body";
    let body = build_rigid(body_frame, "chassis", urdf, meshes);
    let body_joint = Joint::new_fixed(Transform3D::move_z(body_frame, WORLD_FRAME, 0.05));

    // left-front
    let lf_leg_frame = "left_front_leg";
    let lf_leg = build_rigid(lf_leg_frame, "bittle_x_leg", urdf, meshes);
    let lf_leg_joint = build_joint(
        lf_leg_frame,
        body_frame,
        "left_front",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[4] as Float).to_radians(),
    );

    let lf_shank_frame = "left_front_shank";
    let lf_shank = build_rigid(lf_shank_frame, "bittle_x_shank_left", urdf, meshes);
    let lf_shank_joint = build_joint(
        lf_shank_frame,
        lf_leg_frame,
        "left_front_knee",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[8] as Float).to_radians(),
    );

    // right-front
    let rf_leg_frame = "right_front_leg";
    let rf_leg = build_rigid(rf_leg_frame, "bittle_x_leg_2", urdf, meshes);
    let rf_leg_joint = build_joint(
        rf_leg_frame,
        body_frame,
        "right_front",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[5] as Float).to_radians(),
    );

    let rf_shank_frame = "right_front_shank";
    let rf_shank = build_rigid(rf_shank_frame, "bittle_x_shank_right", urdf, meshes);
    let rf_shank_joint = build_joint(
        rf_shank_frame,
        rf_leg_frame,
        "right_front_knee",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[9] as Float).to_radians(),
    );

    let articulated = Articulated::new(
        vec![body, lf_leg, lf_shank, rf_leg, rf_shank],
        vec![
            body_joint,
            lf_leg_joint,
            lf_shank_joint,
            rf_leg_joint,
            rf_shank_joint,
        ],
    );

    state.add_articulated(articulated);

    state
}

// #[cfg(target_arch = "wasm32")]
#[allow(non_snake_case)]
#[wasm_bindgen]
pub async fn createBittleX() -> InterfaceHybrid {
    let urdf_path = "robot.urdf";
    let urdf_file = read_web_file(urdf_path).await;
    let urdf_robot = urdf_rs::read_from_string(&urdf_file).unwrap();

    let mut meshes = URDFMeshes::new(&urdf_robot).await;

    let mut state = build_bittle_x(&mut meshes, &urdf_robot);

    let controller = BittleXEsp32Controller::new().await;
    // let controller = NullArticulatedController {};
    state.set_controller(0, controller);

    InterfaceHybrid::new(state)
}
