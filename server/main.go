package main

import (
	"database/sql"
	"embed"
	"encoding/json"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	_ "modernc.org/sqlite"
)

//go:embed web/*
var webFS embed.FS

type Metric struct {
	ID        int64     `json:"id"`
	Voltage   float64   `json:"voltage"`
	Current   float64   `json:"current"`
	Power     float64   `json:"power"`
	Energy    float64   `json:"energy"`
	Frequency float64   `json:"frequency"`
	PF        float64   `json:"pf"`
	CreatedAt time.Time `json:"created_at"`
}

type Summary struct {
	Latest          *Metric `json:"latest"`
	IsOnline        bool    `json:"is_online"`
	LastSeenSeconds int     `json:"last_seen_seconds"`
	EnergyTodayKWh  float64 `json:"energy_today_kwh"`
	PeakPowerToday  float64 `json:"peak_power_today"`
	AvgVoltageToday float64 `json:"avg_voltage_today"`
	AvgPFToday      float64 `json:"avg_pf_today"`
	EstimatedCost   float64 `json:"estimated_cost"`
	PLNRate         float64 `json:"pln_rate"`
	TotalRecords    int64   `json:"total_records"`
}

type Server struct {
	db      *sql.DB
	apiKey  string
	plnRate float64
}

func parseDBTime(str string) time.Time {
	formats := []string{
		time.RFC3339,
		"2006-01-02T15:04:05Z",
		"2006-01-02 15:04:05",
		"2006-01-02T15:04:05",
	}
	for _, f := range formats {
		if t, err := time.Parse(f, str); err == nil {
			return t.UTC()
		}
	}
	return time.Time{}
}

func main() {
	port := getEnv("PORT", "8080")
	dbPath := getEnv("DB_PATH", "./data/power.db")
	apiKey := getEnv("API_KEY", "")
	rateStr := getEnv("PLN_RATE", "1444.70")

	plnRate, err := strconv.ParseFloat(rateStr, 64)
	if err != nil {
		plnRate = 1444.70
	}

	// Ensure database directory exists
	dir := filepath.Dir(dbPath)
	if err := os.MkdirAll(dir, 0755); err != nil {
		log.Fatalf("Failed to create db dir: %v", err)
	}

	// Open SQLite with WAL mode & busy timeout
	dsn := fmt.Sprintf("%s?_pragma=journal_mode(WAL)&_pragma=synchronous(NORMAL)&_pragma=busy_timeout(5000)", dbPath)
	db, err := sql.Open("sqlite", dsn)
	if err != nil {
		log.Fatalf("Failed to open database: %v", err)
	}
	defer db.Close()

	if err := initDB(db); err != nil {
		log.Fatalf("Failed to init db schema: %v", err)
	}

	srv := &Server{
		db:      db,
		apiKey:  apiKey,
		plnRate: plnRate,
	}

	mux := http.NewServeMux()

	// API Routes
	mux.HandleFunc("POST /api/metrics", srv.handlePostMetrics)
	mux.HandleFunc("GET /api/metrics/latest", srv.handleGetLatest)
	mux.HandleFunc("GET /api/metrics/history", srv.handleGetHistory)
	mux.HandleFunc("GET /api/metrics/summary", srv.handleGetSummary)

	// Web UI
	webContent, err := fs.Sub(webFS, "web")
	if err != nil {
		log.Fatalf("Failed to load embedded web assets: %v", err)
	}
	fileServer := http.FileServer(http.FS(webContent))
	mux.Handle("/", fileServer)

	// Wrap with CORS & logger
	handler := loggingMiddleware(corsMiddleware(mux))

	log.Printf("⚡ Power Meter Server running on :%s", port)
	log.Printf("📁 Database: %s", dbPath)
	if apiKey != "" {
		log.Printf("🔒 API Key protection enabled")
	} else {
		log.Printf("⚠️  No API_KEY configured (open access)")
	}

	if err := http.ListenAndServe(":"+port, handler); err != nil {
		log.Fatalf("Server error: %v", err)
	}
}

func initDB(db *sql.DB) error {
	schema := `
	CREATE TABLE IF NOT EXISTS metrics (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		voltage REAL NOT NULL,
		current REAL NOT NULL,
		power REAL NOT NULL,
		energy REAL NOT NULL,
		frequency REAL NOT NULL,
		pf REAL NOT NULL,
		created_at DATETIME DEFAULT CURRENT_TIMESTAMP
	);
	CREATE INDEX IF NOT EXISTS idx_metrics_created_at ON metrics(created_at);
	`
	_, err := db.Exec(schema)
	return err
}

