package main

import (
	"crypto/rand"
	"database/sql"
	"embed"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
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

type MonthSummary struct {
	YearMonth     string  `json:"year_month"`     // "2026-09"
	MonthName     string  `json:"month_name"`     // "September 2026"
	TotalKWh      float64 `json:"total_kwh"`      // 45.2
	EstimatedCost float64 `json:"estimated_cost"` // 65300
	PeakPower     float64 `json:"peak_power"`     // 850.5
	AvgPower      float64 `json:"avg_power"`      // 320.1
	TotalRecords  int64   `json:"total_records"`
	IsCurrent     bool    `json:"is_current"`
}

var idMonths = map[string]string{
	"01": "Januari", "02": "Februari", "03": "Maret", "04": "April",
	"05": "Mei", "06": "Juni", "07": "Juli", "08": "Agustus",
	"09": "September", "10": "Oktober", "11": "November", "12": "Desember",
}

func formatMonthID(ym string) string {
	parts := strings.Split(ym, "-")
	if len(parts) == 2 {
		if name, ok := idMonths[parts[1]]; ok {
			return name + " " + parts[0]
		}
	}
	return ym
}

type AuthConfig struct {
	Username string `json:"username"`
	Password string `json:"password"`
	APIKey   string `json:"api_key"`
}

type Server struct {
	db          *sql.DB
	authFile    string
	plnRate     float64
	authMu      sync.RWMutex
	cachedAuth  *AuthConfig
	authModTime time.Time
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
	rateStr := getEnv("PLN_RATE", "1444.70")

	plnRate, err := strconv.ParseFloat(rateStr, 64)
	if err != nil {
		plnRate = 1444.70
	}

	dataDir := filepath.Dir(dbPath)
	if err := os.MkdirAll(dataDir, 0755); err != nil {
		log.Fatalf("Failed to create data dir: %v", err)
	}

	authFile := filepath.Join(dataDir, "auth.json")
	ensureAuthFile(authFile)

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
		db:       db,
		authFile: authFile,
		plnRate:  plnRate,
	}

	mux := http.NewServeMux()

	// Auth Endpoints (Public)
	mux.HandleFunc("POST /api/auth/login", srv.handleLogin)
	mux.HandleFunc("POST /api/auth/logout", srv.handleLogout)
	mux.HandleFunc("GET /api/auth/check", srv.handleAuthCheck)

	// API Routes (Protected)
	mux.HandleFunc("POST /api/metrics", srv.handlePostMetrics)
	mux.HandleFunc("GET /api/metrics/latest", srv.requireAuth(srv.handleGetLatest))
	mux.HandleFunc("GET /api/metrics/history", srv.requireAuth(srv.handleGetHistory))
	mux.HandleFunc("GET /api/metrics/summary", srv.requireAuth(srv.handleGetSummary))
	mux.HandleFunc("GET /api/metrics/monthly", srv.requireAuth(srv.handleGetMonthly))
	mux.HandleFunc("GET /api/metrics/recent", srv.requireAuth(srv.handleGetRecent))
	mux.HandleFunc("DELETE /api/metrics", srv.requireAuth(srv.handleDeleteMetric))
	mux.HandleFunc("POST /api/metrics/delete", srv.requireAuth(srv.handleDeleteMetric))

	// Web UI
	webContent, err := fs.Sub(webFS, "web")
	if err != nil {
		log.Fatalf("Failed to load embedded web assets: %v", err)
	}
	fileServer := http.FileServer(http.FS(webContent))
	mux.Handle("/", fileServer)

	handler := loggingMiddleware(corsMiddleware(mux))

	log.Printf("⚡ Power Meter Server running on :%s", port)
	log.Printf("📁 Database: %s", dbPath)
	log.Printf("🔐 Auth Credentials file: %s", authFile)

	if err := http.ListenAndServe(":"+port, handler); err != nil {
		log.Fatalf("Server error: %v", err)
	}
}

func ensureAuthFile(path string) {
	if _, err := os.Stat(path); os.IsNotExist(err) {
		defaultAuth := AuthConfig{
			Username: "admin",
			Password: "admin123",
			APIKey:   "esp32_secret_token_123",
		}
		data, err := json.MarshalIndent(defaultAuth, "", "  ")
		if err == nil {
			_ = os.WriteFile(path, data, 0644)
			log.Printf("🔑 Created default credentials file at %s (username: admin, password: admin123)", path)
		}
	}
}

