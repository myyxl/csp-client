use crate::network::interface::{Interface, Receive, ReceiveTransmit, Transmit};
use crate::network::interface::zmqproxy::ZmqProxyInterface;

pub struct LoopInterface;

impl Receive for LoopInterface {
    fn receive(&self) -> () {
        todo!()
    }
}

impl Transmit for LoopInterface {
    fn transmit(&self) -> () {
        todo!()
    }
}

impl ReceiveTransmit for LoopInterface {}