#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>

#include <windows.h>

#include "UI.hpp"

class SerialHandling {
public:
    SerialHandling();
	SerialHandling(const SerialHandling& obj) = delete;
    
    void ProcessSerialDataTeleBT(HANDLE hSerial, UI::data_values* data);
    void ProcessSerialDataSRAD(HANDLE hSerial, UI::data_values* data);

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

    std::mutex* valueLock;

    static SerialHandling* serialhandling;
};