func (s *Server) getAuth() AuthConfig {
	s.authMu.Lock()
	defer s.authMu.Unlock()

	stat, err := os.Stat(s.authFile)
	if err == nil && (s.cachedAuth == nil || stat.ModTime().After(s.authModTime)) {
		data, err := os.ReadFile(s.authFile)
		if err == nil {
			var conf AuthConfig
			if err := json.Unmarshal(data, &conf); err == nil {
				s.cachedAuth = &conf
				s.authModTime = stat.ModTime()
			}
		}
	}

	if s.cachedAuth != nil {
		return *s.cachedAuth
	}
	return AuthConfig{Username: "admin", Password: "admin123", APIKey: "esp32_secret_token_123"}
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

	CREATE TABLE IF NOT EXISTS sessions (
		token TEXT PRIMARY KEY,
		username TEXT NOT NULL,
		created_at DATETIME DEFAULT CURRENT_TIMESTAMP
	);
	`
	_, err := db.Exec(schema)
	return err
}

func generateToken() string {
	b := make([]byte, 24)
	rand.Read(b)
	return hex.EncodeToString(b)
}

func (s *Server) handleLogin(w http.ResponseWriter, r *http.Request) {
	var req struct {
		Username string `json:"username"`
		Password string `json:"password"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, `{"error":"invalid json"}`, http.StatusBadRequest)
		return
	}

	auth := s.getAuth()
	if req.Username != auth.Username || req.Password != auth.Password {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusUnauthorized)
		w.Write([]byte(`{"error":"Username atau password salah"}`))
		return
	}

	token := generateToken()
	_, err := s.db.Exec(`INSERT INTO sessions (token, username, created_at) VALUES (?, ?, datetime('now'))`, token, req.Username)
	if err != nil {
		http.Error(w, `{"error":"session creation failed"}`, http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]any{
		"status":   "success",
		"token":    token,
		"username": req.Username,
	})
}

func (s *Server) handleLogout(w http.ResponseWriter, r *http.Request) {
	token := extractToken(r)
	if token != "" {
		s.db.Exec(`DELETE FROM sessions WHERE token = ?`, token)
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write([]byte(`{"status":"logged_out"}`))
}

func (s *Server) handleAuthCheck(w http.ResponseWriter, r *http.Request) {
	token := extractToken(r)
	w.Header().Set("Content-Type", "application/json")
	if token == "" {
		w.Write([]byte(`{"authenticated":false}`))
		return
	}

	var username string
	err := s.db.QueryRow(`SELECT username FROM sessions WHERE token = ?`, token).Scan(&username)
	if err != nil {
		w.Write([]byte(`{"authenticated":false}`))
		return
	}

	json.NewEncoder(w).Encode(map[string]any{
		"authenticated": true,
		"username":      username,
	})
}

func extractToken(r *http.Request) string {
	if key := r.Header.Get("X-API-Key"); key != "" {
		return key
	}
	if tok := r.Header.Get("X-Auth-Token"); tok != "" {
		return tok
	}
	auth := r.Header.Get("Authorization")
	if strings.HasPrefix(auth, "Bearer ") {
		return strings.TrimPrefix(auth, "Bearer ")
	}
	return ""
}

// requireAuth protects Web UI data endpoints (checks session token or ESP32 API Key)
func (s *Server) requireAuth(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		token := extractToken(r)
		if token == "" {
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusUnauthorized)
			w.Write([]byte(`{"error":"unauthorized","authenticated":false}`))
			return
		}

		// Check if matches API Key from auth.json
		auth := s.getAuth()
		if auth.APIKey != "" && token == auth.APIKey {
			next(w, r)
			return
		}

		// Check if valid session token in database
		var username string
		err := s.db.QueryRow(`SELECT username FROM sessions WHERE token = ?`, token).Scan(&username)
		if err == nil {
			next(w, r)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusUnauthorized)
		w.Write([]byte(`{"error":"unauthorized","authenticated":false}`))
	}
}

