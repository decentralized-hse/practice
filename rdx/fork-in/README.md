#   Fork: ingestion

Standard UNIX tools like `diff`, `patch` etc are all line based.
Similarly, `git` is line based as well as all the historical 
revision control systems. That has an advantage of simplicity,
but can be crude or misleading at times. Consider the cases
of miscalculated diffs or single-character changes. In the
first case, repeating lines can confuse the diff algorithm 
into producing a weird diff. In the second case, the change
may not be immediately obvious to a reader, as it is one 
character in a long line of text. Line-based is coarse.

Real-time editors (Google Docs et all) tend to be character 
based. First of all, that allows for real time collaboration.
Second, merges become easier: changes to the same line no
longer produce a conflict. This approach has its downsides
though. First, per-character diff is computationally expensive
as the algorithm's run time is quadratic to the length of the
input. Second, per-character diff is easier to confuse into
producing absurd diffs as characters are much less unique than
strings. The google's `diff-match-patch` library uses a ton
of heuristics to "normalize" diffs, for example.

JetBrains experimented with AST based revision control at some
point, but the project was cancelled due to its excessive
complexity. Syntax-aware diffs and merges are cool, but once
they start entangling with the grammar of the language, things
become weird real soon.

The idea of this project is to experiment with token based 
revision control on top of RDX. It is kind of a middle ground:
sufficiently syntax-aware, reasonably fine-grained, but not too
much. The advantages we plan to realize are many:

 1. being completely indifferent to whitespace and formatting;
    each developer can choose "tabs or spaces" privately;
 2. being fine-grained enough for real time collaboration
    without the downsides of character-based diffing;
 3. meaningful units of change, e.g. function and variable names
 4. better diff combinatorics;
 5. more readable diffs.

The approach is to use some tokenizer to parse a file into an
RDX collection (an array of tokens). Then, run diff/merge/
whatever over tokens, not lines or characters.
In JDR, a regular C Hello World may look like this:
````
    [
        "#include" "<stdio.h>"
        "#include" "<stdint.h>"

        int main "(" int argn "," char "**" args ")" "{"
            uint8_t a "[]" "=" "{" 1 "," 2 "," 3 "}" ";"
            printf "(" "\"Hello world %i\\n'\"" "," a "[" 0 "]" ")" ";"
        "}"
    ]
````
Not too ugly. Essentially, this is a stream of tokens.
Here, `int`, `main` etc are RDX Terms, i.e. unquoted Base64 strings.
Any numbers can go as-as, i.e. RDX Integer or Float.
Everything else has to go quoted as an RDX String.

##  `fork-in`

Rules of tokenisation obviously differ for different languages.
`fork-in` is a common interface to three implementations:

 1. `fork-in-lsp` Language Server Protocol based,
 2. `fork-in-dt` difftastic based,
 3. `fork-in-regex` regex based.

 The frontend detects the file format and invokes the relevant 
 backend with relevant parameters.

In all the cases, input/outputs are like:

  - input: a file
  - output: an RDX blob (the token stream)

##  `fork-out`

The command outputs an RDX blob back into the original format 
and optionally formats it according to the local settings. One
may use formatting programs for common languages (C, Markdown, 
whatever).

  - input: an RDX blob
  - output: nicely formatted file

##  `fork-diff`

The command produces a tokenized patch by comparing two RDX blobs
(token streams). 

1. `fork-diff-myers` implements the Myers' algorithm,
2. `fork-diff-histogram` implements git's histogram or any other
   advanced algorithm that is better than raw Myers.

  - input: two RDX blobs (likely, old and new from `fork-in`)
  - output: one RDX diff

For example, if we rename `argn` to `argnum` in the hello world 
example above, the RDX diff may look like:
````
    [<8>, <-1>, "argnum", <29>]
````
Here, one-int-tuples express untouched or removed tokens while
strings, terms, ints and floats are all new tokens.

##  Challenges

The main challenge is to maintain the round-trip guarantee.
That means, the revision control system produces a file that is
equivalent to the original one.

Ideally, that should be verified by a compiler, i.e. the result
of the code being compiled should be the same, bitwise, for the
original file and the file that was round-tripped through the
system: `fork-in`, `fork-out`. Another option is to format the
file using some standard settings and compare the results byte
by byte.

Both ways of verifying the correctness have some potential 
gotchas. For example, different versions of `clang-format` can 
produce different files from same inputs, same settings.
Compilers can be whitespace-sensitive because of debugging
info being included or maybe for no particular reason at all.

Find your way and fuzz it.

##  Prior art

There was a quite promising tokenized diff project a year ago:

http://github.com/shishyando/tokenized-myers-diff

Feel free to borrow any necessary code.
