#!/bin/bash
# 159: run a command and record it. usage: tools/rec.sh NAME cmd...   -> raw/NAME.{stdout,stderr,exit}, line in raw/commands.txt
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/159-p1-exact-phrase-hash"; N=$1; shift
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START $N: $*" >> "$E/raw/commands.txt"
"$@" > "$E/raw/$N.stdout" 2> "$E/raw/$N.stderr"; rc=$?; echo $rc > "$E/raw/$N.exit"
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END $N exit=$rc" >> "$E/raw/commands.txt"; exit $rc
