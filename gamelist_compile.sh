#!/bin/env bash

echo "com.example.gamelist1|com.example.gamelist2$(awk '!/^[[:space:]]*$/ && !/^#/ && !(/[[:alnum:]]+[[:space:]]+[[:alnum:]]+[[:space:]]+[[:alnum:]]+/) {sub("-e ", ""); printf "|%s", $0}' "./gamelist.txt")" >module/gamelist.txt
