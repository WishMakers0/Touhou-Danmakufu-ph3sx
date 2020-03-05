#ifndef __DIRECTX_HLSL__
#define __DIRECTX_HLSL__

#include "../pch.h"

#include "DxConstant.hpp"

namespace directx {
	static const std::wstring NAME_DEFAULT_SKINNED_MESH = L"__NAME_DEFAULT_SKINNED_MESH__";
	static const std::string HLSL_DEFAULT_SKINNED_MESH =
		"float4 lightDirection;"
		"float4 materialAmbient : MATERIALAMBIENT;"
		"float4 materialDiffuse : MATERIALDIFFUSE;"
		"float fogNear;"
		"float fogFar;"
		"static const int MAX_MATRICES = 80;"
		"float4x3 mWorldMatrixArray[MAX_MATRICES] : WORLDMATRIXARRAY;"
		"float4x4 mViewProj : VIEWPROJECTION;"

		"struct VS_INPUT {"
			"float4 pos : POSITION;"
			"float4 blendWeights : BLENDWEIGHT;"
			"float4 blendIndices : BLENDINDICES;"
			"float3 normal : NORMAL;"
			"float2 tex0 : TEXCOORD0;"
		"};"

		"struct VS_OUTPUT {"
			"float4 pos : POSITION;"
			"float4 diffuse : COLOR;"
			"float2 tex0 : TEXCOORD0;"
			"float fog : FOG;"
		"};"

		"float3 CalcDiffuse(float3 normal) {"
			"float res;"
			"res = max(0.0f, dot(normal, lightDirection.xyz));"
			"return (res);"
		"}"

		"VS_OUTPUT DefaultTransform(VS_INPUT i, uniform int numBones) {"
			"VS_OUTPUT o;"
			"float3 pos = 0.0f;"
			"float3 normal = 0.0f;"
			"float lastWeight = 0.0f;"

			"float blendWeights[4] = (float[4])i.blendWeights;"
			"int idxAry[4] = (int[4])i.blendIndices;"

			"for (int iBone = 0; iBone < numBones-1; iBone++) {"
				"lastWeight = lastWeight + blendWeights[iBone];"
				"pos += mul(i.pos, mWorldMatrixArray[idxAry[iBone]]) * blendWeights[iBone];"
				"normal += mul(i.normal, mWorldMatrixArray[idxAry[iBone]]) * blendWeights[iBone];"
			"}"
			"lastWeight = 1.0f - lastWeight;"

			"pos += (mul(i.pos, mWorldMatrixArray[idxAry[numBones-1]]) * lastWeight);"
			"normal += (mul(i.normal, mWorldMatrixArray[idxAry[numBones-1]]) * lastWeight);"
			"o.pos = mul(float4(pos.xyz, 1.0f), mViewProj);"

			"normal = normalize(normal);"
			"o.diffuse.xyz = materialAmbient.xyz + CalcDiffuse(normal) * materialDiffuse.xyz;"
			"o.diffuse.w = materialAmbient.w * materialDiffuse.w;"

			"o.tex0 = i.tex0;"
			"o.fog = 1.0f - (o.pos.z - fogNear) / (fogFar - fogNear);"

			"return o;"
		"}"

		"technique BasicTec {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 DefaultTransform(4);"
			"}"
		"}";

	static const std::wstring NAME_DEFAULT_RENDER3D = L"__NAME_DEFAULT_RENDER3D__";
	static const std::string HLSL_DEFAULT_RENDER3D =
		"sampler samp0_ : register(s0);"
		"static const int MAX_MATRICES = 30;"
		"int countMatrixTransform;"
		"float4x4 matTransform[MAX_MATRICES];"
		"float4x4 matViewProj : VIEWPROJECTION;"

		"bool useFog;"
		"bool useTexture;"

		"float3 fogColor;"
		"float2 fogPos;"

		"struct VS_INPUT {"
			"float3 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
			"float fog : FOG;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs = (VS_OUTPUT)0;"

