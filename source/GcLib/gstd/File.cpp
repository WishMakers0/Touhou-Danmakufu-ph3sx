#include "source/GcLib/pch.h"

#include "File.hpp"
#include "GstdUtility.hpp"

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
#include "Logger.hpp"
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
#include "ArchiveFile.h"
#endif

using namespace gstd;

/**********************************************************
//ByteBuffer
**********************************************************/
ByteBuffer::ByteBuffer() {
	data_ = nullptr;
	Clear();
}
ByteBuffer::ByteBuffer(ByteBuffer& buffer) {
	data_ = nullptr;
	Clear();
	Copy(buffer);
}
ByteBuffer::ByteBuffer(std::stringstream& stream) {
	data_ = nullptr;
	Clear();
	Copy(stream);
}
ByteBuffer::~ByteBuffer() {
	Clear();
	ptr_delete_scalar(data_);
}
size_t ByteBuffer::_GetReservedSize() {
	return reserve_;
}
void ByteBuffer::_Resize(size_t size) {
	char* oldData = data_;
	size_t oldSize = size_;

	data_ = new char[size];
	ZeroMemory(data_, size);

	//元のデータをコピー
	size_t sizeCopy = std::min(size, oldSize);
	memcpy(data_, oldData, sizeCopy);
	reserve_ = size;
	size_ = size;

	//古いデータを削除
	delete[] oldData;
}
void ByteBuffer::Copy(ByteBuffer& src) {
	if (data_ != nullptr && src.reserve_ != reserve_) {
		delete[] data_;
		data_ = new char[src.reserve_];
		ZeroMemory(data_, src.reserve_);
	}

	offset_ = src.offset_;
	reserve_ = src.reserve_;
	size_ = src.size_;

	memcpy(data_, src.data_, reserve_);
}
void ByteBuffer::Copy(std::stringstream& src) {
	std::streampos org = src.tellg();
	src.seekg(0, std::ios::end);
	size_t size = src.tellg();
	src.seekg(0, std::ios::beg);

	size_t newReserve = 0x1u << (size_t)ceil(log2f(size));

	if (data_ != nullptr) {
		delete[] data_;
		data_ = new char[newReserve];
		ZeroMemory(data_, newReserve);
	}

	offset_ = org;
	reserve_ = newReserve;
	size_ = size;

	{
		size_t count = 0U;
		do {
			src.read(data_ + count, 2048);
			count += src.gcount();
		} while (src.gcount() > 0U);

		src.clear();
	}

	src.seekg(org, std::ios::beg);
}
void ByteBuffer::Clear() {
	if (data_ != nullptr)
		delete[] data_;

	data_ = new char[0];
	offset_ = 0;
	reserve_ = 0;
	size_ = 0;
}
void ByteBuffer::Seek(size_t pos) {
	offset_ = pos;
	if (offset_ < 0)offset_ = 0;
	else if (offset_ > size_)offset_ = size_;
}
void ByteBuffer::SetSize(size_t size) {
	_Resize(size);
}
DWORD ByteBuffer::Write(LPVOID buf, DWORD size) {
	if (offset_ + size > reserve_) {
		size_t sizeNew = (offset_ + size) * 2;
		_Resize(sizeNew);
		size_ = 0;//あとで再計算
	}

	memcpy(&data_[offset_], buf, size);
	offset_ += size;
	size_ = std::max(size_, offset_);
	return size;
}
DWORD ByteBuffer::Read(LPVOID buf, DWORD size) {
	memcpy(buf, &data_[offset_], size);
	offset_ += size;
	return size;
}
_NODISCARD char* ByteBuffer::GetPointer(size_t offset) {
#if _DEBUG
	if (offset > size_)
		throw gstd::wexception(L"ByteBuffer: Index out of bounds.");
#endif
	return &data_[offset];
}
_NODISCARD char& ByteBuffer::operator[](size_t offset) {
#if _DEBUG
	if (offset > size_)
		throw gstd::wexception(L"ByteBuffer: Index out of bounds.");
#endif
	return data_[offset];
}
ByteBuffer& ByteBuffer::operator=(const ByteBuffer& other) noexcept {
	if (this != std::addressof(other)) {
		Clear();
		Copy(const_cast<ByteBuffer&>(other));
	}
	return (*this);
}

