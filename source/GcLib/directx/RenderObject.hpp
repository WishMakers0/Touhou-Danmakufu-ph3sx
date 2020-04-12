#ifndef __DIRECTX_RENDEROBJECT__
#define __DIRECTX_RENDEROBJECT__

#include "../pch.h"

#include "DxConstant.hpp"
#include "DxUtility.hpp"
#include "Vertex.hpp"
#include "Texture.hpp"
#include "Shader.hpp"

namespace directx {
	class RenderObjectBase;
	class RenderManager;
	class RenderStateFunction;
	class RenderBlock;
	class RenderObject;

	/**********************************************************
	//RenderBlock
	**********************************************************/
	class RenderBlock {
	protected:
		float posSortKey_;
		gstd::ref_count_ptr<RenderStateFunction> func_;
		gstd::ref_count_ptr<RenderObject> obj_;

		D3DXVECTOR3 position_;//移動先座標
		D3DXVECTOR3 angle_;//回転角度
		D3DXVECTOR3 scale_;//拡大率

	public:
		RenderBlock();
		virtual ~RenderBlock();
		void SetRenderFunction(gstd::ref_count_ptr<RenderStateFunction> func) { func_ = func; }
		virtual void Render();

		virtual void CalculateZValue() = 0;
		float GetZValue() { return posSortKey_; }
		void SetZValue(float pos) { posSortKey_ = pos; }
		virtual bool IsTranslucent() = 0;//Zソート対象に使用

		void SetRenderObject(gstd::ref_count_ptr<RenderObject> obj) { obj_ = obj; }
		gstd::ref_count_ptr<RenderObject> GetRenderObject() { return obj_; }
		void SetPosition(D3DXVECTOR3& pos) { position_ = pos; }
		void SetAngle(D3DXVECTOR3& angle) { angle_ = angle; }
		void SetScale(D3DXVECTOR3& scale) { scale_ = scale; }
	};

	class RenderBlocks {
	protected:
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlock_;
	public:
		RenderBlocks() {};
		virtual ~RenderBlocks() {};
		void Add(gstd::ref_count_ptr<RenderBlock> block) { listBlock_.push_back(block); }
		std::list<gstd::ref_count_ptr<RenderBlock> >& GetList() { return listBlock_; }

	};

	/**********************************************************
	//RenderManager
	//レンダリング管理
	//3D不透明オブジェクト
	//3D半透明オブジェクトZソート順
	//2Dオブジェクト
	//順に描画する
	**********************************************************/
	class RenderManager {
		class ComparatorRenderBlockTranslucent;
	protected:
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlockOpaque_;
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlockTranslucent_;
	public:
		RenderManager();
		virtual ~RenderManager();
		virtual void Render();
		void AddBlock(gstd::ref_count_ptr<RenderBlock> block);
		void AddBlock(gstd::ref_count_ptr<RenderBlocks> blocks);
	};

	class RenderManager::ComparatorRenderBlockTranslucent {
	public:
		bool operator()(gstd::ref_count_ptr<RenderBlock> l, gstd::ref_count_ptr<RenderBlock> r) {
			return l->GetZValue() > r->GetZValue();
		}
	};

	/**********************************************************
	//RenderStateFunction
	**********************************************************/
	class RenderStateFunction {
		friend RenderObjectBase;
		enum FUNC_TYPE {
			FUNC_LIGHTING,
			FUNC_CULLING,
			FUNC_ZBUFFER_ENABLE,
			FUNC_ZBUFFER_WRITE_ENABLE,
			FUNC_BLEND,
			FUNC_TEXTURE_FILTER,
		};

		std::map<FUNC_TYPE, gstd::ref_count_ptr<gstd::ByteBuffer> > mapFuncRenderState_;
	public:
		RenderStateFunction();
		virtual ~RenderStateFunction();
		void CallRenderStateFunction();

		//レンダリングステート設定(RenderManager用)
		void SetLightingEnable(bool bEnable);//ライティング
		void SetCullingMode(DWORD mode);//カリング
		void SetZBufferEnable(bool bEnable);//Zバッファ参照
		void SetZWriteEnable(bool bEnable);//Zバッファ書き込み
		void SetBlendMode(DWORD mode, int stage = 0);
		void SetTextureFilter(DWORD mode, int stage = 0);
	};

