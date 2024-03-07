use colored::Colorize;
use crate::command::Execute;
use crate::state::state;

pub struct IfcCommand;

impl Execute for IfcCommand {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String> {
        let mut ret = String::new();
        let interfaces = { &state().lock().unwrap().interfaces };

        for interface in interfaces {
            ret.push_str(
                format!("{}\n\ttx: {} rx: {} txe: {} rxe: {}\n\ttxb: {} (0.0B) rxb: {} (0.0B)\n\n",
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