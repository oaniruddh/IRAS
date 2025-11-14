# IRAS System Monitor

A comprehensive, real-time system monitoring application for Windows, written in C++. This tool provides both graphical (GUI) and console interfaces for monitoring system performance, including CPU, memory, disk, and network usage.

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

## Features

### 📊 **Real-time Monitoring**
- **CPU Usage**: Total and per-core CPU utilization
- **Memory Usage**: Physical and virtual memory statistics
- **Disk Usage**: Drive space utilization and I/O metrics
- **Network Activity**: Adapter statistics and traffic monitoring
- **Process Monitoring**: Top processes by memory usage

### 🎨 **User Interface Options**
- **Windows GUI**: Modern Windows application with ListView controls
- **Console Mode**: Command-line interface for server environments
- **Real-time Updates**: Configurable refresh intervals

### 📤 **Data Export**
- **JSON Export**: Structured data for integration
- **CSV Export**: Spreadsheet-compatible format
- **XML Export**: Hierarchical data representation

### 🔧 **Advanced Features**
- **System Event Logs**: Windows Event Log integration
- **Historical Data**: Configurable data retention
- **Configuration System**: INI-based settings management
- **Logging System**: Multi-level logging with file output
- **Alert Thresholds**: Configurable warning levels

## System Requirements

- **Operating System**: Windows 7 or later (Windows 10/11 recommended)
- **Compiler**: Visual Studio 2017+ or MinGW with C++17 support
- **Dependencies**: Windows API, PDH, PSAPI, IPHLPAPI
- **Memory**: 50MB minimum, 100MB recommended
- **Disk Space**: 10MB for installation

## Quick Start

### Building the Project

#### Option 1: Using the Build Script (Recommended)
```batch
# Clone or download the project
cd IRAS
./build.bat
```

#### Option 2: Using CMake Manually
```batch
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### Option 3: Using Visual Studio
1. Open Visual Studio
2. File → Open → CMakeLists.txt
3. Build → Build All

### Running the Application

#### GUI Mode (Default)
```batch
SystemMonitor.exe
```

#### Console Mode
```batch
SystemMonitor.exe -console
```

## Usage Guide

### GUI Mode

The main window provides:
- **Control Buttons**: Start/Stop monitoring, collect data, export options
- **Real-time Display**: ListView showing current system metrics
- **Status Bar**: Current operation status
- **Menu Options**: Export to JSON, CSV, XML formats

**Key Features:**
- Click "Start Monitoring" to begin continuous data collection
- Use "Collect Now" for instant system snapshot
- Export buttons save current data in various formats
- "View Logs" displays recent system events

### Console Mode

Interactive command-line interface with options:
1. **Collect current system data** - Instant system snapshot
2. **Start continuous monitoring** - Background monitoring
3. **Stop monitoring** - Halt continuous collection
4. **Export options** - Save data in JSON/CSV/XML
5. **View system logs** - Display Windows Event Log entries
6. **View top processes** - Show memory usage leaders

## Configuration

The application uses `config.ini` for settings:

```ini
# IRAS System Monitor Configuration
monitoring_interval=5          # Seconds between data collection
max_history_size=1000         # Maximum historical data points
auto_start_monitoring=false    # Auto-start monitoring on launch
cpu_alert_threshold=80        # CPU usage alert percentage
memory_alert_threshold=85     # Memory usage alert percentage
disk_alert_threshold=90       # Disk usage alert percentage
enable_notifications=true     # Enable system notifications
window_width=900             # GUI window width
window_height=600            # GUI window height
```

## API Reference

### Core Classes

#### `SystemMonitor`
Main monitoring class handling data collection and export.

```cpp
SystemMonitor monitor;

// Collect instant data
CPUData cpu = monitor.CollectCPUData();
MemoryData memory = monitor.CollectMemoryData();
std::vector<DiskData> disks = monitor.CollectDiskData();

// Start continuous monitoring
monitor.StartMonitoring(5); // 5-second intervals

