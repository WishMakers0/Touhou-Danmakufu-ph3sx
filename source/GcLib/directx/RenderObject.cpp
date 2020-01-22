#include"RenderObject.hpp"

#include"DirectGraphics.hpp"
#include"Shader.hpp"

#include"MetasequoiaMesh.hpp"
#include"ElfreinaMesh.hpp"

#include "HLSL.hpp"

using namespace gstd;
using namespace directx;


/**********************************************************
//RenderBlock
**********************************************************/
RenderBlock::RenderBlock() {
	posSortKey_ = 0;
}
RenderBlock::~RenderBlock() {
	func_ = nullptr;
	obj_ = nullptr;
}
void RenderBlock::Render() {
	RenderObject* obj = (RenderObject*)obj_.GetPointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	if (func_ != nullptr)
		func_->CallRenderStateFunction();
	obj->Render();
}

/**********************************************************
//RenderManager
**********************************************************/
RenderManager::RenderManager() {}
RenderManager::~RenderManager() {}
void RenderManager::Render() {
	DirectGraphics* graph = DirectGraphics::GetBase();

	//不透明
	graph->SetZBufferEnable(true);
	graph->SetZWriteEnable(true);
	std::list<gstd::ref_count_ptr<RenderBlock> >::iterator itrOpaque;
	for (itrOpaque = listBlockOpaque_.begin(); itrOpaque != listBlockOpaque_.end(); ++itrOpaque) {
		(*itrOpaque)->Render();
	}

	//半透明
	graph->SetZBufferEnable(true);
	graph->SetZWriteEnable(false);
	std::list<gstd::ref_count_ptr<RenderBlock> >::iterator itrTrans;
	for (itrTrans = listBlockTranslucent_.begin(); itrTrans != listBlockTranslucent_.end(); ++itrTrans) {
		(*itrTrans)->CalculateZValue();
	}
	//SortUtility::CombSort(listBlockTranslucent_.begin(), listBlockTranslucent_.end(), ComparatorRenderBlockTranslucent());
	listBlockTranslucent_.sort(ComparatorRenderBlockTranslucent());
	for (itrTrans = listBlockTranslucent_.begin(); itrTrans != listBlockTranslucent_.end(); ++itrTrans) {
		(*itrTrans)->Render();
	}

	listBlockOpaque_.clear();
	listBlockTranslucent_.clear();
}
void RenderManager::AddBlock(gstd::ref_count_ptr<RenderBlock> block) {
	if (block == nullptr)return;
	if (block->IsTranslucent()) {
		listBlockTranslucent_.push_back(block);
	}
	else {
		listBlockOpaque_.push_back(block);
	}
}
void RenderManager::AddBlock(gstd::ref_count_ptr<RenderBlocks> blocks) {
	std::list<gstd::ref_count_ptr<RenderBlock> >& listBlock = blocks->GetList();
	int size = listBlock.size();
	std::list<gstd::ref_count_ptr<RenderBlock> >::iterator itr;
	for (itr = listBlock.begin(); itr != listBlock.end(); ++itr) {
		AddBlock(*itr);
	}
}

/**********************************************************
//RenderStateFunction
**********************************************************/
RenderStateFunction::RenderStateFunction() {

}
RenderStateFunction::~RenderStateFunction() {

}
void RenderStateFunction::CallRenderStateFunction() {
	std::map<RenderStateFunction::FUNC_TYPE, gstd::ref_count_ptr<gstd::ByteBuffer> >::iterator itr;
	for (itr = mapFuncRenderState_.begin(); itr != mapFuncRenderState_.end(); ++itr) {
		RenderStateFunction::FUNC_TYPE type = (*itr).first;
		gstd::ByteBuffer* args = (*itr).second.GetPointer();
		args->Seek(0);
		if (type == RenderStateFunction::FUNC_LIGHTING) {
			bool bEnable = args->ReadBoolean();
			DirectGraphics::GetBase()->SetLightingEnable(bEnable);
		}
		//TODO
	}
	mapFuncRenderState_.clear();
}

void RenderStateFunction::SetLightingEnable(bool bEnable) {
	int sizeArgs = sizeof(bEnable);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteBoolean(bEnable);
	mapFuncRenderState_[RenderStateFunction::FUNC_LIGHTING] = args;
}
void RenderStateFunction::SetCullingMode(DWORD mode) {
	int sizeArgs = sizeof(mode);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteInteger(mode);
	mapFuncRenderState_[RenderStateFunction::FUNC_CULLING] = args;
}
void RenderStateFunction::SetZBufferEnable(bool bEnable) {
	int sizeArgs = sizeof(bEnable);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteBoolean(bEnable);
	mapFuncRenderState_[RenderStateFunction::FUNC_ZBUFFER_ENABLE] = args;
}
void RenderStateFunction::SetZWriteEnable(bool bEnable) {
	int sizeArgs = sizeof(bEnable);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteBoolean(bEnable);
	mapFuncRenderState_[RenderStateFunction::FUNC_ZBUFFER_WRITE_ENABLE] = args;
}
void RenderStateFunction::SetBlendMode(DWORD mode, int stage) {
	int sizeArgs = sizeof(mode) + sizeof(stage);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteInteger(mode);
	args->WriteInteger(stage);
	mapFuncRenderState_[RenderStateFunction::FUNC_BLEND] = args;
}
void RenderStateFunction::SetTextureFilter(DWORD mode, int stage) {
	int sizeArgs = sizeof(mode) + sizeof(stage);
	gstd::ByteBuffer* args = new gstd::ByteBuffer();
	args->SetSize(sizeArgs);
	args->WriteInteger(mode);
	args->WriteInteger(stage);
	mapFuncRenderState_[RenderStateFunction::FUNC_TEXTURE_FILTER] = args;
}

/**********************************************************
//RenderObject
**********************************************************/
RenderObject::RenderObject() {
	typePrimitive_ = D3DPT_TRIANGLELIST;
	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	posWeightCenter_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	D3DXMatrixIdentity(&matRelative_);

	pVertexDecl_ = nullptr;
	pVertexBuffer_ = nullptr;
	pIndexBuffer_ = nullptr;
	bCoordinate2D_ = false;

	flgUseVertexBufferMode_ = true;

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;
	filterMip_ = D3DTEXF_NONE;

	modeCulling_ = D3DCULL_NONE;

	disableMatrixTransform_ = false;
}
RenderObject::~RenderObject() {
	_ReleaseVertexBuffer();
}
void RenderObject::_ReleaseVertexBuffer() {
	if (pIndexBuffer_ != nullptr)pIndexBuffer_->Release();
	if (pVertexBuffer_ != nullptr)pVertexBuffer_->Release();
	if (pVertexDecl_ != nullptr)pVertexDecl_->Release();
	pVertexDecl_ = nullptr;
	pVertexBuffer_ = nullptr;
	pIndexBuffer_ = nullptr;
}
void RenderObject::_RestoreVertexBuffer() {
	InitializeVertexBuffer();
}
int RenderObject::_GetPrimitiveCount() {
	return _GetPrimitiveCount(GetVertexCount());
}
int RenderObject::_GetPrimitiveCount(int count) {
	int res = 0;
	if (vertexIndices_.size() != 0)
		count = vertexIndices_.size();
	switch (typePrimitive_) {
	case D3DPT_POINTLIST://ポイントリスト
		res = count;
		break;
	case D3DPT_LINELIST://ラインリスト
		res = count / 2;
		break;
	case D3DPT_LINESTRIP://ラインストリップ
		res = count - 1;
		break;
	case D3DPT_TRIANGLELIST://トライアングルリスト
		res = count / 3;
		break;
	case D3DPT_TRIANGLESTRIP://トライアングルストリップ
		res = count - 2;
		break;
	case D3DPT_TRIANGLEFAN://トライアングルファン
		res = count - 2;
		break;
	}

	return res;
}


//---------------------------------------------------------------------

