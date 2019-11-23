#ifndef __DIRECTX_METASEQUOIAMESH__
#define __DIRECTX_METASEQUOIAMESH__

#include"RenderObject.hpp"

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
			std::vector<int> listIndex_;
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
		std::wstring name_;//�ގ���
		D3DMATERIAL9 mat_;
		gstd::ref_count_ptr<Texture> texture_;//�͗l�}�b�s���O ���΃p�X
		std::string pathTextureAlpha_;//�����}�b�s���O�̖��O ���΃p�X(���g�p)
		std::string pathTextureBump_;//���ʃ}�b�s���O�̖��O ���΃p�X(���g�p)

	public:
		Material() { ZeroMemory(&mat_, sizeof(D3DMATERIAL9)); };
		virtual ~Material() {};
	};

	class MetasequoiaMeshData::Object {
		friend MetasequoiaMeshData;
	protected:
		struct Face {
			//�ʂ̒��_
			struct Vertex {
				long indexVertex_;//���_�̃C���f�b�N�X
				D3DXVECTOR2 tcoord_;//�e�N�X�`���̍��W
			};
			long indexMaterial_;//�}�e���A���̃C���f�b�N�X
			std::vector<Vertex> vertices_;//�ʂ̒��_
			Face() { indexMaterial_ = -1; }
		};
		bool bVisible_;
		std::wstring name_;//�I�u�W�F�N�g��
		std::vector<D3DXVECTOR3> vertices_;//���_����
		std::vector<Face> faces_;//�ʂ���
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
		virtual void Render();
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
