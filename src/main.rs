use bittle_x::builder::build_bittle_x;
use esp32rs::util::read_file;
use gorilla_physics::hybrid::{control::NullArticulatedController, mesh::URDFMeshes};

fn main() {
    let mut meshes = URDFMeshes::empty();
    let urdf_path = "onshape/robot.urdf";
    let urdf_file = read_file(urdf_path);
    let urdf_robot = urdf_rs::read_from_string(&urdf_file).unwrap();
    let mut state = build_bittle_x(&mut meshes, &urdf_robot);

    let controller = NullArticulatedController {};
    state.set_controller(0, controller);
}