func (s *Server) handlePostMetrics(w http.ResponseWriter, r *http.Request) {
	if s.apiKey != "" {
		key := r.Header.Get("X-API-Key")
		if key == "" {
			auth := r.Header.Get("Authorization")
			if strings.HasPrefix(auth, "Bearer ") {
				key = strings.TrimPrefix(auth, "Bearer ")
			}
		}
		if key != s.apiKey {
			http.Error(w, `{"error":"unauthorized"}`, http.StatusUnauthorized)
			return
		}
	}

	var m struct {
		Voltage   float64 `json:"voltage"`
		Current   float64 `json:"current"`
		Power     float64 `json:"power"`
		Energy    float64 `json:"energy"`
		Frequency float64 `json:"frequency"`
		PF        float64 `json:"pf"`
	}

	if err := json.NewDecoder(r.Body).Decode(&m); err != nil {
		http.Error(w, `{"error":"invalid json body"}`, http.StatusBadRequest)
		return
	}

	query := `INSERT INTO metrics (voltage, current, power, energy, frequency, pf, created_at) 
	          VALUES (?, ?, ?, ?, ?, ?, datetime('now'))`
	res, err := s.db.Exec(query, m.Voltage, m.Current, m.Power, m.Energy, m.Frequency, m.PF)
	if err != nil {
		log.Printf("Insert error: %v", err)
		http.Error(w, `{"error":"db insert failed"}`, http.StatusInternalServerError)
		return
	}

	id, _ := res.LastInsertId()
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]any{
		"status":  "success",
		"id":      id,
		"message": "metric recorded",
	})
}

func (s *Server) handleGetLatest(w http.ResponseWriter, r *http.Request) {
	row := s.db.QueryRow(`
		SELECT id, voltage, current, power, energy, frequency, pf, strftime('%Y-%m-%dT%H:%M:%SZ', created_at) 
		FROM metrics 
		ORDER BY id DESC LIMIT 1
	`)

	var m Metric
	var createdAtStr string
	err := row.Scan(&m.ID, &m.Voltage, &m.Current, &m.Power, &m.Energy, &m.Frequency, &m.PF, &createdAtStr)
	if err != nil {
		if err == sql.ErrNoRows {
			w.Header().Set("Content-Type", "application/json")
			w.Write([]byte(`{"data":null,"message":"no records found"}`))
			return
		}
		http.Error(w, `{"error":"db query failed"}`, http.StatusInternalServerError)
		return
	}
	m.CreatedAt = parseDBTime(createdAtStr)

	diffSec := int(time.Now().UTC().Sub(m.CreatedAt).Seconds())
	if diffSec < 0 {
		diffSec = 0
	}
	isOnline := diffSec <= 90

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]any{
		"data":              m,
		"is_online":         isOnline,
		"last_seen_seconds": diffSec,
	})
}

