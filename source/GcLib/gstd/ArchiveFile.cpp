#include "ArchiveFile.h"

using namespace gstd;

/**********************************************************
//ArchiveFileEntry
**********************************************************/
void ArchiveFileEntry::_WriteEntryRecord(std::stringstream& buf) {
	uint32_t directorySize = directory.size();
	buf.write((char*)&directorySize, sizeof(uint32_t));
	buf.write((char*)directory.c_str(), directorySize * sizeof(wchar_t));

	uint32_t nameSize = name.size();
	buf.write((char*)&nameSize, sizeof(uint32_t));
	buf.write((char*)name.c_str(), nameSize * sizeof(wchar_t));

	buf.write((char*)&compressionType, sizeof(TypeCompression));
	buf.write((char*)&sizeFull, sizeof(uint32_t));
	buf.write((char*)&sizeStored, sizeof(uint32_t));
	buf.write((char*)&offsetPos, sizeof(uint32_t));
}
void ArchiveFileEntry::_ReadEntryRecord(std::stringstream& buf) {
	uint32_t directorySize = 0U;
	buf.read((char*)&directorySize, sizeof(uint32_t));
	directory.resize(directorySize);
	buf.read((char*)directory.c_str(), directorySize * sizeof(wchar_t));

	uint32_t nameSize = 0U;
	buf.read((char*)&nameSize, sizeof(uint32_t));
	name.resize(nameSize);
	buf.read((char*)name.c_str(), nameSize * sizeof(wchar_t));

	buf.read((char*)&compressionType, sizeof(TypeCompression));
	buf.read((char*)&sizeFull, sizeof(uint32_t));
	buf.read((char*)&sizeStored, sizeof(uint32_t));
	buf.read((char*)&offsetPos, sizeof(uint32_t));
}
/**********************************************************
//FileArchiver
**********************************************************/
FileArchiver::FileArchiver() {

}
FileArchiver::~FileArchiver() {

}
bool FileArchiver::CreateArchiveFile(std::wstring path) {
	bool res = true;

	std::ofstream fileArchive;
	fileArchive.open(path, std::ios::binary);

	if (!fileArchive.is_open())
		throw gstd::wexception(StringUtility::Format(L"Cannot create an archive at [%s].", path.c_str()).c_str());

	ArchiveFileHeader header;
	memcpy(header.magic, HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH);
	header.entryCount = listEntry_.size();
	//header.headerCompressed = true;
	header.headerOffset = 0U;
	header.headerSize = 0U;

	fileArchive.write((char*)&header, sizeof(ArchiveFileHeader));

	std::streampos sDataBegin = fileArchive.tellp();

	//Write the files and record their information.
	for (auto itr = listEntry_.begin(); itr != listEntry_.end(); itr++) {
		ArchiveFileEntry* entry = *itr;

		std::ifstream file;
		file.open(entry->name, std::ios::binary);
		if (!file.is_open())
			throw gstd::wexception(StringUtility::Format(L"Cannot open file for reading. [%s]", path.c_str()).c_str());

		file.seekg(0, std::ios::end);
		entry->sizeFull = file.tellg();
		entry->sizeStored = entry->sizeFull;
		entry->offsetPos = fileArchive.tellp();
		file.seekg(0, std::ios::beg);

		//Small files actually get bigger upon compression.
		if (entry->sizeFull < 0x100) entry->compressionType = ArchiveFileEntry::CT_NONE;

		switch (entry->compressionType) {
		case ArchiveFileEntry::CT_NONE:
		{
			fileArchive << file.rdbuf();
			break;
		}
		case ArchiveFileEntry::CT_ZLIB:
		{
			size_t countByte = 0U;
			Compressor::Deflate(file, fileArchive, entry->sizeFull, &countByte);
			entry->sizeStored = countByte;
			break;
		}
		}

		file.close();
	}

	std::streampos sOffsetInfoBegin = fileArchive.tellp();

	fileArchive.flush();

	//Write the info header at the end, always compressed.
	{
		fileArchive.seekp(sOffsetInfoBegin, std::ios::beg);

		std::stringstream buf;
		size_t totalSize = 0U;

		for (auto itr = listEntry_.begin(); itr != listEntry_.end(); itr++) {
			ArchiveFileEntry* entry = *itr;

			std::wstring name = entry->name;
			entry->name = PathProperty::GetFileName(name);

			uint32_t sz = entry->GetRecordSize();
			buf.write((char*)&sz, sizeof(uint32_t));
			entry->_WriteEntryRecord(buf);

			entry->name = name;
			totalSize += sz + sizeof(uint32_t);
		}

		size_t countByte = 0U;
		buf.seekg(0, std::ios::beg);
		Compressor::Deflate(buf, fileArchive, totalSize, &countByte);
		//fileArchive << buf.rdbuf();

		header.headerSize = countByte;
		//header.headerSize = totalSize;
		header.headerOffset = sOffsetInfoBegin;
	}

	fileArchive.seekp(ArchiveFileHeader::MAGIC_LENGTH + 4);
	fileArchive.write((char*)&header.headerOffset, sizeof(uint32_t));
	fileArchive.write((char*)&header.headerSize, sizeof(uint32_t));

	fileArchive.close();
	return res;
}

