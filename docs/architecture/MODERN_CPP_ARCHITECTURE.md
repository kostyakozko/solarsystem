# Modern C++ Architecture Design

## 🎯 **Phase 0: Architecture Transformation Overview**

This document outlines the transformation from procedural C99/C++11 code to modern C++20 object-oriented architecture.

## 🏗️ **Core Architecture Principles**

### **Design Philosophy**
- **RAII (Resource Acquisition Is Initialization)**: All resources managed automatically
- **Type Safety**: Leverage C++20 concepts for compile-time validation
- **Zero-Cost Abstractions**: Modern C++ without performance penalty
- **Composition over Inheritance**: Prefer composition for flexibility
- **Single Responsibility**: Each class has one clear purpose

### **Modern C++ Features Used**
- **C++20 Concepts**: Type constraints and validation
- **Custom Expected Type**: Error handling without exceptions (C++20 compatible)
- **std::optional**: Nullable values
- **Ranges and Views**: Elegant data processing
- **Coroutines**: Async JPL data fetching
- **Smart Pointers**: Automatic memory management
- **CTAD**: Class Template Argument Deduction

## 🌟 **Core Class Hierarchy**

### **1. Mathematical Foundation**

```cpp
namespace SolarSystem::Math {
    // Modern 3D vector with concepts
    template<std::floating_point T = double>
    class Vector3 {
    public:
        constexpr Vector3() = default;
        constexpr Vector3(T x, T y, T z) : x_(x), y_(y), z_(z) {}
        
        // C++20 three-way comparison
        auto operator<=>(const Vector3&) const = default;
        
        // Mathematical operations
        constexpr Vector3 operator+(const Vector3& other) const;
        constexpr Vector3 operator-(const Vector3& other) const;
        constexpr Vector3 operator*(T scalar) const;
        constexpr T dot(const Vector3& other) const;
        constexpr Vector3 cross(const Vector3& other) const;
        constexpr T magnitude() const;
        constexpr Vector3 normalized() const;
        
        // Modern accessors
        constexpr T x() const noexcept { return x_; }
        constexpr T y() const noexcept { return y_; }
        constexpr T z() const noexcept { return z_; }
        
    private:
        T x_{}, y_{}, z_{};
    };
    
    using Vector3d = Vector3<double>;
    using Vector3f = Vector3<float>;
    
    // Physics constants with strong typing
    namespace Constants {
        inline constexpr double G = 6.67430e-11; // m³/kg/s²
        inline constexpr double AU = 1.495978707e11; // meters
        inline constexpr double SECONDS_PER_DAY = 86400.0;
    }
}
```

### **2. Celestial Body System**

