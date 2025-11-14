#include "SystemMonitor.h"
#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>

#pragma comment(lib, "comctl32.lib")

// Window controls IDs
#define ID_BTN_START_MONITOR 1001
#define ID_BTN_STOP_MONITOR 1002
#define ID_BTN_COLLECT_NOW 1003
#define ID_BTN_EXPORT_JSON 1004
#define ID_BTN_EXPORT_CSV 1005
#define ID_BTN_EXPORT_XML 1006
#define ID_BTN_VIEW_LOGS 1007
#define ID_LISTVIEW_DATA 1008
#define ID_STATIC_STATUS 1009
#define ID_TIMER_UPDATE 1010

// Global variables
HWND g_hWnd = NULL;
HWND g_hListView = NULL;
HWND g_hStatusLabel = NULL;
SystemMonitor* g_pMonitor = nullptr;
std::atomic<bool> g_isMonitoring(false);

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hWnd);
void InitListView(HWND hListView);
void UpdateListView();
void UpdateStatus(const std::string& status);
void CollectAndDisplayData();
void ExportData(const std::string& format);
void ShowSystemLogs();

// Console mode function
void RunConsoleMode() {
    std::cout << "==================================================" << std::endl;
    std::cout << "      System Monitor - Console Mode              " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;
    
    SystemMonitor monitor;
    
    // Display system info
    SystemInfo sysInfo = monitor.GetSystemInfo();
    std::cout << "System Information:" << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "OS Version: " << sysInfo.osVersion << std::endl;
    std::cout << "Computer Name: " << sysInfo.computerName << std::endl;
    std::cout << "User: " << sysInfo.userName << std::endl;
    std::cout << "Processors: " << sysInfo.processorCount << std::endl;
    std::cout << "Total Memory: " << Utils::FormatBytes(sysInfo.totalMemory) << std::endl;
    std::cout << "Available Memory: " << Utils::FormatBytes(sysInfo.availableMemory) << std::endl;
    std::cout << std::endl;
    
    while (true) {
        std::cout << "Select an option:" << std::endl;
        std::cout << "1. Collect current system data" << std::endl;
        std::cout << "2. Start continuous monitoring" << std::endl;
        std::cout << "3. Stop monitoring" << std::endl;
        std::cout << "4. Export to JSON" << std::endl;
        std::cout << "5. Export to CSV" << std::endl;
        std::cout << "6. Export to XML" << std::endl;
        std::cout << "7. View system logs" << std::endl;
        std::cout << "8. View top processes" << std::endl;
        std::cout << "9. Exit" << std::endl;
        std::cout << "Choice: ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1: {
                std::cout << "\nCollecting system data..." << std::endl;
                
                // CPU Data
                CPUData cpu = monitor.CollectCPUData();
                std::cout << "\nCPU Usage: " << cpu.usagePercent << "%" << std::endl;
                
                // Memory Data
                MemoryData memory = monitor.CollectMemoryData();
                std::cout << "Memory Usage: " << memory.usagePercent << "%" << std::endl;
                std::cout << "Available: " << Utils::FormatBytes(memory.availablePhysical) 
                          << " / " << Utils::FormatBytes(memory.totalPhysical) << std::endl;
                
                // Disk Data
                auto disks = monitor.CollectDiskData();
                std::cout << "\nDisk Usage:" << std::endl;
                for (const auto& disk : disks) {
                    std::cout << "  " << disk.driveLetter << " " 
                              << Utils::FormatBytes(disk.freeSpace) << " free of " 
                              << Utils::FormatBytes(disk.totalSpace) 
                              << " (" << disk.usagePercent << "% used)" << std::endl;
                }
                
                // Network Data
                auto networks = monitor.CollectNetworkData();
                std::cout << "\nNetwork Adapters:" << std::endl;
                for (const auto& net : networks) {
                    std::cout << "  " << net.adapterName << std::endl;
                    std::cout << "    IP: " << net.ipAddress << std::endl;
                    std::cout << "    Received: " << Utils::FormatBytes(net.bytesReceived) << std::endl;
                    std::cout << "    Sent: " << Utils::FormatBytes(net.bytesSent) << std::endl;
                }
                break;
            }
            
            case 2: {
                if (!monitor.IsMonitoring()) {
                    std::cout << "Starting continuous monitoring (5-second intervals)..." << std::endl;
                    monitor.StartMonitoring(5);
                } else {
                    std::cout << "Monitoring is already active." << std::endl;
                }
                break;
            }
            
            case 3: {
                if (monitor.IsMonitoring()) {
                    std::cout << "Stopping monitoring..." << std::endl;
                    monitor.StopMonitoring();
                } else {
                    std::cout << "Monitoring is not active." << std::endl;
                }
                break;
            }
            
            case 4: {
                std::string filename = "system_monitor_data.json";
                if (monitor.ExportToJSON(filename)) {
                    std::cout << "Data exported to " << filename << std::endl;
                } else {
                    std::cout << "Failed to export data." << std::endl;
                }
                break;
            }
            
            case 5: {
                std::string filename = "system_monitor_data.csv";
                if (monitor.ExportToCSV(filename)) {
                    std::cout << "Data exported to " << filename << std::endl;
                } else {
                    std::cout << "Failed to export data." << std::endl;
                }
                break;
            }
            
            case 6: {
                std::string filename = "system_monitor_data.xml";
                if (monitor.ExportToXML(filename)) {
                    std::cout << "Data exported to " << filename << std::endl;
                } else {
                    std::cout << "Failed to export data." << std::endl;
                }
                break;
            }
            
            case 7: {
                std::cout << "\nSystem Event Logs (last 10 entries):" << std::endl;
                auto logs = monitor.CollectSystemLogs("System", 10);
                for (const auto& log : logs) {
                    std::cout << log << std::endl;
                }
                break;
            }
            
            case 8: {
                std::cout << "\nTop 10 Processes by Memory Usage:" << std::endl;
                auto processes = monitor.CollectTopProcesses(10);
                for (const auto& proc : processes) {
                    std::cout << proc.processName << " (PID: " << proc.processId << ")"
                              << " - Memory: " << Utils::FormatBytes(proc.workingSetSize) << std::endl;
                }
                break;
            }
            
            case 9:
                std::cout << "Exiting..." << std::endl;
                return;
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
        
        std::cout << std::endl;
    }
}

