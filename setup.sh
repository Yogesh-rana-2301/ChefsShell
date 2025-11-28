#!/bin/bash

# Quick start script for ChefsShell Web Deployment

set -e

echo " ChefsShell Web Deployment Setup"
echo "===================================="
echo ""

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo " Node.js is not installed. Please install Node.js first."
    exit 1
fi

echo " Node.js version: $(node --version)"

# Check if npm is installed
if ! command -v npm &> /dev/null; then
    echo " npm is not installed. Please install npm first."
    exit 1
fi

echo " npm version: $(npm --version)"

# Check if gcc is installed
if ! command -v gcc &> /dev/null; then
    echo " gcc is not installed. Please install gcc first."
    exit 1
fi

echo " gcc version: $(gcc --version | head -n1)"
echo ""

# Navigate to deploy directory
cd "$(dirname "$0")/deploy"

# Install Node.js dependencies
echo " Installing Node.js dependencies..."
npm install

# Compile the C shell
echo ""
echo " Compiling ChefsShell..."
npm run build-shell

echo ""
echo " Setup complete!"
echo ""
echo " To start the server, run:"
echo "   cd deploy && npm start"
echo ""
echo "Then open http://localhost:3000 in your browser"