```cpp
namespace SolarSystem::Bodies {
    // Strong typing for celestial body properties
    enum class BodyType {
        Star, Planet, Moon, DwarfPlanet, Asteroid, Spacecraft
    };
    
    // Body classification for smart handling
    enum class BodyPriority {
        Essential,   // Planets - always included
        Important,   // Major moons - included by default
        Optional     // Spacecraft - historical date aware
    };
    
    class CelestialBody {
    public:
        // Modern constructor with designated initializers support
        struct Properties {
            std::string name;
            double mass;
            Math::Vector3d position;
            Math::Vector3d velocity;
            BodyType type;
            BodyPriority priority;
            std::optional<std::string> jpl_id;
        };
        
        explicit CelestialBody(Properties props);
        
        // Accessors with modern C++ style
        [[nodiscard]] std::string_view name() const noexcept { return name_; }
        [[nodiscard]] double mass() const noexcept { return mass_; }
        [[nodiscard]] const Math::Vector3d& position() const noexcept { return position_; }
        [[nodiscard]] const Math::Vector3d& velocity() const noexcept { return velocity_; }
        [[nodiscard]] BodyType type() const noexcept { return type_; }
        [[nodiscard]] BodyPriority priority() const noexcept { return priority_; }
        
        // Physics operations
        void apply_force(const Math::Vector3d& force, double dt);
        void update_position(double dt);
        void set_state(const Math::Vector3d& pos, const Math::Vector3d& vel);
        
        // Gravitational interaction
        [[nodiscard]] Math::Vector3d gravitational_force_to(const CelestialBody& other) const;
        [[nodiscard]] double distance_to(const CelestialBody& other) const;
        
    private:
        std::string name_;
        double mass_;
        Math::Vector3d position_;
        Math::Vector3d velocity_;
        Math::Vector3d acceleration_{};
        BodyType type_;
        BodyPriority priority_;
        std::optional<std::string> jpl_id_;
    };
    
    // Body collection with smart filtering
    class BodyCollection {
    public:
        void add_body(CelestialBody body);
        void remove_body(std::string_view name);
        
        // Modern range-based access
        [[nodiscard]] auto bodies() const -> const std::vector<CelestialBody>& { return bodies_; }
        [[nodiscard]] auto bodies() -> std::vector<CelestialBody>& { return bodies_; }
        
        // Smart filtering with ranges
        [[nodiscard]] auto essential_bodies() const;
        [[nodiscard]] auto bodies_by_type(BodyType type) const;
        [[nodiscard]] auto bodies_by_priority(BodyPriority priority) const;
        
        // Find operations
        [[nodiscard]] std::optional<std::reference_wrapper<CelestialBody>> 
            find_body(std::string_view name);
        [[nodiscard]] std::optional<std::reference_wrapper<const CelestialBody>> 
            find_body(std::string_view name) const;
            
        [[nodiscard]] size_t size() const noexcept { return bodies_.size(); }
        [[nodiscard]] bool empty() const noexcept { return bodies_.empty(); }
        
    private:
        std::vector<CelestialBody> bodies_;
        std::unordered_map<std::string, size_t> name_index_;
    };
}
```

### **3. Simulation Engine**

```cpp
namespace SolarSystem::Simulation {
    // Configuration with validation
    struct SimulationConfig {
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        std::chrono::seconds time_step{3600};
        std::vector<std::string> included_bodies;
        std::vector<std::string> excluded_bodies;
        bool use_jpl_data{true};
        bool verbose_output{false};
        
        // Validation with concepts
        template<typename Duration>
        requires std::chrono::is_duration_v<Duration>
        [[nodiscard]] auto validate() const -> Utils::Expected<void, std::string>;
    };
    
    // Simulation state management
    class SimulationState {
    public:
        explicit SimulationState(Bodies::BodyCollection bodies, 
                               std::chrono::system_clock::time_point current_time);
        
        // State access
        [[nodiscard]] const Bodies::BodyCollection& bodies() const { return bodies_; }
        [[nodiscard]] Bodies::BodyCollection& bodies() { return bodies_; }
        [[nodiscard]] std::chrono::system_clock::time_point current_time() const { return current_time_; }
        
        // State manipulation
        void advance_time(std::chrono::seconds dt);
        void set_time(std::chrono::system_clock::time_point time);
        
        // Serialization for web interface
        [[nodiscard]] std::string to_json() const;
        [[nodiscard]] static Utils::Expected<SimulationState, std::string> from_json(std::string_view json);
        
    private:
        Bodies::BodyCollection bodies_;
        std::chrono::system_clock::time_point current_time_;
    };
    
    // Main simulation engine
    class Engine {
    public:
        explicit Engine(SimulationConfig config);
        
        // Simulation control
        [[nodiscard]] auto initialize() -> Utils::Expected<void, std::string>;
        [[nodiscard]] auto step() -> Utils::Expected<void, std::string>;
        [[nodiscard]] auto run_to_time(std::chrono::system_clock::time_point target) 
            -> Utils::Expected<void, std::string>;
        
        // State access
        [[nodiscard]] const SimulationState& state() const { return state_; }
        [[nodiscard]] const SimulationConfig& config() const { return config_; }
        
        // Progress tracking
        [[nodiscard]] double progress() const;
        [[nodiscard]] std::chrono::seconds estimated_remaining_time() const;
        
    private:
        SimulationConfig config_;
        SimulationState state_;
        std::chrono::steady_clock::time_point start_wall_time_;
        
        void perform_physics_step(std::chrono::seconds dt);
        void update_gravitational_forces();
    };
}
```

