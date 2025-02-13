//////////////////////////////////////
//          RTL2832-Interface
// https://github.com/steve-m/librtlsdr/tree/master
// 
//////////////////////////////////////
#include <windows.h>
#include <setupapi.h>
#include <winusb.h>
#include <iostream>
#include <vector>
#include <string>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

//winusb device guid
const GUID WinUsbInterfaceGUID = { 0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED} };

std::wstring GetDevicePath(const GUID& interfaceGuid, const std::wstring& instanceId) {
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&interfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device info set. Error: " << GetLastError() << std::endl;
        return L"";
    }
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    std::wstring devicePath;
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &interfaceGuid, i, &deviceInterfaceData); ++i) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

        //allocate memory
        std::vector<__int8> buffer(requiredSize);
        PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(buffer.data());
        deviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SP_DEVINFO_DATA deviceInfoData = {};
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, deviceInterfaceDetail, requiredSize, NULL, &deviceInfoData)) {
            WCHAR instanceIdBuffer[256]; //make sure device matches instance
            if (SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, instanceIdBuffer, sizeof(instanceIdBuffer) / sizeof(WCHAR), NULL)) {
                if (instanceId == instanceIdBuffer) {
                    devicePath = deviceInterfaceDetail->DevicePath;
                    break;
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return devicePath;
}

int main() {
    //this is the path for the device so hopefully hardcoding it works
    std::wstring instanceId = L"USB\\VID_0BDA&PID_2832\\1090";

    std::wstring devicePath = GetDevicePath(WinUsbInterfaceGUID, instanceId);
    if (devicePath.empty()) {
        std::cerr << "Could not find device path" << std::endl;
        return 1;
    }

    std::wcout << L"Device Path: " << devicePath << std::endl;

    //open device
    HANDLE hDevice = CreateFile(
        devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open device. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Device opened successfully!" << std::endl;

    //init winusb
    WINUSB_INTERFACE_HANDLE winUsbHandle = NULL;
    if (!WinUsb_Initialize(hDevice, &winUsbHandle)) {
        std::cerr << "Failed to initialize WinUSB. Error: " << GetLastError() << std::endl;
        CloseHandle(hDevice);
        return 1;
    }

    std::cout << "Successful WinUSB init" << std::endl;

    WinUsb_Free(winUsbHandle);
    CloseHandle(hDevice);
    return 0;
}