//////////////////////////////////////////////////////////////////
//                    RTL2832-Interface                         //
//                                                              //
// - libusb will be statically linked                           //
// - VID and PID are hardcoded for the generic RTL2832U chip    //
// - Makes use of exported rtlsdr.dll functions                 //
//////////////////////////////////////////////////////////////////

#include <iostream>
#include <windows.h>
#include "src/rtl-sdr.h"
#include <iomanip>
#include "src/structs.h"
#include "src/message_handler.h"
#include "include/cef_callback.h"
#include "include/base/cef_bind.h"   // For base::BindOnce
#include "include/cef_task.h"        // For CefPostTask
#include "include/wrapper/cef_closure_task.h" // For CefCreateClosureTask
#include "include/cef_v8.h"
#include <sstream>
#include <src/json.h>
#include "RTL_interface.h"

std::string dlabel = "Driver";

using json = nlohmann::json;
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
#define ICAO_LEN 7 //includes \0

//these definitions could possibly be configured
#define BUFFER_SIZE 1024 * 32

//make the loop run forever?
#define BULK_TIMEOUT 0

//struct to proccess modeS
struct { //adapted from dump1090
    unsigned char* data;            /* Raw IQ samples buffer */
    uint16_t* magnitude;            /* Magnitude vector */
    int data_ready;                 /* Data ready to be processed. */
    uint32_t* icao_cache;           /* Recently seen ICAO addresses cache. */
    uint16_t* maglut;               /* I/Q -> Magnitude lookup table. */
}dataloop ;

//struct for decrypted message
uint32_t modes_checksum_table[112] = { // used to verify integrity of packet
0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,
0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,
0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,
0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,
0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,
0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,
0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,
0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,
0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,
0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,
0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};

/* Capability table. */
const char* ca_str[8] = {
    /* 0 */ "Level 1 (Survillance Only)",
    /* 1 */ "Level 2 (DF0,4,5,11)",
    /* 2 */ "Level 3 (DF0,4,5,11,20,21)",
    /* 3 */ "Level 4 (DF0,4,5,11,20,21,24)",
    /* 4 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on ground)",
    /* 5 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on airborne)",
    /* 6 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7)",
    /* 7 */ "Level 7 ???"
};

/* Flight status table. */
const char* fs_str[8] = {
    /* 0 */ "Normal, Airborne",
    /* 1 */ "Normal, On the ground",
    /* 2 */ "ALERT,  Airborne",
    /* 3 */ "ALERT,  On the ground",
    /* 4 */ "ALERT & Special Position Identification. Airborne or Ground",
    /* 5 */ "Special Position Identification. Airborne or Ground",
    /* 6 */ "Value 6 is not assigned",
    /* 7 */ "Value 7 is not assigned"
};

MessageHandler* messageHandler;


void calculateMag();
void detectModeS(uint16_t* m, uint32_t mlen);
int modesMessageLenByType(int type);
uint32_t modesChecksum(unsigned char* msg, int bits);
void decodeModesMessage(struct modesMessage* mm, unsigned char* msg);
int decodeAC12Field(unsigned char* msg, int* unit);
int decodeAC13Field(unsigned char* msg, int* unit);
void displayModesMessage(struct modesMessage* mm);
void sendModesData(modesMessage& mm);
const char* getMEDescription(int metype, int mesub);
int fixTwoBitsErrors(unsigned char* msg, int bits);

