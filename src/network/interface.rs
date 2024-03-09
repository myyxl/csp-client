use std::sync::{Mutex, OnceLock};
use crate::network::interface::r#loop::LoopInterface;
use crate::network::interface::zmqproxy::ZmqProxyInterface;

pub mod r#loop;
pub mod zmqproxy;

pub trait Receive {
    fn receive(&self) -> ();
}

pub trait Transmit {
    fn transmit(&self) -> ();
}

pub trait ReceiveTransmit: Receive + Transmit {}

pub struct Interface<T> {
    pub name: & 'static str,
    pub addr: u32,
    pub netmask: u32,
    pub tx: u32,
    pub rx: u32,
    pub tx_error: u32,
    pub rx_error: u32,
    pub tx_bytes: u32,
    pub rx_bytes: u32,
    pub handler: T,
}

impl<T: ReceiveTransmit + Send + 'static> Interface<T> {
    pub fn new(name: &'static str, addr: u32, netmask: u32, handler: Box<T>) -> Interface<Box<dyn ReceiveTransmit + Send>> {
        Interface::<Box<dyn ReceiveTransmit + Send>> {
            name,
            addr,
            netmask,
            tx: 0,
            rx: 0,
            tx_error: 0,
            rx_error: 0,
            tx_bytes: 0,
            rx_bytes: 0,
            handler,
        }
    }
}

pub struct InterfaceState {
    pub interfaces: Vec<Interface<Box<dyn ReceiveTransmit + Send>>>
}

pub fn interfaces() -> &'static Mutex<InterfaceState> {
    static STATE: OnceLock<Mutex<InterfaceState>> = OnceLock::new();
    STATE.get_or_init(|| {
        Mutex::new({
            InterfaceState {
                interfaces: vec![
                    Interface::new("LOOP", 30, 0, Box::new(LoopInterface)),
                    Interface::new("ZMQPROXY", 10, 0, Box::new(ZmqProxyInterface)),
                ],
            }
        })
    })
}