/**********************************************************
//File
**********************************************************/
File::File() {
	hFile_ = nullptr;
	path_ = L"";
}
File::File(std::wstring path) {
	hFile_ = nullptr;
	path_ = path;
}
File::~File() {
	Close();
}
bool File::CreateDirectory() {
#ifdef __L_STD_FILESYSTEM
	stdfs::path dir = stdfs::path(path_).parent_path();
	if (stdfs::exists(dir)) return true;

	return stdfs::create_directories(dir);
#else
	std::wstring dir = PathProperty::GetFileDirectory(path_);
	if (File::IsExists(dir))return true;

	std::vector<std::wstring> str = StringUtility::Split(dir, L"\\/");
	std::wstring tPath = L"";
	for (int iDir = 0; iDir < str.size(); iDir++) {
		tPath += str[iDir] + L"\\";
		WIN32_FIND_DATA fData;
		HANDLE hFile = ::FindFirstFile(tPath.c_str(), &fData);
		if (hFile == INVALID_HANDLE_VALUE) {
			SECURITY_ATTRIBUTES attr;
			attr.nLength = sizeof(SECURITY_ATTRIBUTES);
			attr.lpSecurityDescriptor = nullptr;
			attr.bInheritHandle = FALSE;
			::CreateDirectory(tPath.c_str(), &attr);
		}
		::FindClose(hFile);
	}
#endif
	return true;
}
void File::Delete() {
	Close();
	::DeleteFile(path_.c_str());
}
bool File::IsExists() {
	if (hFile_ != nullptr) return true;

	bool res = IsExists(path_);
	return res;
}
bool File::IsExists(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
	path_t _p = path;
	bool res = stdfs::exists(_p) && (stdfs::status(_p).type() != stdfs::file_type::directory);
#else
	bool res = PathFileExists(path.c_str()) == TRUE;
#endif
	return res;
}
bool File::IsDirectory() {
	return IsDirectory(path_);
}
bool File::IsDirectory(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
	path_t _p = path;
	bool res = stdfs::exists(_p) && stdfs::is_directory(_p);
#else
	WIN32_FIND_DATA fData;
	HANDLE hFile = ::FindFirstFile(path.c_str(), &fData);
	bool res = hFile != INVALID_HANDLE_VALUE ? true : false;
	if (res)res = (FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes) > 0;

	::FindClose(hFile);
#endif
	return res;
}
int File::GetSize() {
	if (hFile_ != nullptr)return ::GetFileSize(hFile_, nullptr);

	int res = 0;
	WIN32_FIND_DATA fData;
	HANDLE hFile = ::FindFirstFile(path_.c_str(), &fData);
	res = hFile != INVALID_HANDLE_VALUE ? ::GetFileSize(hFile, nullptr) : 0;
	::FindClose(hFile);
	return res;
}

bool File::Open() {
	return this->Open(AccessType::READ);
}
bool File::Open(AccessType typeAccess) {
	if (hFile_ != nullptr)this->Close();

	DWORD access = typeAccess == AccessType::READ ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;
	hFile_ = ::CreateFile(path_.c_str(), access,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile_ == INVALID_HANDLE_VALUE) {
		hFile_ = nullptr;
		return false;
	}
	return true;
}
bool File::Create() {
	if (hFile_ != nullptr)this->Close();
	hFile_ = CreateFile(path_.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile_ == INVALID_HANDLE_VALUE) {
		hFile_ = nullptr;
		return false;
	}
	return true;
}
void File::Close() {
	if (hFile_ != nullptr)CloseHandle(hFile_);
	hFile_ = nullptr;
}