D3DXMATRIX RenderObject::CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
	D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
	D3DXMATRIX* matRelative, bool bCoordinate2D) 
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	/*
	if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, scale.x, scale.y, scale.z);
		mat = mat * matScale;
	}
	if (angle.x != 0.0f || angle.y != 0.0f || angle.z != 0.0f) {
		D3DXMATRIX matRot;
		D3DXMatrixRotationYawPitchRoll(&matRot, angle.y, angle.x, angle.z);
		mat = mat * matRot;
	}
	if (position.x != 0.0f || position.y != 0.0f || position.z != 0.0f) {
		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, position.x, position.y, position.z);
		mat = mat * matTrans;
	}
	*/
	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
		bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

		{
			float cx = angleX.x;
			float sx = angleX.y;
			float cy = angleY.x;
			float sy = angleY.y;
			float cz = angleZ.x;
			float sz = angleZ.y;
			matSRT._11 = cy * cz - sx * sy * sz;
			matSRT._12 = -cx * sz;
			matSRT._13 = sy * cz + sx * cy * sz;
			matSRT._21 = cy * sz + sx * sy * cz;
			matSRT._22 = cx * cz;
			matSRT._23 = sy * sz - sx * cy * cz;
			matSRT._31 = -cx * sy;
			matSRT._32 = sx;
			matSRT._33 = cx * cy;
		}
		if (bScale) {
			matSRT._11 *= scale.x;
			matSRT._12 *= scale.x;
			matSRT._13 *= scale.x;
			matSRT._21 *= scale.y;
			matSRT._22 *= scale.y;
			matSRT._23 *= scale.y;
			matSRT._31 *= scale.z;
			matSRT._32 *= scale.z;
			matSRT._33 *= scale.z;
		}
		if (bPos) {
			matSRT._41 = position.x;
			matSRT._42 = position.y;
			matSRT._43 = position.z;
		}

		mat = mat * matSRT;
	}
	if (matRelative != nullptr) mat = mat * (*matRelative);

	if (bCoordinate2D) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		D3DVIEWPORT9 viewPort;
		device->GetViewport(&viewPort);
		float width = viewPort.Width * 0.5f;
		float height = viewPort.Height * 0.5f;

		ref_count_ptr<DxCamera> camera = graphics->GetCamera();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		if (camera2D->IsEnable()) {
			D3DXMATRIX matCamera = camera2D->GetMatrix();
			mat = mat * matCamera;
		}

		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, -width, -height, 0);

		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, 200.0f, 200.0f, -0.002f);

		D3DXMATRIX& matInvView = camera->GetViewInverseMatrix();
		D3DXMATRIX& matInvProj = camera->GetProjectionInverseMatrix();
		D3DXMATRIX matInvViewPort;
		D3DXMatrixIdentity(&matInvViewPort);
		matInvViewPort._11 = 1.0f / width;
		matInvViewPort._22 = -1.0f / height;
		matInvViewPort._41 = -1.0f;
		matInvViewPort._42 = 1.0f;

		mat = mat * matTrans * matScale * matInvViewPort * matInvProj * matInvView;
		mat._43 *= -1;
	}

	return mat;
}
D3DXMATRIX RenderObject::CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, D3DXVECTOR3& angle,
	D3DXMATRIX* matRelative, bool bCoordinate2D) {
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
		bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

		{
			float cx = cosf(-angle.x);
			float sx = sinf(-angle.x);
			float cy = cosf(-angle.y);
			float sy = sinf(-angle.y);
			float cz = cosf(-angle.z);
			float sz = sinf(-angle.z);
			matSRT._11 = cy * cz - sx * sy * sz;
			matSRT._12 = -cx * sz;
			matSRT._13 = sy * cz + sx * cy * sz;
			matSRT._21 = cy * sz + sx * sy * cz;
			matSRT._22 = cx * cz;
			matSRT._23 = sy * sz - sx * cy * cz;
			matSRT._31 = -cx * sy;
			matSRT._32 = sx;
			matSRT._33 = cx * cy;
		}
		if (bScale) {
			matSRT._11 *= scale.x;
			matSRT._12 *= scale.x;
			matSRT._13 *= scale.x;
			matSRT._21 *= scale.y;
			matSRT._22 *= scale.y;
			matSRT._23 *= scale.y;
			matSRT._31 *= scale.z;
			matSRT._32 *= scale.z;
			matSRT._33 *= scale.z;
		}
		if (bPos) {
			matSRT._41 = position.x;
			matSRT._42 = position.y;
			matSRT._43 = position.z;
		}

		mat = mat * matSRT;
	}
	if (matRelative != nullptr) mat = mat * (*matRelative);

	if (bCoordinate2D) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		D3DVIEWPORT9 viewPort;
		device->GetViewport(&viewPort);
		float width = viewPort.Width * 0.5f;
		float height = viewPort.Height * 0.5f;

		ref_count_ptr<DxCamera> camera = graphics->GetCamera();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		if (camera2D->IsEnable()) {
			D3DXMATRIX matCamera = camera2D->GetMatrix();
			mat = mat * matCamera;
		}

		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, -width, -height, 0);

		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, 200.0f, 200.0f, -0.002f);

		D3DXMATRIX& matInvView = camera->GetViewInverseMatrix();
		D3DXMATRIX& matInvProj = camera->GetProjectionInverseMatrix();
		D3DXMATRIX matInvViewPort;
		D3DXMatrixIdentity(&matInvViewPort);
		matInvViewPort._11 = 1.0f / width;
		matInvViewPort._22 = -1.0f / height;
		matInvViewPort._41 = -1.0f;
		matInvViewPort._42 = 1.0f;

		mat = mat * matTrans * matScale * matInvViewPort * matInvProj * matInvView;
		mat._43 *= -1;
	}

	return mat;
}

D3DXMATRIX RenderObject::CreateWorldMatrixSprite3D(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
	D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
	D3DXMATRIX* matRelative, bool bBillboard) 
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);
	/*
	if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, scale.x, scale.y, scale.z);
		mat = mat * matScale;
	}
	if (angle.x != 0.0f || angle.y != 0.0f || angle.z != 0.0f) {
		D3DXMATRIX matRot;
		D3DXMatrixRotationYawPitchRoll(&matRot, angle.y, angle.x, angle.z);
		mat = mat * matRot;
	}
	*/
	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;

		{
			float cx = angleX.x;
			float sx = angleX.y;
			float cy = angleY.x;
			float sy = angleY.y;
			float cz = angleZ.x;
			float sz = angleZ.y;
			matSRT._11 = cy * cz - sx * sy * sz;
			matSRT._12 = -cx * sz;
			matSRT._13 = sy * cz + sx * cy * sz;
			matSRT._21 = cy * sz + sx * sy * cz;
			matSRT._22 = cx * cz;
			matSRT._23 = sy * sz - sx * cy * cz;
			matSRT._31 = -cx * sy;
			matSRT._32 = sx;
			matSRT._33 = cx * cy;
		}
		if (bScale) {
			matSRT._11 *= scale.x;
			matSRT._12 *= scale.x;
			matSRT._13 *= scale.x;
			matSRT._21 *= scale.y;
			matSRT._22 *= scale.y;
			matSRT._23 *= scale.y;
			matSRT._31 *= scale.z;
			matSRT._32 *= scale.z;
			matSRT._33 *= scale.z;
		}

		mat = mat * matSRT;
	}
	if (bBillboard) {
		DirectGraphics* graph = DirectGraphics::GetBase();
		D3DXMATRIX& matViewTs = graph->GetCamera()->GetViewTransposedMatrix();
		mat = mat * matViewTs;
	}
	if (position.x != 0.0f || position.y != 0.0f || position.z != 0.0f) {
		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, position.x, position.y, position.z);
		mat = mat * matTrans;
	}

	if (matRelative != nullptr) {
		if (bBillboard) {
			D3DXMATRIX matRelativeE;
			D3DXVECTOR3 pos;
			D3DXVec3TransformCoord(&pos, &D3DXVECTOR3(0, 0, 0), matRelative);
			D3DXMatrixTranslation(&matRelativeE, pos.x, pos.y, pos.z);
			mat = mat * matRelativeE;
		}
		else {
			mat = mat * (*matRelative);
		}
	}
	return mat;
}

D3DXMATRIX RenderObject::CreateWorldMatrix2D(D3DXVECTOR3& position, D3DXVECTOR3& scale,
	D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
	D3DXMATRIX* matCamera) 
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
		bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

		{
			float cx = angleX.x;
			float sx = angleX.y;
			float cy = angleY.x;
			float sy = angleY.y;
			float cz = angleZ.x;
			float sz = angleZ.y;
			matSRT._11 = cy * cz - sx * sy * sz;
			matSRT._12 = -cx * sz;
			matSRT._13 = sy * cz + sx * cy * sz;
			matSRT._21 = cy * sz + sx * sy * cz;
			matSRT._22 = cx * cz;
			matSRT._23 = sy * sz - sx * cy * cz;
			matSRT._31 = -cx * sy;
			matSRT._32 = sx;
			matSRT._33 = cx * cy;
		}
		if (bScale) {
			matSRT._11 *= scale.x;
			matSRT._12 *= scale.x;
			matSRT._13 *= scale.x;
			matSRT._21 *= scale.y;
			matSRT._22 *= scale.y;
			matSRT._23 *= scale.y;
			matSRT._31 *= scale.z;
			matSRT._32 *= scale.z;
			matSRT._33 *= scale.z;
		}
		if (bPos) {
			matSRT._41 = position.x;
			matSRT._42 = position.y;
			matSRT._43 = position.z;
		}

		mat = mat * matSRT;
	}
	if (matCamera != nullptr) mat = mat * (*matCamera);

	return mat;
}

//---------------------------------------------------------------------

void RenderObject::SetCoordinate2dDeviceMatrix() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	float width = graphics->GetScreenWidth();
	float height = graphics->GetScreenHeight();

	D3DVIEWPORT9 viewPort;
	device->GetViewport(&viewPort);

	D3DXMATRIX viewMat;
	D3DXMATRIX persMat;
	D3DVECTOR viewFrom = D3DXVECTOR3(0, 0, -100);
	D3DXMatrixLookAtLH(&viewMat, (D3DXVECTOR3*)&viewFrom, &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 1, 0));
	D3DXMatrixPerspectiveFovLH(&persMat, D3DXToRadian(180),
		(float)viewPort.Width / (float)viewPort.Height, 1, 2000);

	viewMat = viewMat * persMat;

	device->SetTransform(D3DTS_VIEW, &viewMat);
}
void RenderObject::SetTexture(Texture* texture, int stage) {
	if (texture == NULL)
		texture_[stage] = nullptr;
	else {
		if (stage >= texture_.size())return;
		texture_[stage] = new Texture(texture);
	}
}
void RenderObject::SetTexture(ref_count_ptr<Texture> texture, int stage) {
	if (texture == NULL)
		texture_[stage] = nullptr;
	else {
		if (stage >= texture_.size())return;
		texture_[stage] = texture;
	}
}

