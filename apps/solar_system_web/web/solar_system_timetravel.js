/**
 * Solar System Time Travel Visualization - FIXED VERSION
 * Enhanced WebGL visualization with time control, orbit trails, and labels
 */

class SolarSystemTimeTravel {
    constructor() {
        this.canvas = document.getElementById('canvas');
        this.gl = null;
        this.program = null;
        this.bodies = [];
        
        // Time control
        this.timeControl = {
            mode: 'realtime',
            currentTime: new Date(),
            startTime: new Date('2020-01-01'),
            speed: 10,
            isPlaying: true,
            lastUpdateTime: Date.now()
        };
        
        // Camera
        this.camera = { rotX: 0, rotY: 0, zoom: 1.0 };
        
        // Settings
        this.settings = {
            showOrbits: false,
            showLabels: false,
            showTrails: false,
            viewMode: '3d'
        };
        
        // Mouse
        this.mouseDown = false;
        this.lastMouseX = 0;
        this.lastMouseY = 0;
        
        // Animation
        this.animationId = null;
        this.fps = 0;
        this.lastFrameTime = 0;
        
        console.log('🚀 Starting Solar System Time Travel...');
        this.init();
    }
    
    async init() {
        try {
            console.log('📡 Loading data...');
            await this.loadData();
            
            console.log('🎨 Initializing WebGL...');
            this.initWebGL();
            
            console.log('🎮 Setting up controls...');
            this.setupControls();
            
            console.log('▶️ Starting animation...');
            this.startAnimation();
            
            this.updateTimeDisplay();
            document.getElementById('loading').style.display = 'none';
            console.log('✅ Initialization complete!');
            
        } catch (error) {
            console.error('❌ Initialization failed:', error);
            this.showError('Failed to initialize: ' + error.message);
        }
    }
    
    async loadData() {
        try {
            const response = await fetch('/api/solar_system');
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const data = await response.json();
            this.bodies = data.bodies;
            
            console.log(`📊 Loaded ${this.bodies.length} celestial bodies`);
            
            // Update status
            document.getElementById('body-count').textContent = this.bodies.length;
            document.getElementById('data-status').textContent = 'Loaded';
            document.getElementById('data-status').className = 'status-value good';
            
            this.updateBodyList();
            
        } catch (error) {
            console.error('Failed to load data:', error);
            throw new Error('Cannot connect to server. Make sure the web server is running.');
        }
    }
    
    initWebGL() {
        this.gl = this.canvas.getContext('webgl') || this.canvas.getContext('experimental-webgl');
        if (!this.gl) {
            throw new Error('WebGL not supported. Try a different browser.');
        }
        
        this.resizeCanvas();
        
        const vertexShaderSource = `
            attribute vec3 position;
            attribute vec3 color;
            uniform mat4 projection;
            uniform mat4 modelView;
            uniform float pointSize;
            varying vec3 vColor;
            
            void main() {
                gl_Position = projection * modelView * vec4(position, 1.0);
                gl_PointSize = pointSize;
                vColor = color;
            }
        `;
        
        const fragmentShaderSource = `
            precision mediump float;
            varying vec3 vColor;
            
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                if (length(coord) > 0.5) discard;
                gl_FragColor = vec4(vColor, 1.0);
            }
        `;
        
        const vertexShader = this.compileShader(this.gl.VERTEX_SHADER, vertexShaderSource);
        const fragmentShader = this.compileShader(this.gl.FRAGMENT_SHADER, fragmentShaderSource);
        
        this.program = this.gl.createProgram();
        this.gl.attachShader(this.program, vertexShader);
        this.gl.attachShader(this.program, fragmentShader);
        this.gl.linkProgram(this.program);
        
        if (!this.gl.getProgramParameter(this.program, this.gl.LINK_STATUS)) {
            throw new Error('Shader program failed to link');
        }
        
        this.gl.useProgram(this.program);
        
        this.positionLocation = this.gl.getAttribLocation(this.program, 'position');
        this.colorLocation = this.gl.getAttribLocation(this.program, 'color');
        this.projectionLocation = this.gl.getUniformLocation(this.program, 'projection');
        this.modelViewLocation = this.gl.getUniformLocation(this.program, 'modelView');
        this.pointSizeLocation = this.gl.getUniformLocation(this.program, 'pointSize');
        
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.BLEND);
        this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
        this.gl.clearColor(0.0, 0.0, 0.1, 1.0);
        
