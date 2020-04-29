#pragma once

//------------------------------------------------------------------------------

#define GAME_VERSION_TCL
//#define GAME_VERSION_SP

//------------------------------------------------------------------------------

//Unicode
#ifdef _MBCS
#undef _MBCS
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

//標準関数対応表
//http://www1.kokusaika.jp/advisory/org/ja/win32_unicode.html

//Win2000以降
#define _WIN32_WINNT 0x0500

//lib
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "pdh.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "psapi.lib")

//pragma
#pragma warning (disable:4786) //STL Warning抑止
#pragma warning (disable:4018) //signed と unsigned の数値を比較
#pragma warning (disable:4244) //'...': conversion from x to y, possible loss of data.
#pragma warning (disable:4503) //

#pragma warning (disable:4302) // 切り詰めます。
#pragma warning (disable:4305) //'...': truncation from 'double' to 'FLOAT'.
#pragma warning (disable:4819) //ファイルは、現在のコード ページ (932) で表示できない文字を含んでいます。データの損失を防ぐために、ファイルを Unicode 形式で保存してください。
#pragma warning (disable:4996) //This function or variable may be unsafe. 

#pragma warning (disable:26451) //Arithmetic overflow : ..... (io.2)
#pragma warning (disable:26495) //Variable x is uninitialized. Always initialize a member variable.
#pragma warning (disable:26812) //The enum type x is unscoped. Prefer 'enum class' over 'enum'.

//define
#ifndef STRICT
#define STRICT 1
#endif

//--------------------------------STD--------------------------------

//debug
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <cstdlib>
#include <crtdbg.h>

#include <cwchar>
#include <exception>

#include <cmath>
#include <cctype>
#include <cstdio>
#include <clocale>
#include <locale>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <bitset>
#include <memory>
#include <algorithm>
#include <iterator>
#include <future>

#include <fstream>
#include <sstream>

#include <cassert>

//Windows
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <pdh.h>
#include <process.h>
#include <wingdi.h>
#include <shlwapi.h>

#include <mlang.h>
#include <psapi.h>

//--------------------------------DIRECTX--------------------------------

//lib
#pragma comment(lib, "msacm32.lib") //for acm
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "d3dxof.lib")
#pragma comment(lib, "dxerr9.lib")

//define
#define D3D_OVERLOADS
#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0900

//include
#include <mmreg.h> //for acm
#include <msacm.h> //for acm

#include <basetsd.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <dsound.h>
#include <dmusici.h>
#include <dxerr9.h>

//--------------------------------EXTERNAL--------------------------------

//OpenMP
#include <omp.h>

//zlib
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
#include <zlib/zlib.h>
#pragma comment(lib, "zlibstatic.lib")
#endif

//libogg + libvorbis
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#pragma comment(lib, "ogg_static.lib")
#pragma comment(lib, "vorbis_static.lib")
#pragma comment(lib, "vorbisfile_static.lib")
#endif

//----------------------------------------------

#if defined(UNICODE) || defined(_UNICODE)
//#pragma comment(linker, "/entry:\"wWinMainCRTStartup\"")
#endif

//In the case crtdbg is used
#ifdef _DEBUG
#define __L_DBG_NEW__  ::new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new __L_DBG_NEW__
#endif

//Experimental
#define __L_STD_FILESYSTEM
#ifdef __L_STD_FILESYSTEM
#include <filesystem>
namespace stdfs = std::filesystem;
using path_t = stdfs::path;
#endif

//Pointer utilities
template<typename T> static constexpr inline void ptr_delete(T*& ptr) {
	if (ptr) delete ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_delete_scalar(T*& ptr) {
	if (ptr) delete[] ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_release(T*& ptr) {
	if (ptr) ptr->Release();
	ptr = nullptr;
}
using std::shared_ptr;
using std::weak_ptr;

#undef min
#undef max

#undef GetObject