/**********************************************************
//RenderObjectTLX
//座標3D変換済み、ライティング済み、テクスチャ有り
**********************************************************/
RenderObjectTLX::RenderObjectTLX() {
	_SetTextureStageCount(1);
	strideVertexStreamZero_ = sizeof(VERTEX_TLX);
	bPermitCamera_ = true;
}
RenderObjectTLX::~RenderObjectTLX() {

}
void RenderObjectTLX::_CreateVertexDeclaration() {
	if (pVertexDecl_ != nullptr)return;

	//D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	D3DVERTEXELEMENT9 element[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END() // 配列の終わり
	};
	device->CreateVertexDeclaration(element, &pVertexDecl_);
}
void RenderObjectTLX::Render() {
	RenderObjectTLX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectTLX::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();

	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	device->SetFVF(VERTEX_TLX::fvf);

	bool bCamera = camera->IsEnable() && bPermitCamera_;
	{
		vertCopy_.Copy(vertex_);
		size_t countVertex = GetVertexCount();

		/*
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			int pos = iVert * strideVertexStreamZero_;
			VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
			D3DXVECTOR4* vPos = &vert->position;

			vPos->x *= scale_.x;
			vPos->y *= scale_.y;
			vPos->z *= scale_.z;
			DxMath::RotatePosFromXYZFactor(*vPos,
				(angle_.x == 0.0f) ? nullptr : &angX,
				(angle_.y == 0.0f) ? nullptr : &angY,
				(angle_.z == 0.0f) ? nullptr : &angZ);
			vPos->x += position_.x;
			vPos->y += position_.y;
			vPos->z += position_.z;

			if (bCamera)
				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &camera->GetMatrix());
		}
		*/

		//TODO: Copy these to the other types of render objects.
		D3DXMATRIX matWorld; 
		if (!disableMatrixTransform_) {
			matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
				angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);
		}

		for (int iVert = 0; iVert < countVertex; ++iVert) {
			int pos = iVert * strideVertexStreamZero_;
			VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
			D3DXVECTOR4* vPos = &vert->position;

			if (!disableMatrixTransform_)
				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matWorld);
		}
		
		{
			VertexBufferManager* vbManager = VertexBufferManager::GetBase();

			IDirect3DVertexBuffer9* vertexBuffer = vbManager->GetVertexBuffer(VertexBufferManager::BUFFER_VERTEX_TLX);
			IDirect3DIndexBuffer9* indexBuffer = vbManager->GetIndexBuffer();
			
			size_t countPrim = _GetPrimitiveCount(countVertex);

			void* tmp;
			bool bUseIndex = vertexIndices_.size() > 0;
			vertexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
			if (bUseIndex) {
				memcpy(tmp, vertCopy_.GetPointer(), countPrim * sizeof(VERTEX_TLX));
				vertexBuffer->Unlock();
				indexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
				memcpy(tmp, &vertexIndices_[0], countVertex * sizeof(uint16_t));
				indexBuffer->Unlock();

				device->SetIndices(indexBuffer);
			}
			else {
				memcpy(tmp, vertCopy_.GetPointer(), countVertex * sizeof(VERTEX_TLX));
				vertexBuffer->Unlock();
			}

			device->SetStreamSource(0, vertexBuffer, 0, sizeof(VERTEX_TLX));

			{
				UINT countPass = 1;
				ID3DXEffect* effect = nullptr;
				if (shader_ != nullptr) {
					effect = shader_->GetEffect();
					//shader_->_SetupParameter();
					effect->Begin(&countPass, 0);
				}
				for (UINT iPass = 0; iPass < countPass; ++iPass) {
					if (effect != nullptr) effect->BeginPass(iPass);

					if (bUseIndex) {
						device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
					}
					else {
						device->DrawPrimitive(typePrimitive_, 0, countPrim);
					}

					if (effect != nullptr) effect->EndPass();
				}
				if (effect != nullptr) effect->End();
			}

			device->SetIndices(nullptr);
		}
	}
}
void RenderObjectTLX::SetVertexCount(int count) {
	RenderObject::SetVertexCount(count);
	SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
	SetAlpha(255);
}
VERTEX_TLX* RenderObjectTLX::GetVertex(int index) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return nullptr;
	return (VERTEX_TLX*)vertex_.GetPointer(pos);
}
void RenderObjectTLX::SetVertex(int index, VERTEX_TLX& vertex) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return;
	memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
}
void RenderObjectTLX::SetVertexPosition(int index, float x, float y, float z, float w) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;

	constexpr float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
	vertex->position.w = w;
}
void RenderObjectTLX::SetVertexUV(int index, float u, float v) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectTLX::SetVertexColor(int index, D3DCOLOR color) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->diffuse_color = color;
}
void RenderObjectTLX::SetVertexColorARGB(int index, int a, int r, int g, int b) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
}
void RenderObjectTLX::SetVertexAlpha(int index, int alpha) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	D3DCOLOR& color = vertex->diffuse_color;
	color = ColorAccess::SetColorA(color, alpha);
}
void RenderObjectTLX::SetVertexColorRGB(int index, int r, int g, int b) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	D3DCOLOR& color = vertex->diffuse_color;
	color = ColorAccess::SetColorR(color, r);
	color = ColorAccess::SetColorG(color, g);
	color = ColorAccess::SetColorB(color, b);
}
void RenderObjectTLX::SetColorRGB(D3DCOLOR color) {
	int r = ColorAccess::GetColorR(color);
	int g = ColorAccess::GetColorG(color);
	int b = ColorAccess::GetColorB(color);
	for (int iVert = 0; iVert < vertex_.GetSize(); ++iVert) {
		SetVertexColorRGB(iVert, r, g, b);
	}
}
void RenderObjectTLX::SetAlpha(int alpha) {
	for (int iVert = 0; iVert < vertex_.GetSize(); ++iVert) {
		SetVertexAlpha(iVert, alpha);
	}
}