        console.log('✅ WebGL initialized');
    }
    
    compileShader(type, source) {
        const shader = this.gl.createShader(type);
        this.gl.shaderSource(shader, source);
        this.gl.compileShader(shader);
        
        if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
            const error = this.gl.getShaderInfoLog(shader);
            this.gl.deleteShader(shader);
            throw new Error('Shader compilation failed: ' + error);
        }
        
        return shader;
    }
    
    setupControls() {
        // Window resize
        window.addEventListener('resize', () => this.resizeCanvas());
        
        // Mouse controls
        this.canvas.addEventListener('mousedown', (e) => {
            this.mouseDown = true;
            this.lastMouseX = e.clientX;
            this.lastMouseY = e.clientY;
        });
        
        this.canvas.addEventListener('mousemove', (e) => {
            if (!this.mouseDown) return;
            
            const deltaX = e.clientX - this.lastMouseX;
            const deltaY = e.clientY - this.lastMouseY;
            
            this.camera.rotY += deltaX * 0.01;
            this.camera.rotX += deltaY * 0.01;
            this.camera.rotX = Math.max(-Math.PI/2, Math.min(Math.PI/2, this.camera.rotX));
            
            this.lastMouseX = e.clientX;
            this.lastMouseY = e.clientY;
        });
        
        this.canvas.addEventListener('mouseup', () => {
            this.mouseDown = false;
        });
        
        this.canvas.addEventListener('wheel', (e) => {
            e.preventDefault();
            // Improved zoom with much wider range
            const zoomFactor = 1 + e.deltaY * 0.001;
            this.camera.zoom *= zoomFactor;
            // Allow much closer zoom (0.01x) and farther zoom (100x)
            this.camera.zoom = Math.max(0.01, Math.min(100.0, this.camera.zoom));
        });
        
        // Time control buttons
        document.getElementById('btn-realtime').addEventListener('click', () => this.setRealtimeMode());
        document.getElementById('btn-simulation').addEventListener('click', () => this.setSimulationMode());
        document.getElementById('btn-start-simulation').addEventListener('click', () => this.startTimeTravel());
        document.getElementById('btn-sync-realtime').addEventListener('click', () => this.syncToRealtime());
        
        // Speed control
        document.getElementById('speed-slider').addEventListener('input', (e) => this.setSpeed(e.target.value));
        
        // View controls
        document.getElementById('btn-3d').addEventListener('click', () => this.setViewMode('3d'));
        document.getElementById('btn-top').addEventListener('click', () => this.setViewMode('top'));
        
        // Display options
        document.getElementById('btn-orbits').addEventListener('click', () => this.toggleOrbits());
        document.getElementById('btn-labels').addEventListener('click', () => this.toggleLabels());
        document.getElementById('btn-trails').addEventListener('click', () => this.toggleTrails());
        
        // Animation controls
        document.getElementById('btn-play').addEventListener('click', () => this.play());
        document.getElementById('btn-pause').addEventListener('click', () => this.pause());
        document.getElementById('btn-reset').addEventListener('click', () => this.reset());
        
        this.updateSpeedDisplay();
        this.updateModeDisplay();
    }
    
    // Time control methods
    setRealtimeMode() {
        this.timeControl.mode = 'realtime';
        this.timeControl.currentTime = new Date();
        this.updateModeDisplay();
        console.log('Switched to real-time mode');
    }
    
    setSimulationMode() {
        this.timeControl.mode = 'simulation';
        // Don't automatically sync back to real-time
        this.timeControl.currentTime = new Date(document.getElementById('start-date').value || '2020-01-01');
        this.updateModeDisplay();
        this.updateTimeDisplay();
        console.log('Switched to simulation mode');
    }
    
    startTimeTravel() {
        const startDateStr = document.getElementById('start-date').value;
        this.timeControl.startTime = new Date(startDateStr);
        this.timeControl.currentTime = new Date(startDateStr);
        this.timeControl.lastUpdateTime = Date.now();
        this.timeControl.mode = 'simulation';
        this.timeControl.isPlaying = true;
        
        // Update UI
        this.updateModeDisplay();
        this.updateTimeDisplay();
        
        // Make sure play button is active
        document.getElementById('btn-play').classList.add('active');
        document.getElementById('btn-pause').classList.remove('active');
        
        console.log(`Starting time travel from ${startDateStr}`);
        console.log(`Current simulation time: ${this.timeControl.currentTime}`);
        console.log(`Speed: ${this.timeControl.speed}x`);
    }
    
    syncToRealtime() {
        this.timeControl.currentTime = new Date();
        this.timeControl.mode = 'realtime';
        this.updateModeDisplay();
        console.log('Synced to real-time');
    }
    
    setSpeed(speedIndex) {
        // More accurate speed values that account for actual time advancement
        const speeds = [
            0.1,        // 0.1x (slow motion)
            0.5,        // 0.5x (half speed)
            1,          // 1x (real-time)
            60,         // 1 minute per second
            3600,       // 1 hour per second
            86400,      // 1 day per second
            365 * 86400 // 1 year per second
        ];
        
        this.timeControl.speed = speeds[parseInt(speedIndex)];
        this.updateSpeedDisplay();
        
        console.log(`Speed set to ${this.timeControl.speed}x (${this.getSpeedDescription()})`);
    }
    
    updateSpeedDisplay() {
        const speed = this.timeControl.speed;
        let displayText = this.getSpeedDescription();
        
        document.getElementById('speed-display').textContent = displayText;
    }
    
    updateModeDisplay() {
        const isRealtime = this.timeControl.mode === 'realtime';
        
        document.getElementById('btn-realtime').classList.toggle('active', isRealtime);
        document.getElementById('btn-simulation').classList.toggle('active', !isRealtime);
        
        const timeControlsEl = document.getElementById('time-travel-controls');
        if (timeControlsEl) {
            timeControlsEl.style.display = isRealtime ? 'none' : 'block';
        }
        
        document.getElementById('mode-status').textContent = isRealtime ? 'Real-time' : 'Time Travel';
    }
    
    updateTimeDisplay() {
        const currentTimeEl = document.getElementById('current-time');
        const timeInfoEl = document.getElementById('time-info');
        
        if (currentTimeEl) {
            const timeStr = this.timeControl.currentTime.toLocaleString();
            currentTimeEl.textContent = timeStr;
        }
        
        if (timeInfoEl) {
            if (this.timeControl.mode === 'simulation') {
                const now = new Date();
                const diffMs = now - this.timeControl.currentTime;
                const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));
                const diffYears = Math.floor(diffDays / 365);
                
                if (Math.abs(diffYears) >= 1) {
                    if (diffYears > 0) {
                        timeInfoEl.textContent = `${diffYears} years ago`;
                    } else {
                        timeInfoEl.textContent = `${Math.abs(diffYears)} years in future`;
                    }
                } else if (Math.abs(diffDays) >= 1) {
                    if (diffDays > 0) {
                        timeInfoEl.textContent = `${diffDays} days ago`;
                    } else {
                        timeInfoEl.textContent = `${Math.abs(diffDays)} days in future`;
                    }
                } else {
                    timeInfoEl.textContent = 'Current time';
                }
                
                // Add speed indicator
                if (this.timeControl.isPlaying) {
                    timeInfoEl.textContent += ` (${this.getSpeedDescription()})`;
                }
            } else {
                timeInfoEl.textContent = 'Live tracking';
            }
        }
    }
    
    getSpeedDescription() {
        const speed = this.timeControl.speed;
        if (speed < 1) {
            return `${speed}x speed`;
        } else if (speed === 1) {
            return 'Real-time';
        } else if (speed < 60) {
            return `${speed}x speed`;
        } else if (speed < 3600) {
            const minutes = Math.round(speed / 60);
            return `${minutes} min/sec`;
        } else if (speed < 86400) {
            const hours = Math.round(speed / 3600);
            return `${hours} hr/sec`;
        } else if (speed < 365 * 86400) {
            const days = Math.round(speed / 86400);
            return `${days} days/sec`;
        } else {
            const years = Math.round(speed / (365 * 86400));
            return `${years} years/sec`;
        }
    }
    
    // UI Controls
    setViewMode(mode) {
        this.settings.viewMode = mode;
        document.getElementById('btn-3d').classList.toggle('active', mode === '3d');
        document.getElementById('btn-top').classList.toggle('active', mode === 'top');
        
        if (mode === 'top') {
            this.camera.rotX = -Math.PI/2;
            this.camera.rotY = 0;
        } else {
            this.camera.rotX = 0;
            this.camera.rotY = 0;
        }
    }
    
    toggleOrbits() {
        this.settings.showOrbits = !this.settings.showOrbits;
        document.getElementById('btn-orbits').classList.toggle('active', this.settings.showOrbits);
    }
    
    toggleLabels() {
        this.settings.showLabels = !this.settings.showLabels;
        document.getElementById('btn-labels').classList.toggle('active', this.settings.showLabels);
    }
    
    toggleTrails() {
        this.settings.showTrails = !this.settings.showTrails;
        document.getElementById('btn-trails').classList.toggle('active', this.settings.showTrails);
        
        if (!this.settings.showTrails) {
            // Clear trails when disabled
            if (this.orbitTrails) {
                this.orbitTrails.clear();
            }
            document.getElementById('orbit-points').textContent = '0';
        } else {
            // Initialize trails when enabled
            if (!this.orbitTrails) {
                this.orbitTrails = new Map();
            }
            console.log('Orbit trails enabled');
        }
    }
    
    play() {
        this.timeControl.isPlaying = true;
        this.timeControl.lastUpdateTime = Date.now();
        document.getElementById('btn-play').classList.add('active');
        document.getElementById('btn-pause').classList.remove('active');
    }
    
    pause() {
        this.timeControl.isPlaying = false;
        document.getElementById('btn-play').classList.remove('active');
        document.getElementById('btn-pause').classList.add('active');
    }
    
    reset() {
        this.camera.rotX = 0;
        this.camera.rotY = 0;
        this.camera.zoom = 1.0;
    }
    
    resizeCanvas() {
        this.canvas.width = window.innerWidth;
        this.canvas.height = window.innerHeight;
        if (this.gl) {
            this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        }
    }
    
    startAnimation() {
        const animate = (currentTime) => {
            // Calculate FPS
            if (this.lastFrameTime) {
                const deltaTime = currentTime - this.lastFrameTime;
                this.fps = Math.round(1000 / deltaTime);
                document.getElementById('fps').textContent = this.fps;
            }
            this.lastFrameTime = currentTime;
            
            // Update simulation time
            this.updateSimulationTime();
            
            this.render();
            this.animationId = requestAnimationFrame(animate);
        };
        
        animate(0);
    }
    
    updateSimulationTime() {
        if (this.timeControl.mode === 'simulation' && this.timeControl.isPlaying) {
            const now = Date.now();
            const deltaMs = now - this.timeControl.lastUpdateTime;
            
            // Adaptive time stepping based on desired speed
            let simulationDeltaMs;
            
            if (this.timeControl.speed <= 1) {
                simulationDeltaMs = deltaMs * this.timeControl.speed;
            } else if (this.timeControl.speed <= 100) {
                simulationDeltaMs = deltaMs * this.timeControl.speed;
            } else {
                // For very high speeds, calculate target advance per frame
                const targetAdvancePerSecond = this.timeControl.speed * 1000;
                const targetAdvancePerFrame = targetAdvancePerSecond * (deltaMs / 1000);
                simulationDeltaMs = targetAdvancePerFrame;
            }
            
            this.timeControl.currentTime = new Date(this.timeControl.currentTime.getTime() + simulationDeltaMs);
            this.timeControl.lastUpdateTime = now;
            
            this.updateTimeDisplay();
            
            // Request updated positions from C++ backend for the current simulation time
            this.requestSimulationData(this.timeControl.currentTime);
            
            // Update orbit trails if enabled
            if (this.settings.showTrails) {
                this.updateOrbitTrails();
            }
            
            console.log(`Simulation time: ${this.timeControl.currentTime.toISOString()}`);
            
            // Only auto-sync if we're more than 50 years in the future
            const now_real = new Date();
            const yearsDifference = (this.timeControl.currentTime - now_real) / (1000 * 60 * 60 * 24 * 365);
            
            if (yearsDifference > 50) {
                console.log('Simulation reached far future, syncing to real-time');
                this.syncToRealtime();
            }
        }
    }
    
    async requestSimulationData(targetDate) {
        try {
            // Format date for API request (YYYY-MM-DD)
            const dateStr = targetDate.toISOString().split('T')[0];
            
            // Request simulation data for specific date
            const response = await fetch(`/api/solar_system?date=${dateStr}`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json();
            
            // Update bodies with new positions
            if (data.bodies && Array.isArray(data.bodies)) {
                this.updateBodies(data);
            }
        } catch (error) {
            console.warn('Failed to fetch simulation data for date:', targetDate, error);
            // Continue with current positions if request fails
        }
    }
    
    updateOrbitTrails() {
        if (!this.orbitTrails) {
            this.orbitTrails = new Map();
        }
        
        const now = Date.now();
        if (!this.lastTrailUpdate) this.lastTrailUpdate = 0;
        
        // Add trail points every 100ms
        if (now - this.lastTrailUpdate > 100) {
            this.bodies.forEach(body => {
                if (body.name === 'Sun') return; // Don't trail the Sun
                
                if (!this.orbitTrails.has(body.name)) {
                    this.orbitTrails.set(body.name, []);
                }
                
                const trail = this.orbitTrails.get(body.name);
                
                // Add current position to trail
                trail.push({
                    x: body.position.x,
                    y: body.position.y,
                    z: body.position.z,
                    timestamp: now
                });
                
                // Limit trail length
                const maxPoints = 200;
                if (trail.length > maxPoints) {
                    trail.shift();
                }
            });
            
            this.lastTrailUpdate = now;
            
            // Update orbit points counter
            const totalPoints = Array.from(this.orbitTrails.values()).reduce((sum, trail) => sum + trail.length, 0);
            document.getElementById('orbit-points').textContent = totalPoints;
        }
    }
    
    render() {
        if (!this.gl || this.bodies.length === 0) return;
        
        this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
        
        const aspect = this.canvas.width / this.canvas.height;
        const projection = this.createPerspectiveMatrix(45 * Math.PI / 180, aspect, 0.1, 1000.0);
        const modelView = this.createModelViewMatrix();
        
        this.gl.uniformMatrix4fv(this.projectionLocation, false, projection);
        this.gl.uniformMatrix4fv(this.modelViewLocation, false, modelView);
        this.gl.uniform1f(this.pointSizeLocation, 15.0);
        
        // Render orbit trails first (behind bodies)
        if (this.settings.showTrails && this.orbitTrails) {
            this.renderOrbitTrails(projection, modelView);
        }
        
        // Render celestial bodies
        this.renderBodies();
        
        // Render labels last (on top)
        if (this.settings.showLabels) {
            this.renderLabels(projection, modelView);
        }
    }
    
    renderOrbitTrails(projectionMatrix, modelViewMatrix) {
        if (!this.orbitTrails || this.orbitTrails.size === 0) return;
        
        // Find max distance for scaling (same as bodies)
        let maxDist = 0;
        this.bodies.forEach(body => {
            const dist = Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y + body.position.z*body.position.z);
            maxDist = Math.max(maxDist, dist);
        });
        const scale = maxDist > 0 ? 40.0 / maxDist : 1.0;
        
        // Render each body's trail
        this.orbitTrails.forEach((trail, bodyName) => {
            if (trail.length < 2) return;
            
            const color = this.getBodyColor(bodyName);
            
            // Create trail positions
            const positions = [];
            trail.forEach(point => {
                positions.push(
                    point.x * scale,
                    point.y * scale,
                    point.z * scale
                );
            });
            
            // Create buffer for trail
            const trailBuffer = this.gl.createBuffer();
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, trailBuffer);
            this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(positions), this.gl.STATIC_DRAW);
            this.gl.enableVertexAttribArray(this.positionLocation);
            this.gl.vertexAttribPointer(this.positionLocation, 3, this.gl.FLOAT, false, 0, 0);
            
            // Create color buffer (same color for all points in trail)
            const colors = [];
            for (let i = 0; i < trail.length; i++) {
                // Fade older points
                const alpha = i / trail.length;
                colors.push(color.r * alpha, color.g * alpha, color.b * alpha);
            }
            
            const colorBuffer = this.gl.createBuffer();
            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, colorBuffer);
            this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(colors), this.gl.STATIC_DRAW);
            this.gl.enableVertexAttribArray(this.colorLocation);
            this.gl.vertexAttribPointer(this.colorLocation, 3, this.gl.FLOAT, false, 0, 0);
            
            // Draw trail as line strip
            this.gl.drawArrays(this.gl.LINE_STRIP, 0, trail.length);
            
            // Cleanup
            this.gl.deleteBuffer(trailBuffer);
            this.gl.deleteBuffer(colorBuffer);
        });
    }
    
    renderBodies() {
        const positions = [];
        const colors = [];
        
        // Find max distance for scaling - but use a more reasonable approach
        let maxDist = 0;
        this.bodies.forEach(body => {
            const dist = Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y + body.position.z*body.position.z);
            maxDist = Math.max(maxDist, dist);
        });
        
        // Improved scaling that works better for inner solar system
        // Use a logarithmic-like scaling to better show both inner and outer planets
        const baseScale = maxDist > 0 ? 50.0 / maxDist : 1.0;
        
        this.bodies.forEach(body => {
            // Apply scaling
            const scaledX = body.position.x * baseScale;
            const scaledY = body.position.y * baseScale;
            const scaledZ = body.position.z * baseScale;
            
            positions.push(scaledX, scaledY, scaledZ);
            
            const color = this.getBodyColor(body.name);
            colors.push(color.r, color.g, color.b);
        });
        
        const posBuffer = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, posBuffer);
        this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(positions), this.gl.STATIC_DRAW);
        this.gl.enableVertexAttribArray(this.positionLocation);
        this.gl.vertexAttribPointer(this.positionLocation, 3, this.gl.FLOAT, false, 0, 0);
        
        const colorBuffer = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, colorBuffer);
        this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(colors), this.gl.STATIC_DRAW);
        this.gl.enableVertexAttribArray(this.colorLocation);
        this.gl.vertexAttribPointer(this.colorLocation, 3, this.gl.FLOAT, false, 0, 0);
        
        this.gl.drawArrays(this.gl.POINTS, 0, this.bodies.length);
        
        this.gl.deleteBuffer(posBuffer);
        this.gl.deleteBuffer(colorBuffer);
    }
    
    renderLabels(projectionMatrix, modelViewMatrix) {
        const labelsContainer = document.getElementById('labels-container');
        if (!labelsContainer) return;
        
        labelsContainer.innerHTML = '';
        
        let maxDist = 0;
        this.bodies.forEach(body => {
            const dist = Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y + body.position.z*body.position.z);
            maxDist = Math.max(maxDist, dist);
        });
        const scale = maxDist > 0 ? 40.0 / maxDist : 1.0;
        
        this.bodies.forEach(body => {
            const worldPos = [
                body.position.x * scale,
                body.position.y * scale,
                body.position.z * scale,
                1.0
            ];
            
            const clipPos = this.multiplyMatrixVector(modelViewMatrix, worldPos);
            const finalPos = this.multiplyMatrixVector(projectionMatrix, clipPos);
            
            if (finalPos[3] > 0) {
                const screenX = (finalPos[0] / finalPos[3] + 1) * 0.5 * this.canvas.width;
                const screenY = (1 - finalPos[1] / finalPos[3]) * 0.5 * this.canvas.height;
                
                if (screenX >= 0 && screenX <= this.canvas.width && 
                    screenY >= 0 && screenY <= this.canvas.height) {
                    
                    const label = document.createElement('div');
                    label.className = 'label';
                    label.textContent = body.name;
                    label.style.left = (screenX + 10) + 'px';
                    label.style.top = (screenY - 10) + 'px';
                    
                    labelsContainer.appendChild(label);
                }
            }
        });
    }
    
    multiplyMatrixVector(matrix, vector) {
        const result = [0, 0, 0, 0];
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                result[i] += matrix[j * 4 + i] * vector[j];
            }
        }
        return result;
    }
    
    updateBodyList() {
        const listEl = document.getElementById('body-list');
        if (!listEl) return;
        
        listEl.innerHTML = '';
        
        this.bodies.slice(0, 10).forEach(body => {
            const div = document.createElement('div');
            div.className = 'body-item';
            
            // Convert from meters to kilometers for display
            // body.position values are now correctly in meters (SI units)
            const distance_km = Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y) / 1000;
            
            let distance_display;
            if (distance_km < 1000) {
                distance_display = `${distance_km.toFixed(0)} km`;
            } else if (distance_km < 1000000) {
                distance_display = `${(distance_km/1000).toFixed(1)}K km`;
            } else {
                distance_display = `${(distance_km/1000000).toFixed(1)}M km`;
            }
            
            div.innerHTML = `
                <div class="body-name">${body.name}</div>
                <div class="body-coords">${distance_display}</div>
            `;
            listEl.appendChild(div);
        });
    }
    
    getBodyColor(name) {
        const colors = {
            'Sun': { r: 1.0, g: 1.0, b: 0.0 },
            'Mercury': { r: 0.8, g: 0.7, b: 0.6 },
            'Venus': { r: 1.0, g: 0.8, b: 0.4 },
            'Earth': { r: 0.4, g: 0.6, b: 1.0 },
            'Moon': { r: 0.8, g: 0.8, b: 0.8 },
            'Mars': { r: 1.0, g: 0.4, b: 0.2 },
            'Jupiter': { r: 0.9, g: 0.7, b: 0.4 },
            'Saturn': { r: 0.9, g: 0.8, b: 0.6 },
            'Uranus': { r: 0.4, g: 0.8, b: 0.9 },
            'Neptune': { r: 0.2, g: 0.4, b: 0.9 },
            'Pluto': { r: 0.7, g: 0.6, b: 0.5 }
        };
        
        return colors[name] || { r: 0.8, g: 0.8, b: 0.8 };
    }
    
    createPerspectiveMatrix(fov, aspect, near, far) {
        const f = 1.0 / Math.tan(fov / 2);
        const rangeInv = 1 / (near - far);
        
        return new Float32Array([
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (near + far) * rangeInv, -1,
            0, 0, near * far * rangeInv * 2, 0
        ]);
    }
    
    createModelViewMatrix() {
        const matrix = new Float32Array(16);
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
        
        // Improved camera distance calculation for better inner solar system viewing
        // Allow much closer viewing with zoom
        const baseDistance = 80.0;
        const zoomDistance = baseDistance * this.camera.zoom;
        matrix[14] = -zoomDistance;
        
        this.rotateX(matrix, this.camera.rotX);
        this.rotateY(matrix, this.camera.rotY);
        
        return matrix;
    }
    
    rotateX(matrix, angle) {
        const c = Math.cos(angle);
        const s = Math.sin(angle);
        const m1 = matrix[4], m2 = matrix[5], m3 = matrix[6], m4 = matrix[7];
        const m5 = matrix[8], m6 = matrix[9], m7 = matrix[10], m8 = matrix[11];
        
        matrix[4] = m1 * c + m5 * s;
        matrix[5] = m2 * c + m6 * s;
        matrix[6] = m3 * c + m7 * s;
        matrix[7] = m4 * c + m8 * s;
        matrix[8] = m5 * c - m1 * s;
        matrix[9] = m6 * c - m2 * s;
        matrix[10] = m7 * c - m3 * s;
        matrix[11] = m8 * c - m4 * s;
    }
    
    rotateY(matrix, angle) {
        const c = Math.cos(angle);
        const s = Math.sin(angle);
        const m1 = matrix[0], m2 = matrix[1], m3 = matrix[2], m4 = matrix[3];
        const m5 = matrix[8], m6 = matrix[9], m7 = matrix[10], m8 = matrix[11];
        
        matrix[0] = m1 * c - m5 * s;
        matrix[1] = m2 * c - m6 * s;
        matrix[2] = m3 * c - m7 * s;
        matrix[3] = m4 * c - m8 * s;
        matrix[8] = m1 * s + m5 * c;
        matrix[9] = m2 * s + m6 * c;
        matrix[10] = m3 * s + m7 * c;
        matrix[11] = m4 * s + m8 * c;
    }
    
    showError(message) {
        document.getElementById('loading').innerHTML = `
            <div style="color: #f44336; text-align: center; padding: 20px;">
                <h3>⚠️ Error</h3>
                <p>${message}</p>
                <p style="font-size: 12px; margin-top: 15px;">
                    <strong>Troubleshooting:</strong><br>
                    • Make sure the web server is running<br>
                    • Try refreshing the page<br>
                    • Check browser console (F12) for details
                </p>
            </div>
        `;
    }
}

// Global functions for preset buttons
function setPresetDate(dateStr) {
    document.getElementById('start-date').value = dateStr;
}

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    console.log('🌌 Initializing Solar System Time Travel...');
    new SolarSystemTimeTravel();
});
