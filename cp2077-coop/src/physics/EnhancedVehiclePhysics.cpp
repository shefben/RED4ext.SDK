#include "EnhancedVehiclePhysics.hpp"
#include "../core/Logger.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace CoopNet {

// EnhancedVehicleState Implementation

EnhancedVehicleState::EnhancedVehicleState() {
    vehicleId = 0;
    ownerId = 0;
    position = {0.0f, 0.0f, 0.0f};
    velocity = {0.0f, 0.0f, 0.0f};
    rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    angularVelocity = {0.0f, 0.0f, 0.0f};

    // Initialize tire positions for a standard car layout
    tires[0].position = {properties.trackWidth * 0.5f, properties.wheelbase * 0.5f, 0.0f};    // Front Left
    tires[1].position = {-properties.trackWidth * 0.5f, properties.wheelbase * 0.5f, 0.0f};   // Front Right
    tires[2].position = {properties.trackWidth * 0.5f, -properties.wheelbase * 0.5f, 0.0f};   // Rear Left
    tires[3].position = {-properties.trackWidth * 0.5f, -properties.wheelbase * 0.5f, 0.0f};  // Rear Right

    // Initialize suspension
    for (auto& susp : suspension) {
        susp.currentLength = susp.restLength;
    }

    lastUpdate = GameClock::GetCurrentTick();
}

TransformSnap EnhancedVehicleState::ToTransformSnap() const {
    TransformSnap snap;
    snap.pos = position;
    snap.vel = velocity;
    snap.rot = rotation;
    return snap;
}

void EnhancedVehicleState::FromTransformSnap(const TransformSnap& snap) {
    position = snap.pos;
    velocity = snap.vel;
    rotation = snap.rot;
    isDirty = true;
}

// EnhancedVehiclePhysics Implementation

EnhancedVehiclePhysics& EnhancedVehiclePhysics::Instance() {
    static EnhancedVehiclePhysics instance;
    return instance;
}

bool EnhancedVehiclePhysics::Initialize() {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    if (m_initialized) {
        return true;
    }

    m_vehicles.clear();
    m_stats = {};
    m_lastStatsUpdate = std::chrono::steady_clock::now();
    m_accumulatedTime = 0.0f;

    m_initialized = true;
    Logger::Log(LogLevel::INFO, "[EnhancedVehiclePhysics] System initialized successfully");
    return true;
}

void EnhancedVehiclePhysics::Shutdown() {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    if (!m_initialized) {
        return;
    }

    m_vehicles.clear();
    m_initialized = false;
    Logger::Log(LogLevel::INFO, "[EnhancedVehiclePhysics] System shutdown complete");
}

bool EnhancedVehiclePhysics::CreateVehicle(uint32_t vehicleId, uint32_t ownerId, const VehicleProperties& properties) {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    if (!m_initialized) {
        Logger::Log(LogLevel::ERROR, "[EnhancedVehiclePhysics] System not initialized");
        return false;
    }

    if (m_vehicles.find(vehicleId) != m_vehicles.end()) {
        Logger::Log(LogLevel::WARNING, "[EnhancedVehiclePhysics] Vehicle " + std::to_string(vehicleId) + " already exists");
        return false;
    }

    auto vehicle = std::make_unique<EnhancedVehicleState>();
    vehicle->vehicleId = vehicleId;
    vehicle->ownerId = ownerId;
    vehicle->properties = properties;

    // Adjust initial state based on vehicle type
    switch (properties.type) {
    case VehicleProperties::VehicleType::Motorcycle:
        vehicle->properties.mass = 200.0f;
        vehicle->properties.trackWidth = 0.0f; // No track width for motorcycles
        vehicle->tires[0].gripCoefficient = 1.2f; // Better grip for motorcycles
        vehicle->tires[1].gripCoefficient = 1.2f;
        break;

    case VehicleProperties::VehicleType::Truck:
        vehicle->properties.mass = 8000.0f;
        vehicle->properties.wheelbase = 4.0f;
        vehicle->properties.trackWidth = 2.0f;
        vehicle->engine.maxTorque = 1200.0f;
        break;

    case VehicleProperties::VehicleType::Tank:
        vehicle->properties.mass = 50000.0f;
        vehicle->properties.hasABS = false;
        vehicle->properties.hasTCS = false;
        vehicle->properties.hasESC = false;
        for (auto& tire : vehicle->tires) {
            tire.gripCoefficient = 0.8f; // Tank treads
        }
        break;

    default: // Car
        break;
    }

    m_vehicles[vehicleId] = std::move(vehicle);
    Logger::Log(LogLevel::DEBUG, "[EnhancedVehiclePhysics] Created vehicle " + std::to_string(vehicleId));
    return true;
}

