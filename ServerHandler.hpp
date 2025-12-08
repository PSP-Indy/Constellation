#pragma once

#include "httplib.h"

#include <mutex>
#include <string>

#include "UI.hpp"
#include "DataValues.hpp"

class ServerHandler {
public:
    void Server(std::mutex* valueLock);
};