/**********************************************************
//ArchiveFile
**********************************************************/
ArchiveFile::ArchiveFile(std::wstring path) {
	file_.open(path, std::ios::binary);
	basePath_ = path;
}
ArchiveFile::~ArchiveFile() {
	Close();
}
bool ArchiveFile::Open() {
	if (!file_.is_open())return false;
	if (mapEntry_.size() != 0)return true;

	bool res = true;
	try {
		ArchiveFileHeader header;

		file_.read((char*)&header, sizeof(ArchiveFileHeader));

		if (memcmp(header.magic, HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) != 0) throw gstd::wexception();

		uint32_t headerSizeTrue = 0U;

		std::stringstream bufInfo;
		file_.seekg(header.headerOffset, std::ios::beg);
		{
			bufInfo.seekg(0, std::ios::beg);
			Compressor::Inflate(file_, bufInfo, header.headerSize, &headerSizeTrue);
		}

		bufInfo.clear();
		bufInfo.seekg(0, std::ios::beg);

		/*
		{
			std::wstring pathTest = PathProperty::GetModuleDirectory() + L"tmp\\hdrDeCmp.dat";
			File file(pathTest);
			file.CreateDirectory();
			file.Create();
			file.Write((char*)bufInfo.rdbuf()->str().c_str(), headerSizeTrue);
			file.Close();
		}
		*/

		for (size_t iEntry = 0U; iEntry < header.entryCount; iEntry++) {
			ArchiveFileEntry* entry = new ArchiveFileEntry();

			uint32_t sizeEntry = 0U; 
			bufInfo.read((char*)&sizeEntry, sizeof(uint32_t));

			entry->_ReadEntryRecord(bufInfo);
			entry->archiveParent = basePath_;

			//std::string key = entry->GetDirectory() + entry->GetName();
			//mapEntry_[key] = entry;
			mapEntry_.insert(std::pair<std::wstring, ArchiveFileEntry*>(entry->name, entry));
		}

		res = true;
	}
	catch (...) {
		res = false;
	}
	file_.close();
	return res;
}
void ArchiveFile::Close() {
	file_.close();

	for (auto& itr : mapEntry_) {
		if (itr.second) delete itr.second;
		itr.second = nullptr;
	}
	mapEntry_.clear();
}
std::set<std::wstring> ArchiveFile::GetKeyList() {
	std::set<std::wstring> res;
	for (auto itr = mapEntry_.begin(); itr != mapEntry_.end(); itr++) {
		ArchiveFileEntry* entry = itr->second;
		//std::wstring key = entry->GetDirectory() + entry->GetName();
		res.insert(entry->name);
	}
	return res;
}
std::vector<ArchiveFileEntry*> ArchiveFile::GetEntryList(std::wstring name) {
	std::vector<ArchiveFileEntry*> res;
	if (!IsExists(name))return res;

	for (auto itrPair = mapEntry_.equal_range(name); itrPair.first != itrPair.second; itrPair.first++) {
		ArchiveFileEntry* entry = (itrPair.first)->second;
		res.push_back(entry);
	}

	return res;
}
bool ArchiveFile::IsExists(std::wstring name) {
	return mapEntry_.find(name) != mapEntry_.end();
}
ref_count_ptr<ByteBuffer> ArchiveFile::CreateEntryBuffer(ArchiveFileEntry* entry) {
	ref_count_ptr<ByteBuffer> res;

	std::ifstream file;
	file.open(entry->archiveParent, std::ios::binary);
	if (file.is_open()) {
		switch (entry->compressionType) {
		case ArchiveFileEntry::CT_NONE:
		{
			file.seekg(entry->offsetPos, std::ios::beg);
			res = new ByteBuffer();
			res->SetSize(entry->sizeFull);
			file.read(res->GetPointer(), entry->sizeFull);
			break;
		}
		case ArchiveFileEntry::CT_ZLIB:
		{
			file.seekg(entry->offsetPos, std::ios::beg);
			res = new ByteBuffer();
			//res->SetSize(entry->sizeFull);

			std::stringstream stream;
			Compressor::Inflate(file, stream, entry->sizeStored, nullptr);

			res->Copy(stream);

			break;
		}
		}
	}
	return res;
}
/*
ref_count_ptr<ByteBuffer> ArchiveFile::GetBuffer(std::string name)
{
	if(!IsExists(name))return NULL;

	if(!file_->Open())return NULL;

	ref_count_ptr<ByteBuffer> res = new ByteBuffer();
	ref_count_ptr<ArchiveFileEntry> entry = mapEntry_[name];
	int offset = entry->GetOffset();
	int size = entry->GetDataSize();

	res->SetSize(size);
	file_->Seek(offset);
	file_->Read(res->GetPointer(), size);

	file_->Close();
	return res;
}
*/

