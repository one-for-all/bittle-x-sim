use gorilla_physics::{
    PI, WORLD_FRAME,
    hybrid::{
        Hybrid, Rigid,
        articulated::Articulated,
        mesh::URDFMeshes,
        rigid::helper::{build_joint, build_rigid},
    },
    joint::Joint,
    na::{Vector3, vector},
    spatial::transform::Transform3D,
    types::Float,
};
#[cfg(any(target_arch = "wasm32", rust_analyzer))]
use gorilla_physics::{
    collision::halfspace::HalfSpace,
    hybrid::static_body::{StaticBody, StaticCuboid},
};
use urdf_rs::Robot;

#[cfg(any(target_arch = "wasm32", rust_analyzer))]
use {
    crate::can::build_beverage_can, crate::control::BittleXEsp32Controller,
    gorilla_physics::hybrid::control::NullArticulatedController,
    gorilla_physics::interface::hybrid::InterfaceHybrid,
    gorilla_physics::interface::util::read_web_file, wasm_bindgen::prelude::wasm_bindgen,
};

pub fn build_bittle_x(meshes: &mut URDFMeshes, urdf: &Robot) -> Hybrid {
    let mut state = Hybrid::empty();
    state.set_friction_mu(0.5);
    // The contact solve was half of every step in Clarabel. Solved densely,
    // robot and can apart, it is the same problem to a tighter tolerance for
    // a quarter of the cost. See `Hybrid::set_dense_cone_solve`.
    state.set_dense_cone_solve(true);

    #[rustfmt::skip]
    let zero_position_angles = [
        135, 225, 135, 135,
        190, 80, 190, 80,
        190, 80, 80, 190
    ];

    let body_frame = "body";
    let mut body = build_rigid(body_frame, "chassis", urdf, meshes, false);
    add_bittle_body_colliders(&mut body, urdf);
    let body_joint = Joint::new_floating(Transform3D::new_xyz_rpy(
        body_frame,
        WORLD_FRAME,
        &vec![0., -0.1, 0.06],
        &vec![0., 0., -PI / 2.],
    ));

    let head_frame = "head";
    let mut head = build_rigid(head_frame, "bittle_x_head", urdf, meshes, false);
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
    let mut lf_leg = build_rigid(lf_leg_frame, "bittle_x_leg", urdf, meshes, false);
    add_bittle_thigh_collision_spheres(&mut lf_leg, "left_front", urdf);
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
    let mut lf_shank = build_rigid(lf_shank_frame, "bittle_x_shank_left", urdf, meshes, false);
    add_bittle_shank_collision_spheres(&mut lf_shank, "left_front", urdf);
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
    let mut rf_leg = build_rigid(rf_leg_frame, "bittle_x_leg_2", urdf, meshes, false);
    add_bittle_thigh_collision_spheres(&mut rf_leg, "right_front", urdf);
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
    let mut rf_shank = build_rigid(rf_shank_frame, "bittle_x_shank_right", urdf, meshes, false);
    add_bittle_shank_collision_spheres(&mut rf_shank, "right_front", urdf);
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
    let mut rb_leg = build_rigid(rb_leg_frame, "bittle_x_leg_3", urdf, meshes, false);
    add_bittle_thigh_collision_spheres(&mut rb_leg, "right_back", urdf);
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
    let mut rb_shank = build_rigid(
        rb_shank_frame,
        "bittle_x_shank_right_2",
        urdf,
        meshes,
        false,
    );
    add_bittle_shank_collision_spheres(&mut rb_shank, "right_back", urdf);
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
    let mut lb_leg = build_rigid(lb_leg_frame, "bittle_x_leg_4", urdf, meshes, false);
    add_bittle_thigh_collision_spheres(&mut lb_leg, "left_back", urdf);
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
    let mut lb_shank = build_rigid(lb_shank_frame, "bittle_x_shank_left_2", urdf, meshes, false);
    add_bittle_shank_collision_spheres(&mut lb_shank, "left_back", urdf);
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

    state
}

