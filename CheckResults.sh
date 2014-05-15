#! /bin/bash

sort $1 -o $1

grep -v -f $1 $2


