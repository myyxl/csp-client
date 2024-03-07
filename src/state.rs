use std::sync::{Mutex, OnceLock};
use crate::network::interface::{Interface, ReceiveTransmit};
use crate::network::interface::r#loop::LoopInterface;
use crate::network::interface::zmqproxy::ZmqProxyInterface;

struct State {
    interfaces: Vec<Interface<Box<dyn ReceiveTransmit + Send>>>
}

fn state() -> &'static Mutex<State> {
    static STATE: OnceLock<Mutex<State>> = OnceLock::new();
    STATE.get_or_init(|| {
        Mutex::new({
            State {
                interfaces: vec![
                    Interface::<LoopInterface>::new(Box::new(LoopInterface {}), "LOOP", 0, 0),
                    Interface::<ZmqProxyInterface>::new(Box::new(ZmqProxyInterface {}), "LOOP", 0, 0),
                ],
            }
        })
    })
}