DWORD File::Read(LPVOID buf, DWORD size) {
	DWORD res = 0;
	::ReadFile(hFile_, buf, size, &res, nullptr);
	return res;
}
DWORD File::Write(LPVOID buf, DWORD size) {
	DWORD res = 0;
	::WriteFile(hFile_, buf, size, &res, nullptr);
	return res;
}
bool File::IsEqualsPath(std::wstring path1, std::wstring path2) {
#ifdef __L_STD_FILESYSTEM
	bool res = (path_t(path1) == path_t(path2));
#else
	path1 = PathProperty::GetUnique(path1);
	path2 = PathProperty::GetUnique(path2);
	bool res = (wcscmp(path1.c_str(), path2.c_str()) == 0);
#endif
	return res;
}
std::vector<std::wstring> File::GetFilePathList(std::wstring dir) {
	std::vector<std::wstring> res;

#ifdef __L_STD_FILESYSTEM
	path_t p = dir;
	if (stdfs::exists(p) && stdfs::is_directory(p)) {
		for (auto itr : stdfs::directory_iterator(p)) {
			if (!itr.is_directory())
				res.push_back(PathProperty::ReplaceYenToSlash(itr.path()));
		}
	}
#else
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name;
			tDir += L"\\";

			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル
		std::wstring path = dir + name;

		res.push_back(path);

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);
#endif

	return res;
}
std::vector<std::wstring> File::GetDirectoryPathList(std::wstring dir) {
	std::vector<std::wstring> res;

#ifdef __L_STD_FILESYSTEM
	path_t p = dir;
	if (stdfs::exists(p) && stdfs::is_directory(p)) {
		for (auto itr : stdfs::directory_iterator(dir)) {
			if (itr.is_directory()) {
				std::wstring str = PathProperty::ReplaceYenToSlash(itr.path());
				str += L'/';
				res.push_back(str);
			}
		}
	}
#else
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name;
			tDir += L"\\";
			res.push_back(tDir);
			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);
#endif

	return res;
}

/**********************************************************
//FileManager
**********************************************************/
FileManager* FileManager::thisBase_ = nullptr;
FileManager::FileManager() {}
FileManager::~FileManager() {
	EndLoadThread();
}
bool FileManager::Initialize() {
	if (thisBase_ != nullptr)return false;
	thisBase_ = this;
	threadLoad_ = new LoadThread();
	threadLoad_->Start();
	return true;
}
void FileManager::EndLoadThread() {
	{
		Lock lock(lock_);
		if (threadLoad_ == nullptr)return;
		threadLoad_->Stop();
		threadLoad_->Join();
		threadLoad_ = nullptr;
	}
}

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
bool FileManager::AddArchiveFile(std::wstring path) {
	if (mapArchiveFile_.find(path) != mapArchiveFile_.end())return true;

	ref_count_ptr<ArchiveFile> file = new ArchiveFile(path);
	if (!file->Open())return false;

	std::set<std::wstring> listKeyIn = file->GetKeyList();
	std::set<std::wstring> listKeyCurrent;
	std::map<std::wstring, ref_count_ptr<ArchiveFile>>::iterator itrFile;
	for (itrFile = mapArchiveFile_.begin(); itrFile != mapArchiveFile_.end(); itrFile++) {
		ref_count_ptr<ArchiveFile> tFile = itrFile->second;
		std::set<std::wstring> tList = tFile->GetKeyList();
		std::set<std::wstring>::iterator itrList = tList.begin();
		for (; itrList != tList.end(); itrList++) {
			listKeyCurrent.insert(*itrList);
		}
	}

	std::set<std::wstring>::iterator itrKey = listKeyIn.begin();
	for (; itrKey != listKeyIn.end(); itrKey++) {
		std::wstring key = *itrKey;
		if (listKeyCurrent.find(key) == listKeyCurrent.end())continue;

		std::wstring log = StringUtility::Format(L"archive file entry already exists[%s]", key.c_str());
		Logger::WriteTop(log);
		throw wexception(log.c_str());
	}

	mapArchiveFile_[path] = file;
	return true;
}
bool FileManager::RemoveArchiveFile(std::wstring path) {
	mapArchiveFile_.erase(path);
	return true;
}
#endif

