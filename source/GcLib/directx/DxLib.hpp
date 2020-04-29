#ifndef __DIRECTX_DXLIB__
#define __DIRECTX_DXLIB__

#include "../pch.h"

#include "DxConstant.hpp"

#include "DxUtility.hpp"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
#include "HLSL.hpp"
#endif

#include "DirectGraphics.hpp"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
#include "Texture.hpp"
#include "Shader.hpp"

#include "RenderObject.hpp"
#include "DxText.hpp"

#include "ElfreinaMesh.hpp"
#include "MetasequoiaMesh.hpp"

#include "TransitionEffect.hpp"
//#include "DxWindow.hpp"
#include "DxScript.hpp"
#include "ScriptManager.hpp"
//#include "EventScript.hpp"

#include "DirectSound.hpp"
#endif

#include "DirectInput.hpp"

#endif
