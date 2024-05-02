#!/bin/bash

# Navigate to your project directory (edit this path accordingly)
cd /Users/ethan/Documents/GitHub/cia-chat

# Check for changes; if none, exit
git diff --exit-code --quiet && git diff --staged --exit-code --quiet && exit

# Automatic git commands
git add .
git commit -m "Automatic commit on $(date)"
git push origin main  # Change 'main' to your branch name if different