ref_count_ptr<FileReader> FileManager::GetFileReader(std::wstring path) {
	std::wstring orgPath = path;
	path = PathProperty::GetUnique(path);

	ref_count_ptr<FileReader> res = nullptr;
	ref_count_ptr<File> fileRaw = new File(path);
	if (fileRaw->IsExists()) {
		res = new ManagedFileReader(fileRaw, nullptr);
	}
	else {
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
		//Cannot find a physical file, search in the archive entries.

		std::vector<ArchiveFileEntry::ptr> listEntry;

		std::map<int, std::wstring> mapArchivePath;
		std::wstring key = PathProperty::GetFileName(path);
		std::map<std::wstring, ref_count_ptr<ArchiveFile>>::iterator itr;
		for (itr = mapArchiveFile_.begin(); itr != mapArchiveFile_.end(); itr++) {
			std::wstring pathArchive = itr->first;
			ref_count_ptr<ArchiveFile> fileArchive = itr->second;
			if (!fileArchive->IsExists(key))continue;

			//ref_count_ptr<File> file = new File(pathArchive);
			std::vector<ArchiveFileEntry::ptr> list = fileArchive->GetEntryList(key);
			listEntry.insert(listEntry.end(), list.begin(), list.end());
			for (int iEntry = 0; iEntry < list.size(); iEntry++) {
				ArchiveFileEntry::ptr entry = list[iEntry];
				int addr = (int)entry.get();
				mapArchivePath[addr] = pathArchive;
			}
		}

		if (listEntry.size() == 1) {
			ArchiveFileEntry::ptr entry = listEntry[0];
			int addr = (int)entry.get();
			std::wstring pathArchive = mapArchivePath[addr];
			ref_count_ptr<File> file = new File(pathArchive);
			res = new ManagedFileReader(file, entry);
		}
		else {
			std::wstring module = PathProperty::GetModuleDirectory();
			module = PathProperty::GetUnique(module);

			std::wstring target = StringUtility::ReplaceAll(path, module, L"");
			for (int iEntry = 0; iEntry < listEntry.size(); iEntry++) {
				ArchiveFileEntry::ptr entry = listEntry[iEntry];
				std::wstring dir = entry->directory;
				if (target.find(dir) == std::wstring::npos)continue;

				int addr = (int)entry.get();
				std::wstring pathArchive = mapArchivePath[addr];
				ref_count_ptr<File> file = new File(pathArchive);
				res = new ManagedFileReader(file, entry);
				break;
			}
		}
#else
		res = nullptr;
#endif
	}
	if (res != nullptr)res->_SetOriginalPath(orgPath);
	return res;
}

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
ref_count_ptr<ByteBuffer> FileManager::_GetByteBuffer(ArchiveFileEntry::ptr entry) {
	ref_count_ptr<ByteBuffer> res = nullptr;
	try {
		Lock lock(lock_);

		std::wstring key = entry->directory + entry->name;
		auto itr = mapByteBuffer_.find(key);

		if (itr != mapByteBuffer_.end()) {
			res = itr->second;
		}
		else {
			res = ArchiveFile::CreateEntryBuffer(entry);
			if (res != nullptr)mapByteBuffer_[key] = res;
		}
	}
	catch (...) {}

	return res;
}
void FileManager::_ReleaseByteBuffer(ArchiveFileEntry::ptr entry) {
	{
		Lock lock(lock_);

		std::wstring key = entry->directory + entry->name;
		auto itr = mapByteBuffer_.find(key);

		if (itr == mapByteBuffer_.end())return;
		ref_count_ptr<ByteBuffer> buffer = itr->second;
		if (buffer.GetReferenceCount() == 2) {
			mapByteBuffer_.erase(itr);
		}
	}
}
#endif

void FileManager::AddLoadThreadEvent(ref_count_ptr<FileManager::LoadThreadEvent> event) {
	{
		Lock lock(lock_);
		if (threadLoad_ == nullptr)return;
		threadLoad_->AddEvent(event);
	}
}
void FileManager::AddLoadThreadListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lock_);
		if (threadLoad_ == nullptr)return;
		threadLoad_->AddListener(listener);
	}
}
void FileManager::RemoveLoadThreadListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lock_);
		if (threadLoad_ == nullptr)return;
		threadLoad_->RemoveListener(listener);
	}
}
void FileManager::WaitForThreadLoadComplete() {
	while (!threadLoad_->IsThreadLoadComplete()) {
		Sleep(1);
	}
}

