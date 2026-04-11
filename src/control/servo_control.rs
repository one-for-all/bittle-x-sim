use esp32rs::servo::petoi_p1s::PetoiP1S;
use gorilla_physics::{
    hybrid::{articulated::Articulated, control::ArticulatedController},
    joint::Joint,
    types::Float,
};
use nalgebra::{DVector, dvector};

pub struct BittleXServoController {
    pub servos: [PetoiP1S; 9],
}

impl BittleXServoController {
    pub fn new() -> Self {
        let mut servos = [PetoiP1S::new(); 9];
        for servo in servos.iter_mut() {
            servo.command_angle = Some((130. as Float).to_radians());
        }
        Self { servos }
    }
}

impl ArticulatedController for BittleXServoController {
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
            let torque = self.servos[i].torque();
            torques.push(torque);
        }

        DVector::from_vec(torques)
    }
}