bool EnhancedVehiclePhysics::DestroyVehicle(uint32_t vehicleId) {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    auto it = m_vehicles.find(vehicleId);
    if (it == m_vehicles.end()) {
        return false;
    }

    m_vehicles.erase(it);
    Logger::Log(LogLevel::DEBUG, "[EnhancedVehiclePhysics] Destroyed vehicle " + std::to_string(vehicleId));
    return true;
}

EnhancedVehicleState* EnhancedVehiclePhysics::GetVehicle(uint32_t vehicleId) {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    auto it = m_vehicles.find(vehicleId);
    return (it != m_vehicles.end()) ? it->second.get() : nullptr;
}

void EnhancedVehiclePhysics::StepSimulation(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    m_accumulatedTime += deltaTime;

    // Use fixed timestep for deterministic physics
    while (m_accumulatedTime >= m_fixedTimeStep) {
        std::lock_guard<std::mutex> lock(m_vehiclesMutex);

        for (auto& [id, vehicle] : m_vehicles) {
            if (vehicle) {
                StepVehicle(id, m_fixedTimeStep, true); // Server is authoritative
            }
        }

        m_accumulatedTime -= m_fixedTimeStep;
    }

    // Update statistics
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastStatsUpdate).count() >= 1) {
        UpdateStatistics();
        m_lastStatsUpdate = now;
    }
}

void EnhancedVehiclePhysics::StepVehicle(uint32_t vehicleId, float deltaTime, bool authoritative) {
    auto vehicle = GetVehicle(vehicleId);
    if (!vehicle) {
        return;
    }

    // Simulate subsystems
    SimulateEngine(vehicle->engine, vehicle->transmission, deltaTime);
    SimulateTransmission(vehicle->transmission, vehicle->engine, deltaTime);
    SimulateSuspension(*vehicle, deltaTime);
    SimulateTires(*vehicle, deltaTime);
    SimulateAerodynamics(*vehicle, deltaTime);

    // Apply safety systems
    if (vehicle->properties.hasABS) {
        ApplyABS(*vehicle, deltaTime);
    }
    if (vehicle->properties.hasTCS) {
        ApplyTCS(*vehicle, deltaTime);
    }
    if (vehicle->properties.hasESC) {
        ApplyESC(*vehicle, deltaTime);
    }

    // Integrate motion
    IntegrateMotion(*vehicle, deltaTime);

    // Mark as dirty for network sync
    vehicle->isDirty = true;
    vehicle->lastUpdate = GameClock::GetCurrentTick();
}

void EnhancedVehiclePhysics::SetVehicleInput(uint32_t vehicleId, float steer, float throttle, float brake, float handbrake) {
    auto vehicle = GetVehicle(vehicleId);
    if (!vehicle) {
        return;
    }

    vehicle->steerInput = std::clamp(steer, -1.0f, 1.0f);
    vehicle->throttleInput = std::clamp(throttle, 0.0f, 1.0f);
    vehicle->brakeInput = std::clamp(brake, 0.0f, 1.0f);
    vehicle->handbrakeInput = std::clamp(handbrake, 0.0f, 1.0f);

    vehicle->engine.throttleInput = vehicle->throttleInput;
    vehicle->engine.brakeInput = vehicle->brakeInput;
}