func (s *Server) handleGetSummary(w http.ResponseWriter, r *http.Request) {
	var latest Metric
	var createdAtStr string
	err := s.db.QueryRow(`
		SELECT id, voltage, current, power, energy, frequency, pf, strftime('%Y-%m-%dT%H:%M:%SZ', created_at) 
		FROM metrics 
		ORDER BY id DESC LIMIT 1
	`).Scan(&latest.ID, &latest.Voltage, &latest.Current, &latest.Power, &latest.Energy, &latest.Frequency, &latest.PF, &createdAtStr)

	hasData := err == nil
	diffSec := 999999
	isOnline := false
	if hasData {
		latest.CreatedAt = parseDBTime(createdAtStr)
		diffSec = int(time.Now().UTC().Sub(latest.CreatedAt).Seconds())
		if diffSec < 0 {
			diffSec = 0
		}
		isOnline = diffSec <= 90
	}

	var peakPower, avgVolt, avgPF, minEnergy, maxEnergy sql.NullFloat64
	var count int64
	s.db.QueryRow(`
		SELECT 
			COALESCE(MAX(power), 0),
			COALESCE(AVG(voltage), 0),
			COALESCE(AVG(pf), 0),
			COALESCE(MIN(energy), 0),
			COALESCE(MAX(energy), 0),
			COUNT(*)
		FROM metrics 
		WHERE date(created_at, 'localtime') = date('now', 'localtime')
	`).Scan(&peakPower, &avgVolt, &avgPF, &minEnergy, &maxEnergy, &count)

	var totalRecords int64
	s.db.QueryRow(`SELECT COUNT(*) FROM metrics`).Scan(&totalRecords)

	energyToday := 0.0
	if maxEnergy.Valid && minEnergy.Valid && maxEnergy.Float64 >= minEnergy.Float64 {
		energyToday = maxEnergy.Float64 - minEnergy.Float64
	}

	cost := energyToday * s.plnRate

	var pLatest *Metric
	if hasData {
		pLatest = &latest
	}

	summary := Summary{
		Latest:          pLatest,
		IsOnline:        isOnline,
		LastSeenSeconds: diffSec,
		EnergyTodayKWh:  energyToday,
		PeakPowerToday:  peakPower.Float64,
		AvgVoltageToday: avgVolt.Float64,
		AvgPFToday:      avgPF.Float64,
		EstimatedCost:   cost,
		PLNRate:         s.plnRate,
		TotalRecords:    totalRecords,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(summary)
}

func (s *Server) handleGetHistory(w http.ResponseWriter, r *http.Request) {
	rangeParam := r.URL.Query().Get("range")
	if rangeParam == "" {
		rangeParam = "24h"
	}

	var downsampleSQL string

	switch rangeParam {
	case "1h":
		downsampleSQL = `
			SELECT id, voltage, current, power, energy, frequency, pf, strftime('%Y-%m-%dT%H:%M:%SZ', created_at) 
			FROM metrics 
			WHERE created_at >= datetime('now', '-1 hour')
			ORDER BY id ASC LIMIT 1000
		`
	case "6h":
		downsampleSQL = `
			SELECT id, voltage, current, power, energy, frequency, pf, strftime('%Y-%m-%dT%H:%M:%SZ', created_at) 
			FROM metrics 
			WHERE created_at >= datetime('now', '-6 hours')
			ORDER BY id ASC LIMIT 1000
		`
	case "7d":
		downsampleSQL = `
			SELECT 
				MIN(id) as id,
				ROUND(AVG(voltage), 1) as voltage,
				ROUND(AVG(current), 2) as current,
				ROUND(AVG(power), 1) as power,
				ROUND(MAX(energy), 2) as energy,
				ROUND(AVG(frequency), 1) as frequency,
				ROUND(AVG(pf), 2) as pf,
				strftime('%Y-%m-%dT%H:00:00Z', created_at) as created_at
			FROM metrics 
			WHERE created_at >= datetime('now', '-7 days')
			GROUP BY strftime('%Y-%m-%d %H', created_at)
			ORDER BY created_at ASC
		`
	case "30d":
		downsampleSQL = `
			SELECT 
				MIN(id) as id,
				ROUND(AVG(voltage), 1) as voltage,
				ROUND(AVG(current), 2) as current,
				ROUND(AVG(power), 1) as power,
				ROUND(MAX(energy), 2) as energy,
				ROUND(AVG(frequency), 1) as frequency,
				ROUND(AVG(pf), 2) as pf,
				strftime('%Y-%m-%dT00:00:00Z', created_at) as created_at
			FROM metrics 
			WHERE created_at >= datetime('now', '-30 days')
			GROUP BY strftime('%Y-%m-%d', created_at)
			ORDER BY created_at ASC
		`
	case "24h":
		fallthrough
	default:
		downsampleSQL = `
			SELECT 
				MIN(id) as id,
				ROUND(AVG(voltage), 1) as voltage,
				ROUND(AVG(current), 2) as current,
				ROUND(AVG(power), 1) as power,
				ROUND(MAX(energy), 2) as energy,
				ROUND(AVG(frequency), 1) as frequency,
				ROUND(AVG(pf), 2) as pf,
				strftime('%Y-%m-%dT%H:%M:00Z', datetime((strftime('%s', created_at) / 300) * 300, 'unixepoch')) as created_at
			FROM metrics 
			WHERE created_at >= datetime('now', '-24 hours')
			GROUP BY (strftime('%s', created_at) / 300)
			ORDER BY created_at ASC
		`
	}

	rows, err := s.db.Query(downsampleSQL)
	if err != nil {
		log.Printf("History query error: %v", err)
		http.Error(w, `{"error":"db query failed"}`, http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	list := make([]Metric, 0)
	for rows.Next() {
		var m Metric
		var timeStr string
		if err := rows.Scan(&m.ID, &m.Voltage, &m.Current, &m.Power, &m.Energy, &m.Frequency, &m.PF, &timeStr); err == nil {
			m.CreatedAt = parseDBTime(timeStr)
			list = append(list, m)
		}
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]any{
		"range": rangeParam,
		"count": len(list),
		"data":  list,
	})
}

func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, X-API-Key, Authorization")
		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}
		next.ServeHTTP(w, r)
	})
}

func loggingMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		if strings.HasPrefix(r.URL.Path, "/api/") {
			log.Printf("%s %s in %v", r.Method, r.URL.Path, time.Since(start))
		}
	})
}

func getEnv(key, defaultVal string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultVal
}
