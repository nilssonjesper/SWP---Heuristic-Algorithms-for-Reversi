#!/bin/bash

# Paths
SERVER_LOG="/home/emy/Documents/Reversi/linux-amd64/server_output.log" # <---- Change as needed
SERVER_DIR="/home/emy/Documents/Reversi/linux-amd64" # <---- Change as needed
CLIENT_DIR="/home/emy/Documents/Reversi/group7-main" # <---- Change as needed

# Terminate game binaries
terminator() {
    echo "Terminating background processes..."
    kill $SERVER_PID $CLIENT_PID $AI_PID 2>/dev/null
    exit 0
}

# Assert correct input formatting
if [ $# -ne 4 ]; then
    echo "Usage: $0 <arg1> <arg2> <arg3> <arg4>"
    exit 1
fi

echo "Received: " "$1" "$2" "$3" "$4"

trap terminator SIGINT

# Launch server
echo "Starting Server..........."
cd $SERVER_DIR
./server Map1.map -t 2 2>&1 | tee "$SERVER_LOG" &
SERVER_PID=$!
sleep 2

# Launch client
echo "Starting Player.........."
cd $CLIENT_DIR
mkdir -p build
cd build
cmake ..
cmake --build .
cd $CLIENT_DIR
cd bin
./swp2025_group7 "$1" "$2" "$3" "$4" & # The main was modified to take the weights as args (rather than hostname and port)
CLIENT_PID=$!
sleep 2

# Launch ai
echo "Starting Enemy AI.........."
cd $SERVER_DIR
./ai > /dev/null 2>&1 &
AI_PID=$!

: > "$SERVER_LOG" 

# Wait for game over
while true; do
    if grep -q -i "Game has ended" "$SERVER_LOG"; then
        echo "'Game over' detected in logs."
        {
        echo "Game Result"
        tail -n 50 "$SERVER_LOG" | grep -iE "Player 1:|Player 2:"
        } >> /home/emy/Documents/Reversi/Results.txt
        terminator
    fi
    sleep 1
done

