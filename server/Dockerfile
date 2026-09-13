# Build Stage
FROM golang:alpine AS builder

WORKDIR /app

# Download dependencies
COPY go.mod go.sum ./
RUN go mod download

# Copy source code
COPY . .

# Build statically linked binary with stripped debug symbols
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o power-server .

# Runtime Stage
FROM alpine:3.21

# Install timezone data & certs
RUN apk --no-cache add ca-certificates tzdata

ENV TZ=Asia/Jakarta
ENV PORT=8080
ENV DB_PATH=/app/data/power.db

WORKDIR /app

# Create directory for SQLite database mount
RUN mkdir -p /app/data

# Copy binary from builder
COPY --from=builder /app/power-server /app/power-server

EXPOSE 8080

VOLUME ["/app/data"]

ENTRYPOINT ["/app/power-server"]
