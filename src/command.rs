use crate::command::exit::ExitCommand;
use crate::command::help::HelpCommand;
use crate::command::ifc::IfcCommand;
use crate::command::ping::PingCommand;
use crate::command::sleep::SleepCommand;

pub mod exit;
pub mod help;
pub mod sleep;
pub mod ping;
pub mod ifc;

pub struct Command<T> {
    pub name: &'static str,
    pub description: &'static str,
    pub syntax: &'static str,
    pub argc: usize,
    pub handler: T,
}

pub trait Execute {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String>;
}

pub fn commands() -> Vec<Command<Box<dyn Execute>>> {
    vec![
        Command {
            name: "ping",
            description: "csp: Ping",
            syntax: "ping <node> [timeout] [size] [opt]",
            argc: 1,
            handler: Box::new(PingCommand),
        },
        Command {
            name: "ifc",
            description: "csp: Show interface",
            syntax: "ifc",
            argc: 0,
            handler: Box::new(IfcCommand),
        },
        Command {
            name: "help",
            description: "shell: Show help",
            syntax: "help",
            argc: 0,
            handler: Box::new(HelpCommand),
        },
        Command {
            name: "sleep",
            description: "shell: Sleep X ms",
            syntax: "sleep <time>",
            argc: 1,
            handler: Box::new(SleepCommand),
        },
        Command {
            name: "exit",
            description: "shell: Exit program",
            syntax: "exit",
            argc: 0,
            handler: Box::new(ExitCommand),
        }
    ]
}