//FileManager::LoadThread
FileManager::LoadThread::LoadThread() {}
FileManager::LoadThread::~LoadThread() {}
void FileManager::LoadThread::_Run() {
	while (this->GetStatus() == RUN) {
		signal_.Wait(10);

		while (this->GetStatus() == RUN) {
			//Logger::WriteTop(StringUtility::Format("ロードイベント取り出し開始"));
			ref_count_ptr<FileManager::LoadThreadEvent> event = nullptr;
			{
				Lock lock(lockEvent_);
				if (listEvent_.size() == 0)break;
				event = listEvent_.front();
				//listPath_.erase(event->GetPath());
				listEvent_.pop_front();
			}
			//Logger::WriteTop(StringUtility::Format("ロードイベント取り出し完了：%s", event->GetPath().c_str()));

			//Logger::WriteTop(StringUtility::Format("ロード開始：%s", event->GetPath().c_str()));
			{
				Lock lock(lockListener_);
				std::list<FileManager::LoadThreadListener*>::iterator itr;
				for (itr = listListener_.begin(); itr != listListener_.end(); itr++) {
					FileManager::LoadThreadListener* listener = (*itr);
					if (event->GetListener() == listener)
						listener->CallFromLoadThread(event);
				}
			}
			//Logger::WriteTop(StringUtility::Format("ロード完了：%s", event->GetPath().c_str()));

		}
		Sleep(1);//TODO なぜか待機入れると落ちづらい？
	}

	{
		Lock lock(lockListener_);
		listListener_.clear();
	}
}
void FileManager::LoadThread::Stop() {
	Thread::Stop();
	signal_.SetSignal();
}
bool FileManager::LoadThread::IsThreadLoadComplete() {
	bool res = false;
	{
		Lock lock(lockEvent_);
		res = listEvent_.size() == 0;
	}
	return res;
}
bool FileManager::LoadThread::IsThreadLoadExists(std::wstring path) {
	bool res = false;
	{
		Lock lock(lockEvent_);
		//res = listPath_.find(path) != listPath_.end();
	}
	return res;
}
void FileManager::LoadThread::AddEvent(ref_count_ptr<FileManager::LoadThreadEvent> event) {
	{
		Lock lock(lockEvent_);
		std::wstring path = event->GetPath();
		if (IsThreadLoadExists(path))return;
		listEvent_.push_back(event);
		//listPath_.insert(path);
		signal_.SetSignal();
		signal_.SetSignal(false);
	}
}
void FileManager::LoadThread::AddListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lockListener_);
		std::list<FileManager::LoadThreadListener*>::iterator itr;
		for (itr = listListener_.begin(); itr != listListener_.end(); itr++) {
			if (*itr == listener)return;
		}

		listListener_.push_back(listener);
	}
}
void FileManager::LoadThread::RemoveListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lockListener_);
		std::list<FileManager::LoadThreadListener*>::iterator itr;
		for (itr = listListener_.begin(); itr != listListener_.end(); itr++) {
			if (*itr != listener)continue;
			listListener_.erase(itr);
			break;
		}
	}
}

/**********************************************************
//ManagedFileReader
**********************************************************/
ManagedFileReader::ManagedFileReader(ref_count_ptr<File> file, std::shared_ptr<ArchiveFileEntry> entry) {
	offset_ = 0;
	file_ = file;

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	entry_ = entry;

	if (entry_ == nullptr) {
		type_ = TYPE_NORMAL;
	}
	else if (entry_->compressionType == ArchiveFileEntry::CT_NONE && entry_ != nullptr) {
		type_ = TYPE_ARCHIVED;
	}
	else if (entry_->compressionType != ArchiveFileEntry::CT_NONE && entry_ != nullptr) {
		type_ = TYPE_ARCHIVED_COMPRESSED;
	}
#else
	type_ = TYPE_NORMAL;
#endif
}
ManagedFileReader::~ManagedFileReader() {
	Close();
}
bool ManagedFileReader::Open() {
	bool res = false;
	offset_ = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->Open();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		buffer_ = FileManager::GetBase()->_GetByteBuffer(entry_);
		res = buffer_ != nullptr;
	}
#endif
	return res;
}
void ManagedFileReader::Close() {
	if (file_ != nullptr)file_->Close();
	if (buffer_ != nullptr) {
		buffer_ = nullptr;
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
		FileManager::GetBase()->_ReleaseByteBuffer(entry_);
#endif
	}
}
int ManagedFileReader::GetFileSize() {
	int res = 0;
	if (type_ == TYPE_NORMAL)res = file_->GetSize();
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if ((type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) && buffer_ != nullptr)
		res = entry_->sizeFull;
#endif
	return res;
}
DWORD ManagedFileReader::Read(LPVOID buf, DWORD size) {
	DWORD res = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->Read(buf, size);
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		int read = size;
		if (buffer_->GetSize() < offset_ + size) {
			read = buffer_->GetSize() - offset_;
		}
		memcpy(buf, &buffer_->GetPointer()[offset_], read);
		res = read;
	}
