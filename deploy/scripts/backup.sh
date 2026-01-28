#!/bin/bash
# Solar System Monitoring - Backup Script
set -e

BACKUP_DIR="${BACKUP_DIR:-/var/backups/solar_system}"
RETENTION_DAYS="${RETENTION_DAYS:-7}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$BACKUP_DIR"

# Backup cache data
if [ -d "/var/cache/solar_system" ]; then
    tar -czf "$BACKUP_DIR/cache_$TIMESTAMP.tar.gz" -C /var/cache solar_system
    echo "Cache backup: $BACKUP_DIR/cache_$TIMESTAMP.tar.gz"
fi

# Backup metrics data
if [ -d "/var/lib/solar_system/metrics" ]; then
    tar -czf "$BACKUP_DIR/metrics_$TIMESTAMP.tar.gz" -C /var/lib/solar_system metrics
    echo "Metrics backup: $BACKUP_DIR/metrics_$TIMESTAMP.tar.gz"
fi

# Cleanup old backups
find "$BACKUP_DIR" -name "*.tar.gz" -mtime +$RETENTION_DAYS -delete
echo "Cleaned backups older than $RETENTION_DAYS days"
