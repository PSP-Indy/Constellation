#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>
#include <cstdlib>

#include <windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <RegStr.h>

#include "UI.hpp"
#include "DataValues.hpp"

class SerialHandling {
public:
    SerialHandling();
	SerialHandling(const SerialHandling& obj) = delete;
    
    void ProcessSerialDataTeleBT(HANDLE hSerial);
    void ProcessSerialDataSRAD();
    bool SendSerialData(HANDLE* hSerial, const char* dataPacket);
    bool SendSRADData(const char* data);
    void FindSerialLocations(std::string* sradloc, std::string* telebtloc);
    bool CreateSerialFile(HANDLE* hSerial, std::string serialLoc);

    ~SerialHandling();
	static SerialHandling* Get();

    void SetValueLock(std::mutex* valueLock)
    {
        this->valueLock = valueLock;
    }

private:
    float CharStringToFloat(char* charString, int idx) {
        float cpy_flt;
        memcpy(&cpy_flt, charString, 4);
        return cpy_flt;
    }

    int CharStringToInt(char* charString, int idx) {
        int cpy_int;
        memcpy(&cpy_int, charString, 4);
        return cpy_int;
    }

    int CharStringToUInt16(char* charString, int idx) {
        int cpy_int;
        memcpy(&cpy_int, charString, 2);
        return cpy_int;
    }

    std::mutex* valueLock;

    static SerialHandling* serialhandling;
};