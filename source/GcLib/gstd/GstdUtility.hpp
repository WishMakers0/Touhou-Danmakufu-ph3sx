#ifndef __GSTD_UTILITIY__
#define __GSTD_UTILITIY__

#include "../pch.h"

#include "SmartPointer.hpp"

namespace gstd {
	//================================================================
	//DebugUtility
	class DebugUtility {
	public:
		static void DumpMemoryLeaksOnExit() {
#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtDumpMemoryLeaks();
#endif
		}
	};

	//================================================================
	//SystemUtility
	class SystemUtility {
	public:

	};

	//================================================================
	//ThreadUtility
	template<class F>
	static void ParallelAscent(size_t countLoop, F&& func) {
		size_t countCore = std::max(std::thread::hardware_concurrency(), 1U);

		std::vector<std::future<void>> workers;
		workers.reserve(countCore);

		auto coreTask = [&](size_t id) {
			const size_t begin = countLoop / countCore * id + std::min(countLoop % countCore, id);
			const size_t end = countLoop / countCore * (id + 1U) + std::min(countLoop % countCore, id + 1U);

			for (size_t i = begin; i < end; ++i)
				func(i);
		};

		for (size_t iCore = 0; iCore < countCore; ++iCore)
			workers.emplace_back(std::async(std::launch::async | std::launch::deferred, coreTask, iCore));
		for (const auto& worker : workers)
			worker.wait();
	}

	//================================================================
	//Encoding
	class Encoding {
		//http://msdn.microsoft.com/ja-jp/library/system.text.encoding(v=vs.110).aspx
		//babel
		//http://d.hatena.ne.jp/A7M/20100801/1280629387
	public:
		enum {
			UNKNOWN = -1,
			SHIFT_JIS = 1,
			UTF16LE,
		};

		enum {
			CP_SHIFT_JIS = 932,
		};
	public:
		static size_t Detect(const void* data, size_t dataSize);
		static bool IsUtf16Le(const void* data, size_t dataSize);
		static size_t GetBomSize(const void* data, size_t dataSize);

		static const unsigned char BOM_UTF16LE[];
	};


	//================================================================
	//StringUtility
	class StringUtility {
	public:
		static std::string ConvertWideToMulti(std::wstring const &wstr, int codeMulti = 932);
		static std::wstring ConvertMultiToWide(std::string const &str, int codeMulti = 932);
		static std::string ConvertUtf8ToMulti(std::vector<char>& text);
		static std::wstring ConvertUtf8ToWide(std::vector<char>& text);

		//----------------------------------------------------------------
		static std::vector<std::string> Split(std::string str, std::string delim);
		static void Split(std::string str, std::string delim, std::vector<std::string>& res);
		static std::string Format(const char* str, ...);

		static size_t CountCharacter(std::string& str, char c);
		static size_t CountCharacter(std::vector<char>& str, char c);
		static int ToInteger(const std::string& s);
		static double ToDouble(const std::string& s);
		static std::string Replace(std::string& source, std::string pattern, std::string placement);
		static std::string ReplaceAll(std::string& source, std::string pattern, std::string placement, 
			size_t replaceCount = UINT_MAX, size_t start = 0, size_t end = 0);
		static std::string Slice(const std::string& s, size_t length);
		static std::string Trim(const std::string& str);

		//----------------------------------------------------------------
		//std::wstring.size�͕�������Ԃ��B�o�C�g���ł͂Ȃ��B
		static std::vector<std::wstring> Split(std::wstring str, std::wstring delim);
		static void Split(std::wstring str, std::wstring delim, std::vector<std::wstring>& res);
		static std::wstring Format(const wchar_t* str, ...);
		static std::wstring FormatToWide(const char* str, ...);

		static size_t CountCharacter(std::wstring& str, wchar_t c);
		static int ToInteger(const std::wstring& s);
		static double ToDouble(const std::wstring& s);
		static std::wstring Replace(std::wstring& source, std::wstring pattern, std::wstring placement);
		static std::wstring ReplaceAll(std::wstring& source, std::wstring pattern, std::wstring placement, 
			size_t replaceCount = UINT_MAX, size_t start = 0, size_t end = 0);
		static std::wstring Slice(const std::wstring& s, size_t length);
		static std::wstring Trim(const std::wstring& str);

		static size_t CountAsciiSizeCharacter(std::wstring& str);
		static size_t GetByteSize(std::wstring& str) {
			return str.size() * sizeof(wchar_t);
		}
	};