void PostCefTask(MessageHandler* NewMessageHandler, const modesMessage& mm) {
    json root;

    // format the ICAO address as a hex string
    char icaoStr[ICAO_LEN];
    snprintf(icaoStr, ICAO_LEN, "%02X%02X%02X", mm.aa1, mm.aa2, mm.aa3);
    root["icao"] = icaoStr;

    // populate JSON based on msgtype
    if (mm.msgtype == 0) {
        root["altitude"] = mm.altitude;
        root["unit"] = mm.unit;
    }
    else if (mm.msgtype == 4 || mm.msgtype == 20) {
        root["flight_status"] = fs_str[mm.fs];
        root["dr"] = mm.dr;
        root["um"] = mm.um;
        root["altitude"] = mm.altitude;
        root["unit"] = mm.unit;
    }
    else if (mm.msgtype == 5 || mm.msgtype == 21) {
        root["flight_status"] = fs_str[mm.fs];
        root["dr"] = mm.dr;
        root["um"] = mm.um;
        root["squawk"] = mm.identity;
    }
    else if (mm.msgtype == 11) {
        root["capability"] = ca_str[mm.ca];
    }
    else if (mm.msgtype == 17) {
        root["capability"] = ca_str[mm.ca];
        root["metype"] = mm.metype;
        root["mesub"] = mm.mesub;
        root["description"] = getMEDescription(mm.metype, mm.mesub);

        if (mm.metype >= 1 && mm.metype <= 4) {
            root["flight"] = mm.flight;
        }
        else if (mm.metype >= 9 && mm.metype <= 18) {
            root["fflag"] = mm.fflag;
            root["tflag"] = mm.tflag;
            root["altitude"] = mm.altitude;
            root["raw_latitude"] = mm.raw_latitude;
            root["raw_longitude"] = mm.raw_longitude;
        }
        else if (mm.metype == 19 && mm.mesub >= 1 && mm.mesub <= 4) {
            if (mm.mesub == 1 || mm.mesub == 2) {
                root["ew_dir"] = mm.ew_dir;
                root["ew_velocity"] = mm.ew_velocity;
                root["ns_dir"] = mm.ns_dir;
                root["ns_velocity"] = mm.ns_velocity;
                root["vert_rate_source"] = mm.vert_rate_source;
                root["vert_rate_sign"] = mm.vert_rate_sign;
                root["vert_rate"] = mm.vert_rate;
            }
            else if (mm.mesub == 3 || mm.mesub == 4) {
                root["heading_is_valid"] = mm.heading_is_valid;
                root["heading"] = mm.heading;
            }
        }
    }

    // Convert JSON object to string
    std::string jsonMessage = root.dump();

    // Send to CEF UI thread
    auto task = new MyCefTask(NewMessageHandler, jsonMessage);
    CefPostTask(TID_UI, task);
}

void labelPrint(std::string msg) {
    if (messageHandler != nullptr) {
        messageHandler->sendDebug("[" + dlabel + "] " + msg);
    }
}

