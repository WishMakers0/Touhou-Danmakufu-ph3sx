#ifndef __TOUHOUDANMAKUFU_DNHCONSTANT__
#define __TOUHOUDANMAKUFU_DNHCONSTANT__

#include "../../GcLib/GcLib.hpp"

using namespace gstd;
using namespace directx;

const int STANDARD_FPS = 60;

//const std::wstring DNH_VERSION = L"[‚Æ‚ ‚é‰J‚Ì’ª¹‘(ƒtƒ@ƒ“ƒQ[ƒ€)]";

#if NDEBUG
typedef bool PriListBool;
#else
typedef byte PriListBool;
#endif

#undef GetObject

#endif


