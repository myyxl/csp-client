use crate::network::interface::{Receive, ReceiveTransmit, Transmit};

pub struct ZmqProxyInterface;

impl Receive for ZmqProxyInterface {
    fn receive(&self) -> () {
        todo!()
    }
}

impl Transmit for ZmqProxyInterface {
    fn transmit(&self) -> () {
        todo!()
    }
}

impl ReceiveTransmit for ZmqProxyInterface {}