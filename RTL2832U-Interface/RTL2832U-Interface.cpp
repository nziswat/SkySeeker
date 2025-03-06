//////////////////////////////////////////////////////////////////
//                    RTL2832-Interface                         //
//                                                              //
// - libusb will be statically linked                           //
// - VID and PID are hardcoded for the generic RTL2832U chip    //
// - Makes use of exported rtlsdr.dll functions                 //
//////////////////////////////////////////////////////////////////

#include <iostream>
#include <windows.h>
#include "rtl-sdr.h"
#include <iomanip>

typedef int (*RtlSdrOpen)(rtlsdr_dev_t**, uint32_t);
typedef int (*RtlSdrClose)(rtlsdr_dev_t*);
typedef int (*RtlSdrSetSampleRate)(rtlsdr_dev_t*, uint32_t);
typedef int (*RtlSdrSetCenterFreq)(rtlsdr_dev_t*, uint32_t);
typedef int (*RtlSdrSetDirectSampling)(rtlsdr_dev_t*, int);
typedef int (*RtlSdrSetTunerGainMode)(rtlsdr_dev_t*, int);
typedef int (*RtlSdrResetBuffer)(rtlsdr_dev_t*);
typedef int (*RtlSdrReadSync)(rtlsdr_dev_t*, void*, int, int*);

int main() {
    HMODULE hDLL = LoadLibrary(L"rtlsdr.dll");
    if (!hDLL) {
        std::cerr << "Failed to load rtlsdr.dll!" << std::endl;
        return 1;
    }

    //func pointers
    RtlSdrOpen rtlsdr_open = (RtlSdrOpen)GetProcAddress(hDLL, "rtlsdr_open");
    RtlSdrClose rtlsdr_close = (RtlSdrClose)GetProcAddress(hDLL, "rtlsdr_close");
    RtlSdrSetSampleRate rtlsdr_set_sample_rate = (RtlSdrSetSampleRate)GetProcAddress(hDLL, "rtlsdr_set_sample_rate");
    RtlSdrSetCenterFreq rtlsdr_set_center_freq = (RtlSdrSetCenterFreq)GetProcAddress(hDLL, "rtlsdr_set_center_freq");
    RtlSdrSetDirectSampling rtlsdr_set_direct_sampling = (RtlSdrSetDirectSampling)GetProcAddress(hDLL, "rtlsdr_set_direct_sampling");
    RtlSdrSetTunerGainMode rtlsdr_set_tuner_gain_mode = (RtlSdrSetTunerGainMode)GetProcAddress(hDLL, "rtlsdr_set_tuner_gain_mode");
    RtlSdrResetBuffer rtlsdr_reset_buffer = (RtlSdrResetBuffer)GetProcAddress(hDLL, "rtlsdr_reset_buffer");
    RtlSdrReadSync rtlsdr_read_sync = (RtlSdrReadSync)GetProcAddress(hDLL, "rtlsdr_read_sync");

    if (!rtlsdr_open || !rtlsdr_close || !rtlsdr_set_sample_rate || !rtlsdr_set_center_freq ||
        !rtlsdr_set_direct_sampling || !rtlsdr_set_tuner_gain_mode || !rtlsdr_reset_buffer || !rtlsdr_read_sync) {
        std::cerr << "Failed to get function addresses!" << std::endl;
        FreeLibrary(hDLL);
        return 1;
    }

    //get first rtl device
    rtlsdr_dev_t* dev = nullptr;
    if (rtlsdr_open(&dev, 0) < 0) {
        std::cerr << "Failed to open RTL-SDR device" << std::endl;
        FreeLibrary(hDLL);
        return 1;
    }

    std::cout << "RTL-SDR device opened successfully!" << std::endl;

    //set gain: 0 = auto gain
    rtlsdr_set_tuner_gain_mode(dev, 0);

    //set sample rate
    if (rtlsdr_set_sample_rate(dev, 2000000) < 0) {
        std::cerr << "Failed to set sample rate" << std::endl;
        rtlsdr_close(dev);
        FreeLibrary(hDLL);
        return 1;
    }

    //set frequency to 1090 MHz (ADS-B)
    if (rtlsdr_set_center_freq(dev, 1090000000) < 0) {
        std::cerr << "Failed to set center frequency" << std::endl;
        rtlsdr_close(dev);
        FreeLibrary(hDLL);
        return 1;
    }

    //reset buffer before read
    if (rtlsdr_reset_buffer(dev) < 0) {
        std::cerr << "Failed to reset buffer" << std::endl;
        rtlsdr_close(dev);
        FreeLibrary(hDLL);
        return 1;
    }

    ////////////////////////////////
    //  Disable Direct Sampling
    ////////////////////////////////
    rtlsdr_set_direct_sampling(dev, 0);
    int direct_sampling_mode = -1;
    typedef int (*RtlSdrGetDirectSampling)(rtlsdr_dev_t*);
    RtlSdrGetDirectSampling rtlsdr_get_direct_sampling = (RtlSdrGetDirectSampling)GetProcAddress(hDLL, "rtlsdr_get_direct_sampling");
    if (rtlsdr_get_direct_sampling) {
        direct_sampling_mode = rtlsdr_get_direct_sampling(dev);
        std::cout << "Direct sampling mode: " << direct_sampling_mode << std::endl;
    }
    if (direct_sampling_mode != 0) {
        std::cerr << "Direct sampling mode is still enabled :/" << std::endl;
        rtlsdr_set_direct_sampling(dev, 0);
    }

    ////////////////////////////////
    //  Reading Raw Data
    ////////////////////////////////
    unsigned char buffer[1024];
    int bytes_read;
    if (rtlsdr_read_sync(dev, buffer, sizeof(buffer), &bytes_read) < 0) {
        std::cerr << "Failed to read from device" << std::endl;
    }
    else {
        std::cout << "Read " << bytes_read << " bytes of raw IQ data" << std::endl;
        for (int i = 0; i < bytes_read; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << " ";
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl;
            }
        }
    }

    rtlsdr_close(dev);
    FreeLibrary(hDLL);
    return 0;
}
