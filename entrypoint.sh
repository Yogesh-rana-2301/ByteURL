#!/bin/sh
set -e

# ── PostgreSQL ────────────────────────────────────────────────────────────────
# Parse DATABASE_URL first (Railway's authoritative Postgres connection string)
# Format: postgresql://user:password@host:port/dbname
if [ -n "$DATABASE_URL" ]; then
    _rest="${DATABASE_URL#postgresql://}"
    _rest="${_rest#postgres://}"
    _userinfo="${_rest%%@*}"
    _hostinfo="${_rest#*@}"
    _raw_user="${_userinfo%%:*}"
    _raw_pass="${_userinfo#*:}"
    _hostport="${_hostinfo%%/*}"
    _raw_host="${_hostport%%:*}"
    _raw_port="${_hostport##*:}"
    _raw_db="${_hostinfo#*/}"
    _raw_db="${_raw_db%%\?*}"

    export POSTGRES_HOST="${_raw_host}"
    export POSTGRES_USER="${_raw_user}"
    export POSTGRES_PASSWORD="${_raw_pass}"
    export POSTGRES_DB="${_raw_db}"
    export DB_PORT="${_raw_port:-5432}"
else
    # Fallback to individual PG* vars
    export POSTGRES_HOST="${PGHOST:-localhost}"
    export POSTGRES_USER="${PGUSER:-postgres}"
    export POSTGRES_PASSWORD="${PGPASSWORD:-}"
    export POSTGRES_DB="${PGDATABASE:-byteurl}"
    export DB_PORT="${PGPORT:-5432}"
fi

export POSTGRES_SSLMODE="${POSTGRES_SSLMODE:-disable}"

# ── Redis ─────────────────────────────────────────────────────────────────────
# Priority order (highest to lowest):
#   1. APP_REDIS_HOST set directly in Railway Variables  (most reliable)
#   2. REDIS_PRIVATE_URL  — Railway's internal network URL (correct hostname)
#   3. REDIS_URL          — may contain 0.0.0.0 (Railway bind addr, wrong)
#   4. REDISHOST          — Railway's individual host variable
#
# NOTE: Never use REDIS_HOST — Railway auto-injects it as 0.0.0.0 (bind addr).
#
# Recommended Railway Variable:  REDIS_PRIVATE_URL = ${{Redis.REDIS_PRIVATE_URL}}

_parse_redis_url() {
    _url="$1"
    _rurl="${_url#redis://}"
    _rurl="${_rurl#rediss://}"
    
    # Check if there is a password part (user:password@)
    case "$_rurl" in
        *@*)
            _user_pass="${_rurl%%@*}"
            _pass="${_user_pass#*:}"
            _rhost_port="${_rurl##*@}"
            ;;
        *)
            _pass=""
            _rhost_port="$_rurl"
            ;;
    esac

    _rhost_port="${_rhost_port%%/*}"    # strip trailing /path
    echo "${_rhost_port%%:*} ${_rhost_port##*:} $_pass"
}

if [ -n "$APP_REDIS_HOST" ]; then
    # Already set directly — use as-is
    APP_REDIS_PORT="${APP_REDIS_PORT:-6379}"
    APP_REDIS_PASS="${APP_REDIS_PASS:-$REDISPASSWORD}"
elif [ -n "$REDIS_PRIVATE_URL" ]; then
    read -r _h _p _pass <<EOF
$(_parse_redis_url "$REDIS_PRIVATE_URL")
EOF
    APP_REDIS_HOST="$_h"
    APP_REDIS_PORT="${_p:-6379}"
    APP_REDIS_PASS="$_pass"
elif [ -n "$REDIS_URL" ]; then
    read -r _h _p _pass <<EOF
$(_parse_redis_url "$REDIS_URL")
EOF
    APP_REDIS_HOST="$_h"
    APP_REDIS_PORT="${_p:-6379}"
    APP_REDIS_PASS="$_pass"
elif [ -n "$REDISHOST" ]; then
    APP_REDIS_HOST="$REDISHOST"
    APP_REDIS_PORT="${REDISPORT:-6379}"
    APP_REDIS_PASS="$REDISPASSWORD"
else
    APP_REDIS_HOST="localhost"
    APP_REDIS_PORT="6379"
    APP_REDIS_PASS=""
fi

# ── trantor::InetAddress DNS Fix ──────────────────────────────────────────────
# drogon/trantor does not resolve hostnames in InetAddress (it expects an IP).
# If given a hostname like redis.railway.internal, it fails and uses 0.0.0.0.
# We must resolve the hostname to an IP address before passing it to C++.
if ! echo "$APP_REDIS_HOST" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$'; then
    _resolved_ip=$(getent hosts "$APP_REDIS_HOST" | awk '{print $1}' | head -n 1)
    if [ -n "$_resolved_ip" ]; then
        echo "[entrypoint] resolved Redis host $APP_REDIS_HOST -> $_resolved_ip"
        APP_REDIS_HOST="$_resolved_ip"
    else
        echo "[entrypoint] WARNING: failed to resolve Redis host $APP_REDIS_HOST"
    fi
fi

export APP_REDIS_HOST APP_REDIS_PORT APP_REDIS_PASS


# ── HTTP listener ─────────────────────────────────────────────────────────────
export LISTEN_PORT="${LISTEN_PORT:-${PORT:-8080}}"
export LISTEN_ADDR="${LISTEN_ADDR:-0.0.0.0}"

# ── Validate ──────────────────────────────────────────────────────────────────
echo "[entrypoint] listening  : ${LISTEN_ADDR}:${LISTEN_PORT}"
echo "[entrypoint] postgres   : ${POSTGRES_USER}@${POSTGRES_HOST}:${DB_PORT}/${POSTGRES_DB} (ssl=${POSTGRES_SSLMODE})"
echo "[entrypoint] redis      : ${APP_REDIS_HOST}:${APP_REDIS_PORT}"

if [ "${APP_REDIS_HOST}" = "localhost" ] || [ "${APP_REDIS_HOST}" = "0.0.0.0" ]; then
    echo "[entrypoint] WARNING: Redis host looks wrong (${APP_REDIS_HOST}) — is REDIS_URL set in Railway?"
fi

# ── Generate config.json ──────────────────────────────────────────────────────
envsubst < /app/config.template.json > /app/config.json

exec /app/byteURL
