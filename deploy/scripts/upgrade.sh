#!/bin/bash
# Solar System Monitoring - Upgrade Script
set -e

NEW_VERSION="${1:-latest}"
BACKUP_BEFORE_UPGRADE="${BACKUP_BEFORE_UPGRADE:-true}"

echo "Upgrading Solar System Monitoring to version: $NEW_VERSION"

# Backup before upgrade
if [ "$BACKUP_BEFORE_UPGRADE" = "true" ]; then
    echo "Creating pre-upgrade backup..."
    "$(dirname "$0")/backup.sh"
fi

# For Docker deployments
if command -v docker &> /dev/null; then
    docker pull "solar-system/monitoring:$NEW_VERSION"
    docker-compose -f "$(dirname "$0")/../docker/docker-compose.yml" up -d
    echo "Docker deployment upgraded"
fi

# For Kubernetes/Helm deployments
if command -v helm &> /dev/null; then
    helm upgrade solar-monitoring "$(dirname "$0")/../helm/solar-monitoring" \
        --set image.tag="$NEW_VERSION" --reuse-values
    echo "Helm deployment upgraded"
fi

echo "Upgrade complete"
