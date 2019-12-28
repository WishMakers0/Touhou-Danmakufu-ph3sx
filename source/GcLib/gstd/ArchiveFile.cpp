#include "ArchiveFile.h"

using namespace gstd;

//Key string for encryption, change this to whatever you want.
static const std::string ARCHIVE_MASTER_KEY = "Tenxka's clay dildoes";

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

	buf.write((char*)&keyBase, sizeof(uint8_t));
	buf.write((char*)&keyStep, sizeof(uint8_t));
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

	buf.read((char*)&keyBase, sizeof(uint8_t));
	buf.read((char*)&keyStep, sizeof(uint8_t));
}

/**********************************************************
//FileArchiver
**********************************************************/
FileArchiver::FileArchiver() {

}
FileArchiver::~FileArchiver() {

}

//-------------------------------------------------------------------------------------------------------------

//Change these to however you want.
void FileArchiver::GetKeyHashHeader(std::string& key, uint8_t& keyBase, uint8_t& keyStep) {
	uint32_t hash = std::hash<std::string>{}(std::string(key));
	keyBase = (hash & 0x000000ff) ^ 0x55;
	keyStep = ((hash & 0x0000ff00) >> 8) ^ 0xc8;
}
void FileArchiver::GetKeyHashFile(std::wstring& key, uint8_t headerBase, uint8_t headerStep,
	uint8_t& keyBase, uint8_t& keyStep) 
{
	uint32_t hash = std::hash<std::wstring>{}(key);
	keyBase = ((hash & 0xff000000) >> 24) ^ 0x4a;
	keyStep = ((hash & 0x00ff0000) >> 16) ^ 0xeb;
}

//-------------------------------------------------------------------------------------------------------------

void FileArchiver::EncodeBlock(char* data, size_t count, uint8_t& base, uint8_t step) {
	for (size_t i = 0; i < count; ++i) {
		data[i] ^= base;
		base = (uint8_t)(((uint32_t)base + (uint32_t)step) % 0xff);
		//base ^= ~((step & 0x66) ^ 0xcc);
	}
}
bool FileArchiver::CreateArchiveFile(std::wstring path) {
	bool res = true;

	DeleteFile(path.c_str());

	uint8_t headerKeyBase = 0;
	uint8_t headerKeyStep = 0;
	GetKeyHashHeader(const_cast<std::string&>(ARCHIVE_MASTER_KEY), headerKeyBase, headerKeyStep);

	std::wstring pathTmp = StringUtility::Format(L"%s_tmp", path.c_str());

	std::ofstream fileArchive;
	fileArchive.open(pathTmp, std::ios::binary);

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
			throw gstd::wexception(StringUtility::Format(L"Cannot open file for reading. [%s]", (entry->name).c_str()).c_str());

		file.seekg(0, std::ios::end);
		entry->sizeFull = file.tellg();
		entry->sizeStored = entry->sizeFull;
		entry->offsetPos = fileArchive.tellp();
		file.seekg(0, std::ios::beg);

		uint8_t localKeyBase = 0;
		uint8_t localKeyStep = 0;
		{
			std::wstring strHash = StringUtility::Format(L"%s%s", entry->directory.c_str(), entry->name.c_str());
			GetKeyHashFile(strHash, headerKeyBase, headerKeyStep, localKeyBase, localKeyStep);
		}

		entry->keyBase = localKeyBase;
		entry->keyStep = localKeyStep;

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

	res = EncryptArchive(path, &header, headerKeyBase, headerKeyStep);

	return res;
}

bool FileArchiver::EncryptArchive(std::wstring path, ArchiveFileHeader* header, uint8_t keyBase, uint8_t keyStep) {
	std::wstring pathTmp = StringUtility::Format(L"%s_tmp", path.c_str());

	std::ifstream src;
	src.open(pathTmp, std::ios::binary);

	std::ofstream dest;
	dest.open(path, std::ios::binary);

	constexpr size_t CHUNK = 4096U;
	char buf[CHUNK];

	size_t read = 0U;
	uint8_t headerBase = keyBase;

	{
		src.read(buf, sizeof(ArchiveFileHeader));
		EncodeBlock(buf, sizeof(ArchiveFileHeader), headerBase, keyStep);
		dest.write(buf, sizeof(ArchiveFileHeader));
	}

	{
		for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr) {
			auto entry = *itr;
			size_t count = entry->sizeStored;

			uint8_t localBase = entry->keyBase;

			src.seekg(entry->offsetPos, std::ios::beg);
			dest.seekp(entry->offsetPos, std::ios::beg);

			do {
				src.read(buf, CHUNK);
				read = src.gcount();
				if (read > count) read = count;

				EncodeBlock(buf, read, localBase, (entry->keyStep << 3) ^ (entry->keyStep & 0x66));

				dest.write(buf, read);
				count -= read;
			} while (count > 0U && read > 0U);
		}
	}

	src.clear(std::ios::eofbit);

	{
		size_t infoSize = header->headerSize;

		src.seekg(header->headerOffset, std::ios::beg);
		dest.seekp(header->headerOffset, std::ios::beg);

		do {
			src.read(buf, CHUNK);
			read = src.gcount();
			if (read > infoSize) read = infoSize;

			EncodeBlock(buf, read, headerBase, keyStep);

			dest.write(buf, read);
			infoSize -= read;
		} while (infoSize > 0U && read > 0U);
	}

	src.close();
	dest.close();

	DeleteFile(pathTmp.c_str());

	return true;
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
		FileArchiver::GetKeyHashHeader(const_cast<std::string&>(ARCHIVE_MASTER_KEY), keyBase_, keyStep_);

		ArchiveFileHeader header;

		file_.read((char*)&header, sizeof(ArchiveFileHeader));
		FileArchiver::EncodeBlock((char*)&header, sizeof(ArchiveFileHeader), keyBase_, keyStep_);

		if (memcmp(header.magic, HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) != 0) throw gstd::wexception();

		uint32_t headerSizeTrue = 0U;

		std::stringstream bufInfo;
		file_.seekg(header.headerOffset, std::ios::beg);
		{
			char* tmpBufInfo = new char[header.headerSize];
			file_.read(tmpBufInfo, header.headerSize);

			FileArchiver::EncodeBlock(tmpBufInfo, header.headerSize, keyBase_, keyStep_);

			std::stringstream sTmpBufInfo;
			sTmpBufInfo.write(tmpBufInfo, header.headerSize);
			Compressor::Inflate(sTmpBufInfo, bufInfo, header.headerSize, &headerSizeTrue);

			delete[] tmpBufInfo;
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

		file_.clear(std::ios::eofbit);

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

			uint8_t keyBase = entry->keyBase;
			FileArchiver::EncodeBlock(res->GetPointer(), entry->sizeFull, keyBase, entry->keyStep);

			break;
		}
		case ArchiveFileEntry::CT_ZLIB:
		{
			file.seekg(entry->offsetPos, std::ios::beg);
			res = new ByteBuffer();
			//res->SetSize(entry->sizeFull);

			char* tmp = new char[entry->sizeStored];
			file.read(tmp, entry->sizeStored);

			uint8_t keyBase = entry->keyBase;
			FileArchiver::EncodeBlock(tmp, entry->sizeStored, keyBase, (entry->keyStep << 3) ^ (entry->keyStep & 0x66));

			size_t sizeVerif = 0U;

			std::stringstream streamTmp;
			streamTmp.write(tmp, entry->sizeStored);
			delete[] tmp;

			std::stringstream stream;
			Compressor::Inflate(streamTmp, stream, entry->sizeStored, &sizeVerif);
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