void EnhancedVehiclePhysics::SimulateEngine(VehicleEngine& engine, VehicleTransmission& transmission, float deltaTime) {
    if (!engine.isRunning) {
        engine.currentRPM = std::max(0.0f, engine.currentRPM - 1000.0f * deltaTime);
        return;
    }

    // Calculate target RPM based on throttle
    float targetRPM = engine.idleRPM + (engine.maxRPM - engine.idleRPM) * engine.throttleInput;

    // Apply engine braking when throttle is released
    if (engine.throttleInput < 0.1f) {
        targetRPM = std::max(engine.idleRPM, targetRPM);
    }

    // Smooth RPM transition
    float rpmDelta = (targetRPM - engine.currentRPM) * 5.0f * deltaTime;
    engine.currentRPM = std::clamp(engine.currentRPM + rpmDelta, 0.0f, engine.maxRPM);

    // Calculate current torque using power band
    float rpmNormalized = engine.currentRPM / engine.maxRPM;
    int powerIndex = static_cast<int>(rpmNormalized * 9.0f);
    powerIndex = std::clamp(powerIndex, 0, 9);

    float torqueMultiplier = VehiclePhysicsUtils::InterpolateArray(engine.powerBand, 10, rpmNormalized * 9.0f);
    float currentTorque = engine.maxTorque * torqueMultiplier * engine.throttleInput;

    // Apply engine damage
    // currentTorque *= (1.0f - engineDamage * 0.8f);
}

void EnhancedVehiclePhysics::SimulateTransmission(VehicleTransmission& transmission, VehicleEngine& engine, float deltaTime) {
    if (!transmission.isAutomatic) {
        return; // Manual transmission handled by player input
    }

    // Automatic transmission logic
    float rpmRatio = engine.currentRPM / engine.maxRPM;

    // Upshift logic
    if (rpmRatio > transmission.shiftThreshold && transmission.currentGear < 7) {
        transmission.currentGear++;
        Logger::Log(LogLevel::DEBUG, "[EnhancedVehiclePhysics] Upshifted to gear " + std::to_string(transmission.currentGear));
    }
    // Downshift logic
    else if (rpmRatio < 0.3f && transmission.currentGear > 1) {
        transmission.currentGear--;
        Logger::Log(LogLevel::DEBUG, "[EnhancedVehiclePhysics] Downshifted to gear " + std::to_string(transmission.currentGear));
    }
}

void EnhancedVehiclePhysics::SimulateTires(EnhancedVehicleState& vehicle, float deltaTime) {
    for (size_t i = 0; i < 4; ++i) {
        auto& tire = vehicle.tires[i];
        auto& susp = vehicle.suspension[i];

        // Calculate tire load from suspension
        float load = vehicle.properties.mass * 9.81f * 0.25f; // Assume even weight distribution
        load += susp.springConstant * susp.compression;

        // Calculate slip angle and ratio
        RED4ext::Vector3 tireVelocity = tire.velocity;
        float slipAngle = CalculateSlipAngle(tireVelocity, {1.0f, 0.0f, 0.0f}); // Simplified
        float slipRatio = CalculateSlipRatio(0.0f, tireVelocity.X); // Simplified

        tire.slipAngle = slipAngle;
        tire.slipRatio = slipRatio;

        // Calculate tire forces using simplified Pacejka model
        float gripCoeff = GetTireGrip(tire, vehicle.temperature);
        tire.force = CalculateTireForces(tire, load, gripCoeff * vehicle.groundFriction);

        // Update tire temperature based on slip
        float slipEnergy = std::abs(slipAngle) + std::abs(slipRatio);
        tire.temperature += slipEnergy * 10.0f * deltaTime;
        tire.temperature = std::max(tire.temperature - 5.0f * deltaTime, vehicle.temperature); // Cool down
    }
}

void EnhancedVehiclePhysics::SimulateSuspension(EnhancedVehicleState& vehicle, float deltaTime) {
    for (size_t i = 0; i < 4; ++i) {
        auto& susp = vehicle.suspension[i];

        // Simple ground detection (in real implementation would raycast)
        float groundHeight = 0.0f;
        float tireBottom = vehicle.position.Z - vehicle.tires[i].wheelRadius;
        float targetLength = std::max(0.0f, tireBottom - groundHeight);

        // Calculate spring and damper forces
        float compression = susp.restLength - targetLength;
        compression = std::clamp(compression, -susp.maxExtension, susp.maxCompression);

        float springForce = susp.springConstant * compression;
        float damperForce = susp.dampingConstant * susp.velocity;

        float totalForce = springForce + damperForce;

        // Update suspension state
        float acceleration = totalForce / vehicle.properties.mass;
        susp.velocity += acceleration * deltaTime;
        susp.currentLength += susp.velocity * deltaTime;
        susp.compression = (susp.restLength - susp.currentLength) / susp.restLength;

        // Update tire ground contact
        vehicle.tires[i].isGrounded = (compression > -0.01f);
    }
}