/**********************************************************
//RenderObjectLX
//ライティング済み、テクスチャ有り
**********************************************************/
RenderObjectLX::RenderObjectLX() {
	_SetTextureStageCount(1);
	strideVertexStreamZero_ = sizeof(VERTEX_LX);
}
RenderObjectLX::~RenderObjectLX() {
}
void RenderObjectLX::_CreateVertexDeclaration() {
	if (pVertexDecl_ != nullptr)return;

	//D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	D3DVERTEXELEMENT9 element[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END() // 配列の終わり
	};
	device->CreateVertexDeclaration(element, &pVertexDecl_);
}
/*
void RenderObjectLX::Render() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);

	ShaderManager* manager = ShaderManager::GetBase();
	RenderShaderManager* shaderManager = manager->GetRenderShaderManager();

	D3DXMATRIX bkMatView;
	D3DXMATRIX bkMatProj;
	bool wasFogEnable = graphics->IsFogEnable();
	device->GetTransform(D3DTS_VIEW, &bkMatView);
	device->GetTransform(D3DTS_PROJECTION, &bkMatProj);
	device->SetTransform(D3DTS_WORLD, shaderManager->GetIdentityMatrix());
	device->SetTransform(D3DTS_VIEW, shaderManager->GetIdentityMatrix());
	device->SetTransform(D3DTS_PROJECTION, shaderManager->GetIdentityMatrix());
	graphics->SetFogEnable(false);

	{
		IDirect3DVertexBuffer9* vertexBuffer = VertexBufferManager::GetBase()->GetVertexBuffer(VertexBufferManager::BUFFER_VERTEX_LX);
		IDirect3DIndexBuffer9* indexBuffer = VertexBufferManager::GetBase()->GetIndexBuffer();

		size_t countVertex = GetVertexCount();
		size_t countPrim = _GetPrimitiveCount(countVertex);

		ID3DXEffect* shader = shaderManager->GetRender3DShader();

		D3DXMATRIX& matViewProj = DirectGraphics::GetBase()->GetCamera()->GetViewProjectionMatrix();

		void* tmp;
		bool bUseIndex = vertexIndices_.size() > 0;

		device->SetStreamSource(0, vertexBuffer, 0, sizeof(VERTEX_LX));
		device->SetVertexDeclaration(shaderManager->GetVertexDeclarationLX());
		if (bUseIndex) device->SetIndices(indexBuffer);

		vertexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
		if (bUseIndex) {
			memcpy(tmp, vertex_.GetPointer(), countPrim * sizeof(VERTEX_LX));
			vertexBuffer->Unlock();
			indexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
			memcpy(tmp, &vertexIndices_[0], countVertex * sizeof(uint16_t));
			indexBuffer->Unlock();
		}
		else {
			memcpy(tmp, vertex_.GetPointer(), countVertex * sizeof(VERTEX_LX));
			vertexBuffer->Unlock();
		}

		if (shader_ != nullptr) {
			ID3DXEffect* eff = shader_->GetEffect();
			eff->SetMatrix(eff->GetParameterBySemantic(nullptr, "VIEWPROJECTION"), &matViewProj);

			HRESULT hr = S_OK;

			BeginShader();
			{
				if (bUseIndex) {
					hr = device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				}
				else {
					hr = device->DrawPrimitive(typePrimitive_, 0, countPrim);
				}
			}
			EndShader();

#if _DEBUG
			if (FAILED(hr)) DebugBreak();
#endif
		}
		else {
			D3DXMATRIX* arrayMatTransform = shaderManager->GetArrayMatrix();
			int countMatArray = _CreateWorldTransformMatrix(arrayMatTransform);

			shader->SetMatrix(shader->GetParameterBySemantic(nullptr, "VIEWPROJECTION"), &matViewProj);
			shader->SetMatrixArray(shader->GetParameterByName(nullptr, "matTransform"), arrayMatTransform, countMatArray);
			shader->SetInt(shader->GetParameterByName(nullptr, "countMatrixTransform"), countMatArray);

			VertexFogState* stateFog = graphics->GetFogState();
			shader->SetBool(shader->GetParameterByName(nullptr, "useFog"), stateFog->bEnable);
			shader->SetBool(shader->GetParameterByName(nullptr, "useTexture"), texture != nullptr);
			if (stateFog->bEnable) {
				shader->SetFloatArray(shader->GetParameterByName(nullptr, "fogColor"), 
					reinterpret_cast<float*>(&(stateFog->color)), 3);
				shader->SetFloatArray(shader->GetParameterByName(nullptr, "fogPos"),
					reinterpret_cast<float*>(&(stateFog->fogDist)), 2);
			}

			HRESULT hr = S_OK;

			UINT cPass = 1;
			shader->Begin(&cPass, 0);
			for (UINT i = 0; i < cPass; ++i) {
				shader->BeginPass(i);

				if (bUseIndex) {
					hr = device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				}
				else {
					hr = device->DrawPrimitive(typePrimitive_, 0, countPrim);
				}

				shader->EndPass();
			}
			shader->End();

#if _DEBUG
			if (FAILED(hr)) DebugBreak();
#endif
		}

		device->SetVertexDeclaration(nullptr);
	}

	device->SetTransform(D3DTS_VIEW, &bkMatView);
	device->SetTransform(D3DTS_PROJECTION, &bkMatProj);
	graphics->SetFogEnable(wasFogEnable);
}
*/
void RenderObjectLX::Render() {
	RenderObjectLX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectLX::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix(position_, scale_, 
		angX, angY, angZ, &matRelative_, false);
	device->SetTransform(D3DTS_WORLD, &matWorld);

	device->SetFVF(VERTEX_LX::fvf);

	{
		IDirect3DVertexBuffer9* vertexBuffer = VertexBufferManager::GetBase()->GetVertexBuffer(VertexBufferManager::BUFFER_VERTEX_LX);
		IDirect3DIndexBuffer9* indexBuffer = VertexBufferManager::GetBase()->GetIndexBuffer();

		size_t countVertex = GetVertexCount();
		size_t countPrim = _GetPrimitiveCount(countVertex);

		void* tmp;
		bool bUseIndex = vertexIndices_.size() > 0;

		device->SetStreamSource(0, vertexBuffer, 0, sizeof(VERTEX_LX));

		vertexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
		if (bUseIndex) {
			memcpy(tmp, vertex_.GetPointer(), countPrim * sizeof(VERTEX_LX));
			vertexBuffer->Unlock();
			indexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
			memcpy(tmp, &vertexIndices_[0], countVertex * sizeof(uint16_t));
			indexBuffer->Unlock();

			device->SetIndices(indexBuffer);
		}
		else {
			memcpy(tmp, vertex_.GetPointer(), countVertex * sizeof(VERTEX_LX));
			vertexBuffer->Unlock();
		}

		UINT countPass = 1;
		ID3DXEffect* effect = nullptr;
		if (shader_ != nullptr) {
			effect = shader_->GetEffect();
			//shader_->_SetupParameter();
			effect->Begin(&countPass, 0);
		}
		for (UINT iPass = 0; iPass < countPass; ++iPass) {
			if (effect != nullptr) effect->BeginPass(iPass);

			if (bUseIndex) {
				device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
			}
			else {
				device->DrawPrimitive(typePrimitive_, 0, countPrim);
			}

			if (effect != nullptr) effect->EndPass();
		}
		if (effect != nullptr) effect->End();

		device->SetIndices(nullptr);
	}
}
void RenderObjectLX::SetVertexCount(int count) {
	RenderObject::SetVertexCount(count);
	SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
	SetAlpha(255);
}
VERTEX_LX* RenderObjectLX::GetVertex(int index) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return nullptr;
	return (VERTEX_LX*)vertex_.GetPointer(pos);
}
void RenderObjectLX::SetVertex(int index, VERTEX_LX& vertex) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return;
	memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
}
void RenderObjectLX::SetVertexPosition(int index, float x, float y, float z) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;

	float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectLX::SetVertexUV(int index, float u, float v) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectLX::SetVertexColor(int index, D3DCOLOR color) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->diffuse_color = color;
}
void RenderObjectLX::SetVertexColorARGB(int index, int a, int r, int g, int b) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
}
void RenderObjectLX::SetVertexAlpha(int index, int alpha) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	D3DCOLOR& color = vertex->diffuse_color;
	color = ColorAccess::SetColorA(color, alpha);
}
void RenderObjectLX::SetVertexColorRGB(int index, int r, int g, int b) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	D3DCOLOR& color = vertex->diffuse_color;
	color = ColorAccess::SetColorR(color, r);
	color = ColorAccess::SetColorG(color, g);
	color = ColorAccess::SetColorB(color, b);
}
void RenderObjectLX::SetColorRGB(D3DCOLOR color) {
	int r = ColorAccess::GetColorR(color);
	int g = ColorAccess::GetColorG(color);
	int b = ColorAccess::GetColorB(color);
	for (int iVert = 0; iVert < vertex_.GetSize(); ++iVert) {
		SetVertexColorRGB(iVert, r, g, b);
	}
}
void RenderObjectLX::SetAlpha(int alpha) {
	for (int iVert = 0; iVert < vertex_.GetSize(); ++iVert) {
		SetVertexAlpha(iVert, alpha);
	}
}
/**********************************************************
//RenderObjectNX
**********************************************************/
RenderObjectNX::RenderObjectNX() {
	_SetTextureStageCount(1);
	strideVertexStreamZero_ = sizeof(VERTEX_NX);

	color_ = 0xffffffff;
}
RenderObjectNX::~RenderObjectNX() {

}
void RenderObjectNX::_CreateVertexDeclaration() {
	if (pVertexDecl_ != nullptr)return;

	//D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	D3DVERTEXELEMENT9 element[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END() // 配列の終わり
	};
	device->CreateVertexDeclaration(element, &pVertexDecl_);
}
void RenderObjectNX::Render() {
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];

	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	device->SetFVF(VERTEX_NX::fvf);

	{
		IDirect3DIndexBuffer9* indexBuffer = VertexBufferManager::GetBase()->GetIndexBuffer();

		size_t countVert = GetVertexCount();
		size_t countPrim = _GetPrimitiveCount(countVert);

		{
			void* tmp;

			bool bUseIndex = vertexIndices_.size() > 0;
			if (bUseIndex) {
				indexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
				memcpy(tmp, &vertexIndices_[0], countVert * sizeof(uint16_t));
				indexBuffer->Unlock();

				device->SetIndices(indexBuffer);
			}

			device->SetStreamSource(0, pVertexBuffer_, 0, sizeof(VERTEX_NX));

			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;
			if (shader_ != nullptr) {
				effect = shader_->GetEffect();
				//shader_->_SetupParameter();
				effect->Begin(&countPass, 0);
			}
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				if (effect != nullptr) effect->BeginPass(iPass);
				if (bUseIndex) {
					device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVert, 0, countPrim);
				}
				else {
					device->DrawPrimitive(typePrimitive_, 0, countPrim);
				}
				if (effect != nullptr) effect->EndPass();
			}
			if (effect != nullptr) effect->End();

			device->SetIndices(nullptr);
		}
	}
}
VERTEX_NX* RenderObjectNX::GetVertex(int index) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return nullptr;
	return (VERTEX_NX*)vertex_.GetPointer(pos);
}
void RenderObjectNX::SetVertex(int index, VERTEX_NX& vertex) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return;
	memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
}
void RenderObjectNX::SetVertexPosition(int index, float x, float y, float z) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;

	float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectNX::SetVertexUV(int index, float u, float v) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectNX::SetVertexNormal(int index, float x, float y, float z) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->normal.x = x;
	vertex->normal.y = y;
	vertex->normal.z = z;
}

