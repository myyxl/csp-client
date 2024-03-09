use crate::command::Execute;

pub struct RouteCommand;

impl Execute for RouteCommand {
    fn execute(&self, _argv: Vec<&str>) -> Result<Option<String>, String> {
        Ok(None)
    }
}