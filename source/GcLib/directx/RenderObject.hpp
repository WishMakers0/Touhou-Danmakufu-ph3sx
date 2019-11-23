#ifndef __DIRECTX_RENDEROBJECT__
#define __DIRECTX_RENDEROBJECT__

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

		D3DXVECTOR3 position_;//�ړ�����W
		D3DXVECTOR3 angle_;//��]�p�x
		D3DXVECTOR3 scale_;//�g�嗦

	public:
		RenderBlock();
		virtual ~RenderBlock();
		void SetRenderFunction(gstd::ref_count_ptr<RenderStateFunction> func) { func_ = func; }
		virtual void Render();

		virtual void CalculateZValue() = 0;
		float GetZValue() { return posSortKey_; }
		void SetZValue(float pos) { posSortKey_ = pos; }
		virtual bool IsTranslucent() = 0;//Z�\�[�g�ΏۂɎg�p

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
	//�����_�����O�Ǘ�
	//3D�s�����I�u�W�F�N�g
	//3D�������I�u�W�F�N�gZ�\�[�g��
	//2D�I�u�W�F�N�g
	//���ɕ`�悷��
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

		//�����_�����O�X�e�[�g�ݒ�(RenderManager�p)
		void SetLightingEnable(bool bEnable);//���C�e�B���O
		void SetCullingMode(DWORD mode);//�J�����O
		void SetZBufferEnable(bool bEnable);//Z�o�b�t�@�Q��
		void SetZWriteEnable(bool bEnable);//Z�o�b�t�@��������
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
	//�����_�����O�I�u�W�F�N�g
	//�`��̍ŏ��P��
	//RenderManager�ɓo�^���ĕ`�悵�Ă��炤
	//(���ڕ`����\)
	**********************************************************/
	class RenderObject {
	protected:
		bool flgUseVertexBufferMode_;

		D3DPRIMITIVETYPE typePrimitive_;//
		int strideVertexStreamZero_;//1���_�̃T�C�Y
		gstd::ByteBuffer vertex_;//���_
		std::vector<short> vertexIndices_;
		std::vector<gstd::ref_count_ptr<Texture> > texture_;//�e�N�X�`��
		D3DXVECTOR3 posWeightCenter_;//�d�S

		D3DTEXTUREFILTERTYPE filterMin_;
		D3DTEXTUREFILTERTYPE filterMag_;
		D3DTEXTUREFILTERTYPE filterMip_;
		D3DCULL modeCulling_;

		//�V�F�[�_�p
		IDirect3DVertexDeclaration9* pVertexDecl_;
		IDirect3DVertexBuffer9* pVertexBuffer_;
		IDirect3DIndexBuffer9* pIndexBuffer_;

		D3DXVECTOR3 position_;//�ړ�����W
		D3DXVECTOR3 angle_;//��]�p�x
		D3DXVECTOR3 scale_;//�g�嗦
		D3DXMATRIX matRelative_;//�֌W�s��
		bool bCoordinate2D_;//2D���W�w��
		gstd::ref_count_ptr<Shader> shader_;

		virtual void _ReleaseVertexBuffer();
		virtual void _RestoreVertexBuffer();
		virtual void _CreateVertexDeclaration() {}

		void _SetTextureStageCount(int count) { texture_.resize(count); for (int i = 0; i < count; i++)texture_[i] = NULL; }
	public:
		RenderObject();
		virtual ~RenderObject();
		virtual void Render() = 0;
		virtual void InitializeVertexBuffer() {}
		virtual void CalculateWeightCenter() {}
		D3DXVECTOR3 GetWeightCenter() { return posWeightCenter_; }
		gstd::ref_count_ptr<Texture> GetTexture(int pos = 0) { return texture_[pos]; }

		int _GetPrimitiveCount();
		int _GetPrimitiveCount(int count);

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
		static void SetCoordinate2dDeviceMatrix();

		//���_�ݒ�
		void SetPrimitiveType(D3DPRIMITIVETYPE type) { typePrimitive_ = type; }
		D3DPRIMITIVETYPE GetPrimitiveType() { return typePrimitive_; }
		virtual void SetVertexCount(int count) { 
			count = min(count, 65536);
			vertex_.SetSize(count * strideVertexStreamZero_); 
			ZeroMemory(vertex_.GetPointer(), vertex_.GetSize()); 
		}
		virtual int GetVertexCount() { return vertex_.GetSize() / strideVertexStreamZero_; }
		void SetVertexIndicies(std::vector<short>& indecies) { vertexIndices_ = indecies; }
		gstd::ByteBuffer* GetVertexPointer() { return &vertex_; }

		//�`��p�ݒ�
		void SetPosition(D3DXVECTOR3& pos) { position_ = pos; }
		void SetPosition(float x, float y, float z) { position_.x = x; position_.y = y; position_.z = z; }
		void SetX(float x) { position_.x = x; }
		void SetY(float y) { position_.y = y; }
		void SetZ(float z) { position_.z = z; }
		void SetAngle(D3DXVECTOR3& angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }
		void SetScale(D3DXVECTOR3& scale) { scale_ = scale; }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) { scale_.x = sx; scale_.y = sy; scale_.z = sz; }
		void SetTexture(Texture* texture, int stage = 0);//�e�N�X�`���ݒ�
		void SetTexture(gstd::ref_count_ptr<Texture> texture, int stage = 0);//�e�N�X�`���ݒ�

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

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
	//���W3D�ϊ��ς݁A���C�e�B���O�ς݁A�e�N�X�`���L��
	//2D���R�ό`�X�v���C�g�p
	**********************************************************/
	class RenderObjectTLX : public RenderObject {
	protected:
		bool bPermitCamera_;
		gstd::ByteBuffer vertCopy_;

		virtual void _CreateVertexDeclaration();
	public:
		RenderObjectTLX();
		~RenderObjectTLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void SetVertexCount(int count);

		//���_�ݒ�
		VERTEX_TLX* GetVertex(int index);
		void SetVertex(int index, VERTEX_TLX& vertex);
		void SetVertexPosition(int index, float x, float y, float z = 1.0f, float w = 1.0f);
		void SetVertexUV(int index, float u, float v);
		void SetVertexColor(int index, D3DCOLOR color);
		void SetVertexColorARGB(int index, int a, int r, int g, int b);
		void SetVertexAlpha(int index, int alpha);
		void SetVertexColorRGB(int index, int r, int g, int b);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);

		//�J����
		bool IsPermitCamera() { return bPermitCamera_; }
		void SetPermitCamera(bool bPermit) { bPermitCamera_ = bPermit; }
	};

	/**********************************************************
	//RenderObjectLX
	//���C�e�B���O�ς݁A�e�N�X�`���L��
	//3D�G�t�F�N�g�p
	**********************************************************/
	class RenderObjectLX : public RenderObject {
	protected:
		virtual void _CreateVertexDeclaration();
	public:
		RenderObjectLX();
		~RenderObjectLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void SetVertexCount(int count);

		//���_�ݒ�
		VERTEX_LX* GetVertex(int index);
		void SetVertex(int index, VERTEX_LX& vertex);
		void SetVertexPosition(int index, float x, float y, float z);
		void SetVertexUV(int index, float u, float v);
		void SetVertexColor(int index, D3DCOLOR color);
		void SetVertexColorARGB(int index, int a, int r, int g, int b);
		void SetVertexAlpha(int index, int alpha);
		void SetVertexColorRGB(int index, int r, int g, int b);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);
	};

	/**********************************************************
	//RenderObjectNX
	//�@���L��A�e�N�X�`���L��
	**********************************************************/
	class RenderObjectNX : public RenderObject {
	protected:
		D3DCOLOR color_;
		virtual void _CreateVertexDeclaration();
	public:
		RenderObjectNX();
		~RenderObjectNX();
		virtual void Render();

		virtual void SetVertexCount(int count) {
			if (count > 65536) return;
			vertex_.SetSize(count * strideVertexStreamZero_); 
			ZeroMemory(vertex_.GetPointer(), vertex_.GetSize());
		}

		//���_�ݒ�
		VERTEX_NX* GetVertex(int index);
		void SetVertex(int index, VERTEX_NX& vertex);
		void SetVertexPosition(int index, float x, float y, float z);
		void SetVertexUV(int index, float u, float v);
		void SetVertexNormal(int index, float x, float y, float z);
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	/**********************************************************
	//RenderObjectBNX
	//���_�u�����h
	//�@���L��
	//�e�N�X�`���L��
	**********************************************************/
	class RenderObjectBNX : public RenderObject {
	public:
		struct Vertex {
			D3DXVECTOR3 position;
			D3DXVECTOR4 blendRate;
			D3DXVECTOR4 blendIndex;
			D3DXVECTOR3 normal;
			D3DXVECTOR2 texcoord;
		};
	protected:
		gstd::ref_count_ptr<Matrices> matrix_;
		D3DCOLOR color_;
		D3DMATERIAL9 materialBNX_;
		virtual void _CreateVertexDeclaration();
		virtual void _CopyVertexBufferOnInitialize() = 0;
	public:
		RenderObjectBNX();
		~RenderObjectBNX();
		virtual void InitializeVertexBuffer();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		//�`��p�ݒ�
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
	//���_�u�����h2
	//�@���L��
	//�e�N�X�`���L��
	**********************************************************/
	class RenderObjectB2NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB2NX();
		~RenderObjectB2NX();

		virtual void CalculateWeightCenter();

		//���_�ݒ�
		VERTEX_B2NX* GetVertex(int index);
		void SetVertex(int index, VERTEX_B2NX& vertex);
		void SetVertexPosition(int index, float x, float y, float z);
		void SetVertexUV(int index, float u, float v);
		void SetVertexBlend(int index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(int index, float x, float y, float z);
	};

	class RenderObjectB2NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB2NXBlock();
		virtual ~RenderObjectB2NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//RenderObjectB4NX
	//���_�u�����h4
	//�@���L��
	//�e�N�X�`���L��
	**********************************************************/
	class RenderObjectB4NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB4NX();
		~RenderObjectB4NX();

		virtual void CalculateWeightCenter();

		//���_�ݒ�
		VERTEX_B4NX* GetVertex(int index);
		void SetVertex(int index, VERTEX_B4NX& vertex);
		void SetVertexPosition(int index, float x, float y, float z);
		void SetVertexUV(int index, float u, float v);
		void SetVertexBlend(int index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(int index, float x, float y, float z);
	};

	class RenderObjectB4NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB4NXBlock();
		virtual ~RenderObjectB4NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//Sprite2D
	//��`�X�v���C�g
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

	/**********************************************************
	//SpriteList2D
	//��`�X�v���C�g���X�g
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
		virtual int GetVertexCount();
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

	/**********************************************************
	//Sprite3D
	//��`�X�v���C�g
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

	/**********************************************************
	//TrajectoryObject3D
	//3D�O��
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
		void SetInitialLine(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2);
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
		D3DXVECTOR3 position_;//�ړ�����W
		D3DXVECTOR3 angle_;//��]�p�x
		D3DXVECTOR3 scale_;//�g�嗦
		D3DCOLOR color_;
		bool bCoordinate2D_;//2D���W�w��
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
		virtual void Add(std::wstring name, gstd::ref_count_ptr<DxMesh> mesh);//�Q�Ƃ�ێ����܂�
		virtual void Release(std::wstring name);//�ێ����Ă���Q�Ƃ�������܂�
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