void EnhancedVehiclePhysics::SimulateAerodynamics(EnhancedVehicleState& vehicle, float deltaTime) {
    float speed = sqrtf(vehicle.velocity.X * vehicle.velocity.X + vehicle.velocity.Y * vehicle.velocity.Y);

    // Calculate drag force
    float dragForce = VehiclePhysicsUtils::CalculateDrag(speed, vehicle.aerodynamics.dragCoefficient,
                                                        vehicle.aerodynamics.frontalArea, vehicle.aerodynamics.airDensity);

    // Calculate downforce
    float downforce = VehiclePhysicsUtils::CalculateDownforce(speed, vehicle.aerodynamics.downforceCoefficient,
                                                             vehicle.aerodynamics.frontalArea, vehicle.aerodynamics.airDensity);

    // Apply forces (simplified - should be applied at center of pressure)
    if (speed > 0.1f) {
        RED4ext::Vector3 dragDirection = {-vehicle.velocity.X / speed, -vehicle.velocity.Y / speed, 0.0f};
        RED4ext::Vector3 dragForceVector = {dragDirection.X * dragForce, dragDirection.Y * dragForce, -downforce};

        // Apply to vehicle (would be integrated in IntegrateMotion)
    }
}

void EnhancedVehiclePhysics::IntegrateMotion(EnhancedVehicleState& vehicle, float deltaTime) {
    // Collect all forces acting on the vehicle
    RED4ext::Vector3 totalForce = {0.0f, 0.0f, 0.0f};
    RED4ext::Vector3 totalTorque = {0.0f, 0.0f, 0.0f};

    // Add tire forces
    for (const auto& tire : vehicle.tires) {
        if (tire.isGrounded) {
            totalForce.X += tire.force.X;
            totalForce.Y += tire.force.Y;
            totalForce.Z += tire.force.Z;

            // Add torque from tire forces (simplified)
            RED4ext::Vector3 leverArm = tire.position;
            totalTorque.Z += leverArm.X * tire.force.Y - leverArm.Y * tire.force.X;
        }
    }

    // Add gravity
    totalForce.Z -= vehicle.properties.mass * 9.81f;

    // Integrate linear motion
    RED4ext::Vector3 acceleration = {totalForce.X / vehicle.properties.mass,
                                    totalForce.Y / vehicle.properties.mass,
                                    totalForce.Z / vehicle.properties.mass};

    vehicle.velocity.X += acceleration.X * deltaTime;
    vehicle.velocity.Y += acceleration.Y * deltaTime;
    vehicle.velocity.Z += acceleration.Z * deltaTime;

    vehicle.position.X += vehicle.velocity.X * deltaTime;
    vehicle.position.Y += vehicle.velocity.Y * deltaTime;
    vehicle.position.Z += vehicle.velocity.Z * deltaTime;

    // Integrate angular motion (simplified)
    RED4ext::Vector3 angularAcceleration = {totalTorque.X / vehicle.properties.inertiaXX,
                                           totalTorque.Y / vehicle.properties.inertiaYY,
                                           totalTorque.Z / vehicle.properties.inertiaZZ};

    vehicle.angularVelocity.X += angularAcceleration.X * deltaTime;
    vehicle.angularVelocity.Y += angularAcceleration.Y * deltaTime;
    vehicle.angularVelocity.Z += angularAcceleration.Z * deltaTime;

    // Update rotation (simplified quaternion integration)
    float angularSpeed = sqrtf(vehicle.angularVelocity.X * vehicle.angularVelocity.X +
                              vehicle.angularVelocity.Y * vehicle.angularVelocity.Y +
                              vehicle.angularVelocity.Z * vehicle.angularVelocity.Z);

    if (angularSpeed > 0.001f) {
        float angle = angularSpeed * deltaTime;
        RED4ext::Vector3 axis = {vehicle.angularVelocity.X / angularSpeed,
                                vehicle.angularVelocity.Y / angularSpeed,
                                vehicle.angularVelocity.Z / angularSpeed};

        float s = sinf(angle * 0.5f);
        float c = cosf(angle * 0.5f);
        RED4ext::Quaternion deltaRotation = {axis.X * s, axis.Y * s, axis.Z * s, c};

        // Multiply quaternions (simplified)
        vehicle.rotation = deltaRotation; // Should be proper quaternion multiplication
    }
}

