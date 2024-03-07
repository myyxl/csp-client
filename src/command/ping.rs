use crate::command::Execute;

pub struct PingCommand;

impl Execute for PingCommand {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String> {
        Ok(None)
    }
}