// handlePostMetrics: Authenticates ESP32 via X-API-Key header (or Bearer token) matching auth.json's api_key
func (s *Server) handlePostMetrics(w http.ResponseWriter, r *http.Request) {
	auth := s.getAuth()
	token := extractToken(r)

	if auth.APIKey != "" && token != auth.APIKey {
		log.Printf("⚠️ Unauthorized push attempt from %s with token '%s'", r.RemoteAddr, token)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusUnauthorized)
		w.Write([]byte(`{"error":"unauthorized: invalid X-API-Key"}`))
		return
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

func (s *Server) handleGetMonthly(w http.ResponseWriter, r *http.Request) {
	currentYM := time.Now().Format("2006-01")

	rows, err := s.db.Query(`
		SELECT 
			strftime('%Y-%m', created_at, 'localtime') as ym,
			COUNT(*),
			COALESCE(MAX(power), 0),
			COALESCE(AVG(power), 0),
			COALESCE(MIN(energy), 0),
			COALESCE(MAX(energy), 0)
		FROM metrics 
		GROUP BY ym 
		ORDER BY ym DESC 
		LIMIT 12
	`)
	if err != nil {
		log.Printf("Monthly query error: %v", err)
		http.Error(w, `{"error":"db query failed"}`, http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	list := make([]MonthSummary, 0)
	for rows.Next() {
		var ym string
		var totalRecords int64
		var peakPower, avgPower, minEnergy, maxEnergy float64

		if err := rows.Scan(&ym, &totalRecords, &peakPower, &avgPower, &minEnergy, &maxEnergy); err == nil {
			kwh := 0.0
			if maxEnergy >= minEnergy {
				kwh = maxEnergy - minEnergy
			}
			cost := kwh * s.plnRate

			list = append(list, MonthSummary{
				YearMonth:     ym,
				MonthName:     formatMonthID(ym),
				TotalKWh:      kwh,
				EstimatedCost: cost,
				PeakPower:     peakPower,
				AvgPower:      avgPower,
				TotalRecords:  totalRecords,
				IsCurrent:     ym == currentYM,
			})
		}
	}

	if len(list) == 0 {
		list = append(list, MonthSummary{
			YearMonth:     currentYM,
			MonthName:     formatMonthID(currentYM),
			TotalKWh:      0,
			EstimatedCost: 0,
			PeakPower:     0,
			AvgPower:      0,
			TotalRecords:  0,
			IsCurrent:     true,
		})
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(list)
}

func (s *Server) handleGetRecent(w http.ResponseWriter, r *http.Request) {
	limitStr := r.URL.Query().Get("limit")
	limit := 20
	if l, err := strconv.Atoi(limitStr); err == nil && l > 0 && l <= 100 {
		limit = l
	}

	rows, err := s.db.Query(`
		SELECT id, voltage, current, power, energy, frequency, pf, strftime('%Y-%m-%dT%H:%M:%SZ', created_at) 
		FROM metrics 
		ORDER BY id DESC LIMIT ?
	`, limit)
	if err != nil {
		log.Printf("Recent query error: %v", err)
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
		"count": len(list),
		"data":  list,
	})
}

func (s *Server) handleDeleteMetric(w http.ResponseWriter, r *http.Request) {
	idStr := r.URL.Query().Get("id")
	if idStr == "" {
		var body struct {
			ID int64 `json:"id"`
		}
		if err := json.NewDecoder(r.Body).Decode(&body); err == nil && body.ID > 0 {
			idStr = strconv.FormatInt(body.ID, 10)
		}
	}

	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil || id <= 0 {
		http.Error(w, `{"error":"id parameter required"}`, http.StatusBadRequest)
		return
	}

	res, err := s.db.Exec(`DELETE FROM metrics WHERE id = ?`, id)
	if err != nil {
		log.Printf("Delete metric error: %v", err)
		http.Error(w, `{"error":"failed to delete"}`, http.StatusInternalServerError)
		return
	}

	rows, _ := res.RowsAffected()
	if rows == 0 {
		http.Error(w, `{"error":"record not found"}`, http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]any{
		"status":  "success",
		"message": "record deleted",
		"id":      id,
	})
}

func (s *Server) handleGetHistory(w http.ResponseWriter, r *http.Request) {
	monthParam := r.URL.Query().Get("month")
	if monthParam != "" {
		downsampleSQL := `
			SELECT 
				MIN(id) as id,
				ROUND(AVG(voltage), 1) as voltage,
				ROUND(AVG(current), 2) as current,
				ROUND(AVG(power), 1) as power,
				ROUND(MAX(energy), 2) as energy,
				ROUND(AVG(frequency), 1) as frequency,
				ROUND(AVG(pf), 2) as pf,
				strftime('%Y-%m-%dT%H:00:00Z', created_at, 'localtime') as created_at
			FROM metrics 
			WHERE strftime('%Y-%m', created_at, 'localtime') = ?
			GROUP BY strftime('%Y-%m-%d %H', created_at, 'localtime')
			ORDER BY created_at ASC
		`
		rows, err := s.db.Query(downsampleSQL, monthParam)
		if err != nil {
			log.Printf("Monthly history error: %v", err)
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
			"month": monthParam,
			"count": len(list),
			"data":  list,
		})
		return
	}

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
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, X-API-Key, X-Auth-Token, Authorization")
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