	class Matrices {
		std::vector<D3DXMATRIX> matrix_;
	public:
		Matrices() {};
		virtual ~Matrices() {};
		void SetSize(int size) { matrix_.resize(size); for (int iMat = 0; iMat < size; iMat++) { D3DXMatrixIdentity(&matrix_[iMat]); } }
		int GetSize() { return matrix_.size(); }
		void SetMatrix(int index, D3DXMATRIX& mat) { matrix_[index] = mat; }
		D3DXMATRIX& GetMatrix(int index) { return matrix_[index]; }
	};

	/**********************************************************
	//RenderObject
	//レンダリングオブジェクト
	//描画の最小単位
	//RenderManagerに登録して描画してもらう
	//(直接描画も可能)
	**********************************************************/
	class RenderObject {
	protected:
		bool flgUseVertexBufferMode_;

		D3DPRIMITIVETYPE typePrimitive_;//
		size_t strideVertexStreamZero_;//1頂点のサイズ
		gstd::ByteBuffer vertex_;//頂点
		std::vector<uint16_t> vertexIndices_;
		std::vector<gstd::ref_count_ptr<Texture> > texture_;//テクスチャ
		D3DXVECTOR3 posWeightCenter_;//重心

		D3DTEXTUREFILTERTYPE filterMin_;
		D3DTEXTUREFILTERTYPE filterMag_;
		D3DTEXTUREFILTERTYPE filterMip_;
		D3DCULL modeCulling_;

		D3DXVECTOR3 position_;//移動先座標
		D3DXVECTOR3 angle_;//回転角度
		D3DXVECTOR3 scale_;//拡大率
		D3DXMATRIX matRelative_;//関係行列
		bool bCoordinate2D_;//2D座標指定
		gstd::ref_count_ptr<Shader> shader_;

		bool disableMatrixTransform_;

		void _SetTextureStageCount(size_t count) { 
			texture_.resize(count); 
			for (size_t i = 0; i < count; i++)
				texture_[i] = nullptr; 
		}
	public:
		RenderObject();
		virtual ~RenderObject();
		virtual void Render() = 0;
		virtual void CalculateWeightCenter() {}
		D3DXVECTOR3 GetWeightCenter() { return posWeightCenter_; }
		gstd::ref_count_ptr<Texture> GetTexture(int pos = 0) { return texture_[pos]; }

		size_t _GetPrimitiveCount();
		size_t _GetPrimitiveCount(size_t count);

		void SetRalativeMatrix(D3DXMATRIX mat) { matRelative_ = mat; }

		static D3DXMATRIX CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
			D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, D3DXVECTOR3& angle,
			D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrixSprite3D(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
			D3DXMATRIX* matRelative, bool bBillboard = false);
		static D3DXMATRIX CreateWorldMatrix2D(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ, D3DXMATRIX* matCamera);
		static D3DXMATRIX CreateWorldMatrixText2D(D3DXVECTOR2& centerPosition, D3DXVECTOR3& scale,
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ, 
			D3DXVECTOR2& objectPosition, D3DXVECTOR2& biasPosition, D3DXMATRIX* matCamera);
		static void SetCoordinate2dDeviceMatrix();

		//頂点設定
		void SetPrimitiveType(D3DPRIMITIVETYPE type) { typePrimitive_ = type; }
		D3DPRIMITIVETYPE GetPrimitiveType() { return typePrimitive_; }
		virtual void SetVertexCount(size_t count) {
			count = std::min(count, 65536U);
			vertex_.SetSize(count * strideVertexStreamZero_);
			ZeroMemory(vertex_.GetPointer(), vertex_.size());
		}
		virtual size_t GetVertexCount() { return vertex_.size() / strideVertexStreamZero_; }
		void SetVertexIndicies(std::vector<uint16_t>& indices) { vertexIndices_ = indices; }

		//描画用設定
		void SetPosition(D3DXVECTOR3& pos) { position_ = pos; }
		void SetPosition(float x, float y, float z) { position_.x = x; position_.y = y; position_.z = z; }
		void SetX(float x) { position_.x = x; }
		void SetY(float y) { position_.y = y; }
		void SetZ(float z) { position_.z = z; }
		void SetAngle(D3DXVECTOR3& angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }
		void SetScale(D3DXVECTOR3& scale) { scale_ = scale; }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) { scale_.x = sx; scale_.y = sy; scale_.z = sz; }
		void SetTexture(Texture* texture, int stage = 0);//テクスチャ設定
		void SetTexture(gstd::ref_count_ptr<Texture> texture, int stage = 0);//テクスチャ設定

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		void SetDisableMatrixTransformation(bool b) { disableMatrixTransform_ = b; }

