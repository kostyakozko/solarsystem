# Solar System Suite - Deployment Guide

## Docker Deployment

### Quick Start
```bash
cd deploy/docker
docker-compose up -d
```

### Build Custom Image
```bash
docker build -f deploy/docker/Dockerfile.monitoring -t solar-system/monitoring:4.0.0 .
```

## Kubernetes Deployment

### Using Helm
```bash
helm install solar-monitoring deploy/helm/solar-monitoring
```

### Custom Values
```bash
helm install solar-monitoring deploy/helm/solar-monitoring \
  --set replicaCount=3 \
  --set ingress.enabled=true
```

## Backup & Recovery

### Manual Backup
```bash
./deploy/scripts/backup.sh
```

### Restore
```bash
./deploy/scripts/restore.sh /var/backups/solar_system/cache_20260128.tar.gz
```

### Automated Backups (Kubernetes)
Enable in values.yaml:
```yaml
backup:
  enabled: true
  schedule: "0 2 * * *"
  retention: 7
```

## Upgrades

### Upgrade to New Version
```bash
./deploy/scripts/upgrade.sh 4.1.0
```

### Rollback
```bash
# Docker
docker-compose down
docker-compose up -d  # Uses previous image

# Helm
helm rollback solar-monitoring
```
