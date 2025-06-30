# Solar System Suite Examples

This document provides practical examples for using the Solar System Suite in various scenarios.

## Basic Usage Examples

### Example 1: Quick System Check

```bash
# Check if everything is working
cd /path/to/solar_system/install
./solar_system_launcher --status

# Expected output:
# === Solar System Suite Status ===
# ✓ Data Status: READY
# ✓ Available Applications: 5 applications found
```

### Example 2: First Simulation

```bash
# Run your first simulation
./solar_system_launcher --simulate --date 2025-07-01

# With automatic data fetching if needed
./solar_system_launcher --simulate --auto-fetch --date 2025-12-31
```

### Example 3: Real-time Solar System

```bash
# Watch the solar system in real-time
./solar_system_realtime

# With velocities and faster updates
./solar_system_realtime --velocities --display-interval 5
```

## Web Interface Examples

### Example 4: Interactive Time Travel

```bash
# Start the web server
./solar_system_web --web-root share/solar_system/web

# Open browser to: http://localhost:8080
# Try these interactions:
# 1. Click "Time Travel" mode
# 2. Set date to "1990-01-01"
# 3. Set speed to "1 year/sec"
# 4. Enable "Orbit Trails"
# 5. Click "Start Time Travel"
# 6. Watch 35 years pass in 35 seconds!
```

### Example 5: Custom Web Server Setup

```bash
# Production setup with custom port
./solar_system_web --port 3000 --web-root share/solar_system/web

# Development with verbose logging
./solar_system_web --port 8080 --verbose --web-root share/solar_system/web
```

## Data Management Examples

### Example 6: Data Update Workflow

```bash
# Check current data status
./solar_system_fetch

# Update data for current year
./solar_system_fetch --update

# Force complete refresh (if needed)
./solar_system_fetch --force

# Validate data integrity
./solar_system_fetch --validate
```

### Example 7: Cache Management

```bash
# Test storage system
./solar_system_fetch --test-storage

# Clean and rebuild cache
./solar_system_fetch --clean
./solar_system_fetch --update

# Check cache performance
time ./solar_system_fetch --update  # Should be ~0.008s after first run
```

## Advanced Usage Examples

### Example 8: Batch Processing Script

```bash
#!/bin/bash
# Process multiple dates for analysis

dates=("2025-01-01" "2025-07-01" "2026-01-01" "2026-07-01")

echo "Processing solar system data for multiple dates..."

for date in "${dates[@]}"; do
    echo "Processing $date..."
    ./solar_system --date $date > "results_$date.txt"
    
    if [ $? -eq 0 ]; then
        echo "✓ Successfully processed $date"
    else
        echo "✗ Failed to process $date"
    fi
done

echo "Batch processing complete!"
```

### Example 9: Automated Data Pipeline

```bash
#!/bin/bash
# Automated data update and simulation pipeline

echo "🔄 Starting automated solar system pipeline..."

# Step 1: Update data
echo "📡 Updating JPL data..."
./solar_system_launcher --fetch --update

if [ $? -eq 0 ]; then
    echo "✅ Data update successful"
    
    # Step 2: Run simulation for today
    today=$(date +%Y-%m-%d)
    echo "🌌 Running simulation for $today..."
    ./solar_system_launcher --simulate --date $today
    
    if [ $? -eq 0 ]; then
        echo "✅ Simulation successful"
        
        # Step 3: Start web server for visualization
        echo "🌐 Starting web server..."
        ./solar_system_web --web-root share/solar_system/web &
        web_pid=$!
        
        echo "🎉 Pipeline complete!"
        echo "📊 Web interface: http://localhost:8080"
        echo "🛑 Stop web server: kill $web_pid"
    else
        echo "❌ Simulation failed"
        exit 1
    fi
else
    echo "❌ Data update failed, using cached data"
    ./solar_system --date $(date +%Y-%m-%d)
fi
```

### Example 10: Performance Monitoring

