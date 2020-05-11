#ifndef __DIRECTX_METASEQUOIAMESH__
#define __DIRECTX_METASEQUOIAMESH__

#include "../pch.h"
#include "RenderObject.hpp"

namespace directx {
	/**********************************************************
	//MetasequoiaMesh
	**********************************************************/
	class MetasequoiaMesh;
	class MetasequoiaMeshData : public DxMeshData {
		friend MetasequoiaMesh;
	public:
		class Material;
		class Object;
		class RenderObject;

		struct NormalData {
			std::vector<uint16_t> listIndex_;
			D3DXVECTOR3 normal_;
			virtual ~NormalData() {}
		};
	protected:
		std::wstring path_;
		std::vector<RenderObject*> renderList_;
		std::vector<Material*> materialList_;

		void _ReadMaterial(gstd::Scanner& scanner);
		void _ReadObject(gstd::Scanner& scanner);
	public:
		MetasequoiaMeshData();
		~MetasequoiaMeshData();
		bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader);
	};

	class MetasequoiaMeshData::Material {
		friend MetasequoiaMesh;
		friend MetasequoiaMeshData;
		friend MetasequoiaMeshData::RenderObject;
	protected:
		std::wstring name_;//材質名
		D3DMATERIAL9 mat_;
		gstd::ref_count_ptr<Texture> texture_;//模様マッピング 相対パス
		std::string pathTextureAlpha_;//透明マッピングの名前 相対パス(未使用)
		std::string pathTextureBump_;//凹凸マッピングの名前 相対パス(未使用)

	public:
		Material() { ZeroMemory(&mat_, sizeof(D3DMATERIAL9)); };
		virtual ~Material() {};
	};

	class MetasequoiaMeshData::Object {
		friend MetasequoiaMeshData;
	protected:
		struct Face {
			//面の頂点
			struct Vertex {
				size_t indexVertex_;//頂点のインデックス
				D3DXVECTOR2 tcoord_;//テクスチャの座標
			};
			size_t indexMaterial_;//マテリアルのインデックス
			std::vector<Vertex> vertices_;//面の頂点
			Face() { indexMaterial_ = -1; }
		};
		bool bVisible_;
		std::wstring name_;//オブジェクト名
		std::vector<D3DXVECTOR3> vertices_;//頂点たち
		std::vector<Face> faces_;//面たち
	public:
		Object() {}
		virtual ~Object() {}
	};

	class MetasequoiaMeshData::RenderObject : public RenderObjectNX {
		friend MetasequoiaMeshData;
	protected:
		Material* material_;
	public:
		RenderObject() {};
		virtual ~RenderObject() {};
		virtual void Render() {
		}
		void Render(D3DXMATRIX* matTransform);
	};

	class MetasequoiaMesh : public DxMesh {
	public:
		MetasequoiaMesh() {}
		virtual ~MetasequoiaMesh() {}
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader);
		virtual bool CreateFromFileInLoadThread(std::wstring path);
		virtual std::wstring GetPath();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
	};
}

#endif
