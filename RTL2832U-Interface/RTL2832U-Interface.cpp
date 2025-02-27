//////////////////////////////////////
//          RTL2832-Interface
// libusb will be statically linked
// VID and PID are hardcoded for the RTL2832
//////////////////////////////////////

#include <iostream>
#include "libusb.h"
#include <vector>
#include <thread>

//RTL2832U
#define VID 0x0BDA 
#define PID 0x2832

void libusbShutdown(libusb_device_handle* handle, int interface, libusb_context* context) {
    std::cout << "Shutting down libusb" << std::endl;
    if (interface > 0) {
        libusb_release_interface(handle, interface);
    }
    libusb_close(handle);
    libusb_exit(context);
}



int main() {
#ifndef _WIN32
    std::cout << "Non-Windows systems are not supported (yet)" << std::endl;
    return -1;
#endif
    libusb_context* ctx = nullptr;
    libusb_device** devs = nullptr;
    libusb_device_handle* deviceHandle = nullptr;
    ssize_t cnt = 0;

    //init libusb
    if (libusb_init(&ctx) < 0) {
        std::cerr << "Failed to initialize libusb" << std::endl;
        return 1;
    }

    //get usb list
    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        std::cerr << "Failed to get device list" << std::endl;
        libusb_exit(ctx);
        return 1;
    }

    libusb_device* found = nullptr;

    //iterate through devices
    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device* device = devs[i];
        libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(device, &desc) == 0) {
            if (desc.idVendor == VID && desc.idProduct == PID) {
                std::cout << "found device" << std::endl;
                found = device;
                break;
            }
        }
    }

    if (found) {
        if (libusb_open(found, &deviceHandle) == 0) {
            std::cout << "Device opened successfully" << std::endl;
        }
        else {
            std::cerr << "Failed to open device" << std::endl;
        }
    }
    else {
        std::cerr << "Device not found" << std::endl;
    }

    //free device list
    libusb_free_device_list(devs, 1);

    int interfaceNum = 0;
    if (libusb_claim_interface(deviceHandle, interfaceNum) < 0) {
        std::cerr << "Failed to claim interface" << std::endl;
        libusb_close(deviceHandle);
        libusb_exit(ctx);
        return 1;
    }

    libusb_device_descriptor* desc = nullptr;
    std::cout << "Interface claimed" << std::endl;


    std::cout << "Getting BULK IN endpoint" << std::endl;

    ///////
    //find the bulk IN endpoint
    libusb_config_descriptor* config = nullptr;
    libusb_device* device = libusb_get_device(deviceHandle);
    if (libusb_get_active_config_descriptor(device, &config) != 0) {
        std::cerr << "Failed to get config descriptor" << std::endl;
        libusbShutdown(deviceHandle, interfaceNum, ctx);
        return 1;
    }

    uint8_t endpointAddr = 0;
    bool foundEndpoint = false;

    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& interface = config->interface[i];
        for (int j = 0; j < interface.num_altsetting; j++) {
            const libusb_interface_descriptor& altsetting = interface.altsetting[j];
            if (i == interfaceNum) {
                for (int k = 0; k < altsetting.bNumEndpoints; k++) {
                    const libusb_endpoint_descriptor& endpoint = altsetting.endpoint[k];
                    if ((endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) {
                        if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
                            endpointAddr = endpoint.bEndpointAddress;
                            foundEndpoint = true;
                            break;
                        }
                    }
                }
            }
            if (foundEndpoint) {
                break;
            }
        }
        if (foundEndpoint) {
            break;
        }
    }

    libusb_free_config_descriptor(config);

    if (!foundEndpoint) {
        std::cerr << "Bulk IN endpoint not found!" << std::endl;
        libusbShutdown(deviceHandle, interfaceNum, ctx);
        return 1;
    }

    //set up async transfers
    const int NUM_TRANSFERS = 16;
    const int TRANSFER_SIZE = 16 * 512; //8KB per transfer

    struct TransferData {
        libusb_transfer* transfer;
        unsigned char* buffer;
    };

    std::vector<TransferData> transfers(NUM_TRANSFERS);

    auto callback = [](libusb_transfer* transfer) {
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
            libusb_submit_transfer(transfer);
        }
        else {
            std::cerr << "Transfer error: " << transfer->status << std::endl;
        }
        };

    //init transfers
    for (int i = 0; i < NUM_TRANSFERS; i++) {
        transfers[i].buffer = new unsigned char[TRANSFER_SIZE];
        transfers[i].transfer = libusb_alloc_transfer(0);

        if (!transfers[i].transfer || !transfers[i].buffer) {
            std::cerr << "Failed to allocate transfer resources" << std::endl;
            for (int j = 0; j <= i; j++) {
                if (transfers[j].transfer) libusb_free_transfer(transfers[j].transfer);
                delete[] transfers[j].buffer;
            }
            libusbShutdown(deviceHandle, interfaceNum, ctx);
            return 1;
        }

        libusb_fill_bulk_transfer(transfers[i].transfer, deviceHandle, endpointAddr,
            transfers[i].buffer, TRANSFER_SIZE, callback, nullptr, 0);

        int rc = libusb_submit_transfer(transfers[i].transfer);
        if (rc != LIBUSB_SUCCESS) {
            std::cerr << "Failed to submit transfer: " << libusb_error_name(rc) << std::endl;
            for (int j = 0; j <= i; j++) {
                libusb_free_transfer(transfers[j].transfer);
                delete[] transfers[j].buffer;
            }
            libusbShutdown(deviceHandle, interfaceNum, ctx);
            return 1;
        }
    }

    //handle events in a separate thread
    std::atomic<bool> running(true);
    std::thread eventThread([&]() {
        while (running) {
            int rc = libusb_handle_events(ctx);
            if (rc != LIBUSB_SUCCESS) {
                std::cerr << "Event handling error: " << libusb_error_name(rc) << std::endl;
                running = false;
            }
        }
        });

    std::cout << "Receiving data..." << std::endl;
    std::cin.ignore();
    std::cout << "Stopped" << std::endl;

    running = false;
    eventThread.join();

    //cancel and free transfers
    for (auto& t : transfers) {
        libusb_cancel_transfer(t.transfer);
        while (t.transfer->status == LIBUSB_TRANSFER_CANCELLED ||
            t.transfer->status == LIBUSB_TRANSFER_COMPLETED) {
            libusb_handle_events_timeout(ctx, nullptr);
        }
        libusb_free_transfer(t.transfer);
        delete[] t.buffer;
    }

    //only call when program will be exiting
    libusbShutdown(deviceHandle, interfaceNum, ctx);

    return 0;
}