#endif
	offset_ += res;
	return res;
}
BOOL ManagedFileReader::SetFilePointerBegin() {
	BOOL res = FALSE;
	offset_ = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->SetFilePointerBegin();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_ != nullptr) {
			offset_ = 0;
			res = TRUE;
		}
	}
#endif
	return res;
}
BOOL ManagedFileReader::SetFilePointerEnd() {
	BOOL res = FALSE;
	if (type_ == TYPE_NORMAL) {
		res = file_->SetFilePointerEnd();
		offset_ = file_->GetSize();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_ != nullptr) {
			offset_ = buffer_->GetSize();
			res = TRUE;
		}
	}
#endif
	return res;
}
BOOL ManagedFileReader::Seek(LONG offset) {
	BOOL res = FALSE;
	if (type_ == TYPE_NORMAL) {
		res = file_->Seek(offset);
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_ != nullptr) {
			res = TRUE;
		}
	}
#endif
	if (res == TRUE)
		offset_ = offset;
	return res;
}
LONG ManagedFileReader::GetFilePointer() {
	LONG res = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->GetFilePointer();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_ != nullptr) {
			res = offset_;
		}
	}
#endif
	return res;
}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER) || defined(DNH_PROJ_FILEARCHIVER)
bool ManagedFileReader::IsArchived() {
	return type_ != TYPE_NORMAL;
}
bool ManagedFileReader::IsCompressed() {
	return type_ == TYPE_ARCHIVED_COMPRESSED;
}
#endif

/**********************************************************
//RecordEntry
**********************************************************/
RecordEntry::RecordEntry() {
	type_ = TYPE_UNKNOWN;
}
RecordEntry::~RecordEntry() {

}
int RecordEntry::_GetEntryRecordSize() {
	int res = 0;
	res += sizeof(char);
	res += sizeof(int);
	res += key_.size();
	res += sizeof(int);
	res += buffer_.GetSize();
	return res;
}
void RecordEntry::_WriteEntryRecord(Writer &writer) {
	writer.WriteCharacter(type_);
	writer.WriteInteger(key_.size());
	writer.Write(&key_[0], key_.size());

	writer.WriteInteger(buffer_.GetSize());
	writer.Write(buffer_.GetPointer(), buffer_.GetSize());
}
void RecordEntry::_ReadEntryRecord(Reader &reader) {
	type_ = reader.ReadCharacter();
	key_.resize(reader.ReadInteger());
	reader.Read(&key_[0], key_.size());

	buffer_.Clear();
	buffer_.SetSize(reader.ReadInteger());
	reader.Read(buffer_.GetPointer(), buffer_.GetSize());
}

/**********************************************************
//RecordBuffer
**********************************************************/
RecordBuffer::RecordBuffer() {

}
RecordBuffer::~RecordBuffer() {
	this->Clear();
}
void RecordBuffer::Clear() {
	mapEntry_.clear();
}
ref_count_ptr<RecordEntry> RecordBuffer::GetEntry(std::string key) {
	ref_count_ptr<RecordEntry> res;

	auto itr = mapEntry_.find(key);
	if (itr != mapEntry_.end()) res = itr->second;

	return res;
}
bool RecordBuffer::IsExists(std::string key) {
	return mapEntry_.find(key) != mapEntry_.end();
}
std::vector<std::string> RecordBuffer::GetKeyList() {
	std::vector<std::string> res;
	std::map<std::string, ref_count_ptr<RecordEntry> >::iterator itr;
	for (itr = mapEntry_.begin(); itr != mapEntry_.end(); itr++) {
		std::string key = itr->first;
		res.push_back(key);
	}
	return res;
}

