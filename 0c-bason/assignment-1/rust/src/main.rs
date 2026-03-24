use std::fs;
use std::path::PathBuf;

use anyhow::{Context, Result};
use bason_codec::{
    bason_decode_all, bason_flatten, bason_to_json, bason_unflatten, bason_validate,
    hexdump_annotated, pretty_print_record,
};
use clap::{Parser, ValueEnum};

#[derive(Debug, Clone, Copy, ValueEnum)]
enum OutputMode {
    Pretty,
    Json,
    Hex,
}

#[derive(Debug, Parser)]
#[command(name = "basondump")]
#[command(about = "Inspect BASON files")]
struct Args {
    #[arg(value_name = "FILE")]
    file: PathBuf,

    #[arg(long, value_enum, default_value = "pretty")]
    mode: OutputMode,

    #[arg(long)]
    strictness: Option<u16>,

    #[arg(long)]
    flatten: bool,

    #[arg(long)]
    unflatten: bool,
}

fn main() -> Result<()> {
    let args = Args::parse();
    let bytes = fs::read(&args.file)
        .with_context(|| format!("failed to read {}", args.file.display()))?;

    match args.mode {
        OutputMode::Hex => {
            print!("{}", hexdump_annotated(&bytes)?);
            return Ok(());
        }
        OutputMode::Json => {
            println!("{}", bason_to_json(&bytes)?);
            return Ok(());
        }
        OutputMode::Pretty => {}
    }

    let mut records = bason_decode_all(&bytes)?;
    if args.flatten {
        let mut flat = Vec::new();
        for record in &records {
            flat.extend(bason_flatten(record));
        }
        records = flat;
    }

    if args.unflatten {
        records = vec![bason_unflatten(&records)?];
    }

    for record in &records {
        if let Some(mask) = args.strictness {
            println!("valid(strictness={mask}): {}", bason_validate(record, mask));
        }
        print!("{}", pretty_print_record(record));
    }

    Ok(())
}
