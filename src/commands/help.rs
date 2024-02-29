use crate::console::command::{commands, Execute};

pub struct HelpCommand;

impl Execute for HelpCommand {
    fn execute(&self, _argv: Vec<&str>) -> Result<Option<String>, String> {
        let mut help = String::new();
        for command in commands() {
            help.push_str(format!("  {}\t\t{}\n", &command.name, &command.description).as_str())
        }
        help.remove(help.len() - 1);
        return Ok(Some(help))
    }
}