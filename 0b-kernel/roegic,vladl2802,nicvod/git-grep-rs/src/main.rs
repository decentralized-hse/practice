use std::path::Path;

use anyhow::{Context, Result, bail};
use clap::Parser;
use git2::{
    Blob, Commit, Diff, DiffOptions, ObjectType, Oid, Pathspec, PathspecFlags, Repository,
    RevparseMode, Revspec, Revwalk, Tree, TreeWalkMode, TreeWalkResult,
};
use regex::bytes::Regex;

struct LineFormatter {
    with_lines: bool,
}

impl LineFormatter {
    fn line_to_display(line: &[u8]) -> String {
        String::from_utf8_lossy(line).trim().to_string()
    }

    fn format_match(&self, path: &Path, short_commit: &str, line_num: usize, line: &[u8]) {
        let line = Self::line_to_display(line);
        let path = path.to_str().unwrap();
        if self.with_lines {
            println!("{short_commit}:{path}:{}: {line}", line_num + 1);
        } else {
            println!("{short_commit}:{path}: {line}");
        }
    }

    fn format_diff_match(
        &self,
        path: &Path,
        short_commit: &str,
        origin: char,
        line_num: usize,
        line: &[u8],
    ) {
        let line = Self::line_to_display(line);
        let path = path.to_str().unwrap();
        if self.with_lines {
            println!("{short_commit}:{path}:{origin}{line_num}: {line}");
        } else {
            println!("{short_commit}:{path}:{origin}: {line}");
        }
    }
}

struct DirWalker<'a> {
    repo: &'a Repository,
    re: Regex,
    pathspec: Pathspec,
    formatter: LineFormatter,
}

impl<'a> DirWalker<'a> {
    fn short_name(commit: &Commit) -> Result<String> {
        let short_id = commit.as_object().short_id()?;
        Ok(String::from_utf8(short_id.to_vec()).unwrap())
    }

    fn grep_commit(&self, tree: &Tree, commit: &Commit) -> Result<()> {
        let short_name = Self::short_name(commit)?;

        tree.walk(TreeWalkMode::PreOrder, |root, entry| {
            if entry.kind() != Some(ObjectType::Blob) {
                return TreeWalkResult::Ok;
            }
            let name = entry.name().unwrap_or("<unknown>");
            let Ok(blob) = self.repo.find_blob(entry.id()) else {
                return TreeWalkResult::Ok;
            };
            let path = Path::new(root).join(name);

            self.grep_blob(&path, &blob, &short_name)
        })
        .context("failed to walk tree")?;

        Ok(())
    }

    fn grep_blob(&self, path: &Path, blob: &Blob, short_commit: &str) -> TreeWalkResult {
        if !self.pathspec.matches_path(path, PathspecFlags::DEFAULT) {
            return TreeWalkResult::Ok;
        }

        let data = blob.content();

        for (line_num, line) in data.split(|&b| b == b'\n').enumerate() {
            if !self.re.is_match(line) {
                continue;
            }

            if self.re.is_match(line) {
                self.formatter
                    .format_match(path, short_commit, line_num, line);
            }
        }

        TreeWalkResult::Ok
    }

    fn grep_diff(&self, diff: &Diff, commit: &Commit) -> Result<()> {
        let short_commit = Self::short_name(commit)?;

        diff.foreach(
            &mut |_, _| true,
            None,
            Some(&mut |_, _| true),
            Some(&mut |delta, _hunk, line| {
                let origin = line.origin();
                if origin != '+' && origin != '-' {
                    return true;
                }

                let path = delta
                    .new_file()
                    .path()
                    .or_else(|| delta.old_file().path())
                    .unwrap();

                let content = line.content();
                if self.re.is_match(content) {
                    let line_num = if let Some(old_num) = line.old_lineno() {
                        if origin != '-' {
                            return false;
                        }
                        old_num
                    } else if let Some(new_num) = line.new_lineno() {
                        if origin != '+' {
                            return false;
                        }
                        new_num
                    } else {
                        return false;
                    };

                    self.formatter.format_diff_match(
                        path,
                        &short_commit,
                        origin,
                        line_num as usize,
                        line.content(),
                    );
                }

                true
            }),
        )
        .context("failed to iter diff")?;

        Ok(())
    }
}