		void SetFilteringMin(D3DTEXTUREFILTERTYPE filter) { filterMin_ = filter; }
		void SetFilteringMag(D3DTEXTUREFILTERTYPE filter) { filterMag_ = filter; }
		void SetFilteringMip(D3DTEXTUREFILTERTYPE filter) { filterMip_ = filter; }
		D3DTEXTUREFILTERTYPE GetFilteringMin() { return filterMin_; }
		D3DTEXTUREFILTERTYPE GetFilteringMag() { return filterMag_; }
		D3DTEXTUREFILTERTYPE GetFilteringMip() { return filterMip_; }

		gstd::ref_count_ptr<Shader> GetShader() { return shader_; }
		void SetShader(gstd::ref_count_ptr<Shader> shader) { shader_ = shader; }
	};

	/**********************************************************
	//RenderObjectTLX
	//座標3D変換済み、ライティング済み、テクスチャ有り
	//2D自由変形スプライト用
	**********************************************************/
	class RenderObjectTLX : public RenderObject {
	protected:
		bool bPermitCamera_;
		gstd::ByteBuffer vertCopy_;
	public:
		RenderObjectTLX();
		~RenderObjectTLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void Render(D3DXMATRIX& matTransform);
		virtual void SetVertexCount(size_t count);

		//頂点設定
		VERTEX_TLX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_TLX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z = 1.0f, float w = 1.0f);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);

		//カメラ
		bool IsPermitCamera() { return bPermitCamera_; }
		void SetPermitCamera(bool bPermit) { bPermitCamera_ = bPermit; }
	};
#pragma region RenderObjectTLX_impl
	inline void RenderObjectTLX::SetVertexCount(size_t count) {
		RenderObject::SetVertexCount(count);
		SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
		SetAlpha(255);
	}
	inline VERTEX_TLX* RenderObjectTLX::GetVertex(size_t index) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size()) return nullptr;
		return (VERTEX_TLX*)vertex_.GetPointer(pos);
	}
	inline void RenderObjectTLX::SetVertex(size_t index, VERTEX_TLX& vertex) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size()) return;
		memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
	}
	inline void RenderObjectTLX::SetVertexPosition(size_t index, float x, float y, float z, float w) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;

		constexpr float bias = -0.5f;
		vertex->position.x = x + bias;
		vertex->position.y = y + bias;
		vertex->position.z = z;
		vertex->position.w = w;
	}
	inline void RenderObjectTLX::SetVertexUV(size_t index, float u, float v) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->texcoord.x = u;
		vertex->texcoord.y = v;
	}
	inline void RenderObjectTLX::SetVertexColor(size_t index, D3DCOLOR color) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->diffuse_color = color;
	}
	inline void RenderObjectTLX::SetVertexColorARGB(size_t index, int a, int r, int g, int b) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
	}
	inline void RenderObjectTLX::SetVertexAlpha(size_t index, int alpha) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		D3DCOLOR& color = vertex->diffuse_color;
		color = ColorAccess::SetColorA(color, alpha);
	}
	inline void RenderObjectTLX::SetVertexColorRGB(size_t index, int r, int g, int b) {
		VERTEX_TLX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		D3DCOLOR& color = vertex->diffuse_color;
		color = ColorAccess::SetColorR(color, r);
		color = ColorAccess::SetColorG(color, g);
		color = ColorAccess::SetColorB(color, b);
	}
	inline void RenderObjectTLX::SetColorRGB(D3DCOLOR color) {
		int r = ColorAccess::GetColorR(color);
		int g = ColorAccess::GetColorG(color);
		int b = ColorAccess::GetColorB(color);
		for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
			SetVertexColorRGB(iVert, r, g, b);
		}
	}
	inline void RenderObjectTLX::SetAlpha(int alpha) {
		for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
			SetVertexAlpha(iVert, alpha);
		}
	}
#pragma endregion RenderObjectTLX_impl

	/**********************************************************
	//RenderObjectLX
	//ライティング済み、テクスチャ有り
	//3Dエフェクト用
	**********************************************************/
	class RenderObjectLX : public RenderObject {
	public:
		RenderObjectLX();
		~RenderObjectLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void SetVertexCount(size_t count);

		//頂点設定
		VERTEX_LX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_LX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);
	};