```bash
#!/bin/bash
# Monitor solar system suite performance

echo "📊 Solar System Suite Performance Monitor"
echo "========================================"

# Test cache performance
echo "🔍 Testing cache performance..."
echo -n "First run (with JPL fetch): "
time ./solar_system_fetch --update 2>/dev/null

echo -n "Second run (cached): "
time ./solar_system_fetch --update 2>/dev/null

# Test simulation performance
echo -n "Simulation performance: "
time ./solar_system --date 2025-07-01 >/dev/null 2>&1

# Test web server response
echo "🌐 Testing web server response..."
./solar_system_web --port 8081 &
web_pid=$!
sleep 2

echo -n "API response time: "
curl -w "@curl-format.txt" -o /dev/null -s http://localhost:8081/api/status 2>/dev/null || echo "Web server not ready"

kill $web_pid 2>/dev/null
```

## Integration Examples

### Example 11: Python Integration

```python
#!/usr/bin/env python3
"""
Solar System Suite Python Integration Example
"""

import subprocess
import json
import requests
from datetime import datetime, timedelta

class SolarSystemSuite:
    def __init__(self, install_path):
        self.install_path = install_path
        self.launcher = f"{install_path}/solar_system_launcher"
        self.web_server = None
    
    def check_status(self):
        """Check system status"""
        result = subprocess.run([self.launcher, "--status"], 
                              capture_output=True, text=True)
        return result.returncode == 0
    
    def update_data(self):
        """Update JPL data"""
        result = subprocess.run([self.launcher, "--fetch", "--update"], 
                              capture_output=True, text=True)
        return result.returncode == 0
    
    def simulate(self, date_str):
        """Run simulation for specific date"""
        result = subprocess.run([self.launcher, "--simulate", "--date", date_str], 
                              capture_output=True, text=True)
        return result.stdout if result.returncode == 0 else None
    
    def start_web_server(self, port=8080):
        """Start web server"""
        cmd = [f"{self.install_path}/solar_system_web", 
               "--port", str(port), 
               "--web-root", f"{self.install_path}/share/solar_system/web"]
        self.web_server = subprocess.Popen(cmd)
        return f"http://localhost:{port}"
    
    def get_solar_system_data(self, port=8080, date=None):
        """Get solar system data via API"""
        url = f"http://localhost:{port}/api/solar_system"
        if date:
            url += f"?date={date}"
        
        try:
            response = requests.get(url)
            return response.json() if response.status_code == 200 else None
        except:
            return None
    
    def stop_web_server(self):
        """Stop web server"""
        if self.web_server:
            self.web_server.terminate()
            self.web_server = None

# Usage example
if __name__ == "__main__":
    # Initialize
    solar = SolarSystemSuite("/path/to/solar_system/install")
    
    # Check status
    if solar.check_status():
        print("✅ Solar System Suite is ready")
        
        # Update data
        if solar.update_data():
            print("✅ Data updated successfully")
            
            # Run simulation
            today = datetime.now().strftime("%Y-%m-%d")
            result = solar.simulate(today)
            if result:
                print(f"✅ Simulation completed for {today}")
                
                # Start web server
                url = solar.start_web_server(8082)
                print(f"🌐 Web server started: {url}")
                
                # Get data via API
                import time
                time.sleep(2)  # Wait for server to start
                data = solar.get_solar_system_data(8082)
                if data:
                    print(f"📊 Retrieved data for {len(data.get('bodies', []))} bodies")
                
                # Cleanup
                solar.stop_web_server()
                print("🛑 Web server stopped")
            else:
                print("❌ Simulation failed")
        else:
            print("❌ Data update failed")
    else:
        print("❌ Solar System Suite not ready")
```

### Example 12: Cron Job Setup

```bash
# Add to crontab (crontab -e)

# Update JPL data daily at 2 AM
0 2 * * * /path/to/solar_system/install/solar_system_launcher --fetch --update >> /var/log/solar_system_update.log 2>&1

# Generate daily simulation report at 6 AM
0 6 * * * /path/to/solar_system/install/solar_system --date $(date +\%Y-\%m-\%d) > /var/log/solar_system_daily.log 2>&1

# Weekly cache validation on Sundays at 3 AM
0 3 * * 0 /path/to/solar_system/install/solar_system_fetch --validate >> /var/log/solar_system_validation.log 2>&1
```

## Educational Examples

### Example 13: Planetary Motion Study

