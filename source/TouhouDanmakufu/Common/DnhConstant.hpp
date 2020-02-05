#ifndef __TOUHOUDANMAKUFU_DNHCONSTANT__
#define __TOUHOUDANMAKUFU_DNHCONSTANT__

#include "../../GcLib/pch.h"
#include "../../GcLib/GcLib.hpp"

using namespace gstd;
using namespace directx;

const int STANDARD_FPS = 60;

const std::wstring DNH_VERSION = L"v0.01a";

#if NDEBUG
typedef bool PriListBool;
#else
typedef byte PriListBool;
#endif

#endif


