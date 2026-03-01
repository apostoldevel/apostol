FROM golang:1.23-alpine AS builder

WORKDIR /app

COPY go.mod ./
COPY main.go .
RUN go mod tidy
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o /bench-go .

FROM alpine:3.20

RUN apk add --no-cache ca-certificates

COPY --from=builder /bench-go /usr/local/bin/bench-go

ENV WORKERS=4

CMD ["bench-go"]

EXPOSE 4000
