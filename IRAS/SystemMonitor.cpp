#include "SystemMonitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iphlpapi.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <evntprov.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// Constructor
SystemMonitor::SystemMonitor() : running(false), cpuQuery(nullptr), cpuTotal(nullptr) {
    Initialize();
}

// Destructor
SystemMonitor::~SystemMonitor() {
    Cleanup();
}

// Initialize the monitor
bool SystemMonitor::Initialize() {
    try {
        // Get initial system info
        systemInfo = GetSystemInfo();
        
        // Initialize performance counters
        InitializePerformanceCounters();
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

// Cleanup resources
void SystemMonitor::Cleanup() {
    StopMonitoring();
    CleanupPerformanceCounters();
}

// Initialize performance counters
void SystemMonitor::InitializePerformanceCounters() {
    PDH_STATUS status;
    
    // Create a query object
    status = PdhOpenQuery(NULL, 0, &cpuQuery);
    if (status != ERROR_SUCCESS) {
        std::cerr << "PdhOpenQuery failed with status: " << status << std::endl;
        return;
    }
    
    // Add the total CPU counter
    status = PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal);
    if (status != ERROR_SUCCESS) {
        std::cerr << "PdhAddCounter failed for total CPU" << std::endl;
    }
    
    // Add individual core counters
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
        HANDLE coreCounter;
        std::wstring counterPath = L"\\Processor(" + std::to_wstring(i) + L")\\% Processor Time";
        status = PdhAddCounter(cpuQuery, counterPath.c_str(), 0, &coreCounter);
        if (status == ERROR_SUCCESS) {
            cpuCores.push_back(coreCounter);
        }
    }
    
    // Collect initial data
    PdhCollectQueryData(cpuQuery);
}

// Cleanup performance counters
void SystemMonitor::CleanupPerformanceCounters() {
    if (cpuQuery) {
        PdhCloseQuery(cpuQuery);
        cpuQuery = nullptr;
    }
    cpuCores.clear();
}

// Get system information
SystemInfo SystemMonitor::GetSystemInfo() {
    SystemInfo info;
    
    // Get OS version
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        std::stringstream ss;
        ss << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion 
           << " Build " << osvi.dwBuildNumber;
        info.osVersion = ss.str();
    }
    
    // Get computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        info.computerName = computerName;
    }
    
    // Get user name
    char userName[256];
    size = sizeof(userName);
    if (GetUserNameA(userName, &size)) {
        info.userName = userName;
    }
    
    // Get processor count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.processorCount = sysInfo.dwNumberOfProcessors;
    
    // Get memory info
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        info.totalMemory = memStatus.ullTotalPhys;
        info.availableMemory = memStatus.ullAvailPhys;
    }
    
    return info;
}

// Collect CPU data
CPUData SystemMonitor::CollectCPUData() {
    CPUData data;
    data.timestamp = std::chrono::system_clock::now();
    
    if (cpuQuery) {
        PDH_STATUS status = PdhCollectQueryData(cpuQuery);
        if (status == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE counterValue;
            
            // Get total CPU usage
            status = PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterValue);
            if (status == ERROR_SUCCESS) {
                data.usagePercent = counterValue.doubleValue;
            }
            
            // Get individual core usage
            for (auto& core : cpuCores) {
                status = PdhGetFormattedCounterValue(core, PDH_FMT_DOUBLE, NULL, &counterValue);
                if (status == ERROR_SUCCESS) {
                    data.coreUsage.push_back(counterValue.doubleValue);
                }
            }
        }
    }
    
    // Temperature would require WMI or specific hardware APIs
    data.temperature = -1; // Not available without additional APIs
    
    return data;
}

// Collect memory data
MemoryData SystemMonitor::CollectMemoryData() {
    MemoryData data;
    data.timestamp = std::chrono::system_clock::now();
    
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    
    if (GlobalMemoryStatusEx(&memStatus)) {
        data.totalPhysical = memStatus.ullTotalPhys;
        data.availablePhysical = memStatus.ullAvailPhys;
        data.totalVirtual = memStatus.ullTotalVirtual;
        data.availableVirtual = memStatus.ullAvailVirtual;
        data.totalPageFile = memStatus.ullTotalPageFile;
        data.availablePageFile = memStatus.ullAvailPageFile;
        data.usagePercent = memStatus.dwMemoryLoad;
    }
    
    return data;
}