fn walk_revspec<'a>(repo: &'a Repository, revspec: Revspec<'a>) -> Result<Revwalk<'a>> {
    let peel_commit_oid = |obj: &git2::Object| -> Result<Oid> {
        let commit = obj.peel_to_commit()?;
        Ok(commit.id())
    };
    let to = revspec.to().map(peel_commit_oid).transpose()?;
    let from = revspec.from().map(peel_commit_oid).transpose()?;

    let mut walk = repo.revwalk().context("failed to create revwalk")?;

    match revspec.mode() {
        RevparseMode::SINGLE => {
            let start = to.or(from).context("single rev missing")?;
            walk.push(start)?;
        }
        RevparseMode::RANGE => {
            let to = to.context("range missing right side")?;
            walk.push(to)?;
            if let Some(from) = from {
                walk.hide(from)?;
            }
        }
        RevparseMode::MERGE_BASE => {
            let l = from.context("merge missing left side")?;
            let r = to.context("merge missing right side")?;

            walk.push(l)?;
            walk.push(r)?;
            walk.hide(repo.merge_base(l, r)?)?;
        }
        mode => bail!("unknown revparse mode: {:?}", mode),
    }
    Ok(walk)
}

#[derive(Parser, Debug)]
#[command(name = "git-grep")]
struct Args {
    repo: String,

    revspec: String,

    pattern: String,

    #[arg(
        long,
        help = "implies first-parent by default, but could be overriden with ignore-merges"
    )]
    use_diff: bool,

    #[arg(long)]
    first_parent: bool,

    #[arg(long)]
    ignore_merges: bool,

    #[arg(long)]
    with_lines: bool,

    #[arg(long = "pathspec")]
    pathspecs: Vec<String>,
}

impl Args {
    fn canonize(mut self) -> Self {
        if self.use_diff {
            if !self.first_parent && !self.ignore_merges {
                self.first_parent = true;
            }
        }

        self
    }
}

fn main() -> Result<()> {
    let args = Args::parse();
    let args = args.canonize();

    let re =
        Regex::new(&args.pattern).with_context(|| format!("invalid regex: {}", args.pattern))?;

    let repo = Repository::open(&args.repo)
        .with_context(|| format!("failed to open repo at {}", args.repo))?;

    let revspec = repo
        .revparse(&args.revspec)
        .with_context(|| format!("failed to rev-parse {}", args.revspec))?;

    let pathspec =
        Pathspec::new(&args.pathspecs).with_context(|| format!("failed to create pathspec"))?;

    let mut walk = walk_revspec(&repo, revspec)?;
    walk.set_sorting(git2::Sort::TIME | git2::Sort::TOPOLOGICAL)?;

    let walker = DirWalker {
        repo: &repo,
        re,
        pathspec,
        formatter: LineFormatter {
            with_lines: args.with_lines,
        },
    };

    for oid in walk {
        let oid = oid?;
        let commit = repo.find_commit(oid)?;

        if args.ignore_merges && commit.parent_count() > 1 {
            continue;
        }

        let tree = commit.tree()?;

        if args.use_diff {
            // Do not need to check for the first_parent here
            let parent = commit.parent(0).ok();
            let parent_tree = parent.map(|parent| parent.tree()).transpose()?;

            let mut diff_opts = DiffOptions::new();
            for pathspec in &args.pathspecs {
                diff_opts.pathspec(pathspec);
            }

            let diff =
                repo.diff_tree_to_tree(parent_tree.as_ref(), Some(&tree), Some(&mut diff_opts))?;
            walker.grep_diff(&diff, &commit)?;
        } else {
            walker.grep_commit(&tree, &commit)?;
        }
    }

    Ok(())
}