/**********************************************************
//RenderObjectBNX
**********************************************************/
RenderObjectBNX::RenderObjectBNX() {
	_SetTextureStageCount(1);
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	_CreateVertexDeclaration();
	materialBNX_.Diffuse.a = 1.0f; materialBNX_.Diffuse.r = 1.0f; materialBNX_.Diffuse.g = 1.0f; materialBNX_.Diffuse.b = 1.0f;
	materialBNX_.Ambient.a = 1.0f; materialBNX_.Ambient.r = 1.0f; materialBNX_.Ambient.g = 1.0f; materialBNX_.Ambient.b = 1.0f;
	materialBNX_.Specular.a = 1.0f; materialBNX_.Specular.r = 1.0f; materialBNX_.Specular.g = 1.0f; materialBNX_.Specular.b = 1.0f;
	materialBNX_.Emissive.a = 1.0f; materialBNX_.Emissive.r = 1.0f; materialBNX_.Emissive.g = 1.0f; materialBNX_.Emissive.b = 1.0f;
}
RenderObjectBNX::~RenderObjectBNX() {}
void RenderObjectBNX::_CreateVertexDeclaration() {
	if (pVertexDecl_ != nullptr)return;
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	D3DVERTEXELEMENT9 element[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
		{ 0, 28, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
		{ 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 56, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END() // 配列の終わり
	};
	device->CreateVertexDeclaration(element, &pVertexDecl_);

}
void RenderObjectBNX::InitializeVertexBuffer() {
	int countVertex = GetVertexCount();
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	device->CreateVertexBuffer(countVertex * sizeof(Vertex), 0, 0, D3DPOOL_MANAGED, &pVertexBuffer_, nullptr);

	//コピー
	_CopyVertexBufferOnInitialize();

	int countIndex = vertexIndices_.size();
	if (countIndex != 0) {
		device->CreateIndexBuffer(sizeof(short) * countIndex,
			D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &pIndexBuffer_, nullptr);

		BYTE *bufIndex;
		HRESULT hrLockIndex = pIndexBuffer_->Lock(0, 0, reinterpret_cast<void**>(&bufIndex), 0);
		if (!FAILED(hrLockIndex)) {
			memcpy(bufIndex, &vertexIndices_[0], sizeof(short) * countIndex);
			pIndexBuffer_->Unlock();
		}
	}
}
void RenderObjectBNX::Render() {
	RenderObjectBNX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectBNX::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	ref_count_ptr<DxCamera> camera = DirectGraphics::GetBase()->GetCamera();

	DWORD bFogEnable = FALSE;
	if (bCoordinate2D_) {
		device->GetTransform(D3DTS_VIEW, &camera->GetIdentity());
		device->GetRenderState(D3DRS_FOGENABLE, &bFogEnable);
		device->SetRenderState(D3DRS_FOGENABLE, FALSE);
		RenderObject::SetCoordinate2dDeviceMatrix();
	}

	if (false) {
		D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix(position_, scale_,
			angX, angY, angZ, &matRelative_, bCoordinate2D_);
		device->SetTransform(D3DTS_WORLD, &matWorld);

		int sizeMatrix = matrix_->GetSize();
		for (int iMatrix = 0; iMatrix < sizeMatrix; ++iMatrix) {
			D3DXMATRIX matrix = matrix_->GetMatrix(iMatrix) * matWorld;
			device->SetTransform(D3DTS_WORLDMATRIX(iMatrix),
				&matrix);
		}

		device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_1WEIGHTS);

		device->SetFVF(VERTEX_B2NX::fvf);
		if (vertexIndices_.size() == 0) {
			device->DrawPrimitiveUP(typePrimitive_, _GetPrimitiveCount(), vertex_.GetPointer(), strideVertexStreamZero_);
		}
		else {
			device->DrawIndexedPrimitiveUP(typePrimitive_, 0,
				GetVertexCount(), _GetPrimitiveCount(),
				&vertexIndices_[0], D3DFMT_INDEX16,
				vertex_.GetPointer(), strideVertexStreamZero_);
		}

		device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
	}
	else {
		RenderShaderManager* shaderManager = RenderShaderManager::GetBase();

		ID3DXEffect* shader = shaderManager->GetSkinnedMeshShader();

		if (shader != nullptr) {
			shader->SetTechnique("BasicTec");

			D3DXVECTOR4 tmpVec;
			shader->SetMatrix(shader->GetParameterBySemantic(nullptr, "VIEWPROJECTION"), &camera->GetViewProjectionMatrix());

			D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix(position_, scale_,
				angX, angY, angZ, &matRelative_, bCoordinate2D_);

			D3DLIGHT9 light;
			device->GetLight(0, &light);
			D3DCOLORVALUE diffuse = materialBNX_.Diffuse;
			diffuse.r = min(diffuse.r + light.Diffuse.r, 1.0f);
			diffuse.g = min(diffuse.g + light.Diffuse.g, 1.0f);
			diffuse.b = min(diffuse.b + light.Diffuse.b, 1.0f);
			diffuse = ColorAccess::SetColor(diffuse, color_);

			D3DCOLORVALUE ambient = materialBNX_.Ambient;
			ambient.r = min(ambient.r + light.Ambient.r, 1.0f);
			ambient.g = min(ambient.g + light.Ambient.g, 1.0f);
			ambient.b = min(ambient.b + light.Ambient.b, 1.0f);
			ambient = ColorAccess::SetColor(ambient, color_);

			{
				tmpVec = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
				shader->SetVector(shader->GetParameterByName(nullptr, "lightDirection"), &tmpVec);
			}
			shader->SetVector(shader->GetParameterBySemantic(nullptr, "MATERIALAMBIENT"), reinterpret_cast<D3DXVECTOR4*>(&diffuse));
			shader->SetVector(shader->GetParameterBySemantic(nullptr, "MATERIALDIFFUSE"), reinterpret_cast<D3DXVECTOR4*>(&ambient));

			//フォグ
			DWORD fogNear = 0;
			DWORD fogFar = 0;
			device->GetRenderState(D3DRS_FOGSTART, &fogNear);
			device->GetRenderState(D3DRS_FOGEND, &fogFar);

			shader->SetFloat(shader->GetParameterByName(nullptr, "fogNear"), *(float*)&fogNear);
			shader->SetFloat(shader->GetParameterByName(nullptr, "fogFar"), *(float*)&fogFar);

			//座標変換
			int sizeMatrix = matrix_->GetSize();
			std::vector<D3DXMATRIX> listMatrix(sizeMatrix);
			for (int iMatrix = 0; iMatrix < sizeMatrix; ++iMatrix) {
				D3DXMATRIX matrix = matrix_->GetMatrix(iMatrix) * matWorld;
				listMatrix[iMatrix] = matrix;
			}
			shader->SetMatrixArray(shader->GetParameterBySemantic(nullptr, "WORLDMATRIXARRAY"),
				reinterpret_cast<const D3DXMATRIX*>(&listMatrix[0]), sizeMatrix);

			device->SetVertexDeclaration(pVertexDecl_);
			device->SetStreamSource(0, pVertexBuffer_, 0, sizeof(Vertex));

			UINT cPass = 1;
			unsigned int numPass = shader->Begin(&cPass, 0);
			for (UINT i = 0; i < cPass; ++i) {
				shader->BeginPass(i);
				if (vertexIndices_.size() == 0) {
					device->SetIndices(nullptr);
					device->DrawPrimitive(typePrimitive_, 0, _GetPrimitiveCount());
				}
				else {
					device->SetIndices(pIndexBuffer_);
					device->DrawIndexedPrimitive(typePrimitive_, 0,
						0, GetVertexCount(),
						0, _GetPrimitiveCount());
				}
				shader->EndPass();
			}
			shader->End();
		}
		else {
			Logger::WriteTop(StringUtility::Format("Shader error. [%s]", "DEFAULT"));
		}


		device->SetVertexDeclaration(nullptr);
		device->SetIndices(nullptr);
		device->SetTexture(0, nullptr);
	}

	if (bCoordinate2D_) {
		device->SetTransform(D3DTS_VIEW, &camera->GetViewProjectionMatrix());
		device->SetRenderState(D3DRS_FOGENABLE, bFogEnable);
	}
}

/**********************************************************
//RenderObjectB2NX
**********************************************************/
RenderObjectB2NX::RenderObjectB2NX() {
	strideVertexStreamZero_ = sizeof(VERTEX_B2NX);
}
RenderObjectB2NX::~RenderObjectB2NX() {

}
void RenderObjectB2NX::_CopyVertexBufferOnInitialize() {
	int countVertex = GetVertexCount();
	Vertex* bufVertex;
	HRESULT hrLockVertex = pVertexBuffer_->Lock(0, 0, reinterpret_cast<void**>(&bufVertex), D3DLOCK_NOSYSLOCK);
	if (!FAILED(hrLockVertex)) {
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			Vertex* dest = &bufVertex[iVert];
			VERTEX_B2NX* src = GetVertex(iVert);
			ZeroMemory(dest, sizeof(Vertex));

			dest->position = src->position;
			dest->normal = src->normal;
			dest->texcoord = src->texcoord;

			for (int iBlend = 0; iBlend < 2; ++iBlend) {
				int indexBlend = BitAccess::GetByte(src->blendIndex, iBlend * 8);
				dest->blendIndex[iBlend] = indexBlend;
			}

			dest->blendRate[0] = src->blendRate;
			dest->blendRate[1] = 1.0f - dest->blendRate[0];
		}
		pVertexBuffer_->Unlock();
	}
}

void RenderObjectB2NX::CalculateWeightCenter() {
	double xTotal = 0;
	double yTotal = 0;
	double zTotal = 0;
	int countVert = GetVertexCount();
	for (int iVert = 0; iVert < countVert; ++iVert) {
		VERTEX_B2NX* vertex = GetVertex(iVert);
		xTotal += vertex->position.x;
		yTotal += vertex->position.y;
		zTotal += vertex->position.z;
	}
	xTotal /= countVert;
	yTotal /= countVert;
	zTotal /= countVert;
	posWeightCenter_.x = (float)xTotal;
	posWeightCenter_.y = (float)yTotal;
	posWeightCenter_.z = (float)zTotal;
}
VERTEX_B2NX* RenderObjectB2NX::GetVertex(int index) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return nullptr;
	return (VERTEX_B2NX*)vertex_.GetPointer(pos);
}
void RenderObjectB2NX::SetVertex(int index, VERTEX_B2NX& vertex) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return;
	memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
}
void RenderObjectB2NX::SetVertexPosition(int index, float x, float y, float z) {
	VERTEX_B2NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;

	float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectB2NX::SetVertexUV(int index, float u, float v) {
	VERTEX_B2NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectB2NX::SetVertexBlend(int index, int pos, BYTE indexBlend, float rate) {
	VERTEX_B2NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	BitAccess::SetByte(vertex->blendIndex, pos * 8, indexBlend);
	if (pos == 0)vertex->blendRate = rate;
}
void RenderObjectB2NX::SetVertexNormal(int index, float x, float y, float z) {
	VERTEX_B2NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->normal.x = x;
	vertex->normal.y = y;
	vertex->normal.z = z;
}

//RenderObjectB2NXBlock
RenderObjectB2NXBlock::RenderObjectB2NXBlock() {}
RenderObjectB2NXBlock::~RenderObjectB2NXBlock() {}
void RenderObjectB2NXBlock::Render() {
	RenderObjectB2NX* obj = (RenderObjectB2NX*)obj_.GetPointer();
	obj->SetMatrix(matrix_);
	RenderBlock::Render();
}

/**********************************************************
//RenderObjectB4NX
**********************************************************/
RenderObjectB4NX::RenderObjectB4NX() {
	strideVertexStreamZero_ = sizeof(VERTEX_B4NX);
}
RenderObjectB4NX::~RenderObjectB4NX() {

}
void RenderObjectB4NX::_CopyVertexBufferOnInitialize() {
	int countVertex = GetVertexCount();
	Vertex* bufVertex;
	HRESULT hrLockVertex = pVertexBuffer_->Lock(0, 0, reinterpret_cast<void**>(&bufVertex), D3DLOCK_NOSYSLOCK);
	if (!FAILED(hrLockVertex)) {
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			Vertex* dest = &bufVertex[iVert];
			VERTEX_B4NX* src = GetVertex(iVert);
			ZeroMemory(dest, sizeof(Vertex));

			dest->position = src->position;
			dest->normal = src->normal;
			dest->texcoord = src->texcoord;

			for (int iBlend = 0; iBlend < 4; ++iBlend) {
				int indexBlend = BitAccess::GetByte(src->blendIndex, iBlend * 8);
				dest->blendIndex[iBlend] = indexBlend;
			}

			float lastRate = 1.0f;
			for (int iRate = 0; iRate < 3; ++iRate) {
				float rate = src->blendRate[iRate];
				dest->blendRate[iRate] = rate;
				lastRate -= rate;
			}
			dest->blendRate[3] = lastRate;
		}
		pVertexBuffer_->Unlock();
	}
}

void RenderObjectB4NX::CalculateWeightCenter() {
	double xTotal = 0;
	double yTotal = 0;
	double zTotal = 0;
	int countVert = GetVertexCount();
	for (int iVert = 0; iVert < countVert; ++iVert) {
		VERTEX_B4NX* vertex = GetVertex(iVert);
		xTotal += vertex->position.x;
		yTotal += vertex->position.y;
		zTotal += vertex->position.z;
	}
	xTotal /= countVert;
	yTotal /= countVert;
	zTotal /= countVert;
	posWeightCenter_.x = (float)xTotal;
	posWeightCenter_.y = (float)yTotal;
	posWeightCenter_.z = (float)zTotal;
}
VERTEX_B4NX* RenderObjectB4NX::GetVertex(int index) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return nullptr;
	return (VERTEX_B4NX*)vertex_.GetPointer(pos);
}
void RenderObjectB4NX::SetVertex(int index, VERTEX_B4NX& vertex) {
	int pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.GetSize())return;
	memcpy(vertex_.GetPointer(pos), &vertex, strideVertexStreamZero_);
}
void RenderObjectB4NX::SetVertexPosition(int index, float x, float y, float z) {
	VERTEX_B4NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;

	float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectB4NX::SetVertexUV(int index, float u, float v) {
	VERTEX_B4NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectB4NX::SetVertexBlend(int index, int pos, BYTE indexBlend, float rate) {
	VERTEX_B4NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	BitAccess::SetByte(vertex->blendIndex, pos * 8, indexBlend);
	if (pos <= 2)vertex->blendRate[pos] = rate;
}
void RenderObjectB4NX::SetVertexNormal(int index, float x, float y, float z) {
	VERTEX_B4NX* vertex = GetVertex(index);
	if (vertex == nullptr)return;
	vertex->normal.x = x;
	vertex->normal.y = y;
	vertex->normal.z = z;
}

//RenderObjectB4NXBlock
RenderObjectB4NXBlock::RenderObjectB4NXBlock() {}
RenderObjectB4NXBlock::~RenderObjectB4NXBlock() {}
void RenderObjectB4NXBlock::Render() {
	RenderObjectB4NX* obj = (RenderObjectB4NX*)obj_.GetPointer();
	obj->SetMatrix(matrix_);
	RenderBlock::Render();
}

/**********************************************************
//Sprite2D
//矩形スプライト
**********************************************************/
Sprite2D::Sprite2D() {
	SetVertexCount(4);//左上、右上、左下、右下
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);

	flgUseVertexBufferMode_ = false;
}
Sprite2D::~Sprite2D() {

}
void Sprite2D::Copy(Sprite2D* src) {
	typePrimitive_ = src->typePrimitive_;
	strideVertexStreamZero_ = src->strideVertexStreamZero_;
	vertex_.Copy(src->vertex_);
	vertexIndices_ = src->vertexIndices_;
	for (int iTex = 0; iTex < texture_.size(); ++iTex) {
		texture_[iTex] = src->texture_[iTex];
	}

	posWeightCenter_ = src->posWeightCenter_;

	position_ = src->position_;
	angle_ = src->angle_;
	scale_ = src->scale_;
	matRelative_ = src->matRelative_;
}
void Sprite2D::SetSourceRect(RECT_D &rcSrc) {
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture == nullptr)return;
	int width = texture->GetWidth();
	int height = texture->GetHeight();

	//テクスチャUV
	SetVertexUV(0, (float)rcSrc.left / (float)width, (float)rcSrc.top / (float)height);
	SetVertexUV(1, (float)rcSrc.right / (float)width, (float)rcSrc.top / (float)height);
	SetVertexUV(2, (float)rcSrc.left / (float)width, (float)rcSrc.bottom / (float)height);
	SetVertexUV(3, (float)rcSrc.right / (float)width, (float)rcSrc.bottom / (float)height);
}
void Sprite2D::SetDestinationRect(RECT_D &rcDest) {
	//頂点位置
	SetVertexPosition(0, rcDest.left, rcDest.top);
	SetVertexPosition(1, rcDest.right, rcDest.top);
	SetVertexPosition(2, rcDest.left, rcDest.bottom);
	SetVertexPosition(3, rcDest.right, rcDest.bottom);
}
void Sprite2D::SetDestinationCenter() {
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture == nullptr || GetVertexCount() < 4)return;
	int width = texture->GetWidth();
	int height = texture->GetHeight();

	VERTEX_TLX* vertLT = GetVertex(0); //左上
	VERTEX_TLX* vertRB = GetVertex(3); //右下

	int vWidth = vertRB->texcoord.x * width - vertLT->texcoord.x * width;
	int vHeight = vertRB->texcoord.y * height - vertLT->texcoord.y * height;
	RECT_D rcDest = { -vWidth / 2., -vHeight / 2., vWidth / 2., vHeight / 2. };

	SetDestinationRect(rcDest);
}
void Sprite2D::SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}
RECT_D Sprite2D::GetDestinationRect() {
	float bias = -0.5f;

	RECT_D rect;
	VERTEX_TLX* vertexLeftTop = GetVertex(0);
	VERTEX_TLX* vertexRightBottom = GetVertex(3);

	rect.left = vertexLeftTop->position.x - bias;
	rect.top = vertexLeftTop->position.y - bias;
	rect.right = vertexRightBottom->position.x - bias;
	rect.bottom = vertexRightBottom->position.y - bias;

	return rect;
}

