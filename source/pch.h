#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "../include/dxsdk/d3d8.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <atomic>
#include <mutex>