void RecordBuffer::Write(Writer& writer) {
	int countEntry = mapEntry_.size();
	writer.WriteInteger(countEntry);

	std::map<std::string, ref_count_ptr<RecordEntry> >::iterator itr;
	for (itr = mapEntry_.begin(); itr != mapEntry_.end(); itr++) {
		ref_count_ptr<RecordEntry> entry = itr->second;
		entry->_WriteEntryRecord(writer);
	}
}
void RecordBuffer::Read(Reader& reader) {
	this->Clear();
	int countEntry = reader.ReadInteger();
	for (int iEntry = 0; iEntry < countEntry; iEntry++) {
		ref_count_ptr<RecordEntry> entry = new RecordEntry();
		entry->_ReadEntryRecord(reader);

		std::string key = entry->GetKey();
		mapEntry_[key] = entry;
	}
}
bool RecordBuffer::WriteToFile(std::wstring path, std::string header) {
	File file(path);
	if (!file.Create())return false;

	file.Write((char*)&header[0], header.size());
	Write(file);
	file.Close();

	return true;
}
bool RecordBuffer::ReadFromFile(std::wstring path, std::string header) {
	File file(path);
	if (!file.Open())return false;

	std::string fHead;
	fHead.resize(header.size());
	file.Read(&fHead[0], fHead.size());
	if (fHead != header)return false;

	Read(file);
	file.Close();

	return true;
}
int RecordBuffer::GetEntryType(std::string key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return RecordEntry::TYPE_NOENTRY;
	return itr->second->GetType();
}
int RecordBuffer::GetEntrySize(std::string key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return 0;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	return buffer.GetSize();
}
bool RecordBuffer::GetRecord(std::string key, LPVOID buf, DWORD size) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return false;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	buffer.Seek(0);
	buffer.Read(buf, size);
	return true;
}
bool RecordBuffer::GetRecordAsBoolean(std::string key) {
	bool res = 0;
	GetRecord(key, res);
	return res;
}
int RecordBuffer::GetRecordAsInteger(std::string key) {
	int res = 0;
	GetRecord(key, res);
	return res;
}
float RecordBuffer::GetRecordAsFloat(std::string key) {
	float res = 0;
	GetRecord(key, res);
	return res;
}
double RecordBuffer::GetRecordAsDouble(std::string key) {
	double res = 0;
	GetRecord(key, res);
	return res;
}
std::string RecordBuffer::GetRecordAsStringA(std::string key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return "";

	std::string res;
	ref_count_ptr<RecordEntry> entry = itr->second;
	int type = entry->GetType();
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.Seek(0);
	if (type == RecordEntry::TYPE_STRING_A) {
		res.resize(buffer.GetSize());
		buffer.Read(&res[0], buffer.GetSize());
	}
	else if (type == RecordEntry::TYPE_STRING_W) {
		std::wstring wstr;
		wstr.resize(buffer.GetSize() / sizeof(wchar_t));
		buffer.Read(&wstr[0], buffer.GetSize());
		res = StringUtility::ConvertWideToMulti(wstr);
	}

	return res;
}
std::wstring RecordBuffer::GetRecordAsStringW(std::string key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return L"";

	std::wstring res;
	ref_count_ptr<RecordEntry> entry = itr->second;
	int type = entry->GetType();
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.Seek(0);
	if (type == RecordEntry::TYPE_STRING_A) {
		std::string str;
		str.resize(buffer.GetSize());
		buffer.Read(&str[0], buffer.GetSize());

		res = StringUtility::ConvertMultiToWide(str);
	}
	else if (type == RecordEntry::TYPE_STRING_W) {
		res.resize(buffer.GetSize() / sizeof(wchar_t));
		buffer.Read(&res[0], buffer.GetSize());
	}

	return res;
}
bool RecordBuffer::GetRecordAsRecordBuffer(std::string key, RecordBuffer& record) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return false;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	buffer.Seek(0);
	record.Read(buffer);
	return true;
}
void RecordBuffer::SetRecord(int type, std::string key, LPVOID buf, DWORD size) {
	ref_count_ptr<RecordEntry> entry = new RecordEntry();
	entry->SetType((char)type);
	entry->SetKey(key);
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.SetSize(size);
	buffer.Write(buf, size);
	mapEntry_[key] = entry;
}
void RecordBuffer::SetRecordAsRecordBuffer(std::string key, RecordBuffer& record) {
	ref_count_ptr<RecordEntry> entry = new RecordEntry();
	entry->SetType((char)RecordEntry::TYPE_RECORD);
	entry->SetKey(key);
	ByteBuffer& buffer = entry->GetBufferRef();
	record.Write(buffer);
	mapEntry_[key] = entry;
}

void RecordBuffer::Read(RecordBuffer& record) {}
void RecordBuffer::Write(RecordBuffer& record) {}