// WinMain - Entry point for Windows application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Check if running with console parameter
    if (strstr(lpCmdLine, "-console") != nullptr) {
        AllocConsole();
        FILE* pCout;
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCout, "CONIN$", "r", stdin);
        
        RunConsoleMode();
        
        FreeConsole();
        return 0;
    }
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create monitor instance
    g_pMonitor = new SystemMonitor();
    
    // Register window class
    const wchar_t CLASS_NAME[] = L"SystemMonitorWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClass(&wc);
    
    // Create the window
    g_hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"System Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (g_hWnd == NULL) {
        return 0;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Set up a timer for periodic updates
    SetTimer(g_hWnd, ID_TIMER_UPDATE, 5000, NULL);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    delete g_pMonitor;
    
    return 0;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            CreateControls(hWnd);
            CollectAndDisplayData();
            return 0;
            
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_BTN_START_MONITOR:
                    if (!g_isMonitoring) {
                        g_pMonitor->StartMonitoring(5);
                        g_isMonitoring = true;
                        UpdateStatus("Monitoring started (5-second intervals)");
                        EnableWindow(GetDlgItem(hWnd, ID_BTN_START_MONITOR), FALSE);
                        EnableWindow(GetDlgItem(hWnd, ID_BTN_STOP_MONITOR), TRUE);
                    }
                    break;
                    
                case ID_BTN_STOP_MONITOR:
                    if (g_isMonitoring) {
                        g_pMonitor->StopMonitoring();
                        g_isMonitoring = false;
                        UpdateStatus("Monitoring stopped");
                        EnableWindow(GetDlgItem(hWnd, ID_BTN_START_MONITOR), TRUE);
                        EnableWindow(GetDlgItem(hWnd, ID_BTN_STOP_MONITOR), FALSE);
                    }
                    break;
                    
                case ID_BTN_COLLECT_NOW:
                    CollectAndDisplayData();
                    UpdateStatus("Data collected");
                    break;
                    
                case ID_BTN_EXPORT_JSON:
                    ExportData("JSON");
                    break;
                    
                case ID_BTN_EXPORT_CSV:
                    ExportData("CSV");
                    break;
                    
                case ID_BTN_EXPORT_XML:
                    ExportData("XML");
                    break;
                    
                case ID_BTN_VIEW_LOGS:
                    ShowSystemLogs();
                    break;
            }
            return 0;
        }
        
        case WM_TIMER:
            if (wParam == ID_TIMER_UPDATE && g_isMonitoring) {
                UpdateListView();
            }
            return 0;
            
        case WM_SIZE: {
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            if (g_hListView) {
                SetWindowPos(g_hListView, NULL, 10, 100, 
                            rcClient.right - 20, rcClient.bottom - 150, 
                            SWP_NOZORDER);
            }
            return 0;
        }
        
        case WM_DESTROY:
            KillTimer(hWnd, ID_TIMER_UPDATE);
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Create window controls
void CreateControls(HWND hWnd) {
    // Create buttons
    CreateWindow(L"BUTTON", L"Start Monitoring",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 10, 120, 30,
        hWnd, (HMENU)ID_BTN_START_MONITOR, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"Stop Monitoring",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        140, 10, 120, 30,
        hWnd, (HMENU)ID_BTN_STOP_MONITOR, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"Collect Now",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        270, 10, 100, 30,
        hWnd, (HMENU)ID_BTN_COLLECT_NOW, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"Export JSON",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        380, 10, 100, 30,
        hWnd, (HMENU)ID_BTN_EXPORT_JSON, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"Export CSV",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        490, 10, 100, 30,
        hWnd, (HMENU)ID_BTN_EXPORT_CSV, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"Export XML",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        600, 10, 100, 30,
        hWnd, (HMENU)ID_BTN_EXPORT_XML, NULL, NULL);
    
    CreateWindow(L"BUTTON", L"View Logs",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        710, 10, 100, 30,
        hWnd, (HMENU)ID_BTN_VIEW_LOGS, NULL, NULL);
    
    // Create status label
    g_hStatusLabel = CreateWindow(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 50, 800, 20,
        hWnd, (HMENU)ID_STATIC_STATUS, NULL, NULL);
    
    // Create list view
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    
    g_hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 100, rcClient.right - 20, rcClient.bottom - 150,
        hWnd, (HMENU)ID_LISTVIEW_DATA, NULL, NULL);
    
    InitListView(g_hListView);
}

