#include "source/GcLib/pch.h"

#include "VertexBuffer.hpp"

namespace directx {
	VertexBufferManager* VertexBufferManager::thisBase_ = nullptr;
	VertexBufferManager::VertexBufferManager(IDirect3DDevice9* device) : indexBuffer_(nullptr) {
		Initialize(device);
	}
	VertexBufferManager::~VertexBufferManager() {
		Release();
	}

	void VertexBufferManager::Initialize(IDirect3DDevice9* device) {
		indexBuffer_ = nullptr;

		const DWORD fvfs[] = {
			VERTEX_TLX::fvf,
			VERTEX_NX::fvf,
			VERTEX_LX::fvf
		};
		const size_t sizes[] = {
			sizeof(VERTEX_TLX),
			sizeof(VERTEX_LX),
			sizeof(VERTEX_NX)
		};

		for (size_t i = 0; i < 3; ++i) {
			IDirect3DVertexBuffer9* buffer = nullptr;

			device->CreateVertexBuffer(sizes[i] * 65536, D3DUSAGE_DYNAMIC, fvfs[i], D3DPOOL_DEFAULT, &buffer, nullptr);

			vertexBuffers_.push_back(buffer);
		}
		device->CreateIndexBuffer(sizeof(uint16_t) * 65536, D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT,
			&indexBuffer_, nullptr);

		thisBase_ = this;
	}
	void VertexBufferManager::Release() {
		ptr_release(indexBuffer_);
		for (IDirect3DVertexBuffer9* buffer : vertexBuffers_)
			ptr_release(buffer);
	}
}