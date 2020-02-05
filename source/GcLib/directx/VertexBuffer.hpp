#ifndef __DIRECTX_VERTEXBUFFER__
#define __DIRECTX_VERTEXBUFFER__

#include "../pch.h"

#include "DxConstant.hpp"
#include "Vertex.hpp"

namespace directx {
	class VertexBufferManager {
		static VertexBufferManager* thisBase_;
	public:
		enum {
			BUFFER_VERTEX_TLX,
			BUFFER_VERTEX_LX,
			BUFFER_VERTEX_NX,

			BYTE_MAX_TLX = 32768 * sizeof(VERTEX_TLX),
			BYTE_MAX_LX = 32768 * sizeof(VERTEX_LX),
			BYTE_MAX_NX = 32768 * sizeof(VERTEX_NX),
			BYTE_MAX_INDEX = 32768 * sizeof(uint16_t),
		};

		VertexBufferManager(IDirect3DDevice9* device);
		~VertexBufferManager();

		void Initialize(IDirect3DDevice9* device);
		void Release();

		IDirect3DVertexBuffer9* GetVertexBuffer(int index) { return vertexBuffers_[index]; }
		IDirect3DIndexBuffer9* GetIndexBuffer() { return indexBuffer_; }

		static VertexBufferManager* GetBase() { return thisBase_; }
	private:
		std::vector<IDirect3DVertexBuffer9*> vertexBuffers_;
		IDirect3DIndexBuffer9* indexBuffer_;
	};
}

#endif