	//================================================================
	//ErrorUtility
	class ErrorUtility {
	public:
		enum {
			ERROR_FILE_NOTFOUND,
			ERROR_PARSE,
			ERROR_END_OF_FILE,
			ERROR_OUTOFRANGE_INDEX,
		};
	public:
		static std::wstring GetLastErrorMessage(DWORD error);
		static std::wstring GetLastErrorMessage() {
			return GetLastErrorMessage(GetLastError());
		}
		static std::wstring GetErrorMessage(int type);
		static std::wstring GetFileNotFoundErrorMessage(std::wstring path);
		static std::wstring GetParseErrorMessage(int line, std::wstring what) {
			return GetParseErrorMessage(L"", line, what);
		}
		static std::wstring GetParseErrorMessage(std::wstring path, int line, std::wstring what);
	};

	//================================================================
	//wexception
	class wexception {
	protected:
		std::wstring message_;
	public:
		wexception() {}
		wexception(std::wstring msg) { message_ = msg; }
		wexception(std::string msg) {
			message_ = StringUtility::ConvertMultiToWide(msg);
		}
		std::wstring GetMessage() { return message_; }
		const wchar_t* what() { return message_.c_str(); }
	};

	//================================================================
	//Math
	constexpr double GM_PI = 3.14159265358979323846;
	constexpr double GM_PI_X2 = GM_PI * 2.0;
	constexpr double GM_PI_X4 = GM_PI * 4.0;
	constexpr double GM_PI_2 = GM_PI / 2.0;
	constexpr double GM_PI_4 = GM_PI / 4.0;
	constexpr double GM_1_PI = 1.0 / GM_PI;
	constexpr double GM_2_PI = 2.0 / GM_PI;
	constexpr double GM_SQRTP = 1.772453850905516027298;
	constexpr double GM_1_SQRTP = 1.0 / GM_SQRTP;
	constexpr double GM_2_SQRTP = 2.0 / GM_SQRTP;
	constexpr double GM_SQRT2 = 1.41421356237309504880;
	constexpr double GM_SQRT2_2 = GM_SQRT2 / 2.0;
	constexpr double GM_SQRT2_X2 = GM_SQRT2 * 2.0;
	constexpr double GM_E = 2.71828182845904523536;
	constexpr double GM_LOG2E = 1.44269504088896340736;		//log2(e)
	constexpr double GM_LOG10E = 0.434294481903251827651;	//log10(e)
	constexpr double GM_LN2 = 0.693147180559945309417;		//ln(2)
	constexpr double GM_LN10 = 2.30258509299404568402;		//ln(10)
	class Math {
	public:
		static inline constexpr double DegreeToRadian(double angle) { return angle * GM_PI / 180.0; }
		static inline constexpr double RadianToDegree(double angle) { return angle * 180.0 / GM_PI; }

		static inline double NormalizeAngleDeg(double angle) { 
			angle = fmod(angle, 360.0);
			if (angle < 0.0) angle += 360.0;
			return angle; 
		}
		static inline double NormalizeAngleRad(double angle) {
			angle = fmod(angle, GM_PI_X2);
			if (angle < 0.0) angle += GM_PI_X2;
			return angle;
		}

		static void InitializeFPU() {
			__asm {
				finit
			};
		}

		static inline const double Round(double val) { return std::round(val); }

		class Lerp {
		public:
			template<typename T, typename L>
			static inline T Linear(T a, T b, L x) {
				return a + (b - a) * x;
			}
			template<typename T, typename L>
			static inline T Smooth(T a, T b, L x) {
				return a + x * x * ((L)3 - (L)2 * x) * (b - a);
			}
			template<typename T, typename L>
			static inline T Smoother(T a, T b, L x) {
				return a + x * x * x * (x * (x * (L)6 - (L)15) + (L)10) * (b - a);
			}
			template<typename T, typename L>
			static inline T Accelerate(T a, T b, L x) {
				return a + x * x * (b - a);
			}
			template<typename T, typename L>
			static inline T Decelerate(T a, T b, L x) {
				return a + ((L)1 - ((L)1 - x) * ((L)1 - x)) * (b - a);
			}
		};
	};

	//================================================================
	//ByteOrder
	class ByteOrder {
	public:
		enum {
			ENDIAN_LITTLE,
			ENDIAN_BIG,
		};

	public:
		static void Reverse(LPVOID buf, DWORD size);

	};

