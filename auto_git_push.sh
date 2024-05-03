#!/bin/bash

# Check for changes; if none, exit
git diff --exit-code --quiet && git diff --staged --exit-code --quiet && exit

# Automatic git commands
git add .
git commit -m "Automatic commit on $(date)"
git push origin main