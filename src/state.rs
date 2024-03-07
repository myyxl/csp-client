use std::sync::{Mutex, OnceLock};
use crate::network::interface::{Interface, ReceiveTransmit};
use crate::network::interface::r#loop::LoopInterface;
use crate::network::interface::zmqproxy::ZmqProxyInterface;

pub struct State {
    interfaces: Vec<Interface<Box<dyn ReceiveTransmit + Send>>>
}

pub fn state() -> &'static Mutex<State> {
    static STATE: OnceLock<Mutex<State>> = OnceLock::new();
    STATE.get_or_init(|| {
        Mutex::new({
            State {
                interfaces: vec![
                    Interface::new("LOOP", 10, 5, Box::new(LoopInterface)),
                    Interface::new("ZMQPROXY", 0, 0, Box::new(ZmqProxyInterface)),
                ],
            }
        })
    })
}