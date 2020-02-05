#ifndef __DIRECTX_SHADER__
#define __DIRECTX_SHADER__

#include "../pch.h"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"
#include "Texture.hpp"

namespace directx {
	//http://msdn.microsoft.com/ja-jp/library/bb944006(v=vs.85).aspx
	//http://msdn.microsoft.com/ja-jp/library/bb509647(v=vs.85).aspx

	class Shader;
	class ShaderData;

	/*
	class ShaderCache {
	public:
		static ShaderCache* base_;
		static gstd::CriticalSection lock_;

		ShaderCache();
		~ShaderCache();

		void Initialize();
		void Release();

		bool CompileFromText(std::string& source);
		bool CompileFromFile(std::wstring path);
		bool CompileFromFileInLoadThread(std::wstring path);

		void AddShader(std::wstring name, std::shared_ptr<ShaderData>);
		void DeleteShader(std::wstring& name);

	protected:
		std::map<std::wstring, std::shared_ptr<ShaderData>> mapShaderData_;
		std::wstring lastError_;
	};
	*/

	/**********************************************************
	//ShaderData
	**********************************************************/
	class ShaderData {
		friend Shader;
	private:
		ID3DXEffect* effect_;
		std::wstring name_;
		volatile bool bLoad_;
		volatile bool bText_;
	public:
		ShaderData();
		virtual ~ShaderData();
		std::wstring GetName() { return name_; }
	};

	/**********************************************************
	//Shader
	**********************************************************/
	class Shader {
	private:
		static gstd::CriticalSection lock_;
		static std::wstring lastError_;
	protected:
		ShaderData* data_;

		std::string technique_;

		ShaderData* _GetShaderData() { return data_; }

		gstd::ref_count_ptr<Texture> paramTexture_;
	public:
		Shader();
		Shader(Shader* shader);
		virtual ~Shader();
		void Release();

		ID3DXEffect* GetEffect();

		bool CreateFromFile(std::wstring path);
		bool CreateFromText(std::string& source);
		bool IsLoad() { return data_ != NULL && data_->bLoad_; }

		static gstd::CriticalSection* GetLock() { return &lock_; }
		static std::wstring& GetLastError() { return lastError_; }

		bool SetTexture(std::string name, gstd::ref_count_ptr<Texture> texture);
	};
}


#endif