/**********************************************************
//PropertyFile
**********************************************************/
PropertyFile::PropertyFile() {}
PropertyFile::~PropertyFile() {}
bool PropertyFile::Load(std::wstring path) {
	mapEntry_.clear();

	std::vector<char> text;
	FileManager* fileManager = FileManager::GetBase();
	if (fileManager != nullptr) {
		ref_count_ptr<FileReader> reader = fileManager->GetFileReader(path);

		if (reader == nullptr || !reader->Open()) {
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
			Logger::WriteTop(ErrorUtility::GetFileNotFoundErrorMessage(path));
#endif
			return false;
		}

		int size = reader->GetFileSize();
		text.resize(size + 1);
		reader->Read(&text[0], size);
		text[size] = '\0';
	}
	else {
		File file(path);
		if (!file.Open())return false;

		int size = file.GetSize();
		text.resize(size + 1);
		file.Read(&text[0], size);
		text[size] = '\0';
	}

	bool res = false;
	gstd::Scanner scanner(text);
	try {
		while (scanner.HasNext()) {
			gstd::Token& tok = scanner.Next();
			if (tok.GetType() != Token::TK_ID)continue;
			std::wstring key = tok.GetElement();
			while (true) {
				tok = scanner.Next();
				if (tok.GetType() == Token::TK_EQUAL)break;
				key += tok.GetElement();
			}

			std::wstring value;
			try {
				int posS = scanner.GetCurrentPointer();
				int posE = posS;
				while (true) {
					bool bEndLine = false;
					if (!scanner.HasNext()) {
						bEndLine = true;
					}
					else {
						tok = scanner.Next();
						bEndLine = tok.GetType() == Token::TK_NEWLINE;
					}

					if (bEndLine) {
						posE = scanner.GetCurrentPointer();
						value = scanner.GetString(posS, posE);
						value = StringUtility::Trim(value);
						break;
					}
				}
			}
			catch (...) {}

			mapEntry_[key] = value;
		}

		res = true;
	}
	catch (gstd::wexception& e) {
		mapEntry_.clear();
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_VIEWER)
		Logger::WriteTop(
			ErrorUtility::GetParseErrorMessage(path, scanner.GetCurrentLine(), e.what()));
#endif
		res = false;
	}

	return res;
}
bool PropertyFile::HasProperty(std::wstring key) {
	return mapEntry_.find(key) != mapEntry_.end();
}
std::wstring PropertyFile::GetString(std::wstring key, std::wstring def) {
	if (!HasProperty(key))return def;
	return mapEntry_[key];
}
int PropertyFile::GetInteger(std::wstring key, int def) {
	if (!HasProperty(key))return def;
	std::wstring strValue = mapEntry_[key];
	return StringUtility::ToInteger(strValue);
}
double PropertyFile::GetReal(std::wstring key, double def) {
	if (!HasProperty(key))return def;
	std::wstring strValue = mapEntry_[key];
	return StringUtility::ToDouble(strValue);
}

/**********************************************************
//SystemValueManager
**********************************************************/
const std::string SystemValueManager::RECORD_SYSTEM = "__RECORD_SYSTEM__";
const std::string SystemValueManager::RECORD_SYSTEM_GLOBAL = "__RECORD_SYSTEM_GLOBAL__";
SystemValueManager* SystemValueManager::thisBase_ = nullptr;
SystemValueManager::SystemValueManager() {}
SystemValueManager::~SystemValueManager() {}
bool SystemValueManager::Initialize() {
	if (thisBase_ != nullptr)return false;

	mapRecord_[RECORD_SYSTEM] = new RecordBuffer();
	mapRecord_[RECORD_SYSTEM_GLOBAL] = new RecordBuffer();

	thisBase_ = this;
	return true;
}
void SystemValueManager::ClearRecordBuffer(std::string key) {
	if (!IsExists(key))return;
	mapRecord_[key]->Clear();
}
bool SystemValueManager::IsExists(std::string key) {
	return mapRecord_.find(key) != mapRecord_.end();
}
bool SystemValueManager::IsExists(std::string keyRecord, std::string keyValue) {
	if (!IsExists(keyRecord))return false;
	gstd::ref_count_ptr<RecordBuffer> record = GetRecordBuffer(keyRecord);
	return record->IsExists(keyValue);
}
gstd::ref_count_ptr<RecordBuffer> SystemValueManager::GetRecordBuffer(std::string key) {
	return IsExists(key) ? mapRecord_[key] : nullptr;
}
