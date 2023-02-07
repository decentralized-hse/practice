#!/bin/bash

if [ $# -lt 2 ]
then
    echo Please specify filename and block number in arguments.
    exit 1
fi
if [ ! -f $1.hashtree ]
then
    echo Hashtree file $1.hashtree does not exist.
    exit 1
fi

echo reading $1.hashtree...
hash_arr=()

while read line
do
    hash_arr+=($line)
done < $1.hashtree

echo putting the proof into $1.$2.proof...

tree_size=${#hash_arr[@]}
level=0
offset=$2
# formula for calculating bin number of cell using level and offset:
cur_cell=$(( (1 << level+1) * offset + (1 << level) - 1))

if [ $cur_cell -ge $tree_size ]
then
    echo Incorrect block number $2 for file $1.hashtree
    exit 1
fi

> $1.$2.proof

((offset = offset ^ 1)) # get sibling
cur_cell=$(( (1 << level+1) * offset + (1 << level) - 1))

while [ $cur_cell -lt $tree_size ]
do
    echo ${hash_arr[$cur_cell]} >> $1.$2.proof
    ((level = level + 1))    # get parent
    ((offset = offset >> 1)) # get parent
    ((offset = offset ^ 1))  # get sibling
    cur_cell=$(( (1 << level+1) * offset + (1 << level) - 1))
done

echo all done!
exit 0
