# Paths varying

To vary routing paths between periods we send announces to subset of entrypoints. Thus, network topology can look different every period. Subsets are selected using a sliding window of half a size of list of entrypoints. So, for $n$ entrypoints algorithm looks like this:
- $entrypoints_segment_start$ is set to 0
- $\lceil\frac{n}{2}\rceil$ entrypoints starting from index $entrypoints_segment_start$ are selected to recieve announce from given node
- $entrypoints_segment_start$ is incremented
- next hour (basic period size) topology stays unchanged
- starting next period announces are resent to $\lceil\frac{n}{2}\rceil$ entrypoints starting from index $entrypoints_segment_start$
- and so on.

To preserve subsets' sizes sliding window are based on circular array. In other words to take $k$-th entrypoint we take entrypoint indexed by k modulo n ($k$ % $n$)
