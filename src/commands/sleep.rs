use std::thread;
use std::time::Duration;
use crate::console::command::Execute;

pub struct SleepCommand;

impl Execute for SleepCommand {
    fn execute(&self, argv: Vec<&str>) -> Result<Option<String>, String> {
        let ms = match argv[0].parse::<u64>() {
            Ok(ms) => ms,
            Err(err) => return Err(err.to_string())
        };
        let duration = Duration::from_millis(ms);
        thread::sleep(duration);
        Ok(None)
    }
}