use crate::command::Execute;

pub struct IfcCommand;

impl Execute for IfcCommand {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String> {
        Ok(None)
    }
}