#pragma region RenderObjectLX_impl
	inline void RenderObjectLX::SetVertexCount(size_t count) {
		RenderObject::SetVertexCount(count);
		SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
		SetAlpha(255);
	}
	inline VERTEX_LX* RenderObjectLX::GetVertex(size_t index) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return nullptr;
		return (VERTEX_LX*)vertex_.GetPointer(pos);
	}
	inline void RenderObjectLX::SetVertex(size_t index, VERTEX_LX& vertex) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return;
		memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
	}
	inline void RenderObjectLX::SetVertexPosition(size_t index, float x, float y, float z) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;

		constexpr float bias = -0.5f;
		vertex->position.x = x + bias;
		vertex->position.y = y + bias;
		vertex->position.z = z;
	}
	inline void RenderObjectLX::SetVertexUV(size_t index, float u, float v) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->texcoord.x = u;
		vertex->texcoord.y = v;
	}
	inline void RenderObjectLX::SetVertexColor(size_t index, D3DCOLOR color) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->diffuse_color = color;
	}
	inline void RenderObjectLX::SetVertexColorARGB(size_t index, int a, int r, int g, int b) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
	}
	inline void RenderObjectLX::SetVertexAlpha(size_t index, int alpha) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		D3DCOLOR& color = vertex->diffuse_color;
		color = ColorAccess::SetColorA(color, alpha);
	}
	inline void RenderObjectLX::SetVertexColorRGB(size_t index, int r, int g, int b) {
		VERTEX_LX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		D3DCOLOR& color = vertex->diffuse_color;
		color = ColorAccess::SetColorR(color, r);
		color = ColorAccess::SetColorG(color, g);
		color = ColorAccess::SetColorB(color, b);
	}
	inline void RenderObjectLX::SetColorRGB(D3DCOLOR color) {
		int r = ColorAccess::GetColorR(color);
		int g = ColorAccess::GetColorG(color);
		int b = ColorAccess::GetColorB(color);
		for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
			SetVertexColorRGB(iVert, r, g, b);
		}
	}
	inline void RenderObjectLX::SetAlpha(int alpha) {
		for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
			SetVertexAlpha(iVert, alpha);
		}
	}
#pragma endregion RenderObjectLX_impl

	/**********************************************************
	//RenderObjectNX
	//法線有り、テクスチャ有り
	**********************************************************/
	class RenderObjectNX : public RenderObject {
	protected:
		D3DCOLOR color_;

		IDirect3DVertexBuffer9* pVertexBuffer_;
		IDirect3DIndexBuffer9* pIndexBuffer_;
	public:
		RenderObjectNX();
		~RenderObjectNX();
		virtual void Render();

		//頂点設定
		VERTEX_NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexNormal(size_t index, float x, float y, float z);
		void SetColor(D3DCOLOR color) { color_ = color; }
	};
#pragma region RenderObjectNX_impl
	inline VERTEX_NX* RenderObjectNX::GetVertex(size_t index) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return nullptr;
		return (VERTEX_NX*)vertex_.GetPointer(pos);
	}
	inline void RenderObjectNX::SetVertex(size_t index, VERTEX_NX& vertex) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return;
		memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
	}
	inline void RenderObjectNX::SetVertexPosition(size_t index, float x, float y, float z) {
		VERTEX_NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;

		constexpr float bias = -0.5f;
		vertex->position.x = x + bias;
		vertex->position.y = y + bias;
		vertex->position.z = z;
	}
	inline void RenderObjectNX::SetVertexUV(size_t index, float u, float v) {
		VERTEX_NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->texcoord.x = u;
		vertex->texcoord.y = v;
	}
	inline void RenderObjectNX::SetVertexNormal(size_t index, float x, float y, float z) {
		VERTEX_NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->normal.x = x;
		vertex->normal.y = y;
		vertex->normal.z = z;
	}
