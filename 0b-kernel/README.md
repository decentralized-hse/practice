
The objective of this practice is to get some insight into real-world git workings

You present code and/or data that solves the problem or answers the question.
- [ ] What is the level of complexity of the git codebase? Compare it to small/medium databases, give a complexity budget for the project, explain where the effort was spent.
- [ ] It is notoriously difficult to be compatible with git. There are several projects of different degree of incompatibility. Meanwhile, the basic model of git (trees,blobs,commits) is very simple. Explain why it did not become the unifying layer, give some practical example, preferably in code.
- [ ] Estimate the efficiency of git as a compressing utility. One way to do it: for a medium-size repo, get a sequence of revisions of every file. Compress them with different tools (I recommend 7z). Compare the resulting sizes.
- [ ] “Buy 100 chebureks, assemble a dog” type of project. Take a sequence of file revisions as diffs to the previous commit. Try to reassemble the file. Describe the cases when it works and when it breaks.
- [ ] Implement grep for historical versions of file(s) in a repo.
- [ ] Take a Linux kernel repo. For any two versions, produce stats (df -kh like): where the changes happened (which dirs, how much).
- [ ] Kernel.org has lots of kernel-related repos.  For arbitrary pairs of repos, try merging them (all branches). Did the repo grow by much? Study what happened.
- [ ] Make a report on kernel.org crypto use. Who signs, how often, what kind of sigs.
- [ ] Try tracing the story of some kernel changes. Those typically get re-bundled into different commits as they progress to mainline. Detail the process, give some stats.
- [ ] How is Rust doing in the kernel? Give some stats on the progress of the effort.
- [ ] Produce a Top100 contributor list for the kernel. Order by commits, LoCs, aggregate by orgs (based on email addresses)
- [ ] Make a git utility: for a given file in a repo, produce Top5 files that get often changed in the same commit (linked files)
- [ ] Track the dynamic of data changed vs lines changed in files, make stats (accumulation of loc-changed vs actual binary diff of first and last versions)
- [ ] Try finding lines of Linux written by Linus Torvalds. Where are they?
- [ ] Devise your own, more logical set of git commands. Explain why is it better.
- [ ] Many package managers use git as a database, many regret that. Find the bottlenecks and bench them on real data.
- [ ] Dig git codebase for low-level performance optimizations, make your own Top10 chart, explain.
- [ ] Devise your own approach to submodules (preferably a better one)

