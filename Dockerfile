# Multi-stage build for ChefsShell
FROM node:20-bookworm AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libreadline-dev \
    python3 \
    python3-setuptools \
    gcc \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy package files
COPY deploy/package*.json ./deploy/

# Install Node dependencies
WORKDIR /app/deploy
RUN npm ci --only=production

# Copy source code
WORKDIR /app
COPY src/ ./src/
COPY deploy/ ./deploy/

# Compile the C shell
RUN cd /app && gcc -Wall -Wextra -o chefs_shell src/main.c -lreadline

# Production stage
FROM node:20-bookworm-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libreadline8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy from builder
COPY --from=builder /app/deploy /app/deploy
COPY --from=builder /app/chefs_shell /app/chefs_shell

# Verify binary exists
RUN ls -lh /app/chefs_shell && chmod +x /app/chefs_shell

WORKDIR /app/deploy

# Expose port
EXPOSE 3000

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD node -e "require('http').get('http://localhost:3000', (r) => process.exit(r.statusCode === 200 ? 0 : 1))"

# Start server
CMD ["node", "server.js"]
