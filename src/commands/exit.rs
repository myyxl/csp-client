use std::process;
use crate::console::command::Execute;

pub struct ExitCommand;

impl Execute for ExitCommand {
    fn execute(&self, _argv: Vec<&str>) -> Result<Option<String>, String> {
        process::exit(0);
    }
}