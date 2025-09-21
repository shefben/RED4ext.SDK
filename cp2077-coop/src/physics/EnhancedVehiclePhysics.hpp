#pragma once

#include "../net/Snapshot.hpp"
#include <RED4ext/RED4ext.hpp>
#include <array>
#include <memory>

namespace CoopNet {

// Enhanced vehicle physics data structures
struct VehicleTire {
    RED4ext::Vector3 position;      // Tire position relative to vehicle center
    RED4ext::Vector3 velocity;      // Current tire velocity
    RED4ext::Vector3 force;         // Forces acting on this tire
    float wheelRadius = 0.35f;      // Wheel radius in meters
    float gripCoefficient = 1.0f;   // Tire grip (0-2, 1 = normal)
    float slipAngle = 0.0f;         // Current slip angle in radians
    float slipRatio = 0.0f;         // Longitudinal slip ratio
    float temperature = 20.0f;      // Tire temperature (affects grip)
    bool isGrounded = true;         // Is tire touching ground
    float compression = 0.0f;       // Suspension compression (0-1)
};

struct VehicleSuspension {
    float springConstant = 50000.0f;     // Spring stiffness (N/m)
    float dampingConstant = 3000.0f;     // Damping coefficient (Ns/m)
    float restLength = 0.3f;             // Suspension rest length
    float maxCompression = 0.15f;        // Maximum compression
    float maxExtension = 0.1f;           // Maximum extension
    float currentLength = 0.3f;          // Current suspension length
    float velocity = 0.0f;               // Suspension velocity
    float compression = 0.0f;            // Current compression ratio (0-1)
};

struct VehicleEngine {
    float maxTorque = 400.0f;           // Maximum engine torque (Nm)
    float maxRPM = 8000.0f;             // Maximum RPM
    float currentRPM = 800.0f;          // Current RPM
    float throttleInput = 0.0f;         // Throttle input (0-1)
    float brakeInput = 0.0f;            // Brake input (0-1)
    float idleRPM = 800.0f;             // Idle RPM
    float powerBand[10] = {             // Power curve (torque multipliers at different RPM ranges)
        0.3f, 0.5f, 0.7f, 0.85f, 1.0f, 0.95f, 0.8f, 0.6f, 0.4f, 0.2f
    };
    bool isRunning = true;              // Engine state
};

struct VehicleTransmission {
    std::array<float, 8> gearRatios = {
        -3.0f,  // Reverse
        0.0f,   // Neutral
        3.8f,   // 1st gear
        2.2f,   // 2nd gear
        1.5f,   // 3rd gear
        1.1f,   // 4th gear
        0.9f,   // 5th gear
        0.7f    // 6th gear
    };
    int currentGear = 1;                // Current gear (-1=reverse, 0=neutral, 1+=forward)
    float finalDriveRatio = 4.1f;       // Final drive ratio
    float clutchEngagement = 1.0f;      // Clutch engagement (0-1)
    bool isAutomatic = true;            // Automatic transmission
    float shiftThreshold = 0.8f;        // Auto shift threshold (0-1 of max RPM)
};

struct VehicleAerodynamics {
    float dragCoefficient = 0.35f;      // Aerodynamic drag coefficient
    float downforceCoefficient = 0.1f;  // Downforce coefficient
    float frontalArea = 2.2f;           // Frontal area in square meters
    float airDensity = 1.225f;          // Air density (kg/m³)
    RED4ext::Vector3 centerOfPressure = {0.0f, 0.0f, 0.5f}; // Center of aerodynamic pressure
};

struct VehicleProperties {
    float mass = 1500.0f;               // Vehicle mass (kg)
    float wheelbase = 2.7f;             // Distance between front and rear axles
    float trackWidth = 1.6f;            // Distance between left and right wheels
    float centerOfMassHeight = 0.5f;    // Height of center of mass from ground
    RED4ext::Vector3 centerOfMass = {0.0f, 0.0f, 0.5f}; // Center of mass offset

    // Inertia tensor components (kg⋅m²)
    float inertiaXX = 2500.0f;          // Roll inertia
    float inertiaYY = 4000.0f;          // Pitch inertia
    float inertiaZZ = 4200.0f;          // Yaw inertia

