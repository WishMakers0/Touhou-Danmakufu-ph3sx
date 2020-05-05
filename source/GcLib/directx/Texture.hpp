#ifndef __DIRECTX_TEXTURE__
#define __DIRECTX_TEXTURE__

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"

namespace directx {
	class TextureData;
	class Texture;
	class TextureManager;
	class TextureInfoPanel;

	/**********************************************************
	//Texture
	**********************************************************/
	class TextureData {
		friend Texture;
		friend TextureManager;
		friend TextureInfoPanel;
	public:
		enum {
			TYPE_TEXTURE,
			TYPE_RENDER_TARGET,
		};
	protected:
		int type_;
		TextureManager* manager_;
		IDirect3DTexture9* pTexture_;
		D3DXIMAGE_INFO infoImage_;
		std::wstring name_;
		volatile bool bLoad_;

		bool useMipMap_;
		bool useNonPowerOfTwo_;

		IDirect3DSurface9* lpRenderSurface_;//�o�b�N�o�b�t�@����(�����_�����O�^�[�Q�b�g�p)
		IDirect3DSurface9* lpRenderZ_;//�o�b�N�o�b�t�@��Z�o�b�t�@����(�����_�����O�^�[�Q�b�g�p)

		size_t resourceSize_;
	public:
		TextureData();
		virtual ~TextureData();
		std::wstring GetName() { return name_; }
		D3DXIMAGE_INFO GetImageInfo() { return infoImage_; }

		size_t GetResourceSize() { return resourceSize_; }
	};

	class Texture : public gstd::FileManager::LoadObject {
		friend TextureData;
		friend TextureManager;
		friend TextureInfoPanel;
	protected:
		shared_ptr<TextureData> data_;
		TextureData* _GetTextureData() { return data_.get(); }
	public:
		Texture();
		Texture(Texture* texture);
		virtual ~Texture();
		void Release();

		std::wstring GetName();
		bool CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo);
		bool CreateRenderTarget(std::wstring name, size_t width = 0U, size_t height = 0U);
		bool CreateFromFileInLoadThread(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo, bool bLoadImageInfo = false);

		void SetTexture(IDirect3DTexture9 *pTexture);
		IDirect3DTexture9* GetD3DTexture();
		IDirect3DSurface9* GetD3DSurface();
		IDirect3DSurface9* GetD3DZBuffer();

		int GetType();

		int GetWidth();
		int GetHeight();
		bool IsLoad() { return data_ != nullptr && data_->bLoad_; }

		static size_t GetFormatBPP(D3DFORMAT format);
	};

	/**********************************************************
	//TextureManager
	**********************************************************/
	class TextureManager : public DirectGraphicsListener, public gstd::FileManager::LoadThreadListener {
		friend Texture;
		friend TextureData;
		friend TextureInfoPanel;
		static TextureManager* thisBase_;
	public:
		static const std::wstring TARGET_TRANSITION;
	protected:
		gstd::CriticalSection lock_;

		std::map<std::wstring, gstd::ref_count_ptr<Texture>> mapTexture_;
		std::map<std::wstring, shared_ptr<TextureData>> mapTextureData_;
		gstd::ref_count_ptr<TextureInfoPanel> panelInfo_;

		void _ReleaseTextureData(std::wstring name);
		bool _CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo);
		bool _CreateRenderTarget(std::wstring name, size_t width = 0U, size_t height = 0U);
	public:
		TextureManager();
		virtual ~TextureManager();
		static TextureManager* GetBase() { return thisBase_; }
		virtual bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Clear();
		virtual void Add(std::wstring name, gstd::ref_count_ptr<Texture> texture);//�e�N�X�`���̎Q�Ƃ�ێ����܂�
		virtual void Release(std::wstring name);//�ێ����Ă���Q�Ƃ�������܂�
		virtual bool IsDataExists(std::wstring name);

		virtual void ReleaseDirectGraphics() { ReleaseDxResource(); }
		virtual void RestoreDirectGraphics() { RestoreDxResource(); }
		void ReleaseDxResource();
		void RestoreDxResource();

		shared_ptr<TextureData> GetTextureData(std::wstring name);
		gstd::ref_count_ptr<Texture> CreateFromFile(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo);//�e�N�X�`����ǂݍ��݂܂��BTextureData�͕ێ����܂����ATexture�͕ێ����܂���B
		gstd::ref_count_ptr<Texture> CreateRenderTarget(std::wstring name, size_t width = 0U, size_t height = 0U);
		gstd::ref_count_ptr<Texture> GetTexture(std::wstring name);//�쐬�ς݂̃e�N�X�`�����擾���܂�
		gstd::ref_count_ptr<Texture> CreateFromFileInLoadThread(std::wstring path, bool genMipmap, bool flgNonPowerOfTwo, bool bLoadImageInfo = false);
		virtual void CallFromLoadThread(gstd::ref_count_ptr<gstd::FileManager::LoadThreadEvent> event);

		void SetInfoPanel(gstd::ref_count_ptr<TextureInfoPanel> panel) { panelInfo_ = panel; }
	};

	/**********************************************************
	//TextureInfoPanel
	**********************************************************/
	class TextureInfoPanel : public gstd::WindowLogger::Panel, public gstd::Thread {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_NAME,
			ROW_FULLNAME,
			ROW_COUNT_REFFRENCE,
			ROW_WIDTH_IMAGE,
			ROW_HEIGHT_IMAGE,
			ROW_SIZE,
		};
		int timeUpdateInterval_;
		gstd::WListView wndListView_;
		virtual bool _AddedLogger(HWND hTab);
		void _Run();
	public:
		TextureInfoPanel();
		~TextureInfoPanel();
		virtual void LocateParts();
		virtual void Update(TextureManager* manager);
	};
}

#endif
