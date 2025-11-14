 #ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

// Structure to hold system information
struct SystemInfo {
    std::string osVersion;
    std::string computerName;
    std::string userName;
    DWORD processorCount;
    DWORDLONG totalMemory;
    DWORDLONG availableMemory;
};

// Structure for CPU performance data
struct CPUData {
    double usagePercent;
    std::vector<double> coreUsage;
    double temperature; // If available
    std::chrono::system_clock::time_point timestamp;
};

// Structure for memory data
struct MemoryData {
    DWORDLONG totalPhysical;
    DWORDLONG availablePhysical;
    DWORDLONG totalVirtual;
    DWORDLONG availableVirtual;
    DWORDLONG totalPageFile;
    DWORDLONG availablePageFile;
    double usagePercent;
    std::chrono::system_clock::time_point timestamp;
};

// Structure for disk data
struct DiskData {
    std::string driveLetter;
    ULONGLONG totalSpace;
    ULONGLONG freeSpace;
    double usagePercent;
    double readSpeed;
    double writeSpeed;
    std::chrono::system_clock::time_point timestamp;
};

// Structure for network data
struct NetworkData {
    std::string adapterName;
    std::string ipAddress;
    ULONGLONG bytesReceived;
    ULONGLONG bytesSent;
    ULONGLONG packetsReceived;
    ULONGLONG packetsSent;
    double downloadSpeed;
    double uploadSpeed;
    std::chrono::system_clock::time_point timestamp;
};

// Structure for process data
struct ProcessData {
    DWORD processId;
    std::string processName;
    SIZE_T workingSetSize;
    SIZE_T virtualSize;
    double cpuUsage;
    std::chrono::system_clock::time_point timestamp;
};

// Main monitoring class
class SystemMonitor {
private:
    bool running;
    SystemInfo systemInfo;
    std::vector<CPUData> cpuHistory;
    std::vector<MemoryData> memoryHistory;
    std::vector<DiskData> diskHistory;
    std::vector<NetworkData> networkHistory;
    std::vector<ProcessData> processHistory;
    
    // Performance counter handles
    HANDLE cpuQuery;
    HANDLE cpuTotal;
    std::vector<HANDLE> cpuCores;
    
    // Helper methods
    void InitializePerformanceCounters();
    void CleanupPerformanceCounters();
    double CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks);
    
public:
    SystemMonitor();
    ~SystemMonitor();
    
    // Initialization
    bool Initialize();
    void Cleanup();
    
    // Data collection methods
    SystemInfo GetSystemInfo();
    CPUData CollectCPUData();
    MemoryData CollectMemoryData();
    std::vector<DiskData> CollectDiskData();
    std::vector<NetworkData> CollectNetworkData();
    std::vector<ProcessData> CollectTopProcesses(int count = 10);
    
    // Event log collection
    std::vector<std::string> CollectSystemLogs(const std::string& logName, int maxEntries = 100);
    
    // Data export methods
    bool ExportToJSON(const std::string& filename);
    bool ExportToCSV(const std::string& filename);
    bool ExportToXML(const std::string& filename);
    
    // Continuous monitoring
    void StartMonitoring(int intervalSeconds = 5);
    void StopMonitoring();
    bool IsMonitoring() const { return running; }
    
    // Data access
    const std::vector<CPUData>& GetCPUHistory() const { return cpuHistory; }
    const std::vector<MemoryData>& GetMemoryHistory() const { return memoryHistory; }
    const std::vector<DiskData>& GetDiskHistory() const { return diskHistory; }
    const std::vector<NetworkData>& GetNetworkHistory() const { return networkHistory; }
};

// Utility functions
namespace Utils {
    std::string GetLastErrorAsString();
    std::string FormatBytes(ULONGLONG bytes);
    std::string GetCurrentTimestamp();
    bool IsRunAsAdministrator();
}

#endif // SYSTEM_MONITOR_H