    // Vehicle type specific properties
    enum class VehicleType {
        Car,
        Motorcycle,
        Truck,
        Tank,
        Aircraft
    } type = VehicleType::Car;

    bool hasABS = true;                 // Anti-lock braking system
    bool hasTCS = true;                 // Traction control system
    bool hasESC = true;                 // Electronic stability control
};

// Main enhanced vehicle physics state
struct EnhancedVehicleState {
    uint32_t vehicleId;
    uint32_t ownerId;

    // Transform state (compatible with existing TransformSnap)
    RED4ext::Vector3 position;
    RED4ext::Vector3 velocity;
    RED4ext::Quaternion rotation;
    RED4ext::Vector3 angularVelocity;

    // Enhanced physics components
    std::array<VehicleTire, 4> tires;           // FL, FR, RL, RR
    std::array<VehicleSuspension, 4> suspension;
    VehicleEngine engine;
    VehicleTransmission transmission;
    VehicleAerodynamics aerodynamics;
    VehicleProperties properties;

    // Input state
    float steerInput = 0.0f;            // Steering input (-1 to 1)
    float throttleInput = 0.0f;         // Throttle input (0 to 1)
    float brakeInput = 0.0f;            // Brake input (0 to 1)
    float handbrakeInput = 0.0f;        // Handbrake input (0 to 1)

    // Environmental factors
    float groundFriction = 1.0f;        // Current ground friction coefficient
    float temperature = 20.0f;         // Ambient temperature (affects tire grip)
    bool isOnGround = true;             // Is vehicle on ground

    // Damage state
    float engineDamage = 0.0f;          // Engine damage (0-1)
    float transmissionDamage = 0.0f;    // Transmission damage (0-1)
    std::array<float, 4> tireDamage = {0.0f, 0.0f, 0.0f, 0.0f}; // Tire damage per wheel
    std::array<float, 4> suspensionDamage = {0.0f, 0.0f, 0.0f, 0.0f}; // Suspension damage

    // Network synchronization
    uint64_t lastUpdate = 0;            // Last update timestamp
    uint32_t networkFrame = 0;          // Network frame counter
    bool isDirty = true;                // Needs network update

    // Constructor with default initialization
    EnhancedVehicleState();

    // Convert to/from legacy TransformSnap for compatibility
    TransformSnap ToTransformSnap() const;
    void FromTransformSnap(const TransformSnap& snap);
};

class EnhancedVehiclePhysics {
public:
    static EnhancedVehiclePhysics& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();

    // Vehicle management
    bool CreateVehicle(uint32_t vehicleId, uint32_t ownerId, const VehicleProperties& properties);
    bool DestroyVehicle(uint32_t vehicleId);
    EnhancedVehicleState* GetVehicle(uint32_t vehicleId);

    // Physics simulation
    void StepSimulation(float deltaTime);
    void StepVehicle(uint32_t vehicleId, float deltaTime, bool authoritative);

    // Input handling
    void SetVehicleInput(uint32_t vehicleId, float steer, float throttle, float brake, float handbrake);
    void SetEngineState(uint32_t vehicleId, bool running);
    void ShiftGear(uint32_t vehicleId, int gear);

    // Environmental factors
    void SetGroundFriction(uint32_t vehicleId, float friction);
    void SetAmbientTemperature(float temperature);

    // Damage system
    void ApplyDamage(uint32_t vehicleId, float engineDamage, float transmissionDamage,
                     const std::array<float, 4>& tireDamage, const std::array<float, 4>& suspensionDamage);

    // Collision handling
    void HandleCollision(uint32_t vehicleA, uint32_t vehicleB, const RED4ext::Vector3& contactPoint,
                         const RED4ext::Vector3& normal, float impulse);

    // Network synchronization
    std::vector<uint32_t> GetDirtyVehicles() const;
    void MarkClean(uint32_t vehicleId);

    // Advanced features
    void EnableABS(uint32_t vehicleId, bool enable);
    void EnableTCS(uint32_t vehicleId, bool enable);
    void EnableESC(uint32_t vehicleId, bool enable);