// Collect disk data
std::vector<DiskData> SystemMonitor::CollectDiskData() {
    std::vector<DiskData> disks;
    
    DWORD drives = GetLogicalDrives();
    char driveLetter[] = "A:\\";
    
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            driveLetter[0] = 'A' + i;
            
            UINT driveType = GetDriveTypeA(driveLetter);
            if (driveType == DRIVE_FIXED) {
                DiskData disk;
                disk.driveLetter = driveLetter;
                disk.timestamp = std::chrono::system_clock::now();
                
                ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
                if (GetDiskFreeSpaceExA(driveLetter, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                    disk.totalSpace = totalBytes.QuadPart;
                    disk.freeSpace = totalFreeBytes.QuadPart;
                    disk.usagePercent = ((double)(disk.totalSpace - disk.freeSpace) / disk.totalSpace) * 100.0;
                }
                
                // Disk speed would require performance counters or direct I/O monitoring
                disk.readSpeed = 0;
                disk.writeSpeed = 0;
                
                disks.push_back(disk);
            }
        }
    }
    
    return disks;
}

// Collect network data
std::vector<NetworkData> SystemMonitor::CollectNetworkData() {
    std::vector<NetworkData> networks;
    
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    
    if (pAddresses == nullptr) {
        return networks;
    }
    
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_UNSPEC;
    
    DWORD result = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);
    
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        if (pAddresses == nullptr) {
            return networks;
        }
        result = GetAdaptersAddresses(family, flags, NULL, pAddresses, &bufferSize);
    }
    
    if (result == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if (pCurrAddresses->OperStatus == IfOperStatusUp) {
                NetworkData network;
                network.timestamp = std::chrono::system_clock::now();
                
                // Convert adapter name from wide char
                char adapterName[256];
                wcstombs(adapterName, pCurrAddresses->FriendlyName, sizeof(adapterName));
                network.adapterName = adapterName;
                
                // Get IP address
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                if (pUnicast != nullptr) {
                    sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ip, INET_ADDRSTRLEN);
                    network.ipAddress = ip;
                }
                
                // Get interface statistics
                MIB_IF_ROW2 ifRow;
                ZeroMemory(&ifRow, sizeof(MIB_IF_ROW2));
                ifRow.InterfaceIndex = pCurrAddresses->IfIndex;
                
                if (GetIfEntry2(&ifRow) == NO_ERROR) {
                    network.bytesReceived = ifRow.InOctets;
                    network.bytesSent = ifRow.OutOctets;
                    network.packetsReceived = ifRow.InUcastPkts + ifRow.InNUcastPkts;
                    network.packetsSent = ifRow.OutUcastPkts + ifRow.OutNUcastPkts;
                }
                
                // Speed calculation would require sampling over time
                network.downloadSpeed = 0;
                network.uploadSpeed = 0;
                
                networks.push_back(network);
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    
    free(pAddresses);
    return networks;
}

// Collect top processes
std::vector<ProcessData> SystemMonitor::CollectTopProcesses(int count) {
    std::vector<ProcessData> processes;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessData process;
            process.processId = pe32.th32ProcessID;
            process.processName = pe32.szExeFile;
            process.timestamp = std::chrono::system_clock::now();
            
            // Get process memory info
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                    process.workingSetSize = pmc.WorkingSetSize;
                    process.virtualSize = pmc.PrivateUsage;
                }
                
                // CPU usage would require sampling over time
                process.cpuUsage = 0;
                
                CloseHandle(hProcess);
            }
            
            processes.push_back(process);
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
    // Sort by memory usage and return top N
    std::sort(processes.begin(), processes.end(), 
        [](const ProcessData& a, const ProcessData& b) {
            return a.workingSetSize > b.workingSetSize;
        });
    
    if (processes.size() > count) {
        processes.resize(count);
    }
    
    return processes;
}