int runRTL(MessageHandler* NewMessageHandler) {
    messageHandler = NewMessageHandler; 
    HMODULE hDLL = LoadLibrary(L"rtlsdr.dll");
    if (!hDLL) {
        labelPrint("Failed to load rtlsdr.dll!");
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
        labelPrint("Failed to get function addresses!");
        FreeLibrary(hDLL);
        return 1;
    }

    //get first rtl device
    rtlsdr_dev_t* dev = nullptr;
    if (rtlsdr_open(&dev, 0) < 0) {
        labelPrint("Failed to open RTL-SDR device");
        FreeLibrary(hDLL);
        return 1;
    }

    //std::cout << "RTL-SDR device opened successfully!" << std::endl;
    labelPrint("RTL-SDR device opened successfully");

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
        labelPrint("Failed to reset buffer");
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
    unsigned char buffer[BUFFER_SIZE]; //this is huge on the stack. TODO move to heap
    int bytes_read;
    exitDriverThread = false;

    while (true) {
        if (rtlsdr_read_sync(dev, buffer, sizeof(buffer), &bytes_read) < 0) {
            labelPrint("Failed to read from device");
        }
        std::memcpy(dataloop.data, buffer, BUFFER_SIZE);
        calculateMag();
        detectModeS(dataloop.magnitude, BUFFER_SIZE / 2);
        exitDriverThread = messageHandler->driverStatus;
        if (exitDriverThread) {
            labelPrint("Shutting Down LibUSB");
            break;
        }
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

int detectOutOfPhase(uint16_t* m) {
    if (m[3] > m[2] / 3) return 1;
    if (m[10] > m[9] / 3) return 1;
    if (m[6] > m[7] / 3) return -1;
    if (m[-1] > m[1] / 3) return -1;
    return 0;
}

void applyPhaseCorrection(uint16_t* m) {
    int j;
    
    m += 16; /* Skip preamble. */
    for (j = 0; j < (LONG_MESSAGE - 1) * 2; j += 2) {
        if (m[j] > m[j + 1]) {
            /* One */
            m[j + 2] = (m[j + 2] * 5) / 4;
        }
        else {
            /* Zero */
            m[j + 2] = (m[j + 2] * 4) / 5;
        }
    }
}

// tries to find a ModeS message within magnitude buffer 'm' of 'mlen' size bytes (which should be BUFFER_SIZE/2 anyway).
void detectModeS(uint16_t* m, uint32_t mlen) {
    unsigned char bits[LONG_MESSAGE];
    unsigned char msg[LONG_MESSAGE / 2];
    uint16_t aux[LONG_MESSAGE * 2];
    uint32_t j;

    for (j = 0; j < mlen - FULL_LENGTH * 2; j++) {
        int low, high, delta, i, errors;
        //int good_message = 0;
        //not used ^

        //first step is to try and find a preamble within the buffer

        /* First check of relations between the first 10 samples
         * representing a valid preamble. We don't even investigate further
         * if this simple test is not passed. */
        if (!(m[j] > m[j + 1] &&
            m[j + 1] < m[j + 2] &&
            m[j + 2] > m[j + 3] &&
            m[j + 3] < m[j] &&
            m[j + 4] < m[j] &&
            m[j + 5] < m[j] &&
            m[j + 6] < m[j] &&
            m[j + 7] > m[j + 8] &&
            m[j + 8] < m[j + 9] &&
            m[j + 9] > m[j + 6]))
        {
            continue;
        }

        /* The samples between the two spikes must be < than the average
         * of the high spikes level. We don't test bits too near to
         * the high levels as signals can be out of phase so part of the
         * energy can be in the near samples. */
        high = (m[j] + m[j + 2] + m[j + 7] + m[j + 9]) / 6;
        if (m[j + 4] >= high ||
            m[j + 5] >= high)
        {
            continue;
        }

        /* Similarly samples in the range 11-14 must be low, as it is the
         * space between the preamble and real data. Again we don't test
         * bits too near to high levels, see above. */
        if (m[j + 11] >= high ||
            m[j + 12] >= high ||
            m[j + 13] >= high ||
            m[j + 14] >= high)
        {
            continue;
        }

        //At this point, we probably have a valid preamble

        //magnitude (phase) correction
        memcpy(aux, m + j + PREAMBLE * 2, sizeof(aux));
        if (j && detectOutOfPhase(m + j)) {
            applyPhaseCorrection(m + j);
        }

        /* Decode all the next 112 bits, regardless of the actual message
         * size. We'll check the actual message type later. */
        errors = 0;
        for (i = 0; i < LONG_MESSAGE * 2; i += 2) {
            low = m[j + i + PREAMBLE * 2];
            high = m[j + i + PREAMBLE * 2 + 1];
            delta = low - high;
            if (delta < 0) delta = -delta;

            if (i > 0 && delta < 256) {
                bits[i / 2] = bits[i / 2 - 1];
            }
            else if (low == high) {
                /* Checking if two adiacent samples have the same magnitude
                 * is an effective way to detect if it's just random noise
                 * that was detected as a valid preamble. */
                bits[i / 2] = 2; /* error */
                if (i < SHORT_MESSAGE * 2) errors++;
            }
            else if (low > high) {
                bits[i / 2] = 1;
            }
            else {
                /* (low < high) for exclusion  */
                bits[i / 2] = 0;
            }
        }

        /* Pack bits into bytes */
        for (i = 0; i < LONG_MESSAGE; i += 8) {
            msg[i / 8] =
                bits[i] << 7 |
                bits[i + 1] << 6 |
                bits[i + 2] << 5 |
                bits[i + 3] << 4 |
                bits[i + 4] << 3 |
                bits[i + 5] << 2 |
                bits[i + 6] << 1 |
                bits[i + 7];
        }

        //after it's all said and done, decode the message
        struct modesMessage mm{};
        decodeModesMessage(&mm, msg);
       // displayModesMessage(&mm);
        sendModesData(mm);
    }
}

/* Try to fix single bit errors using the checksum. On success modifies
 * the original buffer with the fixed version, and returns the position
 * of the error bit. Otherwise if fixing failed -1 is returned. */
int fixSingleBitErrors(unsigned char* msg, int bits) {
    int j;
    unsigned char aux[LONG_MESSAGE / 8];

    for (j = 0; j < bits; j++) {
        int byte = j / 8;
        int bitmask = 1 << (7 - (j % 8));
        uint32_t crc1, crc2;

        memcpy(aux, msg, bits / 8);
        aux[byte] ^= bitmask; /* Flip j-th bit. */

        crc1 = ((uint32_t)aux[(bits / 8) - 3] << 16) |
            ((uint32_t)aux[(bits / 8) - 2] << 8) |
            (uint32_t)aux[(bits / 8) - 1];
        crc2 = modesChecksum(aux, bits);

        if (crc1 == crc2) {
            /* The error is fixed. Overwrite the original buffer with
             * the corrected sequence, and returns the error bit
             * position. */
            memcpy(msg, aux, bits / 8);
            return j;
        }
    }
    return -1;
}

/* Similar to fixSingleBitErrors() but try every possible two bit combination.
 * This is very slow and should be tried only against DF17 messages that
 * don't pass the checksum, and only in Aggressive Mode. */
int fixTwoBitsErrors(unsigned char* msg, int bits) {
    int j, i;
    unsigned char aux[LONG_MESSAGE / 8];

    for (j = 0; j < bits; j++) {
        int byte1 = j / 8;
        int bitmask1 = 1 << (7 - (j % 8));

        /* Don't check the same pairs multiple times, so i starts from j+1 */
        for (i = j + 1; i < bits; i++) {
            int byte2 = i / 8;
            int bitmask2 = 1 << (7 - (i % 8));
            uint32_t crc1, crc2;

            memcpy(aux, msg, bits / 8);

            aux[byte1] ^= bitmask1; /* Flip j-th bit. */
            aux[byte2] ^= bitmask2; /* Flip i-th bit. */

            crc1 = ((uint32_t)aux[(bits / 8) - 3] << 16) |
                ((uint32_t)aux[(bits / 8) - 2] << 8) |
                (uint32_t)aux[(bits / 8) - 1];
            crc2 = modesChecksum(aux, bits);

            if (crc1 == crc2) {
                /* The error is fixed. Overwrite the original buffer with
                 * the corrected sequence, and returns the error bit
                 * position. */
                //labelPrint("fixTwoBitsErrors repaired packet");
                memcpy(msg, aux, bits / 8);
                /* We return the two bits as a 16 bit integer by shifting
                 * 'i' on the left. This is possible since 'i' will always
                 * be non-zero because i starts from j+1. */
                return j | (i << 8);
            }
        }
    }
    return -1;
}

void decodeModesMessage(struct modesMessage* mm, unsigned char* msg) {
    uint32_t crc2;   /* Computed CRC, used to verify the message CRC. */
    const char* ais_charset = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";


    /* Work on our local copy */
    memcpy(mm->msg, msg, LONG_MESSAGE);
    msg = mm->msg;

    /* Get the message type ASAP as other operations depend on this */
    mm->msgtype = msg[0] >> 3;    /* Downlink Format */
    mm->msgbits = modesMessageLenByType(mm->msgtype);

    /* CRC is always the last three bytes. */
    mm->crc = ((uint32_t)msg[(mm->msgbits / 8) - 3] << 16) |
        ((uint32_t)msg[(mm->msgbits / 8) - 2] << 8) |
        (uint32_t)msg[(mm->msgbits / 8) - 1];
    crc2 = modesChecksum(msg, mm->msgbits);

    /* Check CRC and fix single bit errors using the CRC when
     * possible (DF 11 and 17). */
    mm->errorbit = -1;  /* No error */
    mm->crcok = (mm->crc == crc2);

    //error fixing
    if (!mm->crcok && (mm->msgtype == 11 || mm->msgtype == 17)) {
        if ((mm->errorbit = fixSingleBitErrors(msg, mm->msgbits)) != -1) {
            mm->crc = modesChecksum(msg, mm->msgbits);
            mm->crcok = 1;
        }
        else if (mm->msgtype == 17 && (mm->errorbit = fixTwoBitsErrors(msg, mm->msgbits)) != -1) {
            mm->crc = modesChecksum(msg, mm->msgbits);
            mm->crcok = 1;
        }
    }
    

    /* Note that most of the other computation happens *after* we fix
     * the single bit errors, otherwise we would need to recompute the
     * fields again. */
    mm->ca = msg[0] & 7;        /* Responder capabilities. */

    /* ICAO address */
    mm->aa1 = msg[1];
    mm->aa2 = msg[2];
    mm->aa3 = msg[3];

    /* DF 17 type (assuming this is a DF17, otherwise not used) */
    mm->metype = msg[4] >> 3;   /* Extended squitter message type. */
    mm->mesub = msg[4] & 7;     /* Extended squitter message subtype. */

    /* Fields for DF4,5,20,21 */
    mm->fs = msg[0] & 7;        /* Flight status for DF4,5,20,21 */
    mm->dr = msg[1] >> 3 & 31;  /* Request extraction of downlink request. */
    mm->um = ((msg[1] & 7) << 3) | /* Request extraction of downlink request. */
        msg[2] >> 5;

    /* In the squawk (identity) field bits are interleaved like that
     * (message bit 20 to bit 32):
     *
     * C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
     *
     * So every group of three bits A, B, C, D represent an integer
     * from 0 to 7.
     *
     * The actual meaning is just 4 octal numbers, but we convert it
     * into a base ten number tha happens to represent the four
     * octal numbers.
     *
     * For more info: http://en.wikipedia.org/wiki/Gillham_code */
    {
        int a, b, c, d;

        a = ((msg[3] & 0x80) >> 5) |
            ((msg[2] & 0x02) >> 0) |
            ((msg[2] & 0x08) >> 3);
        b = ((msg[3] & 0x02) << 1) |
            ((msg[3] & 0x08) >> 2) |
            ((msg[3] & 0x20) >> 5);
        c = ((msg[2] & 0x01) << 2) |
            ((msg[2] & 0x04) >> 1) |
            ((msg[2] & 0x10) >> 4);
        d = ((msg[3] & 0x01) << 2) |
            ((msg[3] & 0x04) >> 1) |
            ((msg[3] & 0x10) >> 4);
        mm->identity = a * 1000 + b * 100 + c * 10 + d;
    }

    /* TODO: DF 13 and 17

    
    /* Decode 13 bit altitude for DF0, DF4, DF16, DF20 */
    if (mm->msgtype == 0 || mm->msgtype == 4 ||
        mm->msgtype == 16 || mm->msgtype == 20) {
        mm->altitude = decodeAC13Field(msg, &mm->unit);
    }

    /* Decode extended squitter specific stuff. */
    if (mm->msgtype == 17) {
        /* Decode the extended squitter message. */

        if (mm->metype >= 1 && mm->metype <= 4) {
            /* Aircraft Identification and Category */
            mm->aircraft_type = mm->metype - 1;
            mm->flight[0] = ais_charset[msg[5] >> 2];
            mm->flight[1] = ais_charset[((msg[5] & 3) << 4) | (msg[6] >> 4)];
            mm->flight[2] = ais_charset[((msg[6] & 15) << 2) | (msg[7] >> 6)];
            mm->flight[3] = ais_charset[msg[7] & 63];
            mm->flight[4] = ais_charset[msg[8] >> 2];
            mm->flight[5] = ais_charset[((msg[8] & 3) << 4) | (msg[9] >> 4)];
            mm->flight[6] = ais_charset[((msg[9] & 15) << 2) | (msg[10] >> 6)];
            mm->flight[7] = ais_charset[msg[10] & 63];
            mm->flight[8] = '\0';
        }
        else if (mm->metype >= 9 && mm->metype <= 18) {
            /* Airborne position Message */
            mm->fflag = msg[6] & (1 << 2);
            mm->tflag = msg[6] & (1 << 3);
            mm->altitude = decodeAC12Field(msg, &mm->unit);
            mm->raw_latitude = ((msg[6] & 3) << 15) |
                (msg[7] << 7) |
                (msg[8] >> 1);
            mm->raw_longitude = ((msg[8] & 1) << 16) |
                (msg[9] << 8) |
                msg[10];
        }
        else if (mm->metype == 19 && mm->mesub >= 1 && mm->mesub <= 4) {
            /* Airborne Velocity Message */
            if (mm->mesub == 1 || mm->mesub == 2) {
                mm->ew_dir = (msg[5] & 4) >> 2;
                mm->ew_velocity = ((msg[5] & 3) << 8) | msg[6];
                mm->ns_dir = (msg[7] & 0x80) >> 7;
                mm->ns_velocity = ((msg[7] & 0x7f) << 3) | ((msg[8] & 0xe0) >> 5);
                mm->vert_rate_source = (msg[8] & 0x10) >> 4;
                mm->vert_rate_sign = (msg[8] & 0x8) >> 3;
                mm->vert_rate = ((msg[8] & 7) << 6) | ((msg[9] & 0xfc) >> 2);
                /* Compute velocity and angle from the two speed
                 * components. */
                mm->velocity = sqrt(mm->ns_velocity * mm->ns_velocity +
                    mm->ew_velocity * mm->ew_velocity);
                if (mm->velocity) {
                    int ewv = mm->ew_velocity;
                    int nsv = mm->ns_velocity;
                    double heading;

                    if (mm->ew_dir) ewv *= -1;
                    if (mm->ns_dir) nsv *= -1;
                    heading = atan2(ewv, nsv);

                    /* Convert to degrees. */
                    mm->heading = heading * 360 / (3.1415 * 2); //TODO: beter PI
                    /* We don't want negative values but a 0-360 scale. */
                    if (mm->heading < 0) mm->heading += 360;
                }
                else {
                    mm->heading = 0;
                }
            }
            else if (mm->mesub == 3 || mm->mesub == 4) {
                mm->heading_is_valid = msg[5] & (1 << 2);
                mm->heading = (360.0 / 128) * (((msg[5] & 3) << 5) |
                    (msg[6] >> 3));
            }
        }
    }
    mm->phase_corrected = 0; /* Set to 1 by the caller if needed. */
}



uint32_t modesChecksum(unsigned char* msg, int bits) {
    uint32_t crc = 0;
    int offset = (bits == 112) ? 0 : (112 - 56);
    int j;

    for (j = 0; j < bits; j++) {
        int byte = j / 8;
        int bit = j % 8;
        int bitmask = 1 << (7 - bit);

        /* If bit is set, xor with corresponding table entry. */
        if (msg[byte] & bitmask)
            crc ^= modes_checksum_table[j + offset];
    }
    return crc; /* 24 bit checksum. */
}

/* Given the Downlink Format (DF) of the message, return the message length
 * in bits. */

int modesMessageLenByType(int type) {
    if (type == 16 || type == 17 ||
        type == 19 || type == 20 ||
        type == 21)
        return LONG_MESSAGE;
    else
        return SHORT_MESSAGE;
}

/* Decode the 13 bit AC altitude field (in DF 20 and others).
 * Returns the altitude, and set 'unit' to either MODES_UNIT_METERS
 * or MDOES_UNIT_FEETS. */
int decodeAC13Field(unsigned char* msg, int* unit) {
    int m_bit = msg[3] & (1 << 6);
    int q_bit = msg[3] & (1 << 4);

    if (!m_bit) {
        *unit = 0;
        if (q_bit) {
            /* N is the 11 bit integer resulting from the removal of bit
             * Q and M */
            int n = ((msg[2] & 31) << 6) |
                ((msg[3] & 0x80) >> 2) |
                ((msg[3] & 0x20) >> 1) |
                (msg[3] & 15);
            /* The final altitude is due to the resulting number multiplied
             * by 25, minus 1000. */
            return n * 25 - 1000;
        }
        else {
            /* TODO: Implement altitude where Q=0 and M=0 */
        }
    }
    else {
        *unit = 1;
        /* TODO: Implement altitude when meter unit is selected. */
    }
    return 0;
}

/* Decode the 12 bit AC altitude field (in DF 17 and others).
 * Returns the altitude or 0 if it can't be decoded. */
int decodeAC12Field(unsigned char* msg, int* unit) {
    int q_bit = msg[5] & 1;

    if (q_bit) {
        /* N is the 11 bit integer resulting from the removal of bit
         * Q */
        *unit = 0;
        int n = ((msg[5] >> 1) << 4) | ((msg[6] & 0xF0) >> 4);
        /* The final altitude is due to the resulting number multiplied
         * by 25, minus 1000. */
        return n * 25 - 1000;
    }
    else {
        return 0;
    }
}

const char* getMEDescription(int metype, int mesub) {
    const char* mename = "Unknown";

    if (metype >= 1 && metype <= 4)
        mename = "Aircraft Identification and Category";
    else if (metype >= 5 && metype <= 8)
        mename = "Surface Position";
    else if (metype >= 9 && metype <= 18)
        mename = "Airborne Position (Baro Altitude)";
    else if (metype == 19 && mesub >= 1 && mesub <= 4)
        mename = "Airborne Velocity";
    else if (metype >= 20 && metype <= 22)
        mename = "Airborne Position (GNSS Height)";
    else if (metype == 23 && mesub == 0)
        mename = "Test Message";
    else if (metype == 24 && mesub == 1)
        mename = "Surface System Status";
    else if (metype == 28 && mesub == 1)
        mename = "Extended Squitter Aircraft Status (Emergency)";
    else if (metype == 28 && mesub == 2)
        mename = "Extended Squitter Aircraft Status (1090ES TCAS RA)";
    else if (metype == 29 && (mesub == 0 || mesub == 1))
        mename = "Target State and Status Message";
    else if (metype == 31 && (mesub == 0 || mesub == 1))
        mename = "Aircraft Operational Status Message";
    return mename;
}



//use this function to send data out of this hellhole
void sendModesData(modesMessage& mm) {
    //CRC failure
    if (!mm.crcok) {
        return;
    }

    //all valid modes data should include this
    char* icaoStr = (char*)malloc(ICAO_LEN);
    if (!icaoStr) {
        std::cout << "OUT OF MEMORY" << std::endl;
        return;
    }

    sprintf_s(icaoStr, ICAO_LEN, "%02x%02x%02x", mm.aa1, mm.aa2, mm.aa3);
    //std::cout << icaoStr << std::endl;


    //TODO: for each of these, i am just leaving what data can be taken from each condition cuz there is no connection between here and CEF yet
    
    //Mr. Kevbo, set up a function in ur CEF shit so that we can call it from here and send the data
    //to CEF

    //Also if you think it would be easier if i just made a struct and filled in what we need i can do that. Then the CEF would just take the struct and use whatever data is filled in

    //Short Air-Air Surveillance
    if (mm.msgtype == 0) {
        
        //send altitude
        mm.altitude;
        mm.unit; //units of measurement (meters [0] or feet [1])
    }
    else if (mm.msgtype == 4 || mm.msgtype == 20) {
        fs_str[mm.fs]; //Flight Status
        mm.dr; //idk what this is
        mm.um; //ummmm
        mm.altitude;
        mm.unit;
    }
    else if (mm.msgtype == 5 || mm.msgtype == 21) {
        fs_str[mm.fs];
        mm.dr;
        mm.um;
        mm.identity; //squawk
    }
    else if (mm.msgtype == 11) {
        ca_str[mm.ca]; //capability
    }
    else if (mm.msgtype == 17) {
        ca_str[mm.ca]; //capability
        mm.metype; //extended squitter type
        mm.mesub; //extended squitter sub
        getMEDescription(mm.metype, mm.mesub); //extended squitter name


        /* Decode the extended squitter message. */
        if (mm.metype >= 1 && mm.metype <= 4) {
            const char* ac_type_str[4] = {
                "Aircraft Type D",
                "Aircraft Type C",
                "Aircraft Type B",
                "Aircraft Type A"
            };
            mm.flight; //identification
        }
        else if (mm.metype >= 9 && mm.metype <= 18) {
            mm.fflag; //F flag; even or odd
            mm.tflag; //T flag UTC or non-UTC
            mm.altitude;
            mm.raw_latitude; //latitude (not decoded)
            mm.raw_longitude; //longitude (not decoded)
        }
        else if (mm.metype == 19 && mm.mesub >= 1 && mm.mesub <= 4) {
            if (mm.mesub == 1 || mm.mesub == 2) {
                //Velocity stuff
                mm.ew_dir; //EW direction
                mm.ew_velocity; //EW velocity
                mm.ns_dir; //NS direction
                mm.ns_velocity; //NS velocity
                mm.vert_rate_source; //Vertical rate src
                mm.vert_rate_sign; //Vertical rate sign
                mm.vert_rate; //Vertical rate
            }
            else if (mm.mesub == 3 || mm.mesub == 4) {
                mm.heading_is_valid; //Heading status
                mm.heading;
            }
        }
    }

    //messageHandler->sendDebug(icaoStr);

    PostCefTask(messageHandler, mm);


}




void displayModesMessage(struct modesMessage* mm) {
    /* Show the raw message. */
    //printf("*");
    //for (int j = 0; j < mm->msgbits / 8; j++) printf("%02x", mm->msg[j]);
    //printf(";\n");

    if (!mm->crcok) {
        return;
    }
    printf("CRC: %06x (%s)\n", (int)mm->crc, mm->crcok ? "ok" : "wrong");
    if (mm->errorbit != -1)
        printf("Single bit error fixed, bit %d\n", mm->errorbit);

    if (mm->msgtype == 0) {
        /* DF 0 */
        printf("DF 0: Short Air-Air Surveillance.\n");
        printf("  Altitude       : %d %s\n", mm->altitude,
            (mm->unit == 0) ? "meters" : "feet");
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
    }
    else if (mm->msgtype == 4 || mm->msgtype == 20) {
        printf("DF %d: %s, Altitude Reply.\n", mm->msgtype,
            (mm->msgtype == 4) ? "Surveillance" : "Comm-B");
        printf("  Flight Status  : %s\n", fs_str[mm->fs]);
        printf("  DR             : %d\n", mm->dr);
        printf("  UM             : %d\n", mm->um);
        printf("  Altitude       : %d %s\n", mm->altitude,
            (mm->unit == 0) ? "meters" : "feet");
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

        if (mm->msgtype == 20) {
            /* TODO: 56 bits DF20 MB additional field. */
        }
    }
    else if (mm->msgtype == 5 || mm->msgtype == 21) {
        printf("DF %d: %s, Identity Reply.\n", mm->msgtype,
            (mm->msgtype == 5) ? "Surveillance" : "Comm-B");
        printf("  Flight Status  : %s\n", fs_str[mm->fs]);
        printf("  DR             : %d\n", mm->dr);
        printf("  UM             : %d\n", mm->um);
        printf("  Squawk         : %d\n", mm->identity);
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

        if (mm->msgtype == 21) {
            /* TODO: 56 bits DF21 MB additional field. */
        }
    }
    else if (mm->msgtype == 11) {
        /* DF 11 */
        printf("DF 11: All Call Reply.\n");
        printf("  Capability  : %s\n", ca_str[mm->ca]);
        printf("  ICAO Address: %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
    }
    else if (mm->msgtype == 17) {
        /* DF 17 */
        printf("DF 17: ADS-B message.\n");
        printf("  Capability     : %d (%s)\n", mm->ca, ca_str[mm->ca]);
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
        printf("  Extended Squitter  Type: %d\n", mm->metype);
        printf("  Extended Squitter  Sub : %d\n", mm->mesub);
        //printf("  Extended Squitter  Name: %s\n");
        getMEDescription(mm->metype, mm->mesub);


        /* Decode the extended squitter message. */
        if (mm->metype >= 1 && mm->metype <= 4) {

            const char* ac_type_str[4] = {
                "Aircraft Type D",
                "Aircraft Type C",
                "Aircraft Type B",
                "Aircraft Type A"
            };


            //printf("    Aircraft Type  : %s\n", ac_type_str[mm->aircraft_type]);
            printf("    Identification : %s\n", mm->flight);
        }
        else if (mm->metype >= 9 && mm->metype <= 18) {
            printf("    F flag   : %s\n", mm->fflag ? "odd" : "even");
            printf("    T flag   : %s\n", mm->tflag ? "UTC" : "non-UTC");
            printf("    Altitude : %d feet\n", mm->altitude);
            printf("    Latitude : %d (not decoded)\n", mm->raw_latitude);
            printf("    Longitude: %d (not decoded)\n", mm->raw_longitude);
        }
        else if (mm->metype == 19 && mm->mesub >= 1 && mm->mesub <= 4) {
            if (mm->mesub == 1 || mm->mesub == 2) {
                /* Velocity */
                printf("    EW direction      : %d\n", mm->ew_dir);
                printf("    EW velocity       : %d\n", mm->ew_velocity);
                printf("    NS direction      : %d\n", mm->ns_dir);
                printf("    NS velocity       : %d\n", mm->ns_velocity);
                printf("    Vertical rate src : %d\n", mm->vert_rate_source);
                printf("    Vertical rate sign: %d\n", mm->vert_rate_sign);
                printf("    Vertical rate     : %d\n", mm->vert_rate);
            }
            else if (mm->mesub == 3 || mm->mesub == 4) {
                printf("    Heading status: %d", mm->heading_is_valid);
                printf("    Heading: %d", mm->heading);
            }
        }
        else {
            printf("    Unrecognized ME type: %d subtype: %d\n",
                mm->metype, mm->mesub);
        }
    }
}


void testLoop(MessageHandler* NewMessageHandler) {
    messageHandler = NewMessageHandler;
    while (true){
        Sleep(250);

    }
}