// Initialize list view columns
void InitListView(HWND hListView) {
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    
    // Add columns
    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"Category";
    lvc.cx = 150;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hListView, 0, &lvc);
    
    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"Metric";
    lvc.cx = 200;
    ListView_InsertColumn(hListView, 1, &lvc);
    
    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"Value";
    lvc.cx = 200;
    ListView_InsertColumn(hListView, 2, &lvc);
    
    lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"Details";
    lvc.cx = 300;
    ListView_InsertColumn(hListView, 3, &lvc);
    
    // Set extended styles
    ListView_SetExtendedListViewStyle(hListView, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

// Helper function to add item to list view
void AddListViewItem(HWND hListView, const std::wstring& category, 
                     const std::wstring& metric, const std::wstring& value, 
                     const std::wstring& details = L"") {
    LVITEM lvi = {};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(hListView);
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)category.c_str();
    
    int index = ListView_InsertItem(hListView, &lvi);
    
    ListView_SetItemText(hListView, index, 1, (LPWSTR)metric.c_str());
    ListView_SetItemText(hListView, index, 2, (LPWSTR)value.c_str());
    ListView_SetItemText(hListView, index, 3, (LPWSTR)details.c_str());
}

// Update list view with current data
void UpdateListView() {
    if (!g_hListView || !g_pMonitor) return;
    
    ListView_DeleteAllItems(g_hListView);
    
    // Get latest data
    auto cpuHistory = g_pMonitor->GetCPUHistory();
    auto memoryHistory = g_pMonitor->GetMemoryHistory();
    auto diskHistory = g_pMonitor->GetDiskHistory();
    auto networkHistory = g_pMonitor->GetNetworkHistory();
    
    // Display latest CPU data
    if (!cpuHistory.empty()) {
        const auto& cpu = cpuHistory.back();
        std::wstringstream ss;
        ss << cpu.usagePercent << L"%";
        AddListViewItem(g_hListView, L"CPU", L"Usage", ss.str(), 
                       L"Overall CPU utilization");
    }
    
    // Display latest memory data
    if (!memoryHistory.empty()) {
        const auto& mem = memoryHistory.back();
        std::wstringstream ss;
        ss << mem.usagePercent << L"%";
        
        std::wstring details = L"Used: ";
        std::string used = Utils::FormatBytes(mem.totalPhysical - mem.availablePhysical);
        details += std::wstring(used.begin(), used.end());
        details += L" / ";
        std::string total = Utils::FormatBytes(mem.totalPhysical);
        details += std::wstring(total.begin(), total.end());
        
        AddListViewItem(g_hListView, L"Memory", L"Usage", ss.str(), details);
    }
    
    // Display disk data
    std::map<std::string, DiskData> latestDisks;
    for (const auto& disk : diskHistory) {
        latestDisks[disk.driveLetter] = disk;
    }
    
    for (const auto& pair : latestDisks) {
        const auto& disk = pair.second;
        std::wstringstream ss;
        ss << disk.usagePercent << L"%";
        
        std::wstring category = L"Disk ";
        category += std::wstring(disk.driveLetter.begin(), disk.driveLetter.end());
        
        std::wstring details = L"Free: ";
        std::string free = Utils::FormatBytes(disk.freeSpace);
        details += std::wstring(free.begin(), free.end());
        details += L" / ";
        std::string total = Utils::FormatBytes(disk.totalSpace);
        details += std::wstring(total.begin(), total.end());
        
        AddListViewItem(g_hListView, category, L"Usage", ss.str(), details);
    }
    
    // Display network data
    std::map<std::string, NetworkData> latestNetworks;
    for (const auto& net : networkHistory) {
        latestNetworks[net.adapterName] = net;
    }
    
    for (const auto& pair : latestNetworks) {
        const auto& net = pair.second;
        std::wstring adapterName(net.adapterName.begin(), net.adapterName.end());
        std::wstring ipAddress(net.ipAddress.begin(), net.ipAddress.end());
        
        std::wstring details = L"IP: " + ipAddress;
        
        std::string received = Utils::FormatBytes(net.bytesReceived);
        std::wstring receivedW(received.begin(), received.end());
        
        AddListViewItem(g_hListView, L"Network", adapterName, receivedW, details);
    }
}