// Collect system logs
std::vector<std::string> SystemMonitor::CollectSystemLogs(const std::string& logName, int maxEntries) {
    std::vector<std::string> logs;
    
    std::wstring wLogName(logName.begin(), logName.end());
    HANDLE hEventLog = OpenEventLogW(NULL, wLogName.c_str());
    
    if (hEventLog == NULL) {
        return logs;
    }
    
    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];
    DWORD bytesRead = 0;
    DWORD minBytesNeeded = 0;
    int entriesRead = 0;
    
    while (entriesRead < maxEntries) {
        if (!ReadEventLogW(hEventLog, 
                          EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                          0, buffer, bufferSize, &bytesRead, &minBytesNeeded)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                // Buffer too small, but we'll skip this entry
                continue;
            }
            break;
        }
        
        BYTE* pRecord = buffer;
        while (bytesRead > 0 && entriesRead < maxEntries) {
            EVENTLOGRECORD* pEventRecord = (EVENTLOGRECORD*)pRecord;
            
            std::stringstream ss;
            ss << "Event ID: " << pEventRecord->EventID;
            ss << ", Type: " << pEventRecord->EventType;
            ss << ", Time: " << pEventRecord->TimeGenerated;
            
            // Get source name
            LPWSTR pSourceName = (LPWSTR)((LPBYTE)pEventRecord + sizeof(EVENTLOGRECORD));
            char sourceName[256];
            wcstombs(sourceName, pSourceName, sizeof(sourceName));
            ss << ", Source: " << sourceName;
            
            logs.push_back(ss.str());
            entriesRead++;
            
            bytesRead -= pEventRecord->Length;
            pRecord += pEventRecord->Length;
        }
    }
    
    CloseEventLog(hEventLog);
    return logs;
}

