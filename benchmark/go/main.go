package main

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"os"
	"runtime"
	"strconv"
	"time"

	"github.com/jackc/pgx/v5/pgxpool"
)

var pool *pgxpool.Pool

var pingResponse = []byte(`{"ok":true,"message":"OK"}`)

const contentType = "application/json; charset=utf-8"

func pingHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", contentType)
	w.Write(pingResponse)
}

func timeHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", contentType)
	fmt.Fprintf(w, `{"serverTime":%d}`, time.Now().Unix())
}

func dbPingHandler(w http.ResponseWriter, r *http.Request) {
	var result string
	err := pool.QueryRow(r.Context(),
		"SELECT json_build_object('ok', true, 'message', 'OK')::text",
	).Scan(&result)
	if err != nil {
		http.Error(w, `{"error":"db error"}`, http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", contentType)
	w.Write([]byte(result))
}

func dbTimeHandler(w http.ResponseWriter, r *http.Request) {
	var result string
	err := pool.QueryRow(r.Context(),
		"SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text",
	).Scan(&result)
	if err != nil {
		http.Error(w, `{"error":"db error"}`, http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", contentType)
	w.Write([]byte(result))
}

func main() {
	workers, _ := strconv.Atoi(os.Getenv("WORKERS"))
	if workers > 0 {
		runtime.GOMAXPROCS(workers)
	}

	dbHost := os.Getenv("DB_HOST")
	if dbHost == "" {
		dbHost = "bench-postgres"
	}
	dbPort := os.Getenv("DB_PORT")
	if dbPort == "" {
		dbPort = "5432"
	}

	dsn := fmt.Sprintf("postgres://bench:bench@%s:%s/bench?pool_min_conns=5&pool_max_conns=15", dbHost, dbPort)

	var err error
	pool, err = pgxpool.New(context.Background(), dsn)
	if err != nil {
		log.Fatalf("Unable to connect to database: %v", err)
	}
	defer pool.Close()

	mux := http.NewServeMux()
	mux.HandleFunc("/api/v4/ping", pingHandler)
	mux.HandleFunc("/api/v4/time", timeHandler)
	mux.HandleFunc("/api/v4/db/ping", dbPingHandler)
	mux.HandleFunc("/api/v4/db/time", dbTimeHandler)

	log.Printf("Go server listening on :4000 (GOMAXPROCS=%d)", runtime.GOMAXPROCS(0))
	if err := http.ListenAndServe(":4000", mux); err != nil {
		log.Fatal(err)
	}
}