// Collect and display data immediately
void CollectAndDisplayData() {
    if (!g_pMonitor) return;
    
    // Collect all data types
    CPUData cpu = g_pMonitor->CollectCPUData();
    MemoryData memory = g_pMonitor->CollectMemoryData();
    auto disks = g_pMonitor->CollectDiskData();
    auto networks = g_pMonitor->CollectNetworkData();
    
    // Clear list view
    ListView_DeleteAllItems(g_hListView);
    
    // Add system info
    SystemInfo sysInfo = g_pMonitor->GetSystemInfo();
    std::wstring osVersion(sysInfo.osVersion.begin(), sysInfo.osVersion.end());
    std::wstring computerName(sysInfo.computerName.begin(), sysInfo.computerName.end());
    std::wstring userName(sysInfo.userName.begin(), sysInfo.userName.end());
    
    AddListViewItem(g_hListView, L"System", L"OS Version", osVersion);
    AddListViewItem(g_hListView, L"System", L"Computer Name", computerName);
    AddListViewItem(g_hListView, L"System", L"User", userName);
    
    std::wstringstream ss;
    ss << sysInfo.processorCount;
    AddListViewItem(g_hListView, L"System", L"Processors", ss.str());
    
    // Add CPU data
    ss.str(L"");
    ss << cpu.usagePercent << L"%";
    AddListViewItem(g_hListView, L"CPU", L"Usage", ss.str());
    
    // Add memory data
    ss.str(L"");
    ss << memory.usagePercent << L"%";
    std::string memUsed = Utils::FormatBytes(memory.totalPhysical - memory.availablePhysical);
    std::string memTotal = Utils::FormatBytes(memory.totalPhysical);
    std::wstring memDetails = std::wstring(memUsed.begin(), memUsed.end()) + 
                             L" / " + std::wstring(memTotal.begin(), memTotal.end());
    AddListViewItem(g_hListView, L"Memory", L"Usage", ss.str(), memDetails);
    
    // Add disk data
    for (const auto& disk : disks) {
        ss.str(L"");
        ss << disk.usagePercent << L"%";
        
        std::wstring category = L"Disk ";
        category += std::wstring(disk.driveLetter.begin(), disk.driveLetter.end());
        
        std::string free = Utils::FormatBytes(disk.freeSpace);
        std::string total = Utils::FormatBytes(disk.totalSpace);
        std::wstring details = std::wstring(free.begin(), free.end()) + 
                              L" free / " + std::wstring(total.begin(), total.end());
        
        AddListViewItem(g_hListView, category, L"Usage", ss.str(), details);
    }
    
    // Add network data
    for (const auto& net : networks) {
        std::wstring adapterName(net.adapterName.begin(), net.adapterName.end());
        std::wstring ipAddress(net.ipAddress.begin(), net.ipAddress.end());
        
        std::string received = Utils::FormatBytes(net.bytesReceived);
        std::wstring receivedW(received.begin(), received.end());
        
        std::wstring details = L"IP: " + ipAddress;
        AddListViewItem(g_hListView, L"Network", adapterName, receivedW, details);
    }
}

