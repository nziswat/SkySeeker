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

//these should all stay constant, but they make the code easier to understand
#define PREAMBLE 8
#define LONG_MESSAGE 112 
#define SHORT_MESSAGE 56
#define FULL_LENGTH (PREAMBLE+LONG_MESSAGE)
#define LONG_MESSAGE_BYTES (LONG_MESSAGE/8)
#define SHORT_MESSAGE_BYTES (SHORT_MESSAGE/8)

//these definitions could possibly be configured
#define BUFFER_SIZE 1024


struct { //adapted from dump1090
    unsigned char* data;            /* Raw IQ samples buffer */
    uint16_t* magnitude;            /* Magnitude vector */
    int data_ready;                 /* Data ready to be processed. */
    uint32_t* icao_cache;           /* Recently seen ICAO addresses cache. */
    uint16_t* maglut;               /* I/Q -> Magnitude lookup table. */
}dataloop ;


void calculateMag();

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

    //populate the magnitude look up table, which basically means we don't have to sqrt everything later == optimization
    dataloop.maglut = new uint16_t[129 * 129 * 2];
    for (int i = 0; i <= 128; i++) {
        for (int q = 0; q <= 128; q++) {
            dataloop.maglut[i * 129 + q] = round(sqrt(i * i + q * q) * 360);
        }
    }

    //allocate memory to data, buffer
    dataloop.data = new unsigned char[BUFFER_SIZE];
    dataloop.magnitude = new uint16_t[BUFFER_SIZE / 2];

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
    unsigned char buffer[BUFFER_SIZE];
    int bytes_read;
    if (rtlsdr_read_sync(dev, buffer, sizeof(buffer), &bytes_read) < 0) {
        std::cerr << "Failed to read from device" << std::endl;
    }
    else {
        /*
        std::cout << "Read " << bytes_read << " bytes of raw IQ data" << std::endl;
        for (int i = 0; i < bytes_read; ++i) {
            std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << " ";
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl;
            }
        }*/

        std::memcpy(dataloop.data, buffer, BUFFER_SIZE);
        calculateMag();
        std::cout << buffer;
        std::cout << "\n\n\n";
        std::cout << dataloop.magnitude;

    }

    

    rtlsdr_close(dev);
    FreeLibrary(hDLL);
    return 0;
}

//takes I/Q from driver and converts to magnitude
void calculateMag() {
    uint16_t* m = dataloop.magnitude;
    unsigned char* p = dataloop.data;
    uint32_t j;

    /* Compute the magnitude vector. It's just SQRT(I^2 + Q^2), but
     * we rescale to the 0-255 range to exploit the full resolution. */
    for (j = 0; j < BUFFER_SIZE; j += 2) {
        int i = p[j] - 127;
        int q = p[j + 1] - 127;

        if (i < 0) i = -i;
        if (q < 0) q = -q;
        m[j / 2] = dataloop.maglut[i * 129 + q];
    }
}



// tries to find a ModeS message within magnitude buffer 'm' of 'mlen' size bytes.
void detectModeS(uint16_t* m, uint32_t mlen) {
}