	//================================================================
	//Sort
	class SortUtility {
	public:
		template <class BidirectionalIterator, class Predicate>
		static void CombSort(BidirectionalIterator first,
			BidirectionalIterator last,
			Predicate pr) {
			int gap = static_cast<int>(std::distance(first, last));
			if (gap < 1)return;

			BidirectionalIterator first2 = last;
			bool swapped = false;
			do {
				int newgap = (gap * 10 + 3) / 13;
				if (newgap < 1) newgap = 1;
				if (newgap == 9 || newgap == 10) newgap = 11;
				std::advance(first2, newgap - gap);
				gap = newgap;
				swapped = false;
				for (BidirectionalIterator target1 = first, target2 = first2;
					target2 != last;
					++target1, ++target2) 
				{
					if (pr(*target2, *target1)) {
						std::iter_swap(target1, target2);
						swapped = true;
					}
				}
			} while ((gap > 1) || swapped);
		}
	};

	//================================================================
	//PathProperty
	class PathProperty {
	public:
		static std::wstring GetFileDirectory(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			p = p.parent_path();
			return p.wstring() + L'/';
#else
			wchar_t pDrive[_MAX_PATH];
			wchar_t pDir[_MAX_PATH];
			_wsplitpath_s(path.c_str(), pDrive, _MAX_PATH, pDir, _MAX_PATH, nullptr, 0, nullptr, 0);
			return std::wstring(pDrive) + std::wstring(pDir);
#endif
		}

		static std::wstring GetDirectoryName(std::wstring path) {
			//Returns the name of the topmost directory.
#ifdef __L_STD_FILESYSTEM
			std::wstring dirChain = ReplaceYenToSlash(path_t(path).parent_path());
			std::vector<std::wstring> listDir = StringUtility::Split(dirChain, L"/");
			return listDir.back();
#else
			std::wstring dir = GetFileDirectory(path);
			dir = StringUtility::ReplaceAll(dir, L"\\", L"/");
			std::vector<std::wstring> strs = StringUtility::Split(dir, L"/");
			return strs[strs.size() - 1];
#endif
		}

		static std::wstring GetFileName(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return p.filename();
#else
			wchar_t pFileName[_MAX_PATH];
			wchar_t pExt[_MAX_PATH];
			_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, pFileName, _MAX_PATH, pExt, _MAX_PATH);
			return std::wstring(pFileName) + std::wstring(pExt);
#endif
		}

		static std::wstring GetDriveName(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return (p.root_name() / p.root_directory());
#else
			wchar_t pDrive[_MAX_PATH];
			_wsplitpath_s(path.c_str(), pDrive, _MAX_PATH, nullptr, 0, nullptr, 0, nullptr, 0);
			return std::wstring(pDrive);
#endif
		}

