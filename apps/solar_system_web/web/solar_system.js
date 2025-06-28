/**
 * Solar System Web Visualization
 * 
 * WebGL-based 3D visualization of the solar system with real-time data
 * from the Solar System Suite backend API.
 */

class SolarSystemVisualization {
    constructor() {
        this.canvas = document.getElementById('canvas');
        this.gl = null;
        this.program = null;
        
        // Solar system data
        this.bodies = [];
        this.lastUpdate = 0;
        
        // Visualization state
        this.camera = {
            x: 0, y: 0, z: 100,
            rotX: 0, rotY: 0,
            zoom: 1.0
        };
        
        this.settings = {
            showOrbits: false,
            showLabels: false,
            showVelocities: false,
            isPlaying: true,
            viewMode: '3d', // '3d' or 'top'
            realtime: false
        };
        
        // Animation
        this.animationId = null;
        this.lastFrameTime = 0;
        this.fps = 0;
        
        // Mouse interaction
        this.mouseDown = false;
        this.lastMouseX = 0;
        this.lastMouseY = 0;
        
        // Debug info
        this.debugMode = true;
        
        this.init();
    }
    
    async init() {
        try {
            this.log('Initializing Solar System Visualization...');
            await this.initWebGL();
            this.setupEventListeners();
            await this.loadInitialData();
            this.startAnimation();
            
            document.getElementById('loading').classList.add('hidden');
            this.log('Initialization complete!');
        } catch (error) {
            console.error('Failed to initialize:', error);
            this.showError('Failed to initialize: ' + error.message);
        }
    }
    
    log(message) {
        if (this.debugMode) {
            console.log('[SolarSystem]', message);
        }
    }
    
    async initWebGL() {
        this.log('Initializing WebGL...');
        
        this.gl = this.canvas.getContext('webgl') || this.canvas.getContext('experimental-webgl');
        if (!this.gl) {
            throw new Error('WebGL not supported in this browser');
        }
        
        this.log('WebGL context created successfully');
        
        // Check WebGL capabilities
        const maxTextureSize = this.gl.getParameter(this.gl.MAX_TEXTURE_SIZE);
        const maxVertexAttribs = this.gl.getParameter(this.gl.MAX_VERTEX_ATTRIBS);
        this.log(`WebGL capabilities: Max texture size: ${maxTextureSize}, Max vertex attributes: ${maxVertexAttribs}`);
        
        // Resize canvas
        this.resizeCanvas();
        
        // Create shader program
        const vertexShader = this.createShader(this.gl.VERTEX_SHADER, `
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
        `);
        
        const fragmentShader = this.createShader(this.gl.FRAGMENT_SHADER, `
            precision mediump float;
            varying vec3 vColor;
            
            void main() {
                // Make points circular
                vec2 coord = gl_PointCoord - vec2(0.5);
                if (length(coord) > 0.5) {
                    discard;
                }
                gl_FragColor = vec4(vColor, 1.0);
            }
        `);
        
        this.program = this.createProgram(vertexShader, fragmentShader);
        this.gl.useProgram(this.program);
        
        // Get attribute and uniform locations
        this.positionLocation = this.gl.getAttribLocation(this.program, 'position');
        this.colorLocation = this.gl.getAttribLocation(this.program, 'color');
        this.projectionLocation = this.gl.getUniformLocation(this.program, 'projection');
        this.modelViewLocation = this.gl.getUniformLocation(this.program, 'modelView');
        this.pointSizeLocation = this.gl.getUniformLocation(this.program, 'pointSize');
        
        this.log(`Shader locations - Position: ${this.positionLocation}, Color: ${this.colorLocation}`);
        
        // Enable depth testing and blending
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.BLEND);
        this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
        this.gl.clearColor(0.0, 0.0, 0.1, 1.0); // Dark blue background
        
