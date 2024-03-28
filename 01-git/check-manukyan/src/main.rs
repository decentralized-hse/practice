use std::env;
use std::path::PathBuf;
use std::fmt;

mod validation;

pub struct Args {
    path: Vec<PathBuf>,
    hash: String,
}

#[derive(Debug)]
pub struct ArgsError {
    info: String,
}

impl fmt::Display for ArgsError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Invalid arguments: {}", self.info)
    }
}

impl From<Box<dyn std::error::Error>> for ArgsError {
    fn from(error: Box<dyn std::error::Error>) -> Self {
        ArgsError {
            info: format!("An error occurred: {}", error),
        }
    }
}

impl std::error::Error for ArgsError {}

#[derive(Debug)]
struct IntegrityError {
    info: String,
}

impl std::fmt::Display for IntegrityError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Integrity violation occured: {}", self.info)
    }
}

impl std::error::Error for IntegrityError {}

fn validate_args(row_args: Vec<String>) -> Result<Args, ArgsError> {
    validation::validate_args(row_args)
}

fn validate_tree(args: &Args) -> Result<(), Box<dyn std::error::Error>> {
    validation::validate_tree(args)
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = validate_args(env::args().collect())?;
    validate_tree(&args)?;
    Ok(())
}