    // Performance monitoring
    struct PhysicsStats {
        uint32_t totalVehicles;
        uint32_t activeVehicles;
        float averageSimulationTime;
        uint32_t collisionsPerSecond;
        float networkBandwidth;
    };

    PhysicsStats GetStatistics() const;

private:
    // Internal utility methods
    void UpdateStatistics();
    EnhancedVehiclePhysics() = default;
    ~EnhancedVehiclePhysics() = default;
    EnhancedVehiclePhysics(const EnhancedVehiclePhysics&) = delete;
    EnhancedVehiclePhysics& operator=(const EnhancedVehiclePhysics&) = delete;

    // Internal simulation methods
    void SimulateEngine(VehicleEngine& engine, VehicleTransmission& transmission, float deltaTime);
    void SimulateTransmission(VehicleTransmission& transmission, VehicleEngine& engine, float deltaTime);
    void SimulateTires(EnhancedVehicleState& vehicle, float deltaTime);
    void SimulateSuspension(EnhancedVehicleState& vehicle, float deltaTime);
    void SimulateAerodynamics(EnhancedVehicleState& vehicle, float deltaTime);
    void IntegrateMotion(EnhancedVehicleState& vehicle, float deltaTime);

    // Safety systems
    void ApplyABS(EnhancedVehicleState& vehicle, float deltaTime);
    void ApplyTCS(EnhancedVehicleState& vehicle, float deltaTime);
    void ApplyESC(EnhancedVehicleState& vehicle, float deltaTime);

    // Helper methods
    RED4ext::Vector3 CalculateTireForces(const VehicleTire& tire, float load, float friction);
    float CalculateSlipAngle(const RED4ext::Vector3& velocity, const RED4ext::Vector3& direction);
    float CalculateSlipRatio(float wheelSpeed, float vehicleSpeed);
    float GetTireGrip(const VehicleTire& tire, float temperature);

    // Network optimization
    bool ShouldSynchronize(const EnhancedVehicleState& vehicle) const;
    void CompressStateForNetwork(const EnhancedVehicleState& vehicle, uint8_t* buffer, size_t& size);

    // Data storage
    std::unordered_map<uint32_t, std::unique_ptr<EnhancedVehicleState>> m_vehicles;
    mutable std::mutex m_vehiclesMutex;

    // Physics configuration
    float m_fixedTimeStep = 1.0f / 120.0f; // 120 Hz physics
    float m_accumulatedTime = 0.0f;
    bool m_initialized = false;

    // Performance tracking
    mutable PhysicsStats m_stats;
    std::chrono::steady_clock::time_point m_lastStatsUpdate;

    // Environmental state
    float m_globalTemperature = 20.0f;
    float m_globalFriction = 1.0f;
};

// Utility functions for vehicle physics calculations
namespace VehiclePhysicsUtils {
    // Convert between coordinate systems
    RED4ext::Vector3 WorldToLocal(const RED4ext::Vector3& worldPos, const RED4ext::Vector3& vehiclePos,
                                  const RED4ext::Quaternion& vehicleRot);
    RED4ext::Vector3 LocalToWorld(const RED4ext::Vector3& localPos, const RED4ext::Vector3& vehiclePos,
                                  const RED4ext::Quaternion& vehicleRot);

    // Physics helpers
    float CalculateDownforce(float speed, float coefficient, float area, float airDensity);
    float CalculateDrag(float speed, float coefficient, float area, float airDensity);
    RED4ext::Vector3 CalculateRollingResistance(const RED4ext::Vector3& velocity, float coefficient, float mass);

    // Tire model functions
    float PacejkaTireModel(float slip, float load, float friction);
    float BrushTireModel(float slipAngle, float slipRatio, float load);

    // Interpolation utilities
    float InterpolateArray(const float* array, size_t size, float index);
    float Lerp(float a, float b, float t);
    RED4ext::Vector3 Lerp(const RED4ext::Vector3& a, const RED4ext::Vector3& b, float t);
    RED4ext::Quaternion Slerp(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b, float t);
}

} // namespace CoopNet