/**********************************************************
//SpriteList2D
**********************************************************/
SpriteList2D::SpriteList2D() {
	countRenderVertex_ = 0;
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	bCloseVertexList_ = false;
	autoClearVertexList_ = false;
}
void SpriteList2D::Render() {
	SpriteList2D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void SpriteList2D::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != NULL)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, NULL);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	device->SetFVF(VERTEX_TLX::fvf);

	bool bCamera = camera->IsEnable() && bPermitCamera_;
	{
		vertCopy_.Copy(vertex_);
		size_t countVertex = GetVertexCount();

		/*
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			int pos = iVert * strideVertexStreamZero_;
			VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
			D3DXVECTOR3* vPos = (D3DXVECTOR3*)&vert->position;

			if (bCloseVertexList_) {
				vPos->x *= scale_.x;
				vPos->y *= scale_.y;
				vPos->z *= scale_.z;
				DxMath::RotatePosFromXYZFactor(*(D3DXVECTOR4*)vPos,
					(angle_.x == 0.0f) ? nullptr : &angX,
					(angle_.y == 0.0f) ? nullptr : &angY,
					(angle_.z == 0.0f) ? nullptr : &angZ);
				vPos->x += position_.x;
				vPos->y += position_.y;
				vPos->z += position_.z;
			}

			if (bCamera)
				D3DXVec3TransformCoord(vPos, vPos, &camera->GetMatrix());
		}
		*/

		if (bCloseVertexList_) {
			D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
				angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);

			size_t countVertex = GetVertexCount();
			for (int iVert = 0; iVert < countVertex; ++iVert) {
				int pos = iVert * strideVertexStreamZero_;
				VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
				D3DXVECTOR4* vPos = &vert->position;

				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matWorld);
			}
		}
		else {
			size_t countVertex = GetVertexCount();
			for (int iVert = 0; iVert < countVertex; ++iVert) {
				int pos = iVert * strideVertexStreamZero_;
				VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
				D3DXVECTOR4* vPos = &vert->position;

				if (bCamera)
					D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &camera->GetMatrix());
			}
		}

		{
			IDirect3DVertexBuffer9* vertexBuffer = VertexBufferManager::GetBase()->GetVertexBuffer(VertexBufferManager::BUFFER_VERTEX_TLX);
			IDirect3DIndexBuffer9* indexBuffer = VertexBufferManager::GetBase()->GetIndexBuffer();

			size_t countPrim = _GetPrimitiveCount(countVertex);

			{
				void* tmp;

				bool bUseIndex = vertexIndices_.size() > 0;
				vertexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
				if (bUseIndex) {
					memcpy(tmp, vertCopy_.GetPointer(), countPrim * sizeof(VERTEX_TLX));
					vertexBuffer->Unlock();
					indexBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
					memcpy(tmp, &vertexIndices_[0], countVertex * sizeof(uint16_t));
					indexBuffer->Unlock();

					device->SetIndices(indexBuffer);
				}
				else {
					memcpy(tmp, vertCopy_.GetPointer(), countVertex * sizeof(VERTEX_TLX));
					vertexBuffer->Unlock();
				}

				device->SetStreamSource(0, vertexBuffer, 0, sizeof(VERTEX_TLX));

				UINT countPass = 1;
				ID3DXEffect* effect = nullptr;
				if (shader_ != nullptr) {
					effect = shader_->GetEffect();
					//shader_->_SetupParameter();
					effect->Begin(&countPass, 0);
				}
				for (UINT iPass = 0; iPass < countPass; ++iPass) {
					if (effect != nullptr) effect->BeginPass(iPass);
					if (bUseIndex) {
						device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
					}
					else {
						device->DrawPrimitive(typePrimitive_, 0, countPrim);
					}
					if (effect != nullptr) effect->EndPass();
				}
				if (effect != nullptr) effect->End();

				device->SetIndices(nullptr);
			}
		}

		/*
		else {
			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;
			if (shader_ != nullptr) {
				effect = shader_->GetEffect();
				//shader_->_SetupParameter();
				effect->Begin(&countPass, 0);
			}
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				if (effect != nullptr) effect->BeginPass(iPass);

				int oldSamplerState = 0;
				if (vertexIndices_.size() == 0) {
					device->DrawPrimitiveUP(typePrimitive_, _GetPrimitiveCount(), vertCopy_.GetPointer(), strideVertexStreamZero_);
				}
				else {
					device->DrawIndexedPrimitiveUP(typePrimitive_, 0,
						GetVertexCount(), _GetPrimitiveCount(),
						&vertexIndices_[0], D3DFMT_INDEX16,
						vertCopy_.GetPointer(), strideVertexStreamZero_);
				}

				if (effect != nullptr) effect->EndPass();
			}
			if (effect != nullptr) effect->End();
		}
		*/

		if (autoClearVertexList_ && (countVertex >= 6)) ClearVertexCount();
	}
}
int SpriteList2D::GetVertexCount() {
	int res = countRenderVertex_;
	res = min(countRenderVertex_, vertex_.GetSize() / strideVertexStreamZero_);
	return res;
}
void SpriteList2D::_AddVertex(VERTEX_TLX& vertex) {
	int count = vertex_.GetSize() / strideVertexStreamZero_;
	if (countRenderVertex_ >= count) {
		//リサイズ
		int newCount = max(10, count * 1.5);
		ByteBuffer buffer(vertex_);
		SetVertexCount(newCount);
		memcpy(vertex_.GetPointer(), buffer.GetPointer(), buffer.GetSize());
	}
	SetVertex(countRenderVertex_, vertex);
	++countRenderVertex_;
}
void SpriteList2D::AddVertex() {
	SpriteList2D::AddVertex(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void SpriteList2D::AddVertex(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	if (bCloseVertexList_ || countRenderVertex_ > 65536 / 6)
		return;

	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture == nullptr)return;

	int width = texture->GetWidth();
	int height = texture->GetHeight();

	D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
		angX, angY, angZ, nullptr);

	DirectGraphics* graphics = DirectGraphics::GetBase();

	VERTEX_TLX verts[4];
	float srcX[] = { (float)rcSrc_.left, (float)rcSrc_.right, (float)rcSrc_.left, (float)rcSrc_.right };
	float srcY[] = { (float)rcSrc_.top, (float)rcSrc_.top, (float)rcSrc_.bottom, (float)rcSrc_.bottom };
	int destX[] = { (int)rcDest_.left, (int)rcDest_.right, (int)rcDest_.left, (int)rcDest_.right };
	int destY[] = { (int)rcDest_.top,(int)rcDest_.top, (int)rcDest_.bottom, (int)rcDest_.bottom };
	for (int iVert = 0; iVert < 4; ++iVert) {
		VERTEX_TLX vt;
		vt.texcoord.x = srcX[iVert] / width;
		vt.texcoord.y = srcY[iVert] / height;

		D3DXVECTOR4 vPos;

		float bias = -0.5f;
		vPos.x = destX[iVert] + bias;
		vPos.y = destY[iVert] + bias;
		vPos.z = 1.0f;
		vPos.w = 1.0f;
		/*
		vPos.x *= scale_.x;
		vPos.y *= scale_.y;
		vPos.z *= scale_.z;
		DxMath::RotatePosFromXYZFactor(vPos,
			(angle_.x == 0.0f) ? nullptr : &angX,
			(angle_.y == 0.0f) ? nullptr : &angY,
			(angle_.z == 0.0f) ? nullptr : &angZ);
		vPos.x += position_.x;
		vPos.y += position_.y;
		vPos.z += position_.z;
		*/
		D3DXVec3TransformCoord((D3DXVECTOR3*)&vPos, (D3DXVECTOR3*)&vPos, &matWorld);
		vt.position = vPos;

		vt.diffuse_color = color_;
		verts[iVert] = vt;
	}

	_AddVertex(verts[0]);
	_AddVertex(verts[2]);
	_AddVertex(verts[1]);
	_AddVertex(verts[1]);
	_AddVertex(verts[2]);
	_AddVertex(verts[3]);
}
void SpriteList2D::SetDestinationCenter() {
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture == nullptr)return;
	int width = texture->GetWidth();
	int height = texture->GetHeight();

	VERTEX_TLX* vertLT = GetVertex(0); //左上
	VERTEX_TLX* vertRB = GetVertex(3); //右下

	int vWidth = rcSrc_.right - rcSrc_.left;
	int vHeight = rcSrc_.bottom - rcSrc_.top;
	RECT_D rcDest = { -vWidth / 2., -vHeight / 2., vWidth / 2., vHeight / 2. };

	SetDestinationRect(rcDest);
}
void SpriteList2D::CloseVertex() {
	bCloseVertexList_ = true;

	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
}

