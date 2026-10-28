#!/bin/bash

INPUT_FILE="/home/emy/Documents/Reversi/input.txt" # <---- Change as needed
RUN_SCRIPT="/home/emy/Documents/Reversi/run_games.sh" # <---- Change as needed

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Fehler: Datei $INPUT_FILE existiert nicht."
    exit 1
fi

# Call run_games.sh for every line of input
while IFS= read -r line; do
    ARGS=($(echo "$line" | tr -d ' ' | tr ',' ' '))
    bash "$RUN_SCRIPT" "${ARGS[@]}"
done < "$INPUT_FILE"

