#include "source/GcLib/pch.h"

#include "RenderObject.hpp"

#include "DirectGraphics.hpp"
#include "Shader.hpp"

#include "MetasequoiaMesh.hpp"
#include "ElfreinaMesh.hpp"

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

	bCoordinate2D_ = false;

	flgUseVertexBufferMode_ = true;

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;
	filterMip_ = D3DTEXF_NONE;

	modeCulling_ = D3DCULL_NONE;

	disableMatrixTransform_ = false;
}
RenderObject::~RenderObject() {
}
size_t RenderObject::_GetPrimitiveCount() {
	return _GetPrimitiveCount(GetVertexCount());
}
size_t RenderObject::_GetPrimitiveCount(size_t count) {
	size_t res = 0;
	if (vertexIndices_.size() > 0)
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
	{
		float cx = angleX.x;
		float sx = angleX.y;
		float cy = angleY.x;
		float sy = angleY.y;
		float cz = angleZ.x;
		float sz = angleZ.y;
		mat._11 = cy * cz - sx * sy * sz;
		mat._12 = -cx * sz;
		mat._13 = sy * cz + sx * cy * sz;
		mat._21 = cy * sz + sx * sy * cz;
		mat._22 = cx * cz;
		mat._23 = sy * sz - sx * cy * cz;
		mat._31 = -cx * sy;
		mat._32 = sx;
		mat._33 = cx * cy;
	}

	bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
	bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;
	if (bScale || bPos) {
		if (bScale) {
			mat._11 *= scale.x;
			mat._12 *= scale.x;
			mat._13 *= scale.x;
			mat._21 *= scale.y;
			mat._22 *= scale.y;
			mat._23 *= scale.y;
			mat._31 *= scale.z;
			mat._32 *= scale.z;
			mat._33 *= scale.z;
		}
		if (bPos) {
			mat._41 = position.x;
			mat._42 = position.y;
			mat._43 = position.z;
		}
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
	{
		float cx = angleX.x;
		float sx = angleX.y;
		float cy = angleY.x;
		float sy = angleY.y;
		float cz = angleZ.x;
		float sz = angleZ.y;
		mat._11 = cy * cz - sx * sy * sz;
		mat._12 = -cx * sz;
		mat._13 = sy * cz + sx * cy * sz;
		mat._21 = cy * sz + sx * sy * cz;
		mat._22 = cx * cz;
		mat._23 = sy * sz - sx * cy * cz;
		mat._31 = -cx * sy;
		mat._32 = sx;
		mat._33 = cx * cy;
	}

	if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
		mat._11 *= scale.x;
		mat._12 *= scale.x;
		mat._13 *= scale.x;
		mat._21 *= scale.y;
		mat._22 *= scale.y;
		mat._23 *= scale.y;
		mat._31 *= scale.z;
		mat._32 *= scale.z;
		mat._33 *= scale.z;
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
		float cx = angleX.x;
		float sx = angleX.y;
		float cy = angleY.x;
		float sy = angleY.y;
		float cz = angleZ.x;
		float sz = angleZ.y;
		mat._11 = cy * cz - sx * sy * sz;
		mat._12 = -cx * sz;
		mat._13 = sy * cz + sx * cy * sz;
		mat._21 = cy * sz + sx * sy * cz;
		mat._22 = cx * cz;
		mat._23 = sy * sz - sx * cy * cz;
		mat._31 = -cx * sy;
		mat._32 = sx;
		mat._33 = cx * cy;
	}

	bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
	bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

	if (bScale || bPos) {
		if (bScale) {
			mat._11 *= scale.x;
			mat._12 *= scale.x;
			mat._13 *= scale.x;
			mat._21 *= scale.y;
			mat._22 *= scale.y;
			mat._23 *= scale.y;
			mat._31 *= scale.z;
			mat._32 *= scale.z;
			mat._33 *= scale.z;
		}
		if (bPos) {
			mat._41 = position.x;
			mat._42 = position.y;
			mat._43 = position.z;
		}
	}
	if (matCamera != nullptr) mat = mat * (*matCamera);

	return mat;
}
D3DXMATRIX RenderObject::CreateWorldMatrixText2D(D3DXVECTOR2& centerPosition, D3DXVECTOR3& scale,
	D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
	D3DXVECTOR2& objectPosition, D3DXVECTOR2& biasPosition, D3DXMATRIX* matCamera)
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation(&mat, -centerPosition.x, -centerPosition.y, 0.0f);

	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
		bool bPos = objectPosition.x != 0.0f || objectPosition.y != 0.0f;

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
			matSRT._41 = objectPosition.x;
			matSRT._42 = objectPosition.y;
			matSRT._43 = 0.0f;
		}

		matSRT = mat * matSRT;
		D3DXMatrixTranslation(&mat, centerPosition.x + biasPosition.x, centerPosition.y + biasPosition.y, 0.0f);
		mat = matSRT * mat;
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
	if (texture == nullptr)
		texture_[stage] = nullptr;
	else {
		if (stage >= texture_.size())return;
		texture_[stage] = new Texture(texture);
	}
}
void RenderObject::SetTexture(ref_count_ptr<Texture> texture, int stage) {
	if (texture == nullptr)
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
void RenderObjectTLX::Render() {
	RenderObjectTLX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectTLX::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	bool bCamera = camera->IsEnable() && bPermitCamera_;

	D3DXMATRIX matWorld;
	if (!disableMatrixTransform_) {
		matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
			angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);
	}

	RenderObjectTLX::Render(matWorld);
}
void RenderObjectTLX::Render(D3DXMATRIX& matTransform) {
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

	{
		vertCopy_ = vertex_;

		size_t countVertex = GetVertexCount();

//#pragma omp for if(flgUseVertexBufferMode_)
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			size_t pos = iVert * strideVertexStreamZero_;
			VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
			D3DXVECTOR4* vPos = &vert->position;

			D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matTransform);
		}
		
		{
			VertexBufferManager* vbManager = VertexBufferManager::GetBase();

			IDirect3DVertexBuffer9* vertexBuffer = vbManager->GetVertexBuffer(VertexBufferManager::BUFFER_VERTEX_TLX);
			IDirect3DIndexBuffer9* indexBuffer = vbManager->GetIndexBuffer();
			
			bool bUseIndex = vertexIndices_.size() > 0;
			size_t countPrim = _GetPrimitiveCount(countVertex);

			if (flgUseVertexBufferMode_) {
				void* tmp;

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
			}

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

					if (flgUseVertexBufferMode_) {
						if (bUseIndex) device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
						else device->DrawPrimitive(typePrimitive_, 0, countPrim);
					}
					else {
						if (bUseIndex)
							device->DrawIndexedPrimitiveUP(typePrimitive_, 0, countVertex, countPrim,
								vertexIndices_.data(), D3DFMT_INDEX16, vertCopy_.GetPointer(), strideVertexStreamZero_);
						else
							device->DrawPrimitiveUP(typePrimitive_, countPrim, vertCopy_.GetPointer(), strideVertexStreamZero_);
					}

					if (effect != nullptr) effect->EndPass();
				}
				if (effect != nullptr) effect->End();
			}

			device->SetIndices(nullptr);
		}
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

		bool bUseIndex = vertexIndices_.size() > 0;
		size_t countVertex = GetVertexCount();
		size_t countPrim = _GetPrimitiveCount(countVertex);

		if (flgUseVertexBufferMode_) {
			void* tmp;
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

			device->SetStreamSource(0, vertexBuffer, 0, sizeof(VERTEX_LX));
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

			if (flgUseVertexBufferMode_) {
				if (bUseIndex) device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				else device->DrawPrimitive(typePrimitive_, 0, countPrim);
			}
			else {
				if (bUseIndex)
					device->DrawIndexedPrimitiveUP(typePrimitive_, 0, countVertex, countPrim,
						vertexIndices_.data(), D3DFMT_INDEX16, vertex_.GetPointer(), strideVertexStreamZero_);
				else
					device->DrawPrimitiveUP(typePrimitive_, countPrim, vertex_.GetPointer(), strideVertexStreamZero_);
			}

			if (effect != nullptr) effect->EndPass();
		}
		if (effect != nullptr) effect->End();

		device->SetIndices(nullptr);
	}
}