#pragma endregion RenderObjectNX_impl

	/**********************************************************
	//RenderObjectBNX
	//頂点ブレンド
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectBNX : public RenderObject {
	protected:
		gstd::ref_count_ptr<Matrices> matrix_;

		D3DCOLOR color_;
		D3DMATERIAL9 materialBNX_;

		IDirect3DVertexBuffer9* pVertexBuffer_;
		IDirect3DIndexBuffer9* pIndexBuffer_;

		virtual void _CopyVertexBufferOnInitialize() = 0;
	public:
		RenderObjectBNX();
		~RenderObjectBNX();

		void InitializeVertexBuffer();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		//描画用設定
		void SetMatrix(gstd::ref_count_ptr<Matrices> matrix) { matrix_ = matrix; }
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	class RenderObjectBNXBlock : public RenderBlock {
	protected:
		gstd::ref_count_ptr<Matrices> matrix_;
		D3DCOLOR color_;

	public:
		void SetMatrix(gstd::ref_count_ptr<Matrices> matrix) { matrix_ = matrix; }
		void SetColor(D3DCOLOR color) { color_ = color; }
		bool IsTranslucent() { return ColorAccess::GetColorA(color_) != 255; }
	};

	/**********************************************************
	//RenderObjectB2NX
	//頂点ブレンド2
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectB2NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB2NX();
		~RenderObjectB2NX();

		virtual void CalculateWeightCenter();

		//頂点設定
		VERTEX_B2NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_B2NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(size_t index, float x, float y, float z);
	};
#pragma region RenderObjectB2X_impl
	inline VERTEX_B2NX* RenderObjectB2NX::GetVertex(size_t index) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return nullptr;
		return (VERTEX_B2NX*)vertex_.GetPointer(pos);
	}
	inline void RenderObjectB2NX::SetVertex(size_t index, VERTEX_B2NX& vertex) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return;
		memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
	}
	inline void RenderObjectB2NX::SetVertexPosition(size_t index, float x, float y, float z) {
		VERTEX_B2NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;

		constexpr float bias = -0.5f;
		vertex->position.x = x + bias;
		vertex->position.y = y + bias;
		vertex->position.z = z;
	}
	inline void RenderObjectB2NX::SetVertexUV(size_t index, float u, float v) {
		VERTEX_B2NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->texcoord.x = u;
		vertex->texcoord.y = v;
	}
	inline void RenderObjectB2NX::SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate) {
		VERTEX_B2NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		gstd::BitAccess::SetByte(vertex->blendIndex, pos * 8, indexBlend);
		if (pos == 0)vertex->blendRate = rate;
	}
	inline void RenderObjectB2NX::SetVertexNormal(size_t index, float x, float y, float z) {
		VERTEX_B2NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->normal.x = x;
		vertex->normal.y = y;
		vertex->normal.z = z;
	}
#pragma endregion RenderObjectB2X_impl

	class RenderObjectB2NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB2NXBlock();
		virtual ~RenderObjectB2NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//RenderObjectB4NX
	//頂点ブレンド4
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectB4NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB4NX();
		~RenderObjectB4NX();

		virtual void CalculateWeightCenter();

		//頂点設定
		VERTEX_B4NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_B4NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(size_t index, float x, float y, float z);
	};
#pragma region RenderObjectB4X_impl
	inline VERTEX_B4NX* RenderObjectB4NX::GetVertex(size_t index) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return nullptr;
		return (VERTEX_B4NX*)vertex_.GetPointer(pos);
	}
	inline void RenderObjectB4NX::SetVertex(size_t index, VERTEX_B4NX& vertex) {
		size_t pos = index * strideVertexStreamZero_;
		if (pos >= vertex_.size())return;
		memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
	}
	inline void RenderObjectB4NX::SetVertexPosition(size_t index, float x, float y, float z) {
		VERTEX_B4NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;

		float bias = -0.5f;
		vertex->position.x = x + bias;
		vertex->position.y = y + bias;
		vertex->position.z = z;
	}
	inline void RenderObjectB4NX::SetVertexUV(size_t index, float u, float v) {
		VERTEX_B4NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->texcoord.x = u;
		vertex->texcoord.y = v;
	}
	inline void RenderObjectB4NX::SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate) {
		VERTEX_B4NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		gstd::BitAccess::SetByte(vertex->blendIndex, pos * 8, indexBlend);
		if (pos <= 2)vertex->blendRate[pos] = rate;
	}
	inline void RenderObjectB4NX::SetVertexNormal(size_t index, float x, float y, float z) {
		VERTEX_B4NX* vertex = GetVertex(index);
		if (vertex == nullptr)return;
		vertex->normal.x = x;
		vertex->normal.y = y;
		vertex->normal.z = z;
	}
#pragma endregion RenderObjectB4X_impl

	class RenderObjectB4NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB4NXBlock();
		virtual ~RenderObjectB4NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//Sprite2D
	//矩形スプライト
	**********************************************************/
	class Sprite2D : public RenderObjectTLX {
	public:
		Sprite2D();
		~Sprite2D();
		void Copy(Sprite2D* src);
		void SetSourceRect(RECT_D &rcSrc);
		void SetDestinationRect(RECT_D &rcDest);
		void SetDestinationCenter();
		void SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));

		RECT_D GetDestinationRect();
	};
