# BASONLite (Assignment 7)

## Overview

This project implements a simplified version of a page-based B-tree storage engine inspired by SQLite.

The system stores data in a single file divided into fixed-size pages and organizes records in a B+ tree keyed by string keys.

This implementation is **partial** and focuses on the storage layer (PageManager and B-tree) and omits some advanced features such as overflow pages, full WAL-based recovery, and transaction management.


### Storage Layout

- The database is stored in a single file split into fixed-size pages.
- Page 1 contains the database header.
- Pages are 1-indexed.
- Remaining pages are used for:
    - B-tree nodes (leaf + interior)
    - freelist pages

### Page Manager

`PageManager` is responsible for:

- reading/writing pages
- allocating new pages
- managing the freelist
- maintaining the database header


Freed pages are added to a singly-linked freelist. Allocation prefers freelist pages before extending the file.
The system guarantees that all unused pages are returned to the freelist and no page leaks occur.

### B-tree

The B-tree is a **B+ tree** with the following properties:

- Keys are stored in sorted order within each node.
- Leaf nodes contain full records.
- Interior nodes contain separator keys and child pointers.
- All data is stored in leaves.
- All leaves are at the same depth.

To maintain B-tree invariants after deletions:

- Nodes below minimum occupancy are rebalanced
- Preference order:
    1. Borrow from left/right sibling
    2. Merge with sibling if redistribution is not possible


We validate the following invariants through tests.