### **4. JPL Data Management**

```cpp
namespace SolarSystem::Data {
    // Error handling with strong typing
    enum class JPLError {
        NetworkError,
        ParseError,
        InvalidBody,
        InvalidTimeRange,
        CacheError,
        RateLimited
    };
    
    // Ephemeris data structure
    struct EphemerisPoint {
        std::chrono::system_clock::time_point time;
        Math::Vector3d position;
        Math::Vector3d velocity;
    };
    
    class EphemerisData {
    public:
        explicit EphemerisData(std::string body_name);
        
        void add_point(EphemerisPoint point);
        [[nodiscard]] std::optional<EphemerisPoint> 
            interpolate_at(std::chrono::system_clock::time_point time) const;
        
        [[nodiscard]] std::string_view body_name() const { return body_name_; }
        [[nodiscard]] const std::vector<EphemerisPoint>& points() const { return points_; }
        [[nodiscard]] bool empty() const { return points_.empty(); }
        
        // Time range queries
        [[nodiscard]] std::optional<std::chrono::system_clock::time_point> earliest_time() const;
        [[nodiscard]] std::optional<std::chrono::system_clock::time_point> latest_time() const;
        
    private:
        std::string body_name_;
        std::vector<EphemerisPoint> points_;
    };
    
    // Async JPL data fetcher with coroutines
    class JPLDataFetcher {
    public:
        explicit JPLDataFetcher(std::filesystem::path cache_dir);
        
        // Async data fetching
        [[nodiscard]] auto fetch_ephemeris_async(
            std::string_view body_name,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end
        ) -> Utils::Expected<EphemerisData, JPLError>;
        
        // Batch fetching with progress
        [[nodiscard]] auto fetch_multiple_async(
            const std::vector<std::string>& body_names,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end,
            std::function<void(size_t, size_t)> progress_callback = {}
        ) -> Utils::Expected<std::vector<EphemerisData>, JPLError>;
        
        // Cache management
        [[nodiscard]] auto get_cached_data(std::string_view body_name) 
            -> std::optional<EphemerisData>;
        void clear_cache();
        [[nodiscard]] std::filesystem::path cache_directory() const { return cache_dir_; }
        
    private:
        std::filesystem::path cache_dir_;
        std::unordered_map<std::string, EphemerisData> memory_cache_;
        
        [[nodiscard]] auto fetch_from_jpl(std::string_view body_name,
                                         std::chrono::system_clock::time_point start,
                                         std::chrono::system_clock::time_point end) 
            -> Utils::Expected<std::string, JPLError>;
        
        [[nodiscard]] auto parse_jpl_response(std::string_view response) 
            -> Utils::Expected<EphemerisData, JPLError>;
    };
    
    // Smart cache with automatic management
    class DataManager {
    public:
        explicit DataManager(std::filesystem::path cache_dir);
        
        // High-level data operations
        [[nodiscard]] auto ensure_data_available(
            const std::vector<std::string>& body_names,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end
        ) -> Utils::Expected<void, JPLError>;
        
        [[nodiscard]] auto get_body_state_at(
            std::string_view body_name,
            std::chrono::system_clock::time_point time
        ) -> Utils::Expected<EphemerisPoint, JPLError>;
        
        // Cache statistics
        struct CacheStats {
            size_t total_bodies;
            size_t cached_bodies;
            std::filesystem::file_time_type last_update;
            uintmax_t cache_size_bytes;
        };
        [[nodiscard]] CacheStats get_cache_stats() const;
        
    private:
        JPLDataFetcher fetcher_;
        std::unordered_map<std::string, EphemerisData> data_cache_;
    };
}
```

### **5. Application Framework**

