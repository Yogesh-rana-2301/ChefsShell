#!/usr/bin/env bash
# Build script for Render deployment

set -e

echo "🔧 Installing system dependencies..."

# Ensure we have build essentials and readline
apt-get update || true
apt-get install -y build-essential libreadline-dev python3 python3-setuptools || true

# For node-pty compilation - set Python path
export PYTHON=$(which python3)

echo "📦 Installing Node.js dependencies..."
cd deploy

# Use legacy OpenSSL provider for older Node versions if needed
export NODE_OPTIONS="--openssl-legacy-provider" || true

# Install with verbose logging
npm install --verbose

echo "🔨 Compiling ChefsShell..."
cd ..
make -f deploy/Makefile

echo "✅ Build complete!"
echo "Binary location: $(pwd)/chefs_shell"
ls -lh chefs_shell || echo "Warning: Binary not found"