// Update status label
void UpdateStatus(const std::string& status) {
    if (g_hStatusLabel) {
        std::wstring wstatus(status.begin(), status.end());
        SetWindowText(g_hStatusLabel, wstatus.c_str());
    }
}

// Export data to file
void ExportData(const std::string& format) {
    if (!g_pMonitor) return;
    
    std::string filename = "system_monitor_data.";
    bool success = false;
    
    if (format == "JSON") {
        filename += "json";
        success = g_pMonitor->ExportToJSON(filename);
    } else if (format == "CSV") {
        filename += "csv";
        success = g_pMonitor->ExportToCSV(filename);
    } else if (format == "XML") {
        filename += "xml";
        success = g_pMonitor->ExportToXML(filename);
    }
    
    if (success) {
        UpdateStatus("Data exported to " + filename);
        MessageBoxA(g_hWnd, ("Data successfully exported to " + filename).c_str(), 
                   "Export Successful", MB_OK | MB_ICONINFORMATION);
    } else {
        UpdateStatus("Export failed");
        MessageBoxA(g_hWnd, "Failed to export data", "Export Failed", 
                   MB_OK | MB_ICONERROR);
    }
}

// Show system logs in a message box
void ShowSystemLogs() {
    if (!g_pMonitor) return;
    
    auto logs = g_pMonitor->CollectSystemLogs("System", 20);
    
    std::stringstream ss;
    ss << "Recent System Event Logs:\n\n";
    for (const auto& log : logs) {
        ss << log << "\n";
    }
    
    MessageBoxA(g_hWnd, ss.str().c_str(), "System Logs", MB_OK | MB_ICONINFORMATION);
}

// Alternative main function for console testing
int main(int argc, char* argv[]) {
    // Check for console mode
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-console") == 0) {
            RunConsoleMode();
            return 0;
        }
    }
    
    // Otherwise run Windows GUI mode
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
