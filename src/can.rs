//! A 355 ml (12 oz) beverage can, placed in the scene as something of known
//! size to judge the robot's scale against, and as a free body the robot can
//! push around or knock over.

use gorilla_physics::{
    PI, WORLD_FRAME,
    hybrid::{
        Rigid,
        articulated::Articulated,
        visual::{
            Visual,
            rigid_mesh::{MeshGroup, RigidMesh},
        },
    },
    inertia::SpatialInertia,
    joint::Joint,
    na::{Isometry3, Matrix3, Translation3, UnitQuaternion, Vector3, vector},
    spatial::transform::Transform3D,
    types::Float,
};

/// Body radius, at the straight wall
pub const CAN_RADIUS: Float = 0.0331;
/// Overall height, from the standing ring to the top of the rim
pub const CAN_HEIGHT: Float = 0.1222;
/// An empty can: light enough for the robot to shove about
pub const CAN_MASS: Float = 0.015;
/// Aluminium sliding on a tabletop or floor
const CAN_MU: Float = 0.4;

/// Share of the mass in the drawn wall; the rest is split between the bottom
/// and the lid
const WALL_MASS_FRACTION: Float = 0.6;

const LABEL_COLOR: [Float; 3] = [0.72, 0.08, 0.1];
const METAL_COLOR: [Float; 3] = [0.8, 0.81, 0.83];

/// Free-standing beverage can, upright, with its bottom centred at `pos`.
///
/// Its frame sits at the centre of the bottom with z up the can's axis, so
/// `pos.z` is the height of whatever it stands on.
pub fn build_beverage_can(pos: &Vector3<Float>) -> Articulated {
    let frame = "beverage_can";
    let mut body = Rigid::new(can_inertia(frame));

    body.visual.push((
        Visual::RigidMesh(can_mesh()),
        Isometry3::identity(),
        Some(Vector3::from(METAL_COLOR)),
        None,
        false,
    ));

    // There is no cylinder collider, and only spheres and cuboids collide with
    // the robot's, so the can collides as a regular 12-sided prism: the union
    // of six thin rectangles through the axis, each spanning between two
    // opposite sides, turned 30 degrees from the last. Every corner lies on the
    // can's wall, so it stands on and rolls along the ground as a 12-gon would.
    let n = 6;
    let half_angle = PI / (2 * n) as Float;
    let w = 2. * CAN_RADIUS * half_angle.cos();
    let d = 2. * CAN_RADIUS * half_angle.sin();
    for i in 0..n {
        let rotation =
            UnitQuaternion::from_axis_angle(&Vector3::z_axis(), i as Float * PI / n as Float);
        let iso = Isometry3::from_parts(Translation3::new(0., 0., CAN_HEIGHT / 2.), rotation);
        body.visual.push((
            Visual::new_cuboid(w, d, CAN_HEIGHT),
            iso,
            None,
            Some(CAN_MU),
            true,
        ));
    }

    let joint = Joint::new_floating(Transform3D::move_xyz(
        frame,
        WORLD_FRAME,
        pos.x,
        pos.y,
        pos.z,
    ));
    let mut can = Articulated::new(vec![body], vec![joint]);
    // Hide the collision cuboids; the mesh is always drawn
    can.show_visual = false;
    can
}

/// Thin-walled cylinder closed at both ends, in the can's frame
fn can_inertia(frame: &str) -> SpatialInertia {
    let m = CAN_MASS;
    let r = CAN_RADIUS;
    let h = CAN_HEIGHT;
    let m_wall = WALL_MASS_FRACTION * m;
    let m_end = (m - m_wall) / 2.;

    // About the centre of mass, halfway up the axis
    let i_axis = m_wall * r * r + 2. * (m_end * r * r / 2.);
    let i_across = m_wall * (r * r / 2. + h * h / 12.) + 2. * m_end * (r * r / 4. + h * h / 4.);
    let moment_com = Matrix3::from_diagonal(&vector![i_across, i_across, i_axis]);

    // Carried to the frame at the bottom of the can
    let com = vector![0., 0., h / 2.];
    let moment =
        moment_com + m * (com.norm_squared() * Matrix3::identity() - com * com.transpose());
    SpatialInertia::new(moment, m * com, m, frame)
}

/// The can's outline as (radius, height) from the middle of the bottom round
/// to the middle of the lid, with which colour the band up to the next point
/// is painted
const PROFILE: [(Float, Float, bool); 9] = [
    (0., 0.004, false),          // bottom, domed up into the can
    (0.0255, 0., false),         // standing ring
    (CAN_RADIUS, 0.010, true),   // bottom of the wall
    (CAN_RADIUS, 0.104, false),  // top of the wall
    (0.0272, 0.1195, false),     // neck
    (0.0272, CAN_HEIGHT, false), // outside of the rim
    (0.0255, CAN_HEIGHT, false), // inside of the rim
    (0.0255, 0.118, false),      // lid, recessed below the rim
    (0., 0.118, false),
];