        this.log('WebGL initialization complete');
    }
    
    createShader(type, source) {
        const shader = this.gl.createShader(type);
        this.gl.shaderSource(shader, source);
        this.gl.compileShader(shader);
        
        if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
            const error = this.gl.getShaderInfoLog(shader);
            this.gl.deleteShader(shader);
            throw new Error('Shader compilation error: ' + error);
        }
        
        return shader;
    }
    
    createProgram(vertexShader, fragmentShader) {
        const program = this.gl.createProgram();
        this.gl.attachShader(program, vertexShader);
        this.gl.attachShader(program, fragmentShader);
        this.gl.linkProgram(program);
        
        if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS)) {
            const error = this.gl.getProgramInfoLog(program);
            this.gl.deleteProgram(program);
            throw new Error('Program linking error: ' + error);
        }
        
        return program;
    }
    
    setupEventListeners() {
        this.log('Setting up event listeners...');
        
        // Window resize
        window.addEventListener('resize', () => this.resizeCanvas());
        
        // Mouse controls
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('wheel', (e) => this.onWheel(e));
        
        // Control buttons
        document.getElementById('btn-3d').addEventListener('click', () => this.setViewMode('3d'));
        document.getElementById('btn-top').addEventListener('click', () => this.setViewMode('top'));
        
        document.getElementById('btn-orbits').addEventListener('click', () => this.toggleOrbits());
        document.getElementById('btn-labels').addEventListener('click', () => this.toggleLabels());
        document.getElementById('btn-velocities').addEventListener('click', () => this.toggleVelocities());
        
        document.getElementById('btn-play').addEventListener('click', () => this.play());
        document.getElementById('btn-pause').addEventListener('click', () => this.pause());
        document.getElementById('btn-reset').addEventListener('click', () => this.reset());
        
        document.getElementById('btn-refresh').addEventListener('click', () => this.refreshData());
        document.getElementById('btn-realtime').addEventListener('click', () => this.toggleRealtime());
    }
    
    resizeCanvas() {
        this.canvas.width = window.innerWidth;
        this.canvas.height = window.innerHeight;
        if (this.gl) {
            this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        }
        this.log(`Canvas resized to ${this.canvas.width}x${this.canvas.height}`);
    }
    
    onMouseDown(e) {
        this.mouseDown = true;
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;
    }
    
    onMouseMove(e) {
        if (!this.mouseDown) return;
        
        const deltaX = e.clientX - this.lastMouseX;
        const deltaY = e.clientY - this.lastMouseY;
        
        this.camera.rotY += deltaX * 0.01;
        this.camera.rotX += deltaY * 0.01;
        
        // Clamp rotation
        this.camera.rotX = Math.max(-Math.PI/2, Math.min(Math.PI/2, this.camera.rotX));
        
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;
    }
    
    onMouseUp() {
        this.mouseDown = false;
    }
    
    onWheel(e) {
        e.preventDefault();
        this.camera.zoom *= (1 + e.deltaY * 0.001);
        this.camera.zoom = Math.max(0.1, Math.min(10.0, this.camera.zoom));
    }
    
    async loadInitialData() {
        this.log('Loading initial data...');
        
        try {
            const [statusResponse, dataResponse] = await Promise.all([
                fetch('/api/status'),
                fetch('/api/solar_system')
            ]);
            
            if (!statusResponse.ok) {
                throw new Error(`Status API error: ${statusResponse.status}`);
            }
            
            if (!dataResponse.ok) {
                throw new Error(`Data API error: ${dataResponse.status}`);
            }
            
            const status = await statusResponse.json();
            const data = await dataResponse.json();
            
            this.log(`Loaded ${data.bodies.length} celestial bodies`);
            
            this.updateStatus(status);
            this.updateBodies(data);
            
        } catch (error) {
            console.error('Failed to load data:', error);
            this.showError('Failed to load solar system data: ' + error.message);
        }
    }
    
    async refreshData() {
        this.log('Refreshing data...');
        await this.loadInitialData();
    }
    
    updateStatus(status) {
        const dataStatusEl = document.getElementById('data-status');
        const bodyCountEl = document.getElementById('body-count');
        
        if (status.data_status.available) {
            dataStatusEl.textContent = status.data_status.current ? 'Current' : 'Outdated';
            dataStatusEl.className = 'status-value ' + (status.data_status.current ? 'good' : 'warning');
        } else {
            dataStatusEl.textContent = 'Hardcoded';
            dataStatusEl.className = 'status-value warning';
        }
        
        bodyCountEl.textContent = status.body_count;
        
        document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
    }
    
    updateBodies(data) {
        this.bodies = data.bodies;
        this.lastUpdate = data.timestamp;
        
        this.log(`Updated ${this.bodies.length} bodies`);
        
        // Log first few bodies for debugging
        if (this.debugMode && this.bodies.length > 0) {
            this.log('Sample body data:');
            for (let i = 0; i < Math.min(3, this.bodies.length); i++) {
                const body = this.bodies[i];
                this.log(`  ${body.name}: pos(${body.position.x.toFixed(0)}, ${body.position.y.toFixed(0)}, ${body.position.z.toFixed(0)})`);
            }
        }
        
        // Update body list
        const bodyListEl = document.getElementById('body-list');
        bodyListEl.innerHTML = '';
        
        this.bodies.forEach(body => {
            const bodyEl = document.createElement('div');
            bodyEl.className = 'body-item';
            bodyEl.innerHTML = `
                <div class="body-name">${body.name}</div>
                <div class="body-coords">
                    x: ${(body.position.x / 1000000).toFixed(1)}M km<br>
                    y: ${(body.position.y / 1000000).toFixed(1)}M km<br>
                    z: ${(body.position.z / 1000000).toFixed(1)}M km
                </div>
            `;
            bodyListEl.appendChild(bodyEl);
        });
    }
    
    setViewMode(mode) {
        this.settings.viewMode = mode;
        
        // Update button states
        document.getElementById('btn-3d').classList.toggle('active', mode === '3d');
        document.getElementById('btn-top').classList.toggle('active', mode === 'top');
        
        // Reset camera for top view
        if (mode === 'top') {
            this.camera.rotX = -Math.PI/2;
            this.camera.rotY = 0;
        } else {
            this.camera.rotX = 0;
            this.camera.rotY = 0;
        }
        
        this.log(`View mode changed to: ${mode}`);
    }
    
    toggleOrbits() {
        this.settings.showOrbits = !this.settings.showOrbits;
        document.getElementById('btn-orbits').classList.toggle('active', this.settings.showOrbits);
    }
    
    toggleLabels() {
        this.settings.showLabels = !this.settings.showLabels;
        document.getElementById('btn-labels').classList.toggle('active', this.settings.showLabels);
    }
    
    toggleVelocities() {
        this.settings.showVelocities = !this.settings.showVelocities;
        document.getElementById('btn-velocities').classList.toggle('active', this.settings.showVelocities);
    }
    
    play() {
        this.settings.isPlaying = true;
        document.getElementById('btn-play').classList.add('active');
        document.getElementById('btn-pause').classList.remove('active');
    }
    
    pause() {
        this.settings.isPlaying = false;
        document.getElementById('btn-play').classList.remove('active');
        document.getElementById('btn-pause').classList.add('active');
    }
    
    reset() {
        this.camera.rotX = 0;
        this.camera.rotY = 0;
        this.camera.zoom = 1.0;
        this.log('Camera reset');
    }
    
    toggleRealtime() {
        this.settings.realtime = !this.settings.realtime;
        document.getElementById('btn-realtime').classList.toggle('active', this.settings.realtime);
        
        if (this.settings.realtime) {
            // Start real-time updates
            this.realtimeInterval = setInterval(() => this.refreshData(), 5000);
            this.log('Real-time mode enabled');
        } else {
            // Stop real-time updates
            if (this.realtimeInterval) {
                clearInterval(this.realtimeInterval);
                this.realtimeInterval = null;
            }
            this.log('Real-time mode disabled');
        }
    }
    
    startAnimation() {
        this.log('Starting animation loop...');
        
        const animate = (currentTime) => {
            // Calculate FPS
            if (this.lastFrameTime) {
                const deltaTime = currentTime - this.lastFrameTime;
                this.fps = Math.round(1000 / deltaTime);
                document.getElementById('fps').textContent = this.fps;
            }
            this.lastFrameTime = currentTime;
            
            this.render();
            this.animationId = requestAnimationFrame(animate);
        };
        
        animate(0);
    }
    
    render() {
        if (!this.gl || !this.program) return;
        
        this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
        
        if (this.bodies.length === 0) {
            return;
        }
        
        // Create projection matrix
        const aspect = this.canvas.width / this.canvas.height;
        const projectionMatrix = this.createPerspectiveMatrix(45 * Math.PI / 180, aspect, 0.1, 1000.0);
        
        // Create model-view matrix
        const modelViewMatrix = this.createModelViewMatrix();
        
        // Set uniforms
        this.gl.uniformMatrix4fv(this.projectionLocation, false, projectionMatrix);
        this.gl.uniformMatrix4fv(this.modelViewLocation, false, modelViewMatrix);
        this.gl.uniform1f(this.pointSizeLocation, 8.0);
        
        // Render bodies
        this.renderBodies();
    }
    
    renderBodies() {
        const positions = [];
        const colors = [];
        
        // Find the range of positions to scale appropriately
        let maxDistance = 0;
        this.bodies.forEach(body => {
            const distance = Math.sqrt(
                body.position.x * body.position.x + 
                body.position.y * body.position.y + 
                body.position.z * body.position.z
            );
            maxDistance = Math.max(maxDistance, distance);
        });
        
        // Scale factor to fit solar system in view
        const scale = maxDistance > 0 ? 50.0 / maxDistance : 1.0;
        
        this.bodies.forEach((body, index) => {
            // Scale positions for better visualization
            const x = body.position.x * scale;
            const y = body.position.y * scale;
            const z = body.position.z * scale;
            
            positions.push(x, y, z);
            
            // Color based on body type
            const color = this.getBodyColor(body.name);
            colors.push(color.r, color.g, color.b);
        });
        
        if (positions.length === 0) {
            this.log('No positions to render');
            return;
        }
        
        // Create and bind position buffer
        const positionBuffer = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, positionBuffer);
        this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(positions), this.gl.STATIC_DRAW);
        this.gl.enableVertexAttribArray(this.positionLocation);
        this.gl.vertexAttribPointer(this.positionLocation, 3, this.gl.FLOAT, false, 0, 0);
        
        // Create and bind color buffer
        const colorBuffer = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, colorBuffer);
        this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(colors), this.gl.STATIC_DRAW);
        this.gl.enableVertexAttribArray(this.colorLocation);
        this.gl.vertexAttribPointer(this.colorLocation, 3, this.gl.FLOAT, false, 0, 0);
        
        // Draw points
        this.gl.drawArrays(this.gl.POINTS, 0, this.bodies.length);
        
        // Clean up buffers
        this.gl.deleteBuffer(positionBuffer);
        this.gl.deleteBuffer(colorBuffer);
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
        
        // Identity matrix
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
        
        // Apply transformations
        this.translate(matrix, 0, 0, -100 * this.camera.zoom);
        this.rotateX(matrix, this.camera.rotX);
        this.rotateY(matrix, this.camera.rotY);
        
        return matrix;
    }
    
    translate(matrix, x, y, z) {
        matrix[12] += x;
        matrix[13] += y;
        matrix[14] += z;
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
        const loading = document.getElementById('loading');
        loading.innerHTML = `<div style="color: #f44336; text-align: center;">
            <h3>⚠️ Error</h3>
            <p>${message}</p>
            <p style="font-size: 12px; margin-top: 10px;">
                Check browser console for details.<br>
                Try refreshing the page or using a different browser.
            </p>
        </div>`;
        loading.classList.remove('hidden');
    }
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', () => {
    console.log('Starting Solar System Visualization...');
    new SolarSystemVisualization();
});