void EnhancedVehiclePhysics::ApplyABS(EnhancedVehicleState& vehicle, float deltaTime) {
    if (vehicle.brakeInput < 0.1f) {
        return; // ABS only active during braking
    }

    for (auto& tire : vehicle.tires) {
        if (std::abs(tire.slipRatio) > 0.1f) { // Wheel is locking
            // Reduce brake force to prevent lockup
            tire.force.X *= 0.8f;
            tire.force.Y *= 0.8f;
        }
    }
}

void EnhancedVehiclePhysics::ApplyTCS(EnhancedVehicleState& vehicle, float deltaTime) {
    if (vehicle.throttleInput < 0.1f) {
        return; // TCS only active during acceleration
    }

    // Check rear wheels for excessive slip (simplified)
    for (size_t i = 2; i < 4; ++i) { // Rear wheels
        if (std::abs(vehicle.tires[i].slipRatio) > 0.15f) {
            // Reduce engine power
            vehicle.engine.throttleInput *= 0.9f;
            break;
        }
    }
}

void EnhancedVehiclePhysics::ApplyESC(EnhancedVehicleState& vehicle, float deltaTime) {
    // Simplified ESC: detect oversteer/understeer and apply corrective measures
    float yawRate = vehicle.angularVelocity.Z;
    float steerAngle = vehicle.steerInput * 0.5f; // Max steering angle

    // Calculate expected yaw rate vs actual
    float expectedYawRate = (vehicle.velocity.X * tanf(steerAngle)) / vehicle.properties.wheelbase;
    float yawError = yawRate - expectedYawRate;

    if (std::abs(yawError) > 0.2f) { // Vehicle is sliding
        // Apply corrective braking (simplified)
        if (yawError > 0) { // Oversteer
            vehicle.tires[1].force.X *= 0.8f; // Brake outside front wheel
        } else { // Understeer
            vehicle.tires[2].force.X *= 0.8f; // Brake inside rear wheel
        }
    }
}

// Helper function implementations

RED4ext::Vector3 EnhancedVehiclePhysics::CalculateTireForces(const VehicleTire& tire, float load, float friction) {
    // Simplified tire force calculation
    float maxForce = load * friction;

    RED4ext::Vector3 force = {0.0f, 0.0f, 0.0f};

    // Longitudinal force (based on slip ratio)
    force.X = VehiclePhysicsUtils::PacejkaTireModel(tire.slipRatio, load, friction);

    // Lateral force (based on slip angle)
    force.Y = VehiclePhysicsUtils::PacejkaTireModel(tire.slipAngle, load, friction);

    return force;
}

float EnhancedVehiclePhysics::CalculateSlipAngle(const RED4ext::Vector3& velocity, const RED4ext::Vector3& direction) {
    if (velocity.X == 0.0f && velocity.Y == 0.0f) {
        return 0.0f;
    }

    float velocityAngle = atan2f(velocity.Y, velocity.X);
    float directionAngle = atan2f(direction.Y, direction.X);

    return velocityAngle - directionAngle;
}

float EnhancedVehiclePhysics::CalculateSlipRatio(float wheelSpeed, float vehicleSpeed) {
    if (vehicleSpeed == 0.0f) {
        return 0.0f;
    }

    return (wheelSpeed - vehicleSpeed) / std::abs(vehicleSpeed);
}

float EnhancedVehiclePhysics::GetTireGrip(const VehicleTire& tire, float temperature) {
    // Temperature affects grip (simplified model)
    float optimalTemp = 80.0f; // Optimal tire temperature
    float tempFactor = 1.0f - std::abs(tire.temperature - optimalTemp) / 100.0f;
    tempFactor = std::clamp(tempFactor, 0.5f, 1.2f);

    return tire.gripCoefficient * tempFactor;
}

std::vector<uint32_t> EnhancedVehiclePhysics::GetDirtyVehicles() const {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    std::vector<uint32_t> dirty;
    for (const auto& [id, vehicle] : m_vehicles) {
        if (vehicle && vehicle->isDirty) {
            dirty.push_back(id);
        }
    }
    return dirty;
}

void EnhancedVehiclePhysics::MarkClean(uint32_t vehicleId) {
    auto vehicle = GetVehicle(vehicleId);
    if (vehicle) {
        vehicle->isDirty = false;
    }
}

void EnhancedVehiclePhysics::UpdateStatistics() {
    std::lock_guard<std::mutex> lock(m_vehiclesMutex);

    m_stats.totalVehicles = static_cast<uint32_t>(m_vehicles.size());
    m_stats.activeVehicles = 0;

    for (const auto& vehiclePair : m_vehicles) {
        const auto& vehicle = vehiclePair.second;
        if (vehicle && (vehicle->velocity.X != 0.0f || vehicle->velocity.Y != 0.0f || vehicle->velocity.Z != 0.0f)) {
            m_stats.activeVehicles++;
        }
    }

    // Calculate average simulation time (simplified)
    m_stats.averageSimulationTime = m_fixedTimeStep * 1000.0f; // Convert to milliseconds
}

EnhancedVehiclePhysics::PhysicsStats EnhancedVehiclePhysics::GetStatistics() const {
    return m_stats;
}

// VehiclePhysicsUtils Implementation

namespace VehiclePhysicsUtils {

float CalculateDownforce(float speed, float coefficient, float area, float airDensity) {
    return 0.5f * airDensity * speed * speed * coefficient * area;
}

float CalculateDrag(float speed, float coefficient, float area, float airDensity) {
    return 0.5f * airDensity * speed * speed * coefficient * area;
}

float PacejkaTireModel(float slip, float load, float friction) {
    // Simplified Pacejka "Magic Formula" tire model
    float B = 10.0f;  // Stiffness factor
    float C = 1.65f;  // Shape factor
    float D = load * friction; // Peak factor
    float E = -0.97f; // Curvature factor

    float x = B * slip;
    return D * sinf(C * atanf(x - E * (x - atanf(x))));
}

float BrushTireModel(float slipAngle, float slipRatio, float load) {
    // Simplified brush tire model
    float combinedSlip = sqrtf(slipAngle * slipAngle + slipRatio * slipRatio);
    float maxForce = load * 1.0f; // Simplified friction coefficient

    if (combinedSlip < 0.1f) {
        return maxForce * combinedSlip / 0.1f;
    } else {
        return maxForce * (1.0f - (combinedSlip - 0.1f) * 0.5f);
    }
}

float InterpolateArray(const float* array, size_t size, float index) {
    if (index <= 0.0f) return array[0];
    if (index >= static_cast<float>(size - 1)) return array[size - 1];

    size_t i = static_cast<size_t>(index);
    size_t j = i + 1;
    float t = index - static_cast<float>(i);

    return Lerp(array[i], array[j], t);
}

float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

RED4ext::Vector3 Lerp(const RED4ext::Vector3& a, const RED4ext::Vector3& b, float t) {
    return {
        Lerp(a.X, b.X, t),
        Lerp(a.Y, b.Y, t),
        Lerp(a.Z, b.Z, t)
    };
}

RED4ext::Quaternion Slerp(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b, float t) {
    // Simplified quaternion SLERP
    return {
        Lerp(a.i, b.i, t),
        Lerp(a.j, b.j, t),
        Lerp(a.k, b.k, t),
        Lerp(a.r, b.r, t)
    };
}

} // namespace VehiclePhysicsUtils

} // namespace CoopNet