/**********************************************************
//Sprite3D
**********************************************************/
Sprite3D::Sprite3D() {
	SetVertexCount(4);//左上、右上、左下、右下
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);
	bBillboard_ = false;

	flgUseVertexBufferMode_ = false;
}
Sprite3D::~Sprite3D() {}

void Sprite3D::Render() {
	Sprite3D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void Sprite3D::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture != nullptr)
		device->SetTexture(0, texture->GetD3DTexture());
	else
		device->SetTexture(0, nullptr);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, filterMin_);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, filterMag_);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, filterMip_);

	D3DXMATRIX matWorld = RenderObject::CreateWorldMatrixSprite3D(position_, scale_,
		angX, angY, angZ, &matRelative_, bBillboard_);
	device->SetTransform(D3DTS_WORLD, &matWorld);

	device->SetFVF(VERTEX_LX::fvf);

	{
		UINT countPass = 1;
		ID3DXEffect* effect = nullptr;
		if (shader_ != nullptr) {
			effect = shader_->GetEffect();
			//shader_->_SetupParameter();
			effect->Begin(&countPass, 0);
		}
		for (UINT iPass = 0; iPass < countPass; ++iPass) {
			if (effect != nullptr) effect->BeginPass(iPass);

			if (vertexIndices_.size() == 0) {
				device->DrawPrimitiveUP(typePrimitive_, _GetPrimitiveCount(), vertex_.GetPointer(), strideVertexStreamZero_);
			}
			else {
				device->DrawIndexedPrimitiveUP(typePrimitive_, 0,
					GetVertexCount(), _GetPrimitiveCount(),
					&vertexIndices_[0], D3DFMT_INDEX16,
					vertex_.GetPointer(), strideVertexStreamZero_);
			}

			if (effect != nullptr) effect->EndPass();
		}
		if (effect != nullptr) effect->End();
	}
}

void Sprite3D::SetSourceRect(RECT_D &rcSrc) {
	ref_count_ptr<Texture>& texture = texture_[0];
	if (texture == nullptr)return;
	int width = texture->GetWidth();
	int height = texture->GetHeight();

	//テクスチャUV
	SetVertexUV(0, (float)rcSrc.left / (float)width, (float)rcSrc.top / (float)height);
	SetVertexUV(1, (float)rcSrc.left / (float)width, (float)rcSrc.bottom / (float)height);
	SetVertexUV(2, (float)rcSrc.right / (float)width, (float)rcSrc.top / (float)height);
	SetVertexUV(3, (float)rcSrc.right / (float)width, (float)rcSrc.bottom / (float)height);
}
void Sprite3D::SetDestinationRect(RECT_D &rcDest) {
	//頂点位置
	SetVertexPosition(0, rcDest.left, rcDest.top, 0);
	SetVertexPosition(1, rcDest.left, rcDest.bottom, 0);
	SetVertexPosition(2, rcDest.right, rcDest.top, 0);
	SetVertexPosition(3, rcDest.right, rcDest.bottom, 0);
}
void Sprite3D::SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);

	//頂点色
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}

void Sprite3D::SetSourceDestRect(RECT_D &rcSrc) {
	int width = rcSrc.right - rcSrc.left;
	int height = rcSrc.bottom - rcSrc.top;

	RECT_D rcDest;
	SetRectD(&rcDest, -width / 2., -height / 2., width / 2., height / 2.);

	rcSrc.right--;
	rcSrc.bottom--;

	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);
}
void Sprite3D::SetVertex(RECT_D &rcSrc, D3DCOLOR color) {
	SetSourceDestRect(rcSrc);

	//頂点色
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}

/**********************************************************
//TrajectoryObject3D
**********************************************************/
TrajectoryObject3D::TrajectoryObject3D() {
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);
	diffAlpha_ = 20;
	countComplement_ = 8;
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
}
TrajectoryObject3D::~TrajectoryObject3D() {}
D3DXMATRIX TrajectoryObject3D::_CreateWorldTransformMatrix() {
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);
	return mat;
}
void TrajectoryObject3D::Work() {
	std::list<Data>::iterator itr;
	for (itr = listData_.begin(); itr != listData_.end();) {
		Data& data = (*itr);
		data.alpha -= diffAlpha_;
		if (data.alpha < 0)itr = listData_.erase(itr);
		else ++itr;
	}
}
void TrajectoryObject3D::Render() {
	TrajectoryObject3D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void TrajectoryObject3D::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	int size = listData_.size() * 2;
	SetVertexCount(size);

	int width = 1;
	gstd::ref_count_ptr<Texture> texture = texture_[0];
	if (texture != NULL) {
		width = texture->GetWidth();
	}

	float dWidth = 1.0 / width / listData_.size();
	int iData = 0;
	std::list<Data>::iterator itr;
	for (itr = listData_.begin(); itr != listData_.end(); ++itr, ++iData) {
		Data data = (*itr);
		int alpha = data.alpha;
		for (int iPos = 0; iPos < 2; ++iPos) {
			int index = iData * 2 + iPos;
			D3DXVECTOR3& pos = iPos == 0 ? data.pos1 : data.pos2;
			float u = dWidth * iData;
			float v = iPos == 0 ? 0 : 1;

			SetVertexPosition(index, pos.x, pos.y, pos.z);
			SetVertexUV(index, u, v);

			float r = ColorAccess::GetColorR(color_) * alpha / 255;
			float g = ColorAccess::GetColorG(color_) * alpha / 255;
			float b = ColorAccess::GetColorB(color_) * alpha / 255;
			SetVertexColorARGB(index, alpha, r, g, b);
		}
	}
	RenderObjectLX::Render(angX, angY, angZ);
}
void TrajectoryObject3D::SetInitialLine(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2) {
	dataInit_.pos1 = pos1;
	dataInit_.pos2 = pos2;
}
void TrajectoryObject3D::AddPoint(D3DXMATRIX mat) {
	Data data;
	data.alpha = 255;
	data.pos1 = dataInit_.pos1;
	data.pos2 = dataInit_.pos2;
	D3DXVec3TransformCoord((D3DXVECTOR3*)&data.pos1, (D3DXVECTOR3*)&data.pos1, &mat);
	D3DXVec3TransformCoord((D3DXVECTOR3*)&data.pos2, (D3DXVECTOR3*)&data.pos2, &mat);

	if (listData_.size() <= 1) {
		listData_.push_back(data);
		dataLast2_ = dataLast1_;
		dataLast1_ = data;
	}
	else {
		float cDiff = 1.0 / (countComplement_);
		float diffAlpha = diffAlpha_ / countComplement_;
		for (int iCount = 0; iCount < countComplement_ - 1; ++iCount) {
			Data cData;
			float flame = cDiff * (iCount + 1);
			for (int iPos = 0; iPos < 2; ++iPos) {
				D3DXVECTOR3& outPos = iPos == 0 ? cData.pos1 : cData.pos2;
				D3DXVECTOR3& cPos = iPos == 0 ? data.pos1 : data.pos2;
				D3DXVECTOR3& lPos1 = iPos == 0 ? dataLast1_.pos1 : dataLast1_.pos2;
				D3DXVECTOR3& lPos2 = iPos == 0 ? dataLast2_.pos1 : dataLast2_.pos2;

				D3DXVECTOR3 vPos1 = lPos1 - lPos2;
				D3DXVECTOR3 vPos2 = lPos2 - cPos + vPos1;

				//				D3DXVECTOR3 vPos1 = lPos2 - lPos1;
				//				D3DXVECTOR3 vPos2 = lPos2 - cPos + vPos1;

								//D3DXVECTOR3 vPos1 = lPos1 - lPos2;
								//D3DXVECTOR3 vPos2 = cPos - lPos1;

				D3DXVec3Hermite(&outPos, &lPos1, &vPos1, &cPos, &vPos2, flame);
			}

			cData.alpha = 255 - diffAlpha * (countComplement_ - iCount - 1);
			listData_.push_back(cData);
		}
		dataLast2_ = dataLast1_;
		dataLast1_ = data;
	}

}