// Export data
monitor.ExportToJSON("data.json");
```

#### `Config`
Configuration management system.

```cpp
Config config;
int interval = config.GetInt("monitoring_interval", 5);
config.Set("cpu_alert_threshold", 85);
```

#### `Logger`
Logging system with multiple levels.

```cpp
Logger logger;
logger.Info("Application started");
logger.Warning("High CPU usage detected");
logger.Error("Failed to collect network data");
```

### Data Structures

#### `SystemInfo`
```cpp
struct SystemInfo {
    std::string osVersion;      // OS version string
    std::string computerName;   // Computer name
    std::string userName;       // Current user
    DWORD processorCount;       // Number of processors
    DWORDLONG totalMemory;      // Total physical memory
    DWORDLONG availableMemory;  // Available memory
};
```

#### `CPUData`
```cpp
struct CPUData {
    double usagePercent;                    // Overall CPU usage
    std::vector<double> coreUsage;          // Per-core usage
    std::chrono::system_clock::time_point timestamp;
};
```

#### `MemoryData`
```cpp
struct MemoryData {
    DWORDLONG totalPhysical;     // Total physical memory
    DWORDLONG availablePhysical; // Available physical memory
    DWORDLONG totalVirtual;      // Total virtual memory
    DWORDLONG availableVirtual;  // Available virtual memory
    double usagePercent;         // Memory usage percentage
    std::chrono::system_clock::time_point timestamp;
};
```

## File Structure

```
IRAS/
├── main.cpp              # Application entry point
├── SystemMonitor.h       # Core monitoring class header
├── SystemMonitor.cpp     # Core monitoring implementation
├── Config.h              # Configuration management
├── Logger.h              # Logging system header
├── Logger.cpp            # Logging implementation
├── CMakeLists.txt        # CMake build configuration
├── build.bat             # Windows build script
├── README.md             # This file
└── build/                # Build output directory
    ├── bin/              # Compiled executables
    └── ...               # CMake generated files
```

## Advanced Usage

### Custom Data Collection

```cpp
// Create monitor with custom settings
SystemMonitor monitor;
monitor.StartMonitoring(2); // 2-second intervals

// Collect specific data types
auto processes = monitor.CollectTopProcesses(20); // Top 20 processes
auto logs = monitor.CollectSystemLogs("Application", 50); // 50 app logs

// Access historical data
const auto& cpuHistory = monitor.GetCPUHistory();
const auto& memoryHistory = monitor.GetMemoryHistory();
```

### Configuration Management

```cpp
Config config("custom_config.ini");

// Read settings
bool autoStart = config.GetBool("auto_start_monitoring");
int refreshRate = config.GetInt("refresh_rate_ms", 1000);

// Update settings
config.Set("monitoring_interval", 3);
config.Set("enable_notifications", true);
```

### Logging Integration

```cpp
// Configure logger
g_logger.SetLogLevel(LogLevel::DEBUG);
g_logger.SetLogFile("monitor.log");

// Use logging macros
LOG_INFO("System monitoring started");
LOG_WARNING("CPU usage above threshold: " + std::to_string(cpuUsage));
LOG_ERROR("Failed to initialize performance counters");
```

## Troubleshooting

### Common Issues

#### Build Errors
- **Missing CMake**: Install CMake 3.16 or later
- **Compiler not found**: Ensure Visual Studio or MinGW is installed
- **Missing libraries**: Windows SDK required for system APIs

#### Runtime Issues
- **Access Denied**: Run as Administrator for full functionality
- **Performance Counter Errors**: Rebuild performance counter registry
- **High Memory Usage**: Reduce `max_history_size` in configuration

#### Performance Tips
- Use longer monitoring intervals for reduced CPU impact
- Limit historical data size for lower memory usage
- Disable console output in production for better performance

### Debug Mode

Enable debug logging by modifying config.ini:
```ini
log_level=DEBUG
show_console_output=true
```

Or use console mode for detailed output:
```batch
SystemMonitor.exe -console
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Commit changes: `git commit -am 'Add feature'`
4. Push to branch: `git push origin feature-name`
5. Create Pull Request

### Development Setup

1. Install Visual Studio 2019+ with C++ development tools
2. Install CMake 3.16+
3. Clone repository
4. Run `build.bat` to verify setup

### Code Style

- Use C++17 features where appropriate
- Follow Windows API conventions for system calls
- Include comprehensive error handling
- Add logging for significant operations
- Document public APIs with comments

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Changelog

### Version 1.0.0
- Initial release
- GUI and console interfaces
- Real-time system monitoring
- Data export capabilities
- Configuration system
- Logging framework

## Support

For support and questions:
- Create an issue on the project repository
- Check troubleshooting section above
- Review system requirements

## Acknowledgments

- Windows API documentation and examples
- Performance Data Helper (PDH) library
- IP Helper API for network statistics
- Windows Event Log API for system logs