		static std::wstring GetFileNameWithoutExtension(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return p.stem();
#else
			wchar_t pFileName[_MAX_PATH];
			_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, pFileName, _MAX_PATH, nullptr, 0);
			return std::wstring(pFileName);
#endif
		}

		static std::wstring GetFileExtension(std::wstring path) {
#ifdef __L_STD_FILESYSTEM
			path_t p(path);
			return p.extension();
#else
			wchar_t pExt[_MAX_PATH];
			_wsplitpath_s(path.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, pExt, _MAX_PATH);
			return std::wstring(pExt);
#endif
		}
		//Returns the path of the executable.
		static std::wstring GetModuleName() {
			wchar_t modulePath[_MAX_PATH];
			ZeroMemory(modulePath, sizeof(modulePath));
			GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
			return GetFileNameWithoutExtension(std::wstring(modulePath));
		}
		//Returns the directory of the executable.
		static std::wstring GetModuleDirectory() {
#ifdef __L_STD_FILESYSTEM
			wchar_t modulePath[_MAX_PATH];
			ZeroMemory(modulePath, sizeof(modulePath));
			GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
			return ReplaceYenToSlash(stdfs::path(modulePath).parent_path()) + L"/";
#else
			wchar_t modulePath[_MAX_PATH];
			ZeroMemory(modulePath, sizeof(modulePath));
			GetModuleFileName(NULL, modulePath, _MAX_PATH - 1);
			return GetFileDirectory(std::wstring(modulePath));
#endif
		}
		//Returns the directory of the path relative to the executable's directory.
		static std::wstring GetDirectoryWithoutModuleDirectory(std::wstring path) {	
			std::wstring res = GetFileDirectory(path);
			std::wstring dirModule = GetModuleDirectory();
			if (res.find(dirModule) != std::wstring::npos) {
				res = res.substr(dirModule.size());
			}
			return res;
		}
		//Returns the the path relative to the executable's directory.
		static std::wstring GetPathWithoutModuleDirectory(std::wstring path) {
			std::wstring dirModule = GetModuleDirectory();
			std::wstring res = Canonicalize(path);
			if (res.find(dirModule) != std::wstring::npos) {
				res = res.substr(dirModule.size());
			}
			return res;
		}
		static std::wstring GetRelativeDirectory(std::wstring from, std::wstring to) {
#ifdef __L_STD_FILESYSTEM
			std::error_code err;
			path_t p = stdfs::relative(from, to, err);

			std::wstring res;
			if (err.value() != 0)
				res = GetFileDirectory(p);
#else
			wchar_t path[_MAX_PATH];
			BOOL b = PathRelativePathTo(path, from.c_str(), FILE_ATTRIBUTE_DIRECTORY, to.c_str(), FILE_ATTRIBUTE_DIRECTORY);

			std::wstring res;
			if (b) {
				res = GetFileDirectory(path);
			}
#endif
			return res;
		}
		static std::wstring ExtendRelativeToFull(std::wstring dir, std::wstring path) {
			path = ReplaceYenToSlash(path);
			if (path.size() >= 2) {
				if (memcmp(&path[0], L"./", sizeof(wchar_t) * 2U) == 0) {
					path = path.substr(2);
					path = dir + path;
				}
			}

			std::wstring drive = GetDriveName(path);
			if (drive.size() == 0) {
				path = GetModuleDirectory() + path;
			}

			return path;
		}
		//Replaces all the '\\' characters with '/'.
		static std::wstring ReplaceYenToSlash(std::wstring path) {
			std::wstring res = StringUtility::ReplaceAll(path, L"\\", L"/");
			return res;
		}
		static std::wstring Canonicalize(std::wstring srcPath) {
#ifdef __L_STD_FILESYSTEM
			path_t p(srcPath);
			std::wstring res = stdfs::weakly_canonical(p);
#else
			wchar_t destPath[_MAX_PATH];
			PathCanonicalize(destPath, srcPath.c_str());
			std::wstring res(destPath);
#endif
			return res;
		}
		//Replaces all the '/' characters with '\\' (at the very least).
		static std::wstring MakePreferred(std::wstring srcPath) {
#ifdef __L_STD_FILESYSTEM
			path_t p(srcPath);
			std::wstring res = p.make_preferred();
#else
			std::wstring res = StringUtility::ReplaceAll(path, L"/", L"\\");
#endif
			return res;
		}
		static std::wstring GetUnique(std::wstring srcPath) {
			path_t p(srcPath);
			p = stdfs::weakly_canonical(p);
			return ReplaceYenToSlash(p);
		}
	};

	//================================================================
	//BitAccess
	class BitAccess {
	public:
		template <typename T> static bool GetBit(T value, int bit) {
			T mask = (T)1 << bit;
			return (value & mask) != 0;
		}
		template <typename T> static T& SetBit(T& value, int bit, bool b) {
			T mask = (T)1 << bit;
			T write = (T)b << bit;
			value &= ~mask;
			value |= write;
			return value;
		}
		template <typename T> static unsigned char GetByte(T value, int bit) {
			return (unsigned char)(value >> bit);
		}
		template <typename T> static T& SetByte(T& value, int bit, unsigned char c) {
			T mask = (T)0xff << bit;
			T write = (T)c << bit;
			value &= ~mask;
			value |= write;
			return value;
		}
	};

	//================================================================
	//IStringInfo
	class IStringInfo {
	public:
		virtual ~IStringInfo() {}
		virtual std::wstring GetInfoAsString() {
			int address = (int)this;
			char* name = (char*)typeid(*this).name();
			std::string str = StringUtility::Format("%s[%08x]", name, address);
			std::wstring res = StringUtility::ConvertMultiToWide(str);
			return res;
		}
	};

	//================================================================
	//InnerClass
	//C++�ɂ͓����N���X���Ȃ��̂ŁA�O���N���X�A�N�Z�X�p
	template <class T>
	class InnerClass {
		T* outer_;
	protected:
		T* _GetOuter() { return outer_; }
		void _SetOuter(T* outer) { outer_ = outer; }
	public:
		InnerClass(T* outer = NULL) { outer_ = outer; }
	};

	//================================================================
	//Singleton
	template<class T> class Singleton {
	protected:
		Singleton() {};
		inline static T*& _This() {
			static T* s = NULL;
			return s;
		}
	public:
		virtual ~Singleton() {};
		static T* CreateInstance() {
			if (_This() == NULL)_This() = new T();
			return _This();
		}
		static T* GetInstance() {
			if (_This() == NULL) {
				throw std::exception("Instance uninitialized.");
			}
			return _This();
		}
		static void DeleteInstance() {
			if (_This() != NULL)delete _This();
			_This() = NULL;
		}
	};

	//================================================================
	//Scanner
	class Scanner;
	class Token {
		friend Scanner;
	public:
		enum Type {
			TK_UNKNOWN, TK_EOF, TK_NEWLINE,
			TK_ID,
			TK_INT, TK_REAL, TK_STRING,

			TK_OPENP, TK_CLOSEP, TK_OPENB, TK_CLOSEB, TK_OPENC, TK_CLOSEC,
			TK_SHARP,
			TK_PIPE, TK_AMPERSAND,

			TK_COMMA, TK_PERIOD, TK_EQUAL,
			TK_ASTERISK, TK_SLASH, TK_COLON, TK_SEMICOLON, TK_TILDE, TK_EXCLAMATION,
			TK_PLUS, TK_MINUS,
			TK_LESS, TK_GREATER,
		};
		static const char* Type_Str[];
	protected:
		Type type_;
		std::wstring element_;
		int posStart_;
		int posEnd_;
	public:
		Token() { type_ = TK_UNKNOWN; posStart_ = 0; posEnd_ = 0; }
		Token(Type type, std::wstring& element, int start, int end) { type_ = type; element_ = element; posStart_ = start; posEnd_ = end; }
		virtual ~Token() {};

		Type GetType() { return type_; }
		std::wstring& GetElement() { return element_; }
		std::string GetElementA();

		int GetStartPointer() { return posStart_; }
		int GetEndPointer() { return posEnd_; }

		int GetInteger();
		double GetReal();
		bool GetBoolean();
		std::wstring GetString();
		std::wstring& GetIdentifier();

		std::string GetStringA();
		std::string GetIdentifierA();
	};

	class Scanner {
	public:
		enum {

		};

	protected:
		int typeEncoding_;
		int textStartPointer_;
		std::vector<char> buffer_;
		int pointer_;//���̈ʒu
		Token token_;//���݂̃g�[�N��
		bool bPermitSignNumber_;
		std::list<Token> listDebugToken_;

		const char* bufStr_;

		wchar_t _CurrentChar();
		wchar_t _NextChar();//�|�C���^��i�߂Ď��̕����𒲂ׂ�

		virtual void _SkipComment();//�R�����g���Ƃ΂�
		virtual void _SkipSpace();//�󔒂��Ƃ΂�
		virtual void _RaiseError(std::wstring str) {	//��O�𓊂��܂�
			throw gstd::wexception(str);
		}
	public:
		Scanner(char* str, int size);
		Scanner(std::string str);
		Scanner(std::wstring wstr);
		Scanner(std::vector<char>& buf);
		virtual ~Scanner();

		void SetPermitSignNumber(bool bEnable) { bPermitSignNumber_ = bEnable; }
		int GetEncoding() { return typeEncoding_; }

		Token& GetToken() {	//���݂̃g�[�N�����擾
			return token_;
		}
		Token& Next();
		bool HasNext();
		void CheckType(Token& tok, Token::Type type);
		void CheckIdentifer(Token& tok, std::wstring id);
		int GetCurrentLine();

		int GetCurrentPointer() { return pointer_; }
		void SetCurrentPointer(int pos) { pointer_ = pos; }
		void SetPointerBegin() { pointer_ = textStartPointer_; }
		std::wstring GetString(int start, int end);

		bool CompareMemory(int start, int end, const char* data);
	};

	//================================================================
	//TextParser
	class TextParser {
	public:
		enum {
			TYPE_REAL,
			TYPE_BOOLEAN,
			TYPE_STRING,
		};

		class Result {
			friend TextParser;
		protected:
			int type_;
			int pos_;
			std::wstring valueString_;
			union {
				double valueReal_;
				bool valueBoolean_;
			};
		public:
			virtual ~Result() {};
			int GetType() { return type_; }
			double GetReal() {
				double res = valueReal_;
				if (IsBoolean())res = valueBoolean_ ? 1.0 : 0.0;
				if (IsString())res = StringUtility::ToDouble(valueString_);
				return res;
			}
			void SetReal(double value) {
				type_ = TYPE_REAL;
				valueReal_ = value;
			}
			bool GetBoolean() {
				bool res = valueBoolean_;
				if (IsReal())res = (valueReal_ != 0.0 ? true : false);
				if (IsString())res = (valueString_ == L"true" ? true : false);
				return res;
			}
			void SetBoolean(bool value) {
				type_ = TYPE_BOOLEAN;
				valueBoolean_ = value;
			}
			std::wstring GetString() {
				std::wstring res = valueString_;
				if (IsReal())res = gstd::StringUtility::Format(L"%f", valueReal_);
				if (IsBoolean())res = (valueBoolean_ ? L"true" : L"false");
				return res;
			}
			void SetString(std::wstring value) {
				type_ = TYPE_STRING;
				valueString_ = value;
			}
			bool IsReal() { return type_ == TYPE_REAL; }
			bool IsBoolean() { return type_ == TYPE_BOOLEAN; }
			bool IsString() { return type_ == TYPE_STRING; }
		};

	protected:
		gstd::ref_count_ptr<Scanner> scan_;

		void _RaiseError(std::wstring message) {
			throw gstd::wexception(message);
		}
		Result _ParseComparison(int pos);
		Result _ParseSum(int pos);
		Result _ParseProduct(int pos);
		Result _ParseTerm(int pos);
		virtual Result _ParseIdentifer(int pos);
	public:
		TextParser();
		TextParser(std::string source);
		virtual ~TextParser();

		void SetSource(std::string source);
		Result GetResult();
		double GetReal();
	};

	//================================================================
	//Font
	class Font {
	public:
		const static wchar_t* GOTHIC;
		const static wchar_t* MINCHOH;
	protected:
		HFONT hFont_;
		LOGFONT info_;
	public:
		Font();
		virtual ~Font();
		void CreateFont(const wchar_t* type, int size, bool bBold = false, bool bItalic = false, bool bLine = false);
		void CreateFontIndirect(LOGFONT& fontInfo);
		void Clear();
		HFONT GetHandle() { return hFont_; }
		LOGFONT GetInfo() { return info_; }
	};

	/*
	//================================================================
	//ObjectPool
	template <class T, bool SYNC>
	class ObjectPool {
	protected:
		std::vector<std::list<gstd::ref_count_ptr<T, SYNC>>> listUsedPool_;
		std::vector<std::vector<gstd::ref_count_ptr<T, SYNC>>> listCachePool_;

		virtual void _CreatePool(int countType) {
			listUsedPool_.resize(countType);
			listCachePool_.resize(countType);
		}
		virtual gstd::ref_count_ptr<T, SYNC> _CreatePoolObject(int type) = 0;
		virtual void _ResetPoolObject(gstd::ref_count_ptr<T, SYNC>& obj) {}
		virtual void _ArrangePool() {
			int countType = listUsedPool_.size();
			for (int iType = 0; iType < countType; iType++) {
				std::list<gstd::ref_count_ptr<T, SYNC>>* listUsed = &listUsedPool_[iType];
				std::vector<gstd::ref_count_ptr<T, SYNC>>* listCache = &listCachePool_[iType];

				std::list<gstd::ref_count_ptr<T, SYNC>>::iterator itr = listUsed->begin();
				for (; itr != listUsed->end(); ) {
					gstd::ref_count_ptr<T, SYNC> obj = (*itr);
					if (obj.GetReferenceCount() == 2) {
						itr = listUsed->erase(itr);
						_ResetPoolObject(obj);
						listCache->push_back(obj);
					}
					else {
						itr++;
					}
				}
			}
		}
	public:
		ObjectPool() {}
		virtual ~ObjectPool() {}
		virtual gstd::ref_count_ptr<T, SYNC> GetPoolObject(int type) {
			gstd::ref_count_ptr<T, SYNC> res = NULL;
			if (listCachePool_[type].size() > 0) {
				res = listCachePool_[type].back();
				listCachePool_[type].pop_back();
			}
			else {
				res = _CreatePoolObject(type);
			}
			listUsedPool_[type].push_back(res);
			return res;
		}

		int GetUsedPoolObjectCount() {
			int res = 0;
			for (int i = 0; i < listUsedPool_.size(); i++) {
				res += listUsedPool_[i].size();
			}
			return res;
		}

		int GetCachePoolObjectCount() {
			int res = 0;
			for (int i = 0; i < listCachePool_.size(); i++) {
				res += listCachePool_[i].size();
			}
			return res;
		}
	};
	*/
}

#endif