/**********************************************************
//RenderObjectNX
**********************************************************/
RenderObjectNX::RenderObjectNX() {
	_SetTextureStageCount(1);
	strideVertexStreamZero_ = sizeof(VERTEX_NX);

	color_ = 0xffffffff;

	pVertexBuffer_ = nullptr;
	pIndexBuffer_ = nullptr;
}
RenderObjectNX::~RenderObjectNX() {
	ptr_release(pVertexBuffer_);
	ptr_release(pIndexBuffer_);
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

/**********************************************************
//RenderObjectBNX
**********************************************************/
RenderObjectBNX::RenderObjectBNX() {
	_SetTextureStageCount(1);
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	materialBNX_.Diffuse.a = 1.0f; materialBNX_.Diffuse.r = 1.0f; materialBNX_.Diffuse.g = 1.0f; materialBNX_.Diffuse.b = 1.0f;
	materialBNX_.Ambient.a = 1.0f; materialBNX_.Ambient.r = 1.0f; materialBNX_.Ambient.g = 1.0f; materialBNX_.Ambient.b = 1.0f;
	materialBNX_.Specular.a = 1.0f; materialBNX_.Specular.r = 1.0f; materialBNX_.Specular.g = 1.0f; materialBNX_.Specular.b = 1.0f;
	materialBNX_.Emissive.a = 1.0f; materialBNX_.Emissive.r = 1.0f; materialBNX_.Emissive.g = 1.0f; materialBNX_.Emissive.b = 1.0f;

	pVertexBuffer_ = nullptr;
	pIndexBuffer_ = nullptr;
}
RenderObjectBNX::~RenderObjectBNX() {
	ptr_release(pVertexBuffer_);
	ptr_release(pIndexBuffer_);
}
void RenderObjectBNX::InitializeVertexBuffer() {
	int countVertex = GetVertexCount();
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	device->CreateVertexBuffer(countVertex * sizeof(VERTEX_BNX), 0, 0, D3DPOOL_MANAGED, &pVertexBuffer_, nullptr);

	//コピー
	_CopyVertexBufferOnInitialize();

	size_t countIndex = vertexIndices_.size();
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
			device->DrawIndexedPrimitiveUP(typePrimitive_, 0, GetVertexCount(), _GetPrimitiveCount(),
				&vertexIndices_[0], D3DFMT_INDEX16, vertex_.GetPointer(), strideVertexStreamZero_);
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
			diffuse.r = std::min(diffuse.r + light.Diffuse.r, 1.0f);
			diffuse.g = std::min(diffuse.g + light.Diffuse.g, 1.0f);
			diffuse.b = std::min(diffuse.b + light.Diffuse.b, 1.0f);
			diffuse = ColorAccess::SetColor(diffuse, color_);

			D3DCOLORVALUE ambient = materialBNX_.Ambient;
			ambient.r = std::min(ambient.r + light.Ambient.r, 1.0f);
			ambient.g = std::min(ambient.g + light.Ambient.g, 1.0f);
			ambient.b = std::min(ambient.b + light.Ambient.b, 1.0f);
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

			device->SetVertexDeclaration(shaderManager->GetVertexDeclarationBNX());
			device->SetStreamSource(0, pVertexBuffer_, 0, sizeof(VERTEX_BNX));

			UINT cPass = 1;
			shader->Begin(&cPass, 0);
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
	size_t countVertex = GetVertexCount();
	VERTEX_BNX* bufVertex;
	HRESULT hrLockVertex = pVertexBuffer_->Lock(0, 0, reinterpret_cast<void**>(&bufVertex), D3DLOCK_NOSYSLOCK);
	if (!FAILED(hrLockVertex)) {
		for (size_t iVert = 0; iVert < countVertex; ++iVert) {
			VERTEX_BNX* dest = &bufVertex[iVert];
			VERTEX_B2NX* src = GetVertex(iVert);
			ZeroMemory(dest, sizeof(VERTEX_BNX));

			dest->position = src->position;
			dest->normal = src->normal;
			dest->texcoord = src->texcoord;

			for (size_t iBlend = 0; iBlend < 2; ++iBlend) {
				size_t indexBlend = BitAccess::GetByte(src->blendIndex, iBlend * 8);
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
	for (size_t iVert = 0; iVert < countVert; ++iVert) {
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
	size_t countVertex = GetVertexCount();
	VERTEX_BNX* bufVertex;
	HRESULT hrLockVertex = pVertexBuffer_->Lock(0, 0, reinterpret_cast<void**>(&bufVertex), D3DLOCK_NOSYSLOCK);
	if (!FAILED(hrLockVertex)) {
		for (size_t iVert = 0; iVert < countVertex; ++iVert) {
			VERTEX_BNX* dest = &bufVertex[iVert];
			VERTEX_B4NX* src = GetVertex(iVert);
			ZeroMemory(dest, sizeof(VERTEX_BNX));

			dest->position = src->position;
			dest->normal = src->normal;
			dest->texcoord = src->texcoord;

			for (size_t iBlend = 0; iBlend < 4; ++iBlend) {
				size_t indexBlend = BitAccess::GetByte(src->blendIndex, iBlend * 8);
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
	size_t countVert = GetVertexCount();
	for (size_t iVert = 0; iVert < countVert; ++iVert) {
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

	vertex_ = src->vertex_;
	vertexIndices_ = src->vertexIndices_;

	for (size_t iTex = 0; iTex < texture_.size(); ++iTex) {
		texture_[iTex] = src->texture_[iTex];
	}

	posWeightCenter_ = src->posWeightCenter_;

	position_ = src->position_;
	angle_ = src->angle_;
	scale_ = src->scale_;
	matRelative_ = src->matRelative_;
}
void Sprite2D::SetSourceRect(RECT_D& rcSrc) {
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
void Sprite2D::SetDestinationRect(RECT_D& rcDest) {
	//頂点位置
	SetVertexPosition(0, rcDest.left, rcDest.top);
	SetVertexPosition(1, rcDest.right, rcDest.top);
	SetVertexPosition(2, rcDest.left, rcDest.bottom);
	SetVertexPosition(3, rcDest.right, rcDest.bottom);
}
void Sprite2D::SetVertex(RECT_D& rcSrc, RECT_D& rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}
RECT_D Sprite2D::GetDestinationRect() {
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
		vertCopy_ = vertex_;
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

		D3DXMATRIX matWorld;
		if (bCloseVertexList_)
			matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
				angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);

//#pragma omp for
		for (int iVert = 0; iVert < countVertex; ++iVert) {
			size_t pos = iVert * strideVertexStreamZero_;
			VERTEX_TLX* vert = (VERTEX_TLX*)vertCopy_.GetPointer(pos);
			D3DXVECTOR4* vPos = &vert->position;

			if (bCloseVertexList_)
				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matWorld);
			else if (bCamera)
				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &camera->GetMatrix());
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
void SpriteList2D::_AddVertex(VERTEX_TLX& vertex) {
	size_t count = vertex_.size() / strideVertexStreamZero_;
	if (countRenderVertex_ >= count) {
		//リサイズ
		size_t newCount = std::max(10U, (size_t)(count * 1.5));
		vertex_.SetSize(newCount * strideVertexStreamZero_);
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
	int destY[] = { (int)rcDest_.top, (int)rcDest_.top, (int)rcDest_.bottom, (int)rcDest_.bottom };

//#pragma omp for
	for (int iVert = 0; iVert < 4; ++iVert) {
		VERTEX_TLX vt;
		vt.texcoord.x = srcX[iVert] / width;
		vt.texcoord.y = srcY[iVert] / height;

		D3DXVECTOR4 vPos;

		constexpr float bias = -0.5f;
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
				device->DrawIndexedPrimitiveUP(typePrimitive_, 0, GetVertexCount(), _GetPrimitiveCount(),
					&vertexIndices_[0], D3DFMT_INDEX16, vertex_.GetPointer(), strideVertexStreamZero_);
			}

			if (effect != nullptr) effect->EndPass();
		}
		if (effect != nullptr) effect->End();
	}
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
void Sprite3D::SetSourceRect(RECT_D& rcSrc) {
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
void Sprite3D::SetDestinationRect(RECT_D& rcDest) {
	//頂点位置
	SetVertexPosition(0, rcDest.left, rcDest.top, 0);
	SetVertexPosition(1, rcDest.left, rcDest.bottom, 0);
	SetVertexPosition(2, rcDest.right, rcDest.top, 0);
	SetVertexPosition(3, rcDest.right, rcDest.bottom, 0);
}
void Sprite3D::SetVertex(RECT_D& rcSrc, RECT_D& rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);

	//頂点色
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}
void Sprite3D::SetVertex(RECT_D& rcSrc, D3DCOLOR color) {
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
	size_t size = listData_.size() * 2;
	SetVertexCount(size);

	int width = 1;
	gstd::ref_count_ptr<Texture> texture = texture_[0];
	if (texture != nullptr) {
		width = texture->GetWidth();
	}

	float dWidth = 1.0 / width / listData_.size();
	size_t iData = 0;
	std::list<Data>::iterator itr;
	for (itr = listData_.begin(); itr != listData_.end(); ++itr, ++iData) {
		Data data = (*itr);
		int alpha = data.alpha;
		for (size_t iPos = 0; iPos < 2; ++iPos) {
			size_t index = iData * 2 + iPos;
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
		for (size_t iCount = 0; iCount < countComplement_ - 1; ++iCount) {
			Data cData;
			float flame = cDiff * (iCount + 1);
			for (size_t iPos = 0; iPos < 2; ++iPos) {
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

		auto itr = mapMeshData_.find(name);
		if (itr != mapMeshData_.end()) {
			res = itr->second;
		}
	}
	return res;
}
void DxMeshManager::_ReleaseMeshData(std::wstring name) {
	{
		Lock lock(lock_);

		auto itr = mapMeshData_.find(name);
		if (itr != mapMeshData_.end()) {
			mapMeshData_.erase(itr);
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

		auto itrMesh = mapMesh_.find(path);
		if (itrMesh != mapMesh_.end()) {
			res = itrMesh->second;
		}
		else {
			if (type == MESH_ELFREINA)res = new ElfreinaMesh();
			else if (type == MESH_METASEQUOIA)res = new MetasequoiaMesh();

			auto itrData = mapMeshData_.find(path);
			if (itrData == mapMeshData_.end()) {
				ref_count_ptr<DxMeshData> data = nullptr;
				if (type == MESH_ELFREINA)data = new ElfreinaMeshData();
				else if (type == MESH_METASEQUOIA)data = new MetasequoiaMeshData();

				itrData = mapMeshData_.insert(std::pair<std::wstring, ref_count_ptr<DxMeshData>>(path, data)).first;

				data->manager_ = this;
				data->name_ = path;
				data->bLoad_ = false;

				ref_count_ptr<FileManager::LoadObject> source = res;
				ref_count_ptr<FileManager::LoadThreadEvent> event = new FileManager::LoadThreadEvent(this, path, res);
				FileManager::GetBase()->AddLoadThreadEvent(event);
			}

			res->data_ = itrData->second;
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