```cpp
namespace SolarSystem::Apps {
    // Modern argument parsing
    template<typename T>
    concept Parseable = requires(const std::string& s) {
        { T::from_string(s) } -> std::same_as<Utils::Expected<T, std::string>>;
    };
    
    class ArgumentParser {
    public:
        ArgumentParser(std::string program_name, std::string description);
        
        // Fluent interface for argument definition
        auto add_option(std::string name, std::string description) -> ArgumentParser&;
        auto add_flag(std::string name, std::string description) -> ArgumentParser&;
        
        template<Parseable T>
        auto add_typed_option(std::string name, std::string description, T default_value = {}) -> ArgumentParser&;
        
        // Parse with modern error handling
        [[nodiscard]] auto parse(int argc, char* argv[]) -> Utils::Expected<void, std::string>;
        
        // Value access
        [[nodiscard]] std::optional<std::string> get_option(std::string_view name) const;
        [[nodiscard]] bool get_flag(std::string_view name) const;
        
        template<Parseable T>
        [[nodiscard]] std::optional<T> get_typed_option(std::string_view name) const;
        
        void print_help() const;
        
    private:
        std::string program_name_;
        std::string description_;
        std::unordered_map<std::string, std::string> options_;
        std::unordered_set<std::string> flags_;
        // ... implementation details
    };
    
    // Base application class
    class Application {
    public:
        explicit Application(std::string name, std::string description);
        virtual ~Application() = default;
        
        // Main entry point
        [[nodiscard]] auto run(int argc, char* argv[]) -> int;
        
    protected:
        // Override points for derived applications
        virtual void setup_arguments(ArgumentParser& parser) = 0;
        virtual auto execute() -> Utils::Expected<void, std::string> = 0;
        
        // Utility methods
        void log_info(std::string_view message) const;
        void log_error(std::string_view message) const;
        void log_verbose(std::string_view message) const;
        
        ArgumentParser parser_;
        bool verbose_mode_{false};
        
    private:
        std::string name_;
    };
}
```

## 🔄 **Migration Strategy**

### **Phase 0.1: Foundation (Week 1-2)**
1. **Create core math classes** (Vector3, constants)
2. **Implement CelestialBody class** with RAII
3. **Update CMake for C++20**
4. **Create basic unit tests**

### **Phase 0.2: Simulation Core (Week 3-4)**
1. **Implement SimulationEngine class**
2. **Create BodyCollection management**
3. **Migrate physics calculations**
4. **Ensure all existing tests pass**

### **Phase 0.3: Data Management (Week 5-6)**
1. **Implement JPLDataManager with async support**
2. **Create modern caching system**
3. **Add error handling with Utils::Expected**
4. **Migrate existing data fetching logic**

### **Phase 0.4: Applications (Week 7-8)**
1. **Create Application base class**
2. **Implement modern ArgumentParser**
3. **Migrate all 5 applications**
4. **Update web server with modern HTTP handling**

## 🧪 **Testing Strategy**

### **Unit Testing**
- Each class has comprehensive unit tests
- Mock objects for external dependencies
- Property-based testing for mathematical operations
- Concept validation tests

### **Integration Testing**
- End-to-end simulation tests
- JPL data fetching integration tests
- Web interface integration tests
- Performance regression tests

### **Migration Validation**
- Bit-for-bit output comparison with original code
- Performance benchmarking
- Memory usage analysis
- Thread safety validation

## 📊 **Expected Benefits**

### **Code Quality**
- **50% reduction** in lines of code through modern abstractions
- **Zero memory leaks** through RAII
- **Compile-time error detection** through concepts
- **Self-documenting code** through strong typing

### **Performance**
- **Zero-cost abstractions** - no runtime overhead
- **Better optimization** through modern compiler features
- **Reduced memory allocations** through move semantics
- **Improved cache locality** through better data structures

### **Maintainability**
- **Clear separation of concerns** through class design
- **Easy unit testing** through dependency injection
- **Extensible architecture** for future features
- **Modern C++ idioms** familiar to developers

This architecture provides a solid foundation for all future development phases while maintaining backward compatibility and improving code quality significantly.