/**********************************************************
//Compressor
**********************************************************/
template<typename TIN, typename TOUT>
bool Compressor::Deflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res) {
	bool ret = true;

	const size_t CHUNK = 16384U;
	char in[CHUNK];
	char out[CHUNK];

	int returnState = 0;
	size_t countBytes = 0U;

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	returnState = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
	if (returnState != Z_OK) return false;

	try {
		int flushType = Z_NO_FLUSH;

		do {
			bufIn.read(in, CHUNK);
			size_t read = bufIn.gcount();
			if (read > count) {
				flushType = Z_FINISH;
				read = count;
			}
			else if (read < CHUNK) { 
				flushType = Z_FINISH; 
			}

			if (read > 0) {
				stream.next_in = (Bytef*)in;
				stream.avail_in = bufIn.gcount();

				do {
					stream.next_out = (Bytef*)out;
					stream.avail_out = CHUNK;

					returnState = deflate(&stream, flushType);

					size_t availWrite = CHUNK - stream.avail_out;
					countBytes += availWrite;
					if (returnState != Z_STREAM_ERROR)
						bufOut.write(out, availWrite);
					else throw returnState;
				} while (stream.avail_out == 0);
			}
			count -= read;
		} while (count > 0U && flushType != Z_FINISH);
	}
	catch (int&) {
		ret = false;
	}

	deflateEnd(&stream);
	if (res) *res = countBytes;
	return ret;
}
template<typename TIN, typename TOUT>
bool Compressor::Inflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res) {
	bool ret = true;

	const size_t CHUNK = 16384U;
	char in[CHUNK];
	char out[CHUNK];

	int returnState = 0;
	size_t countBytes = 0U;

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in = Z_NULL;
	returnState = inflateInit(&stream);
	if (returnState != Z_OK) return false;

	try {
		size_t read = 0U;

		do {
			bufIn.read(in, CHUNK);
			read = bufIn.gcount();
			if (read > count) read = count;

			if (read > 0U) {
				stream.avail_in = read;
				stream.next_in = (Bytef*)in;

				do {
					stream.next_out = (Bytef*)out;
					stream.avail_out = CHUNK;

					returnState = inflate(&stream, Z_NO_FLUSH);
					switch (returnState) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					case Z_STREAM_ERROR:
						throw returnState;
					}

					size_t availWrite = CHUNK - stream.avail_out;
					countBytes += availWrite;
					bufOut.write(out, availWrite);
				} while (stream.avail_out == 0);
			}
			count -= read;
		} while (count > 0U && read > 0U);
	}
	catch (int&) {
		ret = false;
	}

	inflateEnd(&stream);
	if (res) *res = countBytes;
	return ret;
}