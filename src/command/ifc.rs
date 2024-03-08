use colored::Colorize;
use crate::command::Execute;
use crate::network::interface::interfaces;

pub struct IfcCommand;

impl Execute for IfcCommand {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String> {
        let mut ret = String::new();
        let interfaces = { &interfaces().lock().unwrap().interfaces };

        for interface in interfaces {
            ret.push_str(
                format!("{}\n\ttx: {:0>5} rx: {:0>5} txe: {:0>5} rxe: {:0>5}\n\ttxb: {} rxb: {}\n\n",
                        interface.name.bold(),
                        interface.tx,
                        interface.rx,
                        interface.tx_error,
                        interface.rx_error,
                        interface.tx_bytes,
                        interface.rx_bytes
                ).as_str()
            )
        }
        ret.remove(ret.len() - 1);
        Ok(Some(ret))
    }
}