			"outVs.position = float4(inVs.position, 1.0f);"
			"for (int i = 0; i < countMatrixTransform; i++) {"
				"outVs.position = mul(outVs.position, matTransform[i]);"
			"}"

			"outVs.diffuse = inVs.diffuse;"
			"outVs.texCoord = inVs.texCoord;"

			"outVs.fog = saturate((outVs.position.z - fogPos.x) / (fogPos.y - fogPos.x));"
			"outVs.position = mul(outVs.position, matViewProj);"

			"return outVs;"
		"}"

		"float4 mainPS(VS_OUTPUT inPs) : COLOR0 {"
			"float4 ret = inPs.diffuse;"
			"if (useTexture) {"
				"ret *= tex2D(samp0_, inPs.texCoord);"
			"}"
			"if (useFog) {"
				"ret.rgb = smoothstep(ret.rgb, fogColor.rgb, inPs.fog);"
			"}"
			"return ret;"
		"}"
		
		"technique Render {"
			"pass P0 {"
				"ZENABLE = true;"
				"VertexShader = compile vs_3_0 mainVS();"
				"PixelShader = compile ps_3_0 mainPS();"
			"}"
		"}";

	static const std::wstring NAME_DEFAULT_RENDER2D = L"__NAME_DEFAULT_RENDER2D__";
	static const std::string HLSL_DEFAULT_RENDER2D =
		"sampler samp0_ : register(s0);"
		"row_major float4x4 g_mWorld : WORLD : register(c0);"

		"struct VS_INPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct VS_OUTPUT {"
			"float4 position : POSITION;"
			"float4 diffuse : COLOR0;"
			"float2 texCoord : TEXCOORD0;"
		"};"
		"struct PS_OUTPUT {"
			"float4 color : COLOR0;"
		"};"

		"VS_OUTPUT mainVS(VS_INPUT inVs) {"
			"VS_OUTPUT outVs;"

			"outVs.diffuse = inVs.diffuse;"
			"outVs.texCoord = inVs.texCoord;"
			"outVs.position = mul(inVs.position, g_mWorld);"
			"outVs.position.z = 1.0f;"

			"return outVs;"
		"}"

		"PS_OUTPUT mainPS(VS_OUTPUT inPs) {"
			"PS_OUTPUT outPs;"

			"outPs.color = tex2D(samp0_, inPs.texCoord) * inPs.diffuse;"

			"return outPs;"
		"}"

		"technique Render {"
			"pass P0 {"
				"VertexShader = compile vs_2_0 mainVS();"
				"PixelShader = compile ps_2_0 mainPS();"
			"}"
		"}";

	class RenderShaderManager {
		static RenderShaderManager* thisBase_;
	public:
		enum {
			MAX_MATRIX = 30
		};
		RenderShaderManager();
		~RenderShaderManager();

		static RenderShaderManager* GetBase() { return thisBase_; }

		void Initialize();
		void Release();

		ID3DXEffect* GetSkinnedMeshShader() { return effectSkinnedMesh_; }
		ID3DXEffect* GetRender2DShader() { return effectRender2D_; }

		IDirect3DVertexDeclaration9* GetVertexDeclarationTLX() { return declarationTLX_; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationLX() { return declarationLX_; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationNX() { return declarationNX_; }
		IDirect3DVertexDeclaration9* GetVertexDeclarationBNX() { return declarationBNX_; }

		D3DXMATRIX* GetArrayMatrix() { return arrayMatrix; }
	private:
		ID3DXEffect* effectSkinnedMesh_;
		ID3DXEffect* effectRender2D_;

		IDirect3DVertexDeclaration9* declarationTLX_;
		IDirect3DVertexDeclaration9* declarationLX_;
		IDirect3DVertexDeclaration9* declarationNX_;
		IDirect3DVertexDeclaration9* declarationBNX_;

		D3DXMATRIX arrayMatrix[MAX_MATRIX];
	};
}

#endif
