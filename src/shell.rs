use colored::Colorize;
use rustyline::DefaultEditor;
use crate::command::commands;

pub fn init_shell() {
    let prompt = format!("{}{}{}{} ", "[".bright_black(), "csp-client".green(), "]".bright_black(), "$");
    let mut sh = DefaultEditor::new().unwrap();

    loop {
        let command = sh.readline(&prompt).unwrap();
        sh.add_history_entry(&command).unwrap();
        let mut args: Vec<&str> = command.trim().split(" ").collect();

        if args[0].trim().is_empty() {
            continue
        }

        if !commands().iter().any(|command| command.name == args[0].to_lowercase()) {
            println!("Unknown command '{}'", args[0].to_lowercase());
            continue;
        }

        for command in commands() {
            if args[0].to_lowercase() == command.name {
                if args.len() > command.argc {
                    if args.len() > 1 {
                        args.remove(0);
                    }
                    match command.handler.execute(args.clone()) {
                        Ok(output) => {
                            if let Some(output) = output {
                                println!("{}", output)
                            }
                        },
                        Err(err) => println!("Error: {}", err)
                    }
                } else {
                    println!("usage: {}", command.syntax);
                    println!("Could not execute command '{}'", command.name)
                }
            }
        }
    }
}