#!/bin/bash

if [ $# -ne 3 ]; then
    echo "usage: $0 <char> <path> <0/1 0:dryrun>"
    exit
fi

if [ $3 == "0" ]; then
    cmd='q!'
elif [ $3 == "1" ]; then
    cmd='wq'
else
    echo "usage: $0 <char> <path> <0/1 0:dryrun>"
    exit
fi

ex -nsc 'redir! >/dev/stderr' -c "f" -c "%s/$1\$/PATTERN/g" -c 'redir END' -c $cmd $2
echo

# for i in *.tbl; do  bash ~/simdstor/tpch/script/removelastc.sh \| $i 0 & done; wait 2> /dev/null