/**********************************************************
//DxMesh
**********************************************************/
DxMeshData::DxMeshData() {
	manager_ = DxMeshManager::GetBase();
	bLoad_ = true;
}
DxMeshData::~DxMeshData() {}

DxMesh::DxMesh() {
	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	bCoordinate2D_ = false;
}
DxMesh::~DxMesh() {
	Release();
}
gstd::ref_count_ptr<DxMeshData> DxMesh::_GetFromManager(std::wstring name) {
	return DxMeshManager::GetBase()->_GetMeshData(name);
}
void DxMesh::_AddManager(std::wstring name, gstd::ref_count_ptr<DxMeshData> data) {
	DxMeshManager::GetBase()->_AddMeshData(name, data);
}
void DxMesh::Release() {
	{
		DxMeshManager* manager = DxMeshManager::GetBase();
		Lock lock(manager->GetLock());
		if (data_ != nullptr) {
			if (manager->IsDataExists(data_->GetName())) {
				int countRef = data_.GetReferenceCount();
				//自身とDxMeshManager内の数だけになったら削除
				if (countRef == 2) {
					manager->_ReleaseMeshData(data_->GetName());
				}
			}
			data_ = nullptr;
		}
	}
}
bool DxMesh::CreateFromFile(std::wstring path) {
	try {
		path = PathProperty::GetUnique(path);
		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr)throw gstd::wexception("File not found.");
		return CreateFromFileReader(reader);
	}
	catch (gstd::wexception& e) {
		std::string str = StringUtility::Format("DxMesh: Mesh load failed. [%s]\n\t%s", path.c_str(), e.what());
		Logger::WriteTop(str);
	}
	return false;
}
bool DxMesh::CreateFromFileInLoadThread(std::wstring path, int type) {
	bool res = false;
	{
		Lock lock(DxMeshManager::GetBase()->GetLock());
		if (data_ != nullptr)Release();
		DxMeshManager* manager = DxMeshManager::GetBase();
		ref_count_ptr<DxMesh> mesh = manager->CreateFromFileInLoadThread(path, type);
		if (mesh != nullptr) {
			data_ = mesh->data_;
		}
		res = data_ != nullptr;
	}

	return res;
}
void DxMesh::SetColorRGB(D3DCOLOR color) {
	int r = ColorAccess::GetColorR(color);
	int g = ColorAccess::GetColorG(color);
	int b = ColorAccess::GetColorB(color);
	ColorAccess::SetColorR(color_, r);
	ColorAccess::SetColorG(color_, g);
	ColorAccess::SetColorB(color_, b);
}
void DxMesh::SetAlpha(int alpha) {
	ColorAccess::SetColorA(color_, alpha);
}
/**********************************************************
//DxMeshManager
**********************************************************/
DxMeshManager* DxMeshManager::thisBase_ = nullptr;
DxMeshManager::DxMeshManager() {}
DxMeshManager::~DxMeshManager() {
	this->Clear();
	FileManager::GetBase()->RemoveLoadThreadListener(this);
	panelInfo_ = nullptr;
	thisBase_ = nullptr;
}
bool DxMeshManager::Initialize() {
	thisBase_ = this;
	FileManager::GetBase()->AddLoadThreadListener(this);
	return true;
}

void DxMeshManager::Clear() {
	{
		Lock lock(lock_);
		mapMesh_.clear();
		mapMeshData_.clear();
	}
}

void DxMeshManager::_AddMeshData(std::wstring name, gstd::ref_count_ptr<DxMeshData> data) {
	{
		Lock lock(lock_);
		if (!IsDataExists(name)) {
			mapMeshData_[name] = data;
		}
	}
}
gstd::ref_count_ptr<DxMeshData> DxMeshManager::_GetMeshData(std::wstring name) {
	gstd::ref_count_ptr<DxMeshData> res = nullptr;
	{
		Lock lock(lock_);
		if (IsDataExists(name)) {
			res = mapMeshData_[name];
		}
	}
	return res;
}
void DxMeshManager::_ReleaseMeshData(std::wstring name) {
	{
		Lock lock(lock_);
		if (IsDataExists(name)) {
			mapMeshData_.erase(name);
			Logger::WriteTop(StringUtility::Format("DxMeshManager: Mesh released. [%s]", name.c_str()));
		}
	}
}
void DxMeshManager::Add(std::wstring name, gstd::ref_count_ptr<DxMesh> mesh) {
	{
		Lock lock(lock_);
		bool bExist = mapMesh_.find(name) != mapMesh_.end();
		if (!bExist) {
			mapMesh_[name] = mesh;
		}
	}
}
void DxMeshManager::Release(std::wstring name) {
	{
		Lock lock(lock_);
		mapMesh_.erase(name);
	}
}
bool DxMeshManager::IsDataExists(std::wstring name) {
	bool res = false;
	{
		Lock lock(lock_);
		res = mapMeshData_.find(name) != mapMeshData_.end();
	}
	return res;
}
gstd::ref_count_ptr<DxMesh> DxMeshManager::CreateFromFileInLoadThread(std::wstring path, int type) {
	gstd::ref_count_ptr<DxMesh> res;
	{
		Lock lock(lock_);
		bool bExist = mapMesh_.find(path) != mapMesh_.end();
		if (bExist) {
			res = mapMesh_[path];
		}
		else {
			if (type == MESH_ELFREINA)res = new ElfreinaMesh();
			else if (type == MESH_METASEQUOIA)res = new MetasequoiaMesh();
			if (!IsDataExists(path)) {
				ref_count_ptr<DxMeshData> data = nullptr;
				if (type == MESH_ELFREINA)data = new ElfreinaMeshData();
				else if (type == MESH_METASEQUOIA)data = new MetasequoiaMeshData();
				mapMeshData_[path] = data;
				data->manager_ = this;
				data->name_ = path;
				data->bLoad_ = false;

				ref_count_ptr<FileManager::LoadObject> source = res;
				ref_count_ptr<FileManager::LoadThreadEvent> event = new FileManager::LoadThreadEvent(this, path, res);
				FileManager::GetBase()->AddLoadThreadEvent(event);
			}

			res->data_ = mapMeshData_[path];
		}
	}
	return res;
}
void DxMeshManager::CallFromLoadThread(ref_count_ptr<FileManager::LoadThreadEvent> event) {
	std::wstring path = event->GetPath();
	{
		Lock lock(lock_);
		ref_count_ptr<DxMesh> mesh = ref_count_ptr<DxMesh>::DownCast(event->GetSource());
		if (mesh == nullptr)return;

		ref_count_ptr<DxMeshData> data = mesh->data_;
		if (data->bLoad_)return;

		bool res = false;
		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader != nullptr && reader->Open()) {
			res = data->CreateFromFileReader(reader);
		}
		if (res) {
			Logger::WriteTop(StringUtility::Format("Mesh loaded.(LT) [%s]", path.c_str()));
		}
		else {
			Logger::WriteTop(StringUtility::Format("Failed to load mesh.(LT) [%s]", path.c_str()));
		}
		data->bLoad_ = true;
	}
}

//DxMeshInfoPanel
DxMeshInfoPanel::DxMeshInfoPanel() {
	timeUpdateInterval_ = 500;
}
DxMeshInfoPanel::~DxMeshInfoPanel() {
	Stop();
	Join(1000);
}
bool DxMeshInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(64, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(96, ROW_NAME, L"Name");
	wndListView_.AddColumn(48, ROW_FULLNAME, L"FullName");
	wndListView_.AddColumn(32, ROW_COUNT_REFFRENCE, L"Ref");

	Start();

	return true;
}
void DxMeshInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void DxMeshInfoPanel::_Run() {
	while (GetStatus() == RUN) {
		DxMeshManager* manager = DxMeshManager::GetBase();
		if (manager != nullptr)
			Update(manager);
		Sleep(timeUpdateInterval_);
	}
}
void DxMeshInfoPanel::Update(DxMeshManager* manager) {
	if (!IsWindowVisible())return;
	std::set<std::wstring> setKey;
	std::map<std::wstring, gstd::ref_count_ptr<DxMeshData> >& mapData = manager->mapMeshData_;
	std::map<std::wstring, gstd::ref_count_ptr<DxMeshData> >::iterator itrMap;
	{
		Lock lock(manager->GetLock());
		for (itrMap = mapData.begin(); itrMap != mapData.end(); ++itrMap) {
			std::wstring name = itrMap->first;
			DxMeshData* data = (itrMap->second).GetPointer();

			int address = (int)data;
			std::wstring key = StringUtility::Format(L"%08x", address);
			int index = wndListView_.GetIndexInColumn(key, ROW_ADDRESS);
			if (index == -1) {
				index = wndListView_.GetRowCount();
				wndListView_.SetText(index, ROW_ADDRESS, key);
			}

			int countRef = (itrMap->second).GetReferenceCount();

			wndListView_.SetText(index, ROW_NAME, PathProperty::GetFileName(name));
			wndListView_.SetText(index, ROW_FULLNAME, name);
			wndListView_.SetText(index, ROW_COUNT_REFFRENCE, StringUtility::Format(L"%d", countRef));

			setKey.insert(key);
		}
	}

	for (int iRow = 0; iRow < wndListView_.GetRowCount();) {
		std::wstring key = wndListView_.GetText(iRow, ROW_ADDRESS);
		if (setKey.find(key) != setKey.end())++iRow;
		else wndListView_.DeleteRow(iRow);
	}
}


