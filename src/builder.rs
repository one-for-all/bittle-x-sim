use gorilla_physics::{
    PI, WORLD_FRAME,
    collision::halfspace::HalfSpace,
    hybrid::{
        Hybrid, Rigid,
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
use nalgebra::vector;
use urdf_rs::Robot;
use wasm_bindgen::prelude::wasm_bindgen;

use crate::control::BittleXEsp32Controller;

pub fn build_bittle_x(meshes: &mut URDFMeshes, urdf: &Robot) -> Hybrid {
    let mut state = Hybrid::empty();
    state.set_friction_mu(0.5);
    state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.));

    #[rustfmt::skip]
    let zero_position_angles = [
        135, 225, 135, 135,
        190, 80, 190, 80,
        190, 80, 80, 190
    ];

    let body_frame = "body";
    let mut body = build_rigid(body_frame, "chassis", urdf, meshes);
    add_bittle_shoulder_collision_sphere(&mut body, urdf);
    add_bittle_tail_collision_point(&mut body, urdf);
    let body_joint = Joint::new_floating(Transform3D::new_xyz_rpy(
        body_frame,
        WORLD_FRAME,
        &vec![0., -0.1, 0.06],
        &vec![0., 0., PI / 2.],
    ));

    let head_frame = "head";
    let mut head = build_rigid(head_frame, "bittle_x_head", urdf, meshes);
    add_bittle_head_collision_sphere(&mut head, urdf);
    let head_joint = build_joint(
        head_frame,
        body_frame,
        "head",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[0] as Float).to_radians(),
    );

    // left-front
    let lf_leg_frame = "left_front_leg";
    let mut lf_leg = build_rigid(lf_leg_frame, "bittle_x_leg", urdf, meshes);
    add_bittle_knee_collision_point(&mut lf_leg, "left_front", urdf);
    let lf_leg_joint = build_joint(
        lf_leg_frame,
        body_frame,
        "left_front",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[4] as Float).to_radians(),
    );

    let lf_shank_frame = "left_front_shank";
    let mut lf_shank = build_rigid(lf_shank_frame, "bittle_x_shank_left", urdf, meshes);
    add_bittle_foot_collision_sphere(&mut lf_shank, "left_front", urdf);
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
    let mut rf_leg = build_rigid(rf_leg_frame, "bittle_x_leg_2", urdf, meshes);
    add_bittle_knee_collision_point(&mut rf_leg, "right_front", urdf);
    let rf_leg_joint = build_joint(
        rf_leg_frame,
        body_frame,
        "right_front",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[5] as Float).to_radians(),
    );

    let rf_shank_frame = "right_front_shank";
    let mut rf_shank = build_rigid(rf_shank_frame, "bittle_x_shank_right", urdf, meshes);
    add_bittle_foot_collision_sphere(&mut rf_shank, "right_front", urdf);
    let rf_shank_joint = build_joint(
        rf_shank_frame,
        rf_leg_frame,
        "right_front_knee",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[9] as Float).to_radians(),
    );

    // right-back
    let rb_leg_frame = "right_back_leg";
    let mut rb_leg = build_rigid(rb_leg_frame, "bittle_x_leg_3", urdf, meshes);
    add_bittle_knee_collision_point(&mut rb_leg, "right_back", urdf);
    let rb_leg_joint = build_joint(
        rb_leg_frame,
        body_frame,
        "right_back",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[6] as Float).to_radians(),
    );

    let rb_shank_frame = "right_back_shank";
    let mut rb_shank = build_rigid(rb_shank_frame, "bittle_x_shank_right_2", urdf, meshes);
    add_bittle_foot_collision_sphere(&mut rb_shank, "right_back", urdf);
    let rb_shank_joint = build_joint(
        rb_shank_frame,
        rb_leg_frame,
        "right_back_knee",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[10] as Float).to_radians(),
    );

    // left-back
    let lb_leg_frame = "left_back_leg";
    let mut lb_leg = build_rigid(lb_leg_frame, "bittle_x_leg_4", urdf, meshes);
    add_bittle_knee_collision_point(&mut lb_leg, "left_back", urdf);
    let lb_leg_joint = build_joint(
        lb_leg_frame,
        body_frame,
        "left_back",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[7] as Float).to_radians(),
    );

    let lb_shank_frame = "left_back_shank";
    let mut lb_shank = build_rigid(lb_shank_frame, "bittle_x_shank_left_2", urdf, meshes);
    add_bittle_foot_collision_sphere(&mut lb_shank, "left_back", urdf);
    let lb_shank_joint = build_joint(
        lb_shank_frame,
        lb_leg_frame,
        "left_back_knee",
        urdf,
        -Vector3::z_axis(),
        (zero_position_angles[11] as Float).to_radians(),
    );

    let mut articulated = Articulated::new(
        vec![
            body, head, lf_leg, lf_shank, rf_leg, rf_shank, rb_leg, rb_shank, lb_leg, lb_shank,
        ],
        vec![
            body_joint,
            head_joint,
            lf_leg_joint,
            lf_shank_joint,
            rf_leg_joint,
            rf_shank_joint,
            rb_leg_joint,
            rb_shank_joint,
            lb_leg_joint,
            lb_shank_joint,
        ],
    );

    articulated.show_visual = false;
    state.add_articulated(articulated);

    // Add a sphere to interact with
    let m = 0.02;
    let w = 0.1;
    let cube = Articulated::new_cube_at("cube", m, w, &vector![2. * w, -0.1, 2. * w]);
    state.add_articulated(cube);

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

fn add_bittle_foot_collision_sphere(rigid: &mut Rigid, which_leg: &str, urdf: &Robot) {
    let joint_name = format!("{}_foot_frame", which_leg);
    let point_joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
    rigid.add_collision_sphere_at(&Vector3::from(point_joint.origin.xyz.0), 0.0055);
}

fn add_bittle_knee_collision_point(rigid: &mut Rigid, which_leg: &str, urdf: &Robot) {
    let joint_name = format!("{}_knee_frame", which_leg);
    let point_joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
    rigid.add_point_at(&Vector3::from(point_joint.origin.xyz.0));
}

fn add_bittle_shoulder_collision_sphere(body: &mut Rigid, urdf: &Robot) {
    for which_leg in ["left_front", "right_front", "right_back", "left_back"] {
        let joint_name = format!("{}_shoulder_frame", which_leg);
        let point_joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
        body.add_collision_sphere_at(&Vector3::from(point_joint.origin.xyz.0), 0.012);
    }
}

fn add_bittle_tail_collision_point(body: &mut Rigid, urdf: &Robot) {
    let joint_name = "tail_frame";
    let point_joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
    body.add_point_at(&Vector3::from(point_joint.origin.xyz.0));
}

fn add_bittle_head_collision_sphere(head: &mut Rigid, urdf: &Robot) {
    let joint_name = "head_frame";
    let point_joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
    head.add_collision_sphere_at(&Vector3::from(point_joint.origin.xyz.0), 0.019);
}