```bash
#!/bin/bash
# Study planetary motion over time

echo "🪐 Planetary Motion Study"
echo "========================"

# Generate data for Earth's orbit over one year
start_date="2025-01-01"
end_date="2025-12-31"

echo "Generating Earth orbital data from $start_date to $end_date..."

# Create monthly snapshots
for month in {01..12}; do
    date="2025-${month}-01"
    echo "Processing $date..."
    
    ./solar_system --date $date | grep "Earth:" > "earth_${month}.txt"
done

echo "✅ Data generation complete!"
echo "📊 Files created: earth_01.txt through earth_12.txt"
echo "🔍 Analyze the data to see Earth's orbital motion"
```

### Example 14: Solar System Scale Demonstration

```bash
#!/bin/bash
# Demonstrate solar system scale

echo "🌌 Solar System Scale Demonstration"
echo "=================================="

# Get current positions
./solar_system_realtime --no-continuous > current_positions.txt

echo "Current distances from Sun:"
echo "=========================="

# Extract and display distances
grep "Distance from Sun:" current_positions.txt | while read line; do
    body=$(echo $line | cut -d: -f1)
    distance=$(echo $line | grep -o '[0-9.]*e[+-][0-9]* km' | head -1)
    au=$(echo $line | grep -o '([0-9.]* AU)' | tr -d '()')
    
    echo "$body: $distance ($au)"
done

echo ""
echo "💡 Note: 1 AU = 149,597,870.7 km (Earth-Sun distance)"
```

## Troubleshooting Examples

### Example 15: Diagnostic Script

```bash
#!/bin/bash
# Comprehensive diagnostic script

echo "🔧 Solar System Suite Diagnostics"
echo "================================="

# Check installation
echo "📁 Checking installation..."
if [ -f "./solar_system_launcher" ]; then
    echo "✅ Installation found"
else
    echo "❌ Installation not found in current directory"
    exit 1
fi

# Check system status
echo "🔍 Checking system status..."
./solar_system_launcher --status

# Check data files
echo "📊 Checking data files..."
if [ -f "ephemeris_cache.bin" ]; then
    size=$(ls -lh ephemeris_cache.bin | awk '{print $5}')
    echo "✅ Binary cache found ($size)"
else
    echo "⚠️ Binary cache not found"
fi

if [ -f "ephemeris_data.json" ]; then
    size=$(ls -lh ephemeris_data.json | awk '{print $5}')
    echo "✅ JSON cache found ($size)"
else
    echo "⚠️ JSON cache not found"
fi

# Test applications
echo "🧪 Testing applications..."
apps=("solar_system" "solar_system_fetch" "solar_system_realtime" "solar_system_web")

for app in "${apps[@]}"; do
    if [ -f "./bin/$app" ]; then
        echo "✅ $app found"
    else
        echo "❌ $app not found"
    fi
done

# Test web server
echo "🌐 Testing web server..."
./solar_system_web --port 8083 &
web_pid=$!
sleep 3

if curl -s http://localhost:8083/api/status > /dev/null; then
    echo "✅ Web server responding"
else
    echo "❌ Web server not responding"
fi

kill $web_pid 2>/dev/null

echo "🎉 Diagnostics complete!"
```

## Performance Examples

### Example 16: Benchmarking Script

```bash
#!/bin/bash
# Benchmark solar system suite performance

echo "⚡ Solar System Suite Benchmarks"
echo "==============================="

# Benchmark data loading
echo "📊 Benchmarking data loading..."
echo -n "Cold start (first load): "
rm -f ephemeris_cache.bin ephemeris_data.json 2>/dev/null
time ./solar_system_fetch --update >/dev/null 2>&1

echo -n "Warm start (cached): "
time ./solar_system_fetch --update >/dev/null 2>&1

# Benchmark simulation
echo "🌌 Benchmarking simulation..."
dates=("2025-01-01" "2025-07-01" "2026-01-01")

for date in "${dates[@]}"; do
    echo -n "Simulation for $date: "
    time ./solar_system --date $date >/dev/null 2>&1
done

# Benchmark web API
echo "🌐 Benchmarking web API..."
./solar_system_web --port 8084 &
web_pid=$!
sleep 2

echo -n "API response time: "
time curl -s http://localhost:8084/api/solar_system >/dev/null

kill $web_pid 2>/dev/null

echo "✅ Benchmarking complete!"
```

These examples demonstrate the full range of capabilities of the Solar System Suite, from basic usage to advanced integration and automation scenarios. Each example is designed to be practical and immediately usable in real-world applications.
