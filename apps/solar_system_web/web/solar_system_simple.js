/**
 * Simplified Solar System Visualization
 * Fast-loading version with better error handling
 */

class SimpleSolarSystem {
    constructor() {
        this.canvas = document.getElementById('canvas');
        this.gl = null;
        this.program = null;
        this.bodies = [];
        
        // Camera
        this.camera = { rotX: 0, rotY: 0, zoom: 1.0 };
        
        // Mouse
        this.mouseDown = false;
        this.lastMouseX = 0;
        this.lastMouseY = 0;
        
        // Animation
        this.animationId = null;
        this.fps = 0;
        this.lastFrameTime = 0;
        
        console.log('🚀 Starting Simple Solar System...');
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
            
            // Hide loading screen
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
            
            // Update body list
            this.updateBodyList();
            
        } catch (error) {
            console.error('Failed to load data:', error);
            throw new Error('Cannot connect to server. Make sure the web server is running.');
        }
    }
    
    initWebGL() {
        // Get WebGL context
        this.gl = this.canvas.getContext('webgl') || this.canvas.getContext('experimental-webgl');
        if (!this.gl) {
            throw new Error('WebGL not supported. Try a different browser (Chrome, Firefox, Safari, Edge).');
        }
        
        console.log('✅ WebGL context created');
        
        // Resize canvas
        this.resizeCanvas();
        
        // Create shaders
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
        
        // Compile shaders
        const vertexShader = this.compileShader(this.gl.VERTEX_SHADER, vertexShaderSource);
        const fragmentShader = this.compileShader(this.gl.FRAGMENT_SHADER, fragmentShaderSource);
        
        // Create program
        this.program = this.gl.createProgram();
        this.gl.attachShader(this.program, vertexShader);
        this.gl.attachShader(this.program, fragmentShader);
        this.gl.linkProgram(this.program);
        
        if (!this.gl.getProgramParameter(this.program, this.gl.LINK_STATUS)) {
            throw new Error('Shader program failed to link: ' + this.gl.getProgramInfoLog(this.program));
        }
        
        this.gl.useProgram(this.program);
        
        // Get locations
        this.positionLocation = this.gl.getAttribLocation(this.program, 'position');
        this.colorLocation = this.gl.getAttribLocation(this.program, 'color');
        this.projectionLocation = this.gl.getUniformLocation(this.program, 'projection');
        this.modelViewLocation = this.gl.getUniformLocation(this.program, 'modelView');
        this.pointSizeLocation = this.gl.getUniformLocation(this.program, 'pointSize');
        
        // Setup WebGL state
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.BLEND);
        this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
        this.gl.clearColor(0.0, 0.0, 0.1, 1.0);
        
        console.log('✅ WebGL shaders compiled and linked');
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
            this.camera.zoom *= (1 + e.deltaY * 0.001);
            this.camera.zoom = Math.max(0.1, Math.min(5.0, this.camera.zoom));
        });
        
        // Button controls
        document.getElementById('btn-reset').addEventListener('click', () => {
            this.camera.rotX = 0;
            this.camera.rotY = 0;
            this.camera.zoom = 1.0;
        });
        
        document.getElementById('btn-top').addEventListener('click', () => {
            this.camera.rotX = -Math.PI/2;
            this.camera.rotY = 0;
        });
        
        document.getElementById('btn-3d').addEventListener('click', () => {
            this.camera.rotX = 0;
            this.camera.rotY = 0;
        });
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
            
            this.render();
            this.animationId = requestAnimationFrame(animate);
        };
        
        animate(0);
    }
    
    render() {
        if (!this.gl || this.bodies.length === 0) return;
        
        this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
        
        // Create matrices
        const aspect = this.canvas.width / this.canvas.height;
        const projection = this.createPerspectiveMatrix(45 * Math.PI / 180, aspect, 0.1, 1000.0);
        const modelView = this.createModelViewMatrix();
        
        // Set uniforms
        this.gl.uniformMatrix4fv(this.projectionLocation, false, projection);
        this.gl.uniformMatrix4fv(this.modelViewLocation, false, modelView);
        this.gl.uniform1f(this.pointSizeLocation, 15.0);
        
        // Prepare data
        const positions = [];
        const colors = [];
        
        // Find max distance for scaling
        let maxDist = 0;
        this.bodies.forEach(body => {
            const dist = Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y + body.position.z*body.position.z);
            maxDist = Math.max(maxDist, dist);
        });
        
        const scale = maxDist > 0 ? 40.0 / maxDist : 1.0;
        
        // Add body positions and colors
        this.bodies.forEach(body => {
            positions.push(
                body.position.x * scale,
                body.position.y * scale,
                body.position.z * scale
            );
            
            const color = this.getBodyColor(body.name);
            colors.push(color.r, color.g, color.b);
        });
        
        // Create buffers
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
        
        // Draw
        this.gl.drawArrays(this.gl.POINTS, 0, this.bodies.length);
        
        // Cleanup
        this.gl.deleteBuffer(posBuffer);
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
    
    updateBodyList() {
        const listEl = document.getElementById('body-list');
        listEl.innerHTML = '';
        
        this.bodies.slice(0, 10).forEach(body => { // Show first 10 bodies
            const div = document.createElement('div');
            div.className = 'body-item';
            div.innerHTML = `
                <div class="body-name">${body.name}</div>
                <div class="body-coords">${(Math.sqrt(body.position.x*body.position.x + body.position.y*body.position.y)/1000000).toFixed(1)}M km</div>
            `;
            listEl.appendChild(div);
        });
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
        
        // Apply transformations
        matrix[14] = -80 * this.camera.zoom; // Move back
        
        // Rotate
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
                    • Check browser console (F12) for details<br>
                    • Try a different browser
                </p>
            </div>
        `;
    }
}

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    console.log('🌌 Initializing Solar System Visualization...');
    new SimpleSolarSystem();
});