#pragma region Sprite2D_impl
	inline void Sprite2D::SetSourceRect(RECT_D& rcSrc) {
		gstd::ref_count_ptr<Texture>& texture = texture_[0];
		if (texture == nullptr)return;
		int width = texture->GetWidth();
		int height = texture->GetHeight();

		//テクスチャUV
		SetVertexUV(0, (float)rcSrc.left / (float)width, (float)rcSrc.top / (float)height);
		SetVertexUV(1, (float)rcSrc.right / (float)width, (float)rcSrc.top / (float)height);
		SetVertexUV(2, (float)rcSrc.left / (float)width, (float)rcSrc.bottom / (float)height);
		SetVertexUV(3, (float)rcSrc.right / (float)width, (float)rcSrc.bottom / (float)height);
	}
	inline void Sprite2D::SetDestinationRect(RECT_D& rcDest) {
		//頂点位置
		SetVertexPosition(0, rcDest.left, rcDest.top);
		SetVertexPosition(1, rcDest.right, rcDest.top);
		SetVertexPosition(2, rcDest.left, rcDest.bottom);
		SetVertexPosition(3, rcDest.right, rcDest.bottom);
	}
	inline void Sprite2D::SetVertex(RECT_D& rcSrc, RECT_D& rcDest, D3DCOLOR color) {
		SetSourceRect(rcSrc);
		SetDestinationRect(rcDest);
		SetColorRGB(color);
		SetAlpha(ColorAccess::GetColorA(color));
	}
	inline RECT_D Sprite2D::GetDestinationRect() {
		constexpr float bias = -0.5f;

		RECT_D rect;
		VERTEX_TLX* vertexLeftTop = GetVertex(0);
		VERTEX_TLX* vertexRightBottom = GetVertex(3);

		rect.left = vertexLeftTop->position.x - bias;
		rect.top = vertexLeftTop->position.y - bias;
		rect.right = vertexRightBottom->position.x - bias;
		rect.bottom = vertexRightBottom->position.y - bias;

		return rect;
	}
