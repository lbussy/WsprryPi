# Define color codes
RED='\033[31m'
GREEN='\033[32m'
NC='\033[0m' # No Color
CLEAR_LINE='\033[2K'
MOVE_UP='\033[1A'

# Function to simulate a task
simulate_task() {
    sleep 2  # Simulate a task taking 2 seconds
}

# Message to display
message="Processing data"

# Print the "[ ] $message" message with red braces
printf "${RED}[ ]${NC} $message\n"

# Simulate the task
simulate_task

# Move the cursor up and rewrite the line with "[✔]"
printf "${MOVE_UP}${CLEAR_LINE}${GREEN}[✔]${NC} $message\n"