#[cfg(any(target_arch = "wasm32", rust_analyzer))]
#[allow(non_snake_case)]
#[wasm_bindgen]
pub async fn createBittleX() -> InterfaceHybrid {
    let urdf_path = "robot.urdf";
    let urdf_file = read_web_file(urdf_path).await;
    let urdf_robot = urdf_rs::read_from_string(&urdf_file).unwrap();

    let mut meshes = URDFMeshes::new(&urdf_robot).await;

    let mut state = build_bittle_x(&mut meshes, &urdf_robot);
    // let mut state = Hybrid::empty();
    // let sphere = Articulated::new_sphere_at("sphere", 1.0, 0.02, &vector![0., 0., 1.0]);
    // state.add_articulated(sphere);

    let controller = BittleXEsp32Controller::new().await;
    // let controller = NullArticulatedController {};
    state.set_controller(0, controller);

    // Add ramp
    // let mut ramp = import_static_body("mesh/ramp.obj").await;
    // ramp.scale(vector![0.2, 0.1, 0.1]);
    // ramp.update_pose(Isometry3::from_parts(
    //     Translation3::new(0., 0.8, 0.1),
    //     UnitQuaternion::from_euler_angles(0., 0., -PI / 2.),
    // ));
    // state.add_static_body(ramp);

    // A beverage can to judge the robot's size by, and for it to push around.
    // It faces -x (screen left, from the default camera) and walks off that
    // way, so the can stands behind its line, to the right of where it starts.
    let can = build_beverage_can(&vector![0.2, 0.1, 0.]);
    state.add_articulated(can);

    // state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.82));
    state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.));
    // state.add_static_cuboid(StaticCuboid::new(
    //     2.0,
    //     0.6,
    //     0.13,
    //     Isometry3::translation(0.6, 0., 0.82 - 0.13 / 2.),
    // ));

    InterfaceHybrid::new(state)
}

/// Foot sphere radius
const FOOT_RADIUS: Float = 0.0055;
/// Radius of the spheres lining the thigh: about its half-width and
/// half-thickness
const THIGH_RADIUS: Float = 0.005;
/// Radius of the spheres lining the shank. Kept under `FOOT_RADIUS`, so a
/// planted shank still stands on its foot, whatever angle it leans at.
const SHANK_RADIUS: Float = 0.005;
/// How far apart the spheres lining a leg part sit, in their radii. At 1.5
/// the outline dips a third of a radius between two of them, which the can's
/// flat sides and blunt corners cannot get into.
const SPHERE_SPACING: Float = 1.5;

fn joint_origin(urdf: &Robot, joint_name: &str) -> Vector3<Float> {
    let joint = urdf.joints.iter().find(|&j| j.name == joint_name).unwrap();
    Vector3::from(joint.origin.xyz.0)
}

/// Spheres of radius `r` evenly along the segment from `from` to `to`, `from`
/// included and `to` not, no more than `SPHERE_SPACING` radii apart
fn add_collision_sphere_chain(
    rigid: &mut Rigid,
    from: &Vector3<Float>,
    to: &Vector3<Float>,
    r: Float,
) {
    let n = ((to - from).norm() / (SPHERE_SPACING * r)).ceil() as usize;
    for i in 0..n {
        let center = from + (to - from) * (i as Float / n as Float);
        rigid.add_collision_sphere_at(&center, r);
    }
}

/// Lines the thigh with spheres, from the hip to the far end of the knee.
///
/// The knee point alone used to be all of the thigh that collided, and a point
/// met nothing but the ground, so the thigh went through the can.
fn add_bittle_thigh_collision_spheres(rigid: &mut Rigid, which_leg: &str, urdf: &Robot) {
    // The knee's axis passes through the thigh's centreline
    let knee = joint_origin(urdf, &format!("{}_knee", which_leg));
    let tip = joint_origin(urdf, &format!("{}_knee_frame", which_leg));
    let hip = Vector3::new(0., 0., knee.z);
    let along = (knee - hip).normalize();
    // The last sphere's far side at the knee's tip
    let end = hip + along * ((tip - hip).dot(&along) - THIGH_RADIUS);
    add_collision_sphere_chain(rigid, &hip, &end, THIGH_RADIUS);
    rigid.add_collision_sphere_at(&end, THIGH_RADIUS);
}

/// Lines the shank with spheres from the knee down to the foot, and puts the
/// foot's at its end.
///
/// The foot alone used to be all of the shank that collided, so the rest of
/// it went through anything but the ground, such as the can.
fn add_bittle_shank_collision_spheres(rigid: &mut Rigid, which_leg: &str, urdf: &Robot) {
    // The shank lies along x, set out along the knee's axis by as much as the
    // foot is
    let foot = joint_origin(urdf, &format!("{}_foot_frame", which_leg));
    let knee = Vector3::new(0., 0., foot.z);
    add_collision_sphere_chain(rigid, &knee, &foot, SHANK_RADIUS);
    rigid.add_collision_sphere_at(&foot, FOOT_RADIUS);
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

fn add_bittle_body_colliders(body: &mut Rigid, urdf: &Robot) {
    add_bittle_shoulder_collision_sphere(body, urdf);
    add_bittle_chassis_collision_cuboids(body);
    add_bittle_tail_collision_spheres(body, urdf);
    add_bittle_tail_collision_point(body, urdf);
}

/// Two boxes around the chassis, in its frame (x across, y from head to tail,
/// z up), measured off its meshes: one from the bottom of the battery case to
/// the top of the frame, over the shoulder servos at either end, and one for
/// the cover raised on the middle of it.
///
/// The shoulder spheres alone used to be all of the chassis that collided, so
/// the can went through the rest of it. The bottom is kept a hair above the
/// shoulder spheres', so lying flat the robot still rests on those.
fn add_bittle_chassis_collision_cuboids(body: &mut Rigid) {
    for (min, max) in [
        // frame, battery case and shoulder servos
        (vector![-0.0507, -0.0621, -0.012], vector![0.0382, 0.0668, 0.0162]),
        // cover
        (vector![-0.042, -0.0346, 0.0066], vector![0.0292, 0.0393, 0.0252]),
    ] {
        let size: Vector3<Float> = max - min;
        body.add_collision_cuboid_at(&((min + max) / 2.), size.x, size.y, size.z);
    }
}

/// Radius of the spheres lining the tail: about its thickness, and most of
/// its half-width
const TAIL_RADIUS: Float = 0.004;

/// Lines the tail with spheres, from where it leaves the chassis boxes up its
/// curve to the tip, its centreline measured off its mesh in the chassis
/// frame. The last sphere's far side is at the tip.
///
/// The point at its tip used to be all of the tail that collided, and a point
/// met nothing but the ground, so the tail went through the can.
fn add_bittle_tail_collision_spheres(body: &mut Rigid, urdf: &Robot) {
    let tip = joint_origin(urdf, "tail_frame");
    let x = tip.x;
    let mut line = vec![
        vector![x, 0.040, 0.0156],
        vector![x, 0.0613, 0.0209],
        vector![x, 0.0738, 0.0294],
        vector![x, 0.0788, 0.0335],
    ];
    let last = line[line.len() - 1];
    let end = tip - (tip - last).normalize() * TAIL_RADIUS;
    line.push(end);
    for segment in line.windows(2) {
        add_collision_sphere_chain(body, &segment[0], &segment[1], TAIL_RADIUS);
    }
    body.add_collision_sphere_at(&end, TAIL_RADIUS);
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

#[cfg(test)]
mod tests {
    use gorilla_physics::{
        PI, WORLD_FRAME,
        hybrid::{
            Hybrid, Rigid, articulated::Articulated, mesh::URDFMeshes, rigid::helper::build_rigid,
        },
        joint::Joint,
        na::vector,
        spatial::transform::Transform3D,
        types::Float,
    };
    use urdf_rs::Robot;

    use super::{
        FOOT_RADIUS, add_bittle_body_colliders, add_bittle_shank_collision_spheres,
        add_bittle_thigh_collision_spheres, joint_origin,
    };
    use crate::can::{CAN_RADIUS, build_beverage_can};

    const DT: Float = 1. / 3600.;
    /// Height the robot part is held at
    const HEIGHT: Float = 0.3;

    fn urdf() -> Robot {
        let path = concat!(env!("CARGO_MANIFEST_DIR"), "/www/static/robot.urdf");
        urdf_rs::read_from_string(&std::fs::read_to_string(path).unwrap()).unwrap()
    }

    fn part(link: &str, urdf: &Robot) -> Rigid {
        build_rigid("part", link, urdf, &mut URDFMeshes::empty(), false)
    }

    /// Holds a part of the robot still in the air, its frame at `HEIGHT` and
    /// turned by `rpy`, drops `dropped` onto it and lets it fall for 0.15 s:
    /// long enough to fall 11 cm, or to come to rest on the part.
    ///
    /// Returns how high the dropped body's frame ends up over the part's.
    fn dropped_on(part: Rigid, rpy: [Float; 3], dropped: Articulated) -> Float {
        let mut state = Hybrid::empty();
        let joint = Joint::new_fixed(Transform3D::new_xyz_rpy(
            "part",
            WORLD_FRAME,
            &vec![0., 0., HEIGHT],
            &rpy.to_vec(),
        ));
        state.add_articulated(Articulated::new(vec![part], vec![joint]));
        state.add_articulated(dropped);
        for _ in 0..(0.15 / DT) as usize {
            state.step(DT, &vec![]);
        }
        state.articulated[1].bodies[0].pose.translation.z - HEIGHT
    }

    /// Drops the can upright onto a leg part lying along world x and turned a
    /// quarter about it, so that the part's y, the plane it swings in, is up:
    /// the can lands on the side a leg sweeps into things with. It comes down
    /// from 5 mm above with its axis at `x` along the part and over its
    /// centreline, which is set `out` along the part's z -- now world -y.
    ///
    /// Returns the height of the can's bottom over the centreline: resting on
    /// the part, a sphere's radius.
    fn can_dropped_on_leg(part: Rigid, x: Float, out: Float) -> Float {
        let can = build_beverage_can(&vector![x, -out, HEIGHT + 0.01]);
        dropped_on(part, [PI / 2., 0., 0.], can)
    }

    #[test]
    fn can_does_not_go_through_the_thigh() {
        let urdf = urdf();
        let mut thigh = part("bittle_x_leg", &urdf);
        add_bittle_thigh_collision_spheres(&mut thigh, "left_front", &urdf);
        // Over the middle of the thigh, which is set out as far as the knee
        let out = joint_origin(&urdf, "left_front_knee").z;
        let z = can_dropped_on_leg(thigh, -0.025, out);
        assert!(z > 0.004, "can fell to {z} on the thigh");
    }

    #[test]
    fn can_does_not_go_through_the_shank() {
        let urdf = urdf();
        let mut shank = part("bittle_x_shank_left", &urdf);
        add_bittle_shank_collision_spheres(&mut shank, "left_front", &urdf);
        // Over the knee end, with all of the can's bottom short of the foot,
        // which used to be all of the shank that collided
        let foot = joint_origin(&urdf, "left_front_foot_frame");
        let x = foot.x + FOOT_RADIUS + CAN_RADIUS + 0.002;
        let z = can_dropped_on_leg(shank, x, foot.z);
        assert!(z > 0.004, "can fell to {z} on the shank");
    }

    #[test]
    fn can_does_not_go_through_the_chassis() {
        let urdf = urdf();
        let mut chassis = part("chassis", &urdf);
        add_bittle_body_colliders(&mut chassis, &urdf);
        // Upright, onto the middle of the cover, its bottom clear of the
        // shoulder spheres, which used to be all of the chassis that collided
        let cover_top = 0.0252;
        let can = build_beverage_can(&vector![-0.0064, 0.0024, HEIGHT + cover_top + 0.005]);
        let z = dropped_on(chassis, [0., 0., 0.], can);
        assert!(
            z > cover_top - 0.001,
            "can fell to {z}, cover top at {cover_top}"
        );
    }

    #[test]
    fn cube_does_not_go_through_the_tail() {
        let urdf = urdf();
        let mut chassis = part("chassis", &urdf);
        add_bittle_body_colliders(&mut chassis, &urdf);
        // The can is too wide to land anywhere on the tail but its tip, so a
        // 1 cm cube instead, onto the middle of the tail behind the chassis,
        // whose top there is 32 mm up: short of the tip, which used to be all
        // of the tail that collided, and clear of the chassis itself. It
        // settles down the tail's slope, but stays up on it, over the top of
        // the chassis, rather than falling past.
        let w = 0.01;
        let cube = Articulated::new_cube_at("cube", 0.01, w, &vector![-0.0064, 0.072, HEIGHT + 0.04]);
        let z = dropped_on(chassis, [0., 0., 0.], cube);
        assert!(z > 0.02, "cube fell to {z} on the tail");
    }
}
