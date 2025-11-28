#!/usr/bin/env bash
# Build script for Render deployment

set -e

echo "🔧 Installing system dependencies..."

# Install readline library (if needed)
# Render provides most common libraries by default

echo "📦 Installing Node.js dependencies..."
cd deploy
npm install

echo "🔨 Compiling ChefsShell..."
cd ..
make -f deploy/Makefile

echo "✅ Build complete!"
