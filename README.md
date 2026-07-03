# BYTEURL — High-Performance URL Shortener

BYTEURL is a production-grade URL shortening service written in **C++20** using the [Drogon](https://github.com/drogonframework/drogon) async web framework. It uses **PostgreSQL** as the durable data store and **Redis** as a write-through cache to minimise redirect latency.

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    HTTP Client                      │
└──────────┬──────────────────────┬───────────────────┘
           │ POST /shorten        │ GET /r/{hash}
           │ GET  /stats/{hash}   │ GET /health
           ▼                      ▼
┌─────────────────────────────────────────────────────┐
│              Drogon HTTP Server (C++20)             │
│  ┌───────────────┐  ┌─────────────────────────────┐ │
│  │LinkController │  │      HealthController       │ │
│  └──────┬────────┘  └─────────────────────────────┘ │
│         │                                            │
│  ┌──────▼────────────────────┐                       │
│  │   LinkService             │  (business logic)     │
│  │   RedirectService         │                       │
│  └──────┬────────────────────┘                       │
│         │                                            │
│  ┌──────▼────────────────────────────────────────┐   │
│  │           ILinkRepository (interface)         │   │
│  │  ┌──────────────────┐  ┌────────────────────┐ │   │
│  │  │ postgres::Link   │  │  redis::Link       │ │   │
│  │  │ Repository       │  │  Repository        │ │   │
│  │  │ (source of truth)│  │  (fast cache)      │ │   │
│  │  └────────┬─────────┘  └────────┬───────────┘ │   │
│  └───────────┼─────────────────────┼─────────────┘   │
└──────────────┼─────────────────────┼─────────────────┘
               ▼                     ▼
          PostgreSQL               Redis
          (persistent)          (in-memory)
```

### Why two databases?

| Concern | PostgreSQL | Redis |
|---|---|---|
| Durability | Survives restarts | Lost on restart (by default) |
| Latency | ~5–10 ms | ~0.1–1 ms |
| Role | Source of truth | Read-through cache |

On every redirect (`GET /r/{hash}`), Redis is checked first. If the key is not cached, Postgres is queried and the result is written back to Redis so subsequent hits are served from cache.

---

## Features

| Feature | Endpoint |
|---|---|
| Shorten a URL | `POST /shorten` |
| Redirect to original URL | `GET /r/{hash}` |
| View link analytics (hits, created date) | `GET /stats/{hash}` |
| Service liveness probe | `GET /health` |
| URL validation (blocks `javascript:`, `file:`, etc.) | (automatic) |
| Hit counter — increments on every redirect | (automatic) |

---

## API Reference

### `POST /shorten`
Shorten a URL. Returns the same short code for duplicate URLs (idempotent).

**Request**
```json
{ "url": "https://www.example.com/some/very/long/path" }
```

**Response `200 OK`**
```json
{ "shortCode": "r/AbC123" }
```

**Response `400 Bad Request`** (invalid URL)
```json
{ "error": "URL must start with http:// or https://. Schemes like javascript:, file:, and data: are not allowed." }
```

---

### `GET /r/{hash}`
Redirects to the original URL and increments the hit counter.

**Response `302 Found`**
```
Location: https://www.example.com/some/very/long/path
```

**Response `404 Not Found`** — short code does not exist.

---

### `GET /stats/{hash}`
Returns analytics for a short link.

**Response `200 OK`**
```json
{
  "shortCode":   "AbC123",
  "originalUrl": "https://www.example.com/some/very/long/path",
  "hits":        42,
  "createdAt":   "2025-01-15T10:30:00+00:00"
}
```

**Response `404 Not Found`** — short code does not exist.

---

### `GET /health`
Liveness probe for Docker `HEALTHCHECK` / Kubernetes probes.

**Response `200 OK`**
```json
{ "status": "ok", "version": "1.0.0" }
```

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++20 (coroutines) |
| Web Framework | [Drogon](https://github.com/drogonframework/drogon) |
| Database | PostgreSQL 16 |
| Cache | Redis (Alpine) |
| Build | CMake ≥ 3.20 |
| Deploy | Docker + Docker Compose |

All database and cache operations are **fully async** via C++20 coroutines (`co_await`). No thread is ever blocked waiting for I/O.

---

## Running Locally with Docker

### 1. Clone with submodules

```bash
git clone --recurse-submodules <your-repo-url>
cd byteURL
```

### 2. Create a `.env` file

```env
POSTGRES_USER=postgres
POSTGRES_PASSWORD=postgres
POSTGRES_DB=byteURL
POSTGRES_HOST=byteURL-postgres
DB_PORT=5432

REDIS_HOST=byteURL-redis
REDIS_PORT=6379

LISTEN_ADDR=0.0.0.0
LISTEN_PORT=8080
```

### 3. Build and start

```bash
docker-compose up --build
```

The service will be available at **http://localhost:8080**.

--- 

## Database Schema

```sql
-- Migrations are applied automatically at container startup
-- via docker-entrypoint-initdb.d/

CREATE TABLE links (
    short_code   TEXT PRIMARY KEY,           -- e.g. "AbC123"
    original_url TEXT NOT NULL,              -- the full URL
    hits         BIGINT DEFAULT 0,           -- redirect counter
    created_at   TIMESTAMPTZ DEFAULT now()   -- creation timestamp
);

-- Index for reverse lookup (original_url → short_code)
CREATE INDEX idx_links_original_url ON links (original_url);
```

---

## Project Structure

```
byteURL/
├── src/
│   ├── controllers/
│   │   ├── link_controller.{h,cpp}     # POST /shorten, GET /r/{hash}, GET /stats/{hash}
│   │   └── health_controller.{h,cpp}   # GET /health
│   ├── services/
│   │   ├── link_service.{h,cpp}        # URL creation + stats business logic
│   │   └── redirect_service.{h,cpp}    # Redirect + hit-counting logic
│   ├── repositories/
│   │   ├── postgres/link.{h,cpp}       # PostgreSQL implementation
│   │   └── redis/link.{h,cpp}          # Redis cache implementation
│   ├── interfaces/
│   │   └── i_link_repository.h         # Abstract repository + LinkStats struct
│   └── utils/
│       ├── shortener.{h,cpp}           # Random 6-char code generator
│       └── url_validator.{h,cpp}       # URL safety validation
├── migrations/                         # Versioned SQL migrations (up + down)
├── public/                             # Static frontend (served by Drogon)
├── cmake/                              # CMake dependency scripts
├── third_party/drogon/                 # Drogon submodule
├── docker-compose.yml
├── Dockerfile
└── config.template.json
```
