use crate::network::interface::{Receive, ReceiveTransmit, Transmit};

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