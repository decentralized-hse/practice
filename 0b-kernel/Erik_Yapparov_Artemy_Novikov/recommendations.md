# How to reduce merge conflicts

1. **Hierarchical merges** — maintainers resolve conflicts at subsystem level before sending upstream. Drops real conflict rate from ~19% to ~3%.
2. **Integration tree (linux-next)** — daily automated merge of all subsystem branches catches conflicts early.
3. **Rebase often** — subsystem branches should rebase on mainline regularly to reduce divergence.
4. **Split global files** — break monolithic files edited by everyone into per-subsystem fragments.
5. **Keep patches small** — fewer files touched per commit = smaller conflict surface. Don't mix formatting with functional changes.
6. **CI conflict detection** — run `git merge-tree` in CI to predict conflicts before the merge window.