// Export to JSON
bool SystemMonitor::ExportToJSON(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    file << "  \"timestamp\": \"" << Utils::GetCurrentTimestamp() << "\",\n";
    file << "  \"system_info\": {\n";
    file << "    \"os_version\": \"" << systemInfo.osVersion << "\",\n";
    file << "    \"computer_name\": \"" << systemInfo.computerName << "\",\n";
    file << "    \"user_name\": \"" << systemInfo.userName << "\",\n";
    file << "    \"processor_count\": " << systemInfo.processorCount << ",\n";
    file << "    \"total_memory\": " << systemInfo.totalMemory << ",\n";
    file << "    \"available_memory\": " << systemInfo.availableMemory << "\n";
    file << "  },\n";
    
    // Add CPU data
    if (!cpuHistory.empty()) {
        file << "  \"cpu_data\": [\n";
        for (size_t i = 0; i < cpuHistory.size(); i++) {
            file << "    {\n";
            file << "      \"usage_percent\": " << cpuHistory[i].usagePercent << ",\n";
            file << "      \"core_count\": " << cpuHistory[i].coreUsage.size() << "\n";
            file << "    }";
            if (i < cpuHistory.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
    }
    
    // Add memory data
    if (!memoryHistory.empty()) {
        file << "  \"memory_data\": [\n";
        for (size_t i = 0; i < memoryHistory.size(); i++) {
            file << "    {\n";
            file << "      \"total_physical\": " << memoryHistory[i].totalPhysical << ",\n";
            file << "      \"available_physical\": " << memoryHistory[i].availablePhysical << ",\n";
            file << "      \"usage_percent\": " << memoryHistory[i].usagePercent << "\n";
            file << "    }";
            if (i < memoryHistory.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
    }
    
    // Add disk data
    if (!diskHistory.empty()) {
        file << "  \"disk_data\": [\n";
        for (size_t i = 0; i < diskHistory.size(); i++) {
            file << "    {\n";
            file << "      \"drive\": \"" << diskHistory[i].driveLetter << "\",\n";
            file << "      \"total_space\": " << diskHistory[i].totalSpace << ",\n";
            file << "      \"free_space\": " << diskHistory[i].freeSpace << ",\n";
            file << "      \"usage_percent\": " << diskHistory[i].usagePercent << "\n";
            file << "    }";
            if (i < diskHistory.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
    }
    
    // Add network data
    if (!networkHistory.empty()) {
        file << "  \"network_data\": [\n";
        for (size_t i = 0; i < networkHistory.size(); i++) {
            file << "    {\n";
            file << "      \"adapter\": \"" << networkHistory[i].adapterName << "\",\n";
            file << "      \"ip_address\": \"" << networkHistory[i].ipAddress << "\",\n";
            file << "      \"bytes_received\": " << networkHistory[i].bytesReceived << ",\n";
            file << "      \"bytes_sent\": " << networkHistory[i].bytesSent << "\n";
            file << "    }";
            if (i < networkHistory.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ]\n";
    }
    
    file << "}\n";
    file.close();
    
    return true;
}

// Export to CSV
bool SystemMonitor::ExportToCSV(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "Timestamp,Category,Metric,Value\n";
    
    std::string timestamp = Utils::GetCurrentTimestamp();
    
    // System info
    file << timestamp << ",System,OS Version," << systemInfo.osVersion << "\n";
    file << timestamp << ",System,Computer Name," << systemInfo.computerName << "\n";
    file << timestamp << ",System,User Name," << systemInfo.userName << "\n";
    file << timestamp << ",System,Processor Count," << systemInfo.processorCount << "\n";
    file << timestamp << ",System,Total Memory," << systemInfo.totalMemory << "\n";
    file << timestamp << ",System,Available Memory," << systemInfo.availableMemory << "\n";
    
    // CPU data
    for (const auto& cpu : cpuHistory) {
        file << timestamp << ",CPU,Usage Percent," << cpu.usagePercent << "\n";
    }
    
    // Memory data
    for (const auto& mem : memoryHistory) {
        file << timestamp << ",Memory,Total Physical," << mem.totalPhysical << "\n";
        file << timestamp << ",Memory,Available Physical," << mem.availablePhysical << "\n";
        file << timestamp << ",Memory,Usage Percent," << mem.usagePercent << "\n";
    }
    
    // Disk data
    for (const auto& disk : diskHistory) {
        file << timestamp << ",Disk " << disk.driveLetter << ",Total Space," << disk.totalSpace << "\n";
        file << timestamp << ",Disk " << disk.driveLetter << ",Free Space," << disk.freeSpace << "\n";
        file << timestamp << ",Disk " << disk.driveLetter << ",Usage Percent," << disk.usagePercent << "\n";
    }
    
    // Network data
    for (const auto& net : networkHistory) {
        file << timestamp << ",Network," << net.adapterName << " Bytes Received," << net.bytesReceived << "\n";
        file << timestamp << ",Network," << net.adapterName << " Bytes Sent," << net.bytesSent << "\n";
    }
    
    file.close();
    return true;
}

// Export to XML
bool SystemMonitor::ExportToXML(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<SystemMonitorData>\n";
    file << "  <Timestamp>" << Utils::GetCurrentTimestamp() << "</Timestamp>\n";
    
    // System info
    file << "  <SystemInfo>\n";
    file << "    <OSVersion>" << systemInfo.osVersion << "</OSVersion>\n";
    file << "    <ComputerName>" << systemInfo.computerName << "</ComputerName>\n";
    file << "    <UserName>" << systemInfo.userName << "</UserName>\n";
    file << "    <ProcessorCount>" << systemInfo.processorCount << "</ProcessorCount>\n";
    file << "    <TotalMemory>" << systemInfo.totalMemory << "</TotalMemory>\n";
    file << "    <AvailableMemory>" << systemInfo.availableMemory << "</AvailableMemory>\n";
    file << "  </SystemInfo>\n";
    
    // CPU data
    if (!cpuHistory.empty()) {
        file << "  <CPUData>\n";
        for (const auto& cpu : cpuHistory) {
            file << "    <Sample>\n";
            file << "      <UsagePercent>" << cpu.usagePercent << "</UsagePercent>\n";
            file << "      <CoreCount>" << cpu.coreUsage.size() << "</CoreCount>\n";
            file << "    </Sample>\n";
        }
        file << "  </CPUData>\n";
    }
    
    // Memory data
    if (!memoryHistory.empty()) {
        file << "  <MemoryData>\n";
        for (const auto& mem : memoryHistory) {
            file << "    <Sample>\n";
            file << "      <TotalPhysical>" << mem.totalPhysical << "</TotalPhysical>\n";
            file << "      <AvailablePhysical>" << mem.availablePhysical << "</AvailablePhysical>\n";
            file << "      <UsagePercent>" << mem.usagePercent << "</UsagePercent>\n";
            file << "    </Sample>\n";
        }
        file << "  </MemoryData>\n";
    }
    
    // Disk data
    if (!diskHistory.empty()) {
        file << "  <DiskData>\n";
        for (const auto& disk : diskHistory) {
            file << "    <Disk>\n";
            file << "      <Drive>" << disk.driveLetter << "</Drive>\n";
            file << "      <TotalSpace>" << disk.totalSpace << "</TotalSpace>\n";
            file << "      <FreeSpace>" << disk.freeSpace << "</FreeSpace>\n";
            file << "      <UsagePercent>" << disk.usagePercent << "</UsagePercent>\n";
            file << "    </Disk>\n";
        }
        file << "  </DiskData>\n";
    }
    
    // Network data
    if (!networkHistory.empty()) {
        file << "  <NetworkData>\n";
        for (const auto& net : networkHistory) {
            file << "    <Adapter>\n";
            file << "      <Name>" << net.adapterName << "</Name>\n";
            file << "      <IPAddress>" << net.ipAddress << "</IPAddress>\n";
            file << "      <BytesReceived>" << net.bytesReceived << "</BytesReceived>\n";
            file << "      <BytesSent>" << net.bytesSent << "</BytesSent>\n";
            file << "    </Adapter>\n";
        }
        file << "  </NetworkData>\n";
    }
    
    file << "</SystemMonitorData>\n";
    file.close();
    
    return true;
}

// Start continuous monitoring
void SystemMonitor::StartMonitoring(int intervalSeconds) {
    if (running) return;
    
    running = true;
    std::thread monitorThread([this, intervalSeconds]() {
        while (running) {
            // Collect data
            cpuHistory.push_back(CollectCPUData());
            memoryHistory.push_back(CollectMemoryData());
            
            auto disks = CollectDiskData();
            diskHistory.insert(diskHistory.end(), disks.begin(), disks.end());
            
            auto networks = CollectNetworkData();
            networkHistory.insert(networkHistory.end(), networks.begin(), networks.end());
            
            auto processes = CollectTopProcesses(10);
            processHistory.insert(processHistory.end(), processes.begin(), processes.end());
            
            // Limit history size to prevent excessive memory usage
            const size_t maxHistorySize = 1000;
            if (cpuHistory.size() > maxHistorySize) {
                cpuHistory.erase(cpuHistory.begin());
            }
            if (memoryHistory.size() > maxHistorySize) {
                memoryHistory.erase(memoryHistory.begin());
            }
            if (diskHistory.size() > maxHistorySize * 10) {
                diskHistory.erase(diskHistory.begin(), diskHistory.begin() + 10);
            }
            if (networkHistory.size() > maxHistorySize * 10) {
                networkHistory.erase(networkHistory.begin(), networkHistory.begin() + 10);
            }
            if (processHistory.size() > maxHistorySize * 10) {
                processHistory.erase(processHistory.begin(), processHistory.begin() + 10);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
    });
    
    monitorThread.detach();
}

// Stop monitoring
void SystemMonitor::StopMonitoring() {
    running = false;
}

// Utility functions implementation
namespace Utils {
    std::string GetLastErrorAsString() {
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0) {
            return std::string();
        }
        
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer, 0, NULL);
        
        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        
        return message;
    }
    
    std::string FormatBytes(ULONGLONG bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && unitIndex < 4) {
            size /= 1024;
            unitIndex++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return ss.str();
    }
    
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    bool IsRunAsAdministrator() {
        BOOL isElevated = FALSE;
        HANDLE hToken = NULL;
        
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
                isElevated = elevation.TokenIsElevated;
            }
            
            CloseHandle(hToken);
        }
        
        return isElevated != FALSE;
    }
}
