# Git Contributors Stats Tool

## Overview

This program analyzes a Git repository and produces statistics about contributors based on their commit history.

It must be executed **from within a Git repository**, as it reads commit data directly from the local Git history.

---

## How It Works
The program reads commit history using Git and extracts:

* Author email
* Commit message

It then:

* Groups commits by contributor
* Analyzes contribution patterns
* Classifies contributors based on activity

---

## What Statistics It Produces

### General Metrics

* Total number of commits
* Total number of contributors

### Contributor Activity

* Percentage of **one-time contributors** (authors with exactly one commit)
* Percentage of **repeat committers** (authors with more than one commit)

### One-Time Contribution Roles

Based on commit message content:

* Percentage of **one-time bugfixers**
  (contributors with a single commit containing keywords like `fix` or `bug`)

* Percentage of **one-feature contributors**
  (contributors with a single commit containing keywords like `feat` or `feature`)

### Contribution Share

For each contributor, the program calculates:

* Number of commits
* Percentage of total commits in the repository

---

## Output Summary

* Total commits
* Total contributors
* % of one-time contributors
* % of repeat committers
* % of one-time bugfixers
* % of one-feature contributors
* Individual contribution percentage per author

---

## Important Note

The program analyzes the repository from the current working directory.
It must be run while located inside the target Git repository to produce valid statistics.