/// The visual mesh: `PROFILE` turned about the z axis. Each band gets its
/// own ring of vertices, so the shading is smooth around the can but creased
/// where the outline turns a corner.
fn can_mesh() -> RigidMesh {
    let sectors = 48;
    let mut vertices: Vec<Vector3<Float>> = vec![];
    let mut faces: Vec<[usize; 3]> = vec![];
    let mut groups: Vec<MeshGroup> = vec![];

    let ring = |vertices: &mut Vec<Vector3<Float>>, r: Float, z: Float| -> usize {
        let start = vertices.len();
        for k in 0..sectors {
            let theta = 2. * PI * k as Float / sectors as Float;
            vertices.push(vector![r * theta.cos(), r * theta.sin(), z]);
        }
        start
    };

    for band in PROFILE.windows(2) {
        let (r0, z0, label) = band[0];
        let (r1, z1, _) = band[1];
        let face_start = faces.len();

        let a = ring(&mut vertices, r0, z0);
        let b = ring(&mut vertices, r1, z1);
        // Going up the outline with the material on the left, these wind
        // counter-clockwise seen from outside the can
        for k in 0..sectors {
            let k1 = (k + 1) % sectors;
            if r0 > 0. {
                faces.push([a + k, a + k1, b + k1]);
            }
            if r1 > 0. {
                faces.push([a + k, b + k1, b + k]);
            }
        }

        let (material, color) = if label {
            ("label", LABEL_COLOR)
        } else {
            ("aluminium", METAL_COLOR)
        };
        match groups.last_mut() {
            Some(group) if group.material == material => {
                group.count = faces.len() - group.start;
            }
            _ => groups.push(MeshGroup {
                start: face_start,
                count: faces.len() - face_start,
                material: material.to_string(),
                color: Some(Vector3::from(color)),
            }),
        }
    }

    RigidMesh::with_groups(vertices, faces, groups)
}

#[cfg(test)]
mod tests {
    use gorilla_physics::{
        PI,
        collision::halfspace::HalfSpace,
        hybrid::Hybrid,
        joint::JointVelocity,
        na::{Isometry3, Point3, Vector3, vector},
        spatial::spatial_vector::SpatialVector,
        types::Float,
    };

    use super::{CAN_HEIGHT, CAN_RADIUS, build_beverage_can};

    const DT: Float = 1. / 3600.;

    fn can_on_ground() -> Hybrid {
        let mut state = Hybrid::empty();
        state.set_friction_mu(1.0);
        state.add_articulated(build_beverage_can(&vector![0.1, 0.2, 0.]));
        state.add_halfspace(HalfSpace::new(Vector3::z_axis(), 0.));
        state
    }

    fn run(state: &mut Hybrid, seconds: Float) {
        for _ in 0..(seconds / DT) as usize {
            state.step(DT, &vec![]);
        }
    }

    fn can_pose(state: &Hybrid) -> Isometry3<Float> {
        state.articulated[0].bodies[0].pose.to_isometry()
    }

    /// How far the can moves, and turns, over another half second. A body at
    /// rest on the ground still carries a step's worth of gravity in its
    /// velocity, so it is judged by where it goes rather than by that.
    fn motion_over_half_a_second(state: &mut Hybrid) -> (Float, Float) {
        let before = can_pose(state);
        run(state, 0.5);
        let after = can_pose(state);
        let moved = (after.translation.vector - before.translation.vector).norm();
        let turned = (before.rotation.inverse() * after.rotation).angle();
        (moved, turned)
    }

    #[test]
    fn stands_still_where_it_is_put() {
        let mut state = can_on_ground();
        run(&mut state, 1.);

        let iso = can_pose(&state);
        let t = iso.translation.vector;
        assert!(
            (t - vector![0.1, 0.2, 0.]).norm() < 1e-3,
            "drifted to {t:?}"
        );
        let axis = iso.rotation * Vector3::z();
        assert!(axis.z > 0.9999, "tilted to {axis:?}");
        let (moved, turned) = motion_over_half_a_second(&mut state);
        assert!(
            moved < 1e-5 && turned < 1e-4,
            "moved {moved}, turned {turned}"
        );
    }

    #[test]
    fn knocked_over_comes_to_rest_on_its_side() {
        let mut state = can_on_ground();
        // A shove at the top, as a leg sweeping into it would give: tipping
        // about the bottom rim at (0, -r, 0), so the frame at the middle of the
        // bottom rises as it turns
        let w = 12.;
        state.articulated[0].set_joint_v(
            0,
            JointVelocity::Spatial(SpatialVector::new(
                vector![w, 0., 0.],
                vector![0., 0., w * CAN_RADIUS],
            )),
        );
        run(&mut state, 3.);

        let iso = can_pose(&state);
        let axis = iso.rotation * Vector3::z();
        assert!(axis.z.abs() < 0.01, "not lying down: axis {axis:?}");

        // Lying on a flat of the 12-gon, its axis is the flat's distance from
        // the axis off the ground
        let apothem = CAN_RADIUS * (PI / 12.).cos();
        let center = iso * Point3::new(0., 0., CAN_HEIGHT / 2.);
        assert!((center.z - apothem).abs() < 1e-3, "axis at {}", center.z);
        let (moved, turned) = motion_over_half_a_second(&mut state);
        assert!(
            moved < 1e-4 && turned < 1e-2,
            "moved {moved}, turned {turned}"
        );
    }
}
