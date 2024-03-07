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