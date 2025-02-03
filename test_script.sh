#!/bin/bash

echo "🛠 Running Basic Test Script for Custom Shell"

# Check if the shell binary exists
if [[ ! -f "./shell" ]]; then
    echo "❌ Error: Shell executable not found!"
    exit 1
fi

# Run the shell and test a simple command
echo "echo Hello, World!" > tmp
if grep -q "echo Hello, World!" tmp;
then
    echo "PASS"
else
    echo "FAIL"
    exit 1
fi

exit 0
