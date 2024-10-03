#!/bin/env bash

echo "com.example.game$(awk '!/^[[:space:]]*$/ && !/^#/ && !(/[[:alnum:]]+[[:space:]]+[[:alnum:]]+[[:space:]]+[[:alnum:]]+/) {sub("-e ", ""); printf "|%s", $0}' "./gamelist.txt")" >module/gamelist.txt
