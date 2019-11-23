#pragma once

#include "File.hpp"

namespace gstd {
	static constexpr const char* HEADER_ARCHIVEFILE = "DNHARC\0\0";

	/**********************************************************
	//ArchiveFileEntry
	**********************************************************/
	class FileArchiver;
	class ArchiveFile;

#pragma pack(push, 1)
	struct ArchiveFileHeader {
		enum {
			MAGIC_LENGTH = 0x8,
		};

		char magic[MAGIC_LENGTH];
		uint32_t entryCount;
		//uint8_t headerCompressed;
		uint32_t headerOffset;
		uint32_t headerSize;
	};

	class ArchiveFileEntry {
	public:
		enum TypeCompression : uint8_t {
			CT_NONE,
			CT_ZLIB,
		};

		//uint32_t directorySize;
		std::wstring directory;
		//uint32_t nameSize;
		std::wstring name;
		TypeCompression compressionType;
		uint32_t sizeFull;
		uint32_t sizeStored;
		uint32_t offsetPos;

		std::wstring archiveParent;

		const size_t GetRecordSize() {
			return static_cast<size_t>(directory.size() * sizeof(wchar_t) + name.size() * sizeof(wchar_t) +
				sizeof(uint32_t) * 5 + sizeof(TypeCompression));
		}

		void _WriteEntryRecord(std::stringstream& buf);
		void _ReadEntryRecord(std::stringstream& buf);
	};
#pragma pack(pop)

	/**********************************************************
	//FileArchiver
	**********************************************************/
	class FileArchiver {
	private:
		std::list<ArchiveFileEntry*> listEntry_;
	public:
		FileArchiver();
		virtual ~FileArchiver();

		void AddEntry(ArchiveFileEntry* entry) { listEntry_.push_back(entry); }
		bool CreateArchiveFile(std::wstring path);
	};

	/**********************************************************
	//ArchiveFile
	**********************************************************/
	class ArchiveFile {
	private:
		std::wstring basePath_;
		std::ifstream file_;
		std::multimap<std::wstring, ArchiveFileEntry*> mapEntry_;
	public:
		ArchiveFile(std::wstring path);
		virtual ~ArchiveFile();
		bool Open();
		void Close();

		std::set<std::wstring> GetKeyList();
		std::multimap<std::wstring, ArchiveFileEntry*> GetEntryMap() { return mapEntry_; }
		std::vector<ArchiveFileEntry*> GetEntryList(std::wstring name);
		bool IsExists(std::wstring name);
		static ref_count_ptr<ByteBuffer> CreateEntryBuffer(ArchiveFileEntry* entry);
		//ref_count_ptr<ByteBuffer> GetBuffer(std::string name);
	};

	/**********************************************************
	//Compressor
	**********************************************************/
	class Compressor {
	public:
		template<typename TIN, typename TOUT>
		static bool Deflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res);
		template<typename TIN, typename TOUT>
		static bool Inflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res);
	};
}