#pragma endregion Sprite2D_impl

	/**********************************************************
	//SpriteList2D
	//矩形スプライトリスト
	**********************************************************/
	class SpriteList2D : public RenderObjectTLX {
		int countRenderVertex_;
		RECT_D rcSrc_;
		RECT_D rcDest_;
		D3DCOLOR color_;
		bool bCloseVertexList_;
		bool autoClearVertexList_;
		void _AddVertex(VERTEX_TLX& vertex);
	public:
		SpriteList2D();
		virtual size_t GetVertexCount();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		void ClearVertexCount() { countRenderVertex_ = 0; bCloseVertexList_ = false; }
		void AddVertex();
		void AddVertex(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		void SetSourceRect(RECT_D &rcSrc) { rcSrc_ = rcSrc; }
		void SetDestinationRect(RECT_D &rcDest) { rcDest_ = rcDest; }
		void SetDestinationCenter();
		D3DCOLOR GetColor() { return color_; }
		void SetColor(D3DCOLOR color) { color_ = color; }
		void CloseVertex();

		void SetAutoClearVertex(bool clear) { autoClearVertexList_ = clear; }
	};
#pragma region SpriteList2D_impl
	inline size_t SpriteList2D::GetVertexCount() {
		size_t res = std::min((size_t)countRenderVertex_, vertex_.size() / strideVertexStreamZero_);
		return res;
	}
	inline void SpriteList2D::CloseVertex() {
		bCloseVertexList_ = true;

		position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	}
#pragma endregion SpriteList2D_impl

	/**********************************************************
	//Sprite3D
	//矩形スプライト
	**********************************************************/
	class Sprite3D : public RenderObjectLX {
	protected:
		bool bBillboard_;
	public:
		Sprite3D();
		~Sprite3D();

		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		void SetSourceRect(RECT_D &rcSrc);
		void SetDestinationRect(RECT_D &rcDest);
		void SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetSourceDestRect(RECT_D &rcSrc);
		void SetVertex(RECT_D &rcSrc, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetBillboardEnable(bool bEnable) { bBillboard_ = bEnable; }
	};
#pragma region Sprite3D_impl
	inline void Sprite3D::SetSourceRect(RECT_D& rcSrc) {
		gstd::ref_count_ptr<Texture>& texture = texture_[0];
		if (texture == nullptr)return;
		int width = texture->GetWidth();
		int height = texture->GetHeight();

		//テクスチャUV
		SetVertexUV(0, (float)rcSrc.left / (float)width, (float)rcSrc.top / (float)height);
		SetVertexUV(1, (float)rcSrc.left / (float)width, (float)rcSrc.bottom / (float)height);
		SetVertexUV(2, (float)rcSrc.right / (float)width, (float)rcSrc.top / (float)height);
		SetVertexUV(3, (float)rcSrc.right / (float)width, (float)rcSrc.bottom / (float)height);
	}
	inline void Sprite3D::SetDestinationRect(RECT_D& rcDest) {
		//頂点位置
		SetVertexPosition(0, rcDest.left, rcDest.top, 0);
		SetVertexPosition(1, rcDest.left, rcDest.bottom, 0);
		SetVertexPosition(2, rcDest.right, rcDest.top, 0);
		SetVertexPosition(3, rcDest.right, rcDest.bottom, 0);
	}
	inline void Sprite3D::SetVertex(RECT_D& rcSrc, RECT_D& rcDest, D3DCOLOR color) {
		SetSourceRect(rcSrc);
		SetDestinationRect(rcDest);

		//頂点色
		SetColorRGB(color);
		SetAlpha(ColorAccess::GetColorA(color));
	}
	inline void Sprite3D::SetVertex(RECT_D& rcSrc, D3DCOLOR color) {
		SetSourceDestRect(rcSrc);

		//頂点色
		SetColorRGB(color);
		SetAlpha(ColorAccess::GetColorA(color));
	}
#pragma endregion Sprite3D_impl

	/**********************************************************
	//TrajectoryObject3D
	//3D軌跡
	**********************************************************/
	class TrajectoryObject3D : public RenderObjectLX {
		struct Data {
			int alpha;
			D3DXVECTOR3 pos1;
			D3DXVECTOR3 pos2;
		};
	protected:
		D3DCOLOR color_;
		int diffAlpha_;
		int countComplement_;
		Data dataInit_;
		Data dataLast1_;
		Data dataLast2_;
		std::list<Data> listData_;
		virtual D3DXMATRIX _CreateWorldTransformMatrix();
	public:
		TrajectoryObject3D();
		~TrajectoryObject3D();
		virtual void Work();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		void SetInitialLine(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2) {
			dataInit_.pos1 = pos1;
			dataInit_.pos2 = pos2;
		}
		void AddPoint(D3DXMATRIX mat);
		void SetAlphaVariation(int diff) { diffAlpha_ = diff; }
		void SetComplementCount(int count) { countComplement_ = count; }
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	/**********************************************************
	//DxMesh
	**********************************************************/
	enum {
		MESH_ELFREINA,
		MESH_METASEQUOIA,
	};
	class DxMeshManager;
	class DxMeshData {
	public:
		friend DxMeshManager;
	protected:
		std::wstring name_;
		DxMeshManager* manager_;
		volatile bool bLoad_;
	public:
		DxMeshData();
		virtual ~DxMeshData();
		void SetName(std::wstring name) { name_ = name; }
		std::wstring& GetName() { return name_; }
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) = 0;
	};
	class DxMesh : public gstd::FileManager::LoadObject {
	public:
		friend DxMeshManager;
	protected:
		D3DXVECTOR3 position_;//移動先座標
		D3DXVECTOR3 angle_;//回転角度
		D3DXVECTOR3 scale_;//拡大率
		D3DCOLOR color_;
		bool bCoordinate2D_;//2D座標指定
		gstd::ref_count_ptr<Shader> shader_;

		gstd::ref_count_ptr<DxMeshData> data_;
		gstd::ref_count_ptr<DxMeshData> _GetFromManager(std::wstring name);
		void _AddManager(std::wstring name, gstd::ref_count_ptr<DxMeshData> data);
	public:
		DxMesh();
		virtual ~DxMesh();
		virtual void Release();
		bool CreateFromFile(std::wstring path);
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) = 0;
		virtual bool CreateFromFileInLoadThread(std::wstring path, int type);
		virtual bool CreateFromFileInLoadThread(std::wstring path) = 0;
		virtual std::wstring GetPath() = 0;

		virtual void Render() = 0;
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) = 0;
		virtual void Render(std::wstring nameAnime, int time) { Render(); }
		virtual void Render(std::wstring nameAnime, int time, 
			D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) { Render(angX, angY, angZ); }

		void SetPosition(D3DXVECTOR3 pos) { position_ = pos; }
		void SetPosition(float x, float y, float z) { position_.x = x; position_.y = y; position_.z = z; }
		void SetX(float x) { position_.x = x; }
		void SetY(float y) { position_.y = y; }
		void SetZ(float z) { position_.z = z; }
		void SetAngle(D3DXVECTOR3 angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }
		void SetScale(D3DXVECTOR3 scale) { scale_ = scale; }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) { scale_.x = sx; scale_.y = sy; scale_.z = sz; }

		void SetColor(D3DCOLOR color) { color_ = color; }
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		gstd::ref_count_ptr<RenderBlocks> CreateRenderBlocks() { return NULL; }
		virtual D3DXMATRIX GetAnimationMatrix(std::wstring nameAnime, double time, std::wstring nameBone) { D3DXMATRIX mat; D3DXMatrixIdentity(&mat); return mat; }
		gstd::ref_count_ptr<Shader> GetShader() { return shader_; }
		void SetShader(gstd::ref_count_ptr<Shader> shader) { shader_ = shader; }
	};
#pragma region DxMesh_impl
	inline void DxMesh::SetColorRGB(D3DCOLOR color) {
		int r = ColorAccess::GetColorR(color);
		int g = ColorAccess::GetColorG(color);
		int b = ColorAccess::GetColorB(color);
		ColorAccess::SetColorR(color_, r);
		ColorAccess::SetColorG(color_, g);
		ColorAccess::SetColorB(color_, b);
	}
	inline void DxMesh::SetAlpha(int alpha) {
		ColorAccess::SetColorA(color_, alpha);
	}
#pragma endregion DxMesh_impl

	/**********************************************************
	//DxMeshManager
	**********************************************************/
	class DxMeshInfoPanel;
	class DxMeshManager : public gstd::FileManager::LoadThreadListener {
		friend DxMeshData;
		friend DxMesh;
		friend DxMeshInfoPanel;
		static DxMeshManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;
		std::map<std::wstring, gstd::ref_count_ptr<DxMesh> > mapMesh_;
		std::map<std::wstring, gstd::ref_count_ptr<DxMeshData> > mapMeshData_;
		gstd::ref_count_ptr<DxMeshInfoPanel> panelInfo_;

		void _AddMeshData(std::wstring name, gstd::ref_count_ptr<DxMeshData> data);
		gstd::ref_count_ptr<DxMeshData> _GetMeshData(std::wstring name);
		void _ReleaseMeshData(std::wstring name);
	public:
		DxMeshManager();
		virtual ~DxMeshManager();
		static DxMeshManager* GetBase() { return thisBase_; }
		bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Clear();
		virtual void Add(std::wstring name, gstd::ref_count_ptr<DxMesh> mesh);//参照を保持します
		virtual void Release(std::wstring name);//保持している参照を解放します
		virtual bool IsDataExists(std::wstring name);

		gstd::ref_count_ptr<DxMesh> CreateFromFileInLoadThread(std::wstring path, int type);
		virtual void CallFromLoadThread(gstd::ref_count_ptr<gstd::FileManager::LoadThreadEvent> event);

		void SetInfoPanel(gstd::ref_count_ptr<DxMeshInfoPanel> panel) { panelInfo_ = panel; }
	};

	class DxMeshInfoPanel : public gstd::WindowLogger::Panel, public gstd::Thread {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_NAME,
			ROW_FULLNAME,
			ROW_COUNT_REFFRENCE,
		};
		int timeUpdateInterval_;
		gstd::WListView wndListView_;
		virtual bool _AddedLogger(HWND hTab);
		void _Run();
	public:
		DxMeshInfoPanel();
		~DxMeshInfoPanel();
		virtual void LocateParts();
		virtual void Update(DxMeshManager* manager);
	};
}

#endif
