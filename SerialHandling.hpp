#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <stdfloat>

#include <serial/serial.h>

#include "UI.hpp"
#include "DataValues.hpp"

class SerialHandling {
public:
    SerialHandling();
    
	void FakeData();
    void ProcessSerialDataTeleBT(serial::Serial* hSerial);
    void ProcessSerialDataSRAD();
    static bool SendRawSerialData(serial::Serial* hSerial, const uint8_t* dataPacket, size_t length);
    static bool SendSRADData(const uint8_t* dataPacket, size_t length);
    static bool SendSRADData(const char* data);
    void FindSerialLocations(std::string* sradloc, std::string* telebtloc);
    bool CreateSerialFile(serial::Serial* hSerial, std::string serialLoc);

    ~SerialHandling();
private:
    float StringToFloat(std::string string, int idx) {
        const char* charString = string.substr(idx, 4).c_str(); 
        float cpy_flt;
        memcpy(&cpy_flt, charString, 4);
        return cpy_flt;
    }

    int32_t StringToUInt32(std::string string, int idx) {
        const char* charString = string.substr(idx, 4).c_str(); 
        int32_t cpy_int;
        memcpy(&cpy_int, charString, 4);
        return cpy_int;
    }

    int16_t  StringToUInt16(std::string string, int idx) {
        const char* charString = string.substr(idx, 2).c_str(); 
        int16_t cpy_int;
        memcpy(&cpy_int, charString, 2);
        return cpy_int;
    }

    static SerialHandling* serialhandling;
};