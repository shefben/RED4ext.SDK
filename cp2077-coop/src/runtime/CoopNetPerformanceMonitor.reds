// CoopNet Performance Monitoring System - REDscript interface for performance tracking

// Performance monitoring and optimization system
public class CoopNetPerformanceMonitor extends IScriptable {

    // === MONITORING CONTROL ===

    // Check if performance monitoring is enabled
    public native func IsEnabled() -> Bool;

    // Enable/disable performance monitoring
    public native func SetEnabled(enabled: Bool) -> Bool;

    // Start monitoring
    public native func StartMonitoring() -> Bool;

    // Stop monitoring
    public native func StopMonitoring() -> Bool;

    // === PERFORMANCE METRICS ===

    // Get current FPS
    public native func GetCurrentFPS() -> Float;

    // Get average FPS over time
    public native func GetAverageFPS() -> Float;

    // Get frame time in milliseconds
    public native func GetFrameTime() -> Float;

    // Get CPU usage percentage (0.0 - 100.0)
    public native func GetCPUUsage() -> Float;

    // Get memory usage percentage (0.0 - 100.0)
    public native func GetMemoryUsage() -> Float;

    // Get GPU usage percentage (0.0 - 100.0)
    public native func GetGPUUsage() -> Float;

    // === PERFORMANCE OPTIMIZATION ===

    // Enable/disable adaptive quality
    public native func EnableAdaptiveQuality(enabled: Bool) -> Bool;

    // Check if adaptive quality is enabled
    public native func IsAdaptiveQualityEnabled() -> Bool;

    // Set performance target FPS
    public native func SetPerformanceTarget(targetFPS: Float) -> Bool;

    // Get performance target FPS
    public native func GetPerformanceTarget() -> Float;

    // === ALERTS AND THRESHOLDS ===

    // Set FPS threshold for alerts
    public native func SetFPSThreshold(minFPS: Float) -> Bool;

    // Set memory threshold for alerts
    public native func SetMemoryThreshold(maxMemoryPercent: Float) -> Bool;

    // Set CPU threshold for alerts
    public native func SetCPUThreshold(maxCPUPercent: Float) -> Bool;

    // Get active performance alerts
    public native func GetActiveAlerts() -> array<String>;

    // === STATISTICS ===

    // Get detailed performance report
    public native func GetPerformanceReport() -> String;

    // Reset performance statistics
    public native func ResetStatistics() -> Bool;

    // === UTILITY METHODS ===

    // Get singleton instance
    public static func GetInstance() -> ref<CoopNetPerformanceMonitor> {
        return GameInstance.GetCoopNetPerformanceMonitor();
    }

    // Initialize with default settings
    public func InitializeWithDefaults() -> Bool {
        if this.SetEnabled(true) {
            this.SetPerformanceTarget(60.0);
            this.SetFPSThreshold(30.0);
            this.SetMemoryThreshold(80.0);
            this.SetCPUThreshold(85.0);
            this.EnableAdaptiveQuality(true);
            return this.StartMonitoring();
        }
        return false;
    }

    // Get overall performance grade (A, B, C, D, F)
    public func GetPerformanceGrade() -> String {
        let fps = this.GetCurrentFPS();
        let cpuUsage = this.GetCPUUsage();
        let memoryUsage = this.GetMemoryUsage();

        let score: Float = 0.0;

        // FPS score (40% weight)
        if fps >= 60.0 {
            score += 40.0;
        } else if fps >= 45.0 {
            score += 30.0;
        } else if fps >= 30.0 {
            score += 20.0;
        } else if fps >= 20.0 {
            score += 10.0;
        }

        // CPU score (30% weight)
        if cpuUsage <= 50.0 {
            score += 30.0;
        } else if cpuUsage <= 70.0 {
            score += 20.0;
        } else if cpuUsage <= 85.0 {
            score += 10.0;
        }

        // Memory score (30% weight)
        if memoryUsage <= 60.0 {
            score += 30.0;
        } else if memoryUsage <= 75.0 {
            score += 20.0;
        } else if memoryUsage <= 85.0 {
            score += 10.0;
        }

        if score >= 90.0 {
            return "A";
        } else if score >= 80.0 {
            return "B";
        } else if score >= 70.0 {
            return "C";
        } else if score >= 60.0 {
            return "D";
        } else {
            return "F";
        }
    }

    // Check if performance is good
    public func IsPerformanceGood() -> Bool {
        let grade = this.GetPerformanceGrade();
        return Equals(grade, "A") || Equals(grade, "B");
    }

    // Check if performance is poor
    public func IsPerformancePoor() -> Bool {
        let grade = this.GetPerformanceGrade();
        return Equals(grade, "D") || Equals(grade, "F");
    }

    // Get bottleneck analysis
    public func GetBottleneckAnalysis() -> String {
        let cpu = this.GetCPUUsage();
        let memory = this.GetMemoryUsage();
        let gpu = this.GetGPUUsage();
        let fps = this.GetCurrentFPS();

        let analysis = "Performance Analysis:\n";

        if fps < 30.0 {
            analysis += "• Low FPS detected (" + ToString(fps) + ")\n";
        }

        if cpu > 85.0 {
            analysis += "• CPU bottleneck: " + ToString(cpu) + "% usage\n";
        } else if cpu > 70.0 {
            analysis += "• High CPU usage: " + ToString(cpu) + "%\n";
        }

        if memory > 85.0 {
            analysis += "• Memory bottleneck: " + ToString(memory) + "% usage\n";
        } else if memory > 70.0 {
            analysis += "• High memory usage: " + ToString(memory) + "%\n";
        }

        if gpu > 90.0 {
            analysis += "• GPU bottleneck: " + ToString(gpu) + "% usage\n";
        } else if gpu > 80.0 {
            analysis += "• High GPU usage: " + ToString(gpu) + "%\n";
        }

        if cpu < 50.0 && memory < 60.0 && gpu < 70.0 && fps >= 60.0 {
            analysis += "• Performance is optimal\n";
        }

        return analysis;
    }

    // === PERFORMANCE PRESETS ===

    // Apply performance preset for different scenarios
    public func ApplyPerformancePreset(preset: String) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if !IsDefined(manager) {
            return false;
        }

        switch preset {
            case "maximum_quality":
                this.SetPerformanceTarget(30.0);
                this.EnableAdaptiveQuality(false);
                manager.SetConfigString("graphics_preset", "ultra");
                break;

            case "balanced":
                this.SetPerformanceTarget(60.0);
                this.EnableAdaptiveQuality(true);
                manager.SetConfigString("graphics_preset", "high");
                break;

            case "maximum_performance":
                this.SetPerformanceTarget(120.0);
                this.EnableAdaptiveQuality(true);
                manager.SetConfigString("graphics_preset", "medium");
                break;

            case "battery_saver":
                this.SetPerformanceTarget(45.0);
                this.EnableAdaptiveQuality(true);
                manager.SetConfigString("graphics_preset", "low");
                break;

            default:
                return false;
        }

        return true;
    }

    // Get current performance preset
    public func GetCurrentPreset() -> String {
        let target = this.GetPerformanceTarget();
        let adaptive = this.IsAdaptiveQualityEnabled();

        if target >= 100.0 {
            return "maximum_performance";
        } else if target >= 60.0 && adaptive {
            return "balanced";
        } else if target <= 35.0 {
            return "maximum_quality";
        } else if target <= 50.0 {
            return "battery_saver";
        } else {
            return "custom";
        }
    }

    // === MONITORING HELPERS ===

    // Start performance monitoring session
    public func StartMonitoringSession(sessionName: String) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            manager.SetConfigString("performance_session_name", sessionName);
            manager.SendSimpleEvent("performance_session_start", sessionName);
            return this.StartMonitoring();
        }
        return false;
    }

    // End performance monitoring session
    public func EndMonitoringSession() -> String {
        let report = this.GetPerformanceReport();
        this.StopMonitoring();

        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            manager.SendSimpleEvent("performance_session_end", "Session completed");
        }

        return report;
    }

    // Check if system meets minimum requirements
    public func CheckMinimumRequirements() -> Bool {
        let fps = this.GetAverageFPS();
        let memory = this.GetMemoryUsage();
        let cpu = this.GetCPUUsage();

        return fps >= 20.0 && memory <= 95.0 && cpu <= 95.0;
    }

    // Get performance recommendations
    public func GetPerformanceRecommendations() -> array<String> {
        let recommendations: array<String>;
        let fps = this.GetCurrentFPS();
        let cpu = this.GetCPUUsage();
        let memory = this.GetMemoryUsage();
        let gpu = this.GetGPUUsage();

        if fps < 30.0 {
            ArrayPush(recommendations, "Consider lowering graphics settings for better FPS");
        }

        if cpu > 80.0 {
            ArrayPush(recommendations, "High CPU usage detected - close unnecessary applications");
        }

        if memory > 80.0 {
            ArrayPush(recommendations, "High memory usage - consider restarting the game");
        }

        if gpu > 85.0 {
            ArrayPush(recommendations, "GPU bottleneck - reduce graphics quality or resolution");
        }

        if !this.IsAdaptiveQualityEnabled() && fps < 45.0 {
            ArrayPush(recommendations, "Enable adaptive quality for automatic optimization");
        }

        if ArraySize(recommendations) == 0 {
            ArrayPush(recommendations, "Performance is good - no recommendations needed");
        }

        return recommendations;
    }

    // === DEBUGGING AND DIAGNOSTICS ===

    // Get detailed system status
    public func GetSystemStatus() -> String {
        let status = "Performance Monitor Status:\n";
        status += "Enabled: " + ToString(this.IsEnabled()) + "\n";
        status += "Current FPS: " + ToString(this.GetCurrentFPS()) + "\n";
        status += "Average FPS: " + ToString(this.GetAverageFPS()) + "\n";
        status += "Frame Time: " + ToString(this.GetFrameTime()) + "ms\n";
        status += "CPU Usage: " + ToString(this.GetCPUUsage()) + "%\n";
        status += "Memory Usage: " + ToString(this.GetMemoryUsage()) + "%\n";
        status += "GPU Usage: " + ToString(this.GetGPUUsage()) + "%\n";
        status += "Performance Grade: " + this.GetPerformanceGrade() + "\n";
        status += "Adaptive Quality: " + ToString(this.IsAdaptiveQualityEnabled()) + "\n";
        status += "Target FPS: " + ToString(this.GetPerformanceTarget()) + "\n";
        status += "Current Preset: " + this.GetCurrentPreset() + "\n";

        let alerts = this.GetActiveAlerts();
        if ArraySize(alerts) > 0 {
            status += "Active Alerts: " + ToString(ArraySize(alerts)) + "\n";
        }

        return status;
    }

    // Run performance benchmark
    public func RunBenchmark(durationSeconds: Float) -> String {
        this.ResetStatistics();
        this.StartMonitoring();

        // The benchmark would run for the specified duration
        // and collect performance metrics

        let report = "Benchmark Results (" + ToString(durationSeconds) + "s):\n";
        report += "Average FPS: " + ToString(this.GetAverageFPS()) + "\n";
        report += "Average CPU: " + ToString(this.GetCPUUsage()) + "%\n";
        report += "Average Memory: " + ToString(this.GetMemoryUsage()) + "%\n";
        report += "Average GPU: " + ToString(this.GetGPUUsage()) + "%\n";
        report += "Performance Grade: " + this.GetPerformanceGrade() + "\n";

        return report;
    }
}

// Extended GameInstance for performance monitor access
@addMethod(GameInstance)
public static func GetCoopNetPerformanceMonitor() -> ref<CoopNetPerformanceMonitor> {
    // This would be implemented in C++ to return the singleton instance
    return null;
}

// Global convenience functions for performance monitoring

// Quick performance monitoring initialization
public static func InitializePerformanceMonitoring() -> Bool {
    let perfMonitor = CoopNetPerformanceMonitor.GetInstance();
    if IsDefined(perfMonitor) {
        return perfMonitor.InitializeWithDefaults();
    }
    return false;
}

// Check if performance is good
public static func IsPerformanceGood() -> Bool {
    let perfMonitor = CoopNetPerformanceMonitor.GetInstance();
    if IsDefined(perfMonitor) {
        return perfMonitor.IsPerformanceGood();
    }
    return false;
}

// Get current FPS quickly
public static func GetCurrentFPS() -> Float {
    let perfMonitor = CoopNetPerformanceMonitor.GetInstance();
    if IsDefined(perfMonitor) {
        return perfMonitor.GetCurrentFPS();
    }
    return 0.0;
}

// Apply performance preset quickly
public static func SetPerformancePreset(preset: String) -> Bool {
    let perfMonitor = CoopNetPerformanceMonitor.GetInstance();
    if IsDefined(perfMonitor) {
        return perfMonitor.ApplyPerformancePreset(preset);
    }
    return false;
}

// Get performance grade quickly
public static func GetPerformanceGrade() -> String {
    let perfMonitor = CoopNetPerformanceMonitor.GetInstance();
    if IsDefined(perfMonitor) {
        return perfMonitor.GetPerformanceGrade();
    }
    return "Unknown";
}