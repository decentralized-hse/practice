use std::path::Path;

mod common;

#[cfg(test)]
mod integration_tests {
    use super::*;
    use crate::common::{TestRepo, run_git_grep, assert_success, assert_output_contains};
    
    #[test]
    fn test_basic_grep_in_single_commit() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("file1.txt", "hello world\nerror found\nanother line");
        test_repo.create_file("file2.txt", "no matches here");
        test_repo.create_file("src/lib.rs", "fn main() {\n    println!(\"hello\");\n}");
        
        test_repo.add_and_commit("Initial commit");
        
        let output = run_git_grep(&["HEAD", "error"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "file1.txt: error found");
        
        let output = run_git_grep(&["HEAD", "error", "--with-lines"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "file1.txt:2: error found");
        
        let output = run_git_grep(&["HEAD", "hello", "--pathspec", "*.txt"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "file1.txt: hello world");
    }
    
    #[test]
    fn test_grep_across_multiple_commits() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("app.log", "INFO: started\nERROR: connection failed");
        let commit1 = test_repo.add_and_commit("Add log file");
        
        test_repo.create_file("app.log", "INFO: started\nINFO: connected\nDEBUG: processing");
        test_repo.add_and_commit("Update log file");
        
        let output = run_git_grep(&["HEAD~1..HEAD", "DEBUG"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "DEBUG: processing");
        
        let output = run_git_grep(&["HEAD", "ERROR"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, &format!("{}:app.log: ERROR: connection failed", &commit1.to_string()[..7]));
    }
    
    #[test]
    fn test_grep_with_diff_mode() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("config.json", "{\n  \"port\": 8080,\n  \"host\": \"localhost\"\n}");
        test_repo.add_and_commit("Add config");
        
        test_repo.create_file("config.json", "{\n  \"port\": 9090,\n  \"host\": \"0.0.0.0\",\n  \"debug\": true\n}");
        test_repo.add_and_commit("Update config");
        
        let output = run_git_grep(&["HEAD^..HEAD", "9090", "--use-diff"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "config.json:+: \"port\": 9090,");
        
        let output = run_git_grep(&["HEAD^..HEAD", "0.0.0.0", "--use-diff"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "config.json:+: \"host\": \"0.0.0.0\",");
        
        let output = run_git_grep(&["HEAD^..HEAD", "debug", "--use-diff"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "config.json:+: \"debug\": true");
    }
    
    #[test]
    fn test_regex_patterns() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("test.log", 
            "2024-01-01 10:00:00 INFO: started\n\
             2024-01-01 10:00:01 ERROR: timeout\n\
             2024-01-01 10:00:02 DEBUG: var=42\n\
             2024-01-01 10:00:03 INFO: completed");
        
        test_repo.add_and_commit("Add log");
        
        let output = run_git_grep(&["HEAD", r"\d{2}"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "42");
        
        let output = run_git_grep(&["HEAD", "^2024"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "2024-01-01");
        
        let output = run_git_grep(&["HEAD", r"ERR(R|OR)"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "ERROR");
    }
    
    #[test]
    fn test_complex_pathspec() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("src/main.rs", "fn main() { println!(\"Hello\"); }");
        test_repo.create_file("src/lib.rs", "pub fn helper() { 42 }");
        test_repo.create_file("tests/test.rs", "#[test] fn test_main() {}");
        test_repo.create_file("docs/readme.md", "# Documentation\nHello world");
        test_repo.create_file("src/gui/mod.rs", "pub fn draw() {}");
        
        test_repo.add_and_commit("Add project files");
        
        let output = run_git_grep(&["HEAD", "fn", "--pathspec", "*.rs"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "main.rs");
        assert_output_contains(&output, "lib.rs");
        assert_output_contains(&output, "test.rs");
        
        let output = run_git_grep(&["HEAD", "Hello", "--pathspec", "*.md", "--pathspec", "*.rs"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "readme.md: Hello world");
        assert_output_contains(&output, "main.rs: fn main() { println!(\"Hello\"); }");
    }
    
    #[test]
    fn test_edge_cases() {
        let test_repo = TestRepo::init().unwrap();
        
        test_repo.create_file("empty.txt", "");
        test_repo.create_file("no_newline.txt", "single line without newline");
        test_repo.create_file("unicode.txt", "Привет мир!\nこんにちは世界\n🌍");
        
        test_repo.add_and_commit("Add edge case files");
        
        let output = run_git_grep(&["HEAD", "Привет"], test_repo.path_ref());
        assert_success(&output);
        assert_output_contains(&output, "Привет мир!");
    }
    
    #[test]
    fn test_error_handling() {
        let output = run_git_grep(&["HEAD", "pattern"], Path::new("/non/existent/path"));
        assert!(!output.status.success());
        
        let test_repo = TestRepo::init().unwrap();
        let output = run_git_grep(&["non-existent-branch", "pattern"], test_repo.path_ref());
        assert!(!output.status.success());
        
        let test_repo = TestRepo::init().unwrap();
        let output = run_git_grep(&["HEAD", r"[invalid"], test_repo.path_ref());
        assert!(!output.status.success());
    }
}