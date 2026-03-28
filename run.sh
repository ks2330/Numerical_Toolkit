#!/bin/sh
# Clear the terminal screen
clear 

echo "Running matrix tests..."

# It's better to use the direct path to the executable
# If your build folder is just 'build', use that:
./build/tests/matrix_test.exe

# Capture the exit code of the test itself
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "Tests passed!"
else
    echo "Tests failed with exit code $EXIT_CODE"
fi

exit $EXIT_CODE