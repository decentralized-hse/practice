use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Output};
use tempfile::TempDir;
use git2::Repository;

pub struct TestRepo {
    pub dir: TempDir,
    pub repo: Repository,
}

impl TestRepo {
    pub fn init() -> Result<Self, Box<dyn std::error::Error>> {
        let dir = TempDir::new()?;
        let repo = Repository::init(dir.path())?;
        
        let mut config = repo.config()?;
        config.set_str("user.name", "test")?;
        config.set_str("user.email", "test@example.com")?;
        
        Ok(TestRepo { dir, repo })
    }
    
    pub fn path_ref(&self) -> &Path {
        self.dir.path()
    }
    
    pub fn create_file(&self, name: &str, content: &str) -> PathBuf {
        let path = self.dir.path().join(name);
        if let Some(parent) = path.parent() {
            let _ = fs::create_dir_all(parent);
        }
        fs::write(&path, content).unwrap();
        path
    }
    
    pub fn add_and_commit(&self, message: &str) -> git2::Oid {
        let mut index = self.repo.index().unwrap();
        let _ = index.add_all(["*"].iter(), git2::IndexAddOption::DEFAULT, None);
        index.write().unwrap();
        
        let tree_id = index.write_tree().unwrap();
        let tree = self.repo.find_tree(tree_id).unwrap();
        let signature = git2::Signature::now("test", "test@example.com").unwrap();
        
        let parent_commit = match self.repo.head() {
            Ok(head) => {
                let head_ref = head.target().unwrap();
                Some(self.repo.find_commit(head_ref).unwrap())
            }
            Err(_) => None,
        };
        
        let parents: Vec<&git2::Commit> = parent_commit.as_ref().map(|c| vec![c]).unwrap_or_default();
        
        self.repo.commit(
            Some("HEAD"),
            &signature,
            &signature,
            message,
            &tree,
            &parents,
        ).unwrap()
    }
}

pub fn get_binary_path() -> PathBuf {
    let bin_path = assert_cmd::cargo::cargo_bin!("git-grep-rs");
    PathBuf::from(bin_path)
}

pub fn run_git_grep(args: &[&str], repo_path: &Path) -> Output {
    Command::new(get_binary_path())
        .arg(repo_path.to_str().unwrap())
        .args(args)
        .output()
        .unwrap()
}

pub fn assert_output_contains(output: &Output, expected: &str) {
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(stdout.contains(expected), "Expected '{}' in:\n{}", expected, stdout);
}

pub fn assert_success(output: &Output) {
    assert!(output.status.success(), "Command failed: {}", String::from_utf8_lossy(&output.stderr));
}