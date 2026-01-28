#!/bin/bash
# Solar System Monitoring - Restore Script
set -e

BACKUP_FILE="$1"
if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup_file.tar.gz>"
    exit 1
fi

if [ ! -f "$BACKUP_FILE" ]; then
    echo "Error: Backup file not found: $BACKUP_FILE"
    exit 1
fi

# Determine backup type from filename
if [[ "$BACKUP_FILE" == *"cache_"* ]]; then
    tar -xzf "$BACKUP_FILE" -C /var/cache
    echo "Cache restored from $BACKUP_FILE"
elif [[ "$BACKUP_FILE" == *"metrics_"* ]]; then
    tar -xzf "$BACKUP_FILE" -C /var/lib/solar_system
    echo "Metrics restored from $BACKUP_FILE"
else
    echo "Unknown backup type"
    exit 1
fi
