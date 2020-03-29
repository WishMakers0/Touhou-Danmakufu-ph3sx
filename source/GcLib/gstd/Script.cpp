#include "source/GcLib/pch.h"

#include "Script.hpp"
#include "GstdUtility.hpp"

#ifdef _MSC_VER
//#define for if(0);else for
namespace std {
	using::wcstombs;
	using::mbstowcs;
	using::isalpha;
	using::fmodl;
	using::powl;
	using::swprintf;
	using::atof;
	using::isdigit;
	using::isxdigit;
	using::floorl;
	using::ceill;
	using::fabsl;
	using::iswdigit;
	using::iswalpha;
}

#endif

using namespace gstd;

double fmodl2(double i, double j) {
	if (j < 0) {
		//return (i < 0) ? -(-i % -j) : (i % -j) + j;
		return (i < 0) ? -fmodl(-i, -j) : fmodl(i, -j) + j;
	}
	else {
		//return (i < 0) ? -(-i % j) + j : i % j;
		return (i < 0) ? -fmodl(-i, j) + j : fmodl(i, j);
	}
}

/* value */

value::value(type_data* t, std::wstring v) {
	data = std::shared_ptr<body>(new body);
	data->type = t;
	for (size_t i = 0; i < v.size(); ++i)
		data->array_value.push_back(value(t->get_element(), v[i]));
}

void value::append(type_data* t, value const& x) {
	unique();
	data->type = t;
	data->array_value.push_back(x);
}

void value::concatenate(value const& x) {
	unique();

	if (length_as_array() == 0) data->type = x.data->type;
	for (auto itr = x.data->array_value.begin(); itr != x.data->array_value.end(); ++itr) {
		value v = *itr;
		data->array_value.push_back(v);
	}
}

double value::as_real() const {
	if (data == nullptr)
		return 0.0;
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_real:
		return data->real_value;
	case type_data::type_kind::tk_char:
		return static_cast<float>(data->char_value);
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? 1.0 : 0.0;
	case type_data::type_kind::tk_array:
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			try {
				return std::stol(as_string());
			}
			catch (...) {
				return 0.0;
			}
		}
		else
			return length_as_array();
	default:
		assert(false);
		return 0.0;
	}
}
wchar_t value::as_char() const {
	if (data == nullptr)
		return L'\0';
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_real:
		return data->real_value;
	case type_data::type_kind::tk_char:
		return data->char_value;
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? L'1' : L'0';
	case type_data::type_kind::tk_array:
		return L'\0';
	default:
		assert(false);
		return L'\0';
	}
}
bool value::as_boolean() const {
	if (data == nullptr)
		return false;
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_real:
		return data->real_value != 0.0;
	case type_data::type_kind::tk_char:
		return data->char_value != L'\0';
	case type_data::type_kind::tk_boolean:
		return data->boolean_value;
	case type_data::type_kind::tk_array:
		return data->array_value.size() != 0;
	default:
		assert(false);
		return false;
	}
}
std::wstring value::as_string() const {
	if (data == nullptr)
		return L"(VOID)";
	switch (data->type->get_kind()) {
	case type_data::type_kind::tk_real:
		return std::to_wstring(data->real_value);
	case type_data::type_kind::tk_char:
		return std::wstring(&data->char_value, 1);
	case type_data::type_kind::tk_boolean:
		return (data->boolean_value) ? L"true" : L"false";
	case type_data::type_kind::tk_array:
	{
		if (data->type->get_element()->get_kind() == type_data::type_kind::tk_char) {
			std::wstring result;
			for (size_t i = 0; i < data->array_value.size(); ++i)
				result += data->array_value[i].as_char();
			return result;
		}
		else {
			std::wstring result = L"[";
			for (size_t i = 0; i < data->array_value.size(); ++i) {
				result += data->array_value[i].as_string();
				if (i != data->array_value.size() - 1)
					result += L",";
			}
			result += L"]";
			return result;
		}
	}
	default:
		assert(false);
		return L"(INTERNAL-ERROR)";
	}
}

unsigned value::length_as_array() const {
	assert(data != nullptr && data->type->get_kind() == type_data::type_kind::tk_array);
	return data->array_value.size();
}

value const& value::index_as_array(size_t i) const {
	assert(data != nullptr && data->type->get_kind() == type_data::type_kind::tk_array);
	assert(i < data->array_value.size());
	return data->array_value[i];
}

value& value::index_as_array(size_t i) {
	assert(data != nullptr && data->type->get_kind() == type_data::type_kind::tk_array);
	assert(i < data->array_value.size());
	return data->array_value[i];
}

type_data* value::get_type() const {
	assert(data != nullptr);
	return data->type;
}

void value::overwrite(value const& source) {
	assert(data != nullptr);

	*data = *source.data;
	unique();

	//	* data = * source.data;
	//	++(data->ref_count);
}

//--------------------------------------

/* parser_error */

class parser_error : public gstd::wexception {
public:
	parser_error(std::wstring const& the_message) : gstd::wexception(the_message) {}

private:

};

/* lexical analyzer */

enum class token_kind : uint8_t {
	tk_end, tk_invalid, tk_word, tk_real, tk_char, tk_string, tk_open_par, tk_close_par, tk_open_bra, tk_close_bra, tk_open_cur,
	tk_close_cur, tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_tilde, tk_assign, tk_plus, tk_minus, tk_inc, tk_dec,
	tk_asterisk, tk_slash, tk_percent, tk_caret, tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, tk_exclamation, tk_ampersand,
	tk_and_then, tk_vertical, tk_or_else, tk_at, tk_add_assign, tk_subtract_assign, tk_multiply_assign, tk_divide_assign,
	tk_remainder_assign, tk_power_assign, tk_range, tk_ALTERNATIVE, tk_ASCENT, tk_BREAK, tk_CASE, tk_DESCENT, tk_ELSE,
	tk_FUNCTION, tk_IF, tk_IN, tk_LET, tk_LOCAL, tk_LOOP, tk_CONTINUE, tk_OTHERS, tk_REAL, tk_RETURN,
	tk_SUB, tk_TASK, tk_TIMES, tk_WHILE, tk_YIELD, tk_WAIT,
	/*tk_bitwise_not, tk_bitwise_and, tk_bitwise_or, tk_bitwise_xor, tk_bitwise_left, tk_bitwise_right,*/
};

const char* token_kind_str[] = {
	"tk_end", "tk_invalid", "tk_word", "tk_real", "tk_char", "tk_string", "tk_open_par", "tk_close_par", "tk_open_bra", "tk_close_bra", "tk_open_cur",
	"tk_close_cur", "tk_open_abs", "tk_close_abs", "tk_comma", "tk_semicolon", "tk_tilde", "tk_assign", "tk_plus", "tk_minus", "tk_inc", "tk_dec",
	"tk_asterisk", "tk_slash", "tk_percent", "tk_caret", "tk_e", "tk_g", "tk_ge", "tk_l", "tk_le", "tk_ne", "tk_exclamation", "tk_ampersand",
	"tk_and_then", "tk_vertical", "tk_or_else", "tk_at", "tk_add_assign", "tk_subtract_assign", "tk_multiply_assign", "tk_divide_assign",
	"tk_remainder_assign", "tk_power_assign", "tk_range", "tk_ALTERNATIVE", "tk_ASCENT", "tk_BREAK", "tk_CASE", "tk_DESCENT", "tk_ELSE",
	"tk_FUNCTION", "tk_IF", "tk_IN", "tk_LET", "tk_LOCAL", "tk_LOOP", "tk_OTHERS", "tk_REAL", "tk_RETURN",
	"tk_SUB", "tk_TASK", "tk_TIMES", "tk_WHILE", "tk_YIELD", "tk_WAIT",
};

class scanner {
	int encoding;
	char const* current;
	char const* endPoint;

	inline wchar_t current_char();
	inline wchar_t index_from_current_char(int index);
	inline wchar_t next_char();
public:
	token_kind next;
	std::string word;
	double real_value;
	wchar_t char_value;
	std::wstring string_value;
	int line;

	scanner(char const* source, char const* end) : current(source), line(1) {
		endPoint = end;
		encoding = Encoding::SHIFT_JIS;
		if (Encoding::IsUtf16Le(source, 2)) {
			encoding = Encoding::UTF16LE;

			int bomSize = Encoding::GetBomSize(source, 2);
			current += bomSize;
		}

		advance();
	}

	scanner(scanner const& source) :
		encoding(source.encoding),
		current(source.current), endPoint(source.endPoint),
		next(source.next),
		word(source.word),
		line(source.line) {
	}

	void skip();
	void advance();

	void AddLog(wchar_t* data);
};

wchar_t scanner::current_char() {
	wchar_t res = L'\0';
	if (encoding == Encoding::UTF16LE) {
		res = (wchar_t&)current[0];
	}
	else {
		res = *current;
	}
	return res;
}
wchar_t scanner::index_from_current_char(int index) {
	wchar_t res = L'\0';
	if (encoding == Encoding::UTF16LE) {
		const char* pos = current + index * 2;
		if (pos >= endPoint)return L'\0';
		res = (wchar_t&)current[index * 2];
	}
	else {
		const char* pos = current + index;
		if (pos >= endPoint)return L'\0';
		res = current[index];
	}

	return res;
}
wchar_t scanner::next_char() {
	if (encoding == Encoding::UTF16LE) {
		current += 2;
	}
	else {
		++current;
	}

	wchar_t res = current_char();
	return res;
}

void scanner::skip() {
	//空白を飛ばす
	wchar_t ch1 = current_char();
	wchar_t ch2 = index_from_current_char(1);
	while (ch1 == '\r' || ch1 == '\n' || ch1 == L'\t' || ch1 == L' '
		|| ch1 == L'#' || (ch1 == L'/' && (ch2 == L'/' || ch2 == L'*'))) {
		//コメントを飛ばす
		if (ch1 == L'#' ||
			(ch1 == L'/' && (ch2 == L'/' || ch2 == L'*'))) {
			if (ch1 == L'#' || ch2 == L'/') {
				do {
					ch1 = next_char();
				} while (ch1 != L'\r' && ch1 != L'\n');
			}
			else {
				next_char();
				ch1 = next_char();
				ch2 = index_from_current_char(1);
				while (ch1 != L'*' || ch2 != L'/') {
					if (ch1 == L'\n') ++line;
					ch1 = next_char();
					ch2 = index_from_current_char(1);
				}
				ch1 = next_char();
				ch1 = next_char();
			}
		}
		else if (ch1 == L'\n') {
			++line;
			ch1 = next_char();
		}
		else
			ch1 = next_char();
		ch2 = index_from_current_char(1);
	}
}

void scanner::AddLog(wchar_t* data) {
	{
		wchar_t* pStart = (wchar_t*)current;
		wchar_t* pEnd = (wchar_t*)(current + std::min(16, endPoint - current));
		std::wstring wstr = std::wstring(pStart, pEnd);
		//Logger::WriteTop(StringUtility::Format(L"%s current=%d, endPoint=%d, val=%d, ch=%s", data, pStart, endPoint, (wchar_t)*current, wstr.c_str()));
	}
}

void scanner::advance() {
	skip();

	wchar_t ch = current_char();
	if (ch == L'\0' || current >= endPoint) {
		next = token_kind::tk_end;
		return;
	}

	switch (ch) {
	case L'[':
		next = token_kind::tk_open_bra;
		ch = next_char();
		break;
	case L']':
		next = token_kind::tk_close_bra;
		ch = next_char();
		break;
	case L'(':
		next = token_kind::tk_open_par;
		ch = next_char();
		if (ch == L'|') {
			next = token_kind::tk_open_abs;
			ch = next_char();
		}
		break;
	case L')':
		next = token_kind::tk_close_par;
		ch = next_char();
		break;
	case L'{':
		next = token_kind::tk_open_cur;
		ch = next_char();
		break;
	case L'}':
		next = token_kind::tk_close_cur;
		ch = next_char();
		break;
	case L'@':
		next = token_kind::tk_at;
		ch = next_char();
		break;
	case L',':
		next = token_kind::tk_comma;
		ch = next_char();
		break;
	case L';':
		next = token_kind::tk_semicolon;
		ch = next_char();
		break;
	case L'~':
		next = token_kind::tk_tilde;
		ch = next_char();
		break;
	case L'*':
		next = token_kind::tk_asterisk;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_multiply_assign;
			ch = next_char();
		}
		break;
	case L'/':
		next = token_kind::tk_slash;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_divide_assign;
			ch = next_char();
		}
		break;
	case L'%':
		next = token_kind::tk_percent;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_remainder_assign;
			ch = next_char();
		}
		break;
	case L'^':
		next = token_kind::tk_caret;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_power_assign;
			ch = next_char();
		}
		break;
	case L'=':
		next = token_kind::tk_assign;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_e;
			ch = next_char();
		}
		break;
	case L'>':
		next = token_kind::tk_g;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_ge;
			ch = next_char();
		}
		break;
	case L'<':
		next = token_kind::tk_l;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_le;
			ch = next_char();
		}
		break;
	case L'!':
		next = token_kind::tk_exclamation;
		ch = next_char();
		if (ch == L'=') {
			next = token_kind::tk_ne;
			ch = next_char();
		}
		break;
	case L'+':
		next = token_kind::tk_plus;
		ch = next_char();
		if (ch == L'+') {
			next = token_kind::tk_inc;
			ch = next_char();
		}
		else if (ch == L'=') {
			next = token_kind::tk_add_assign;
			ch = next_char();
		}
		break;
	case L'-':
		next = token_kind::tk_minus;
		ch = next_char();
		if (ch == L'-') {
			next = token_kind::tk_dec;
			ch = next_char();
		}
		else if (ch == L'=') {
			next = token_kind::tk_subtract_assign;
			ch = next_char();
		}
		break;
	case L'&':
		next = token_kind::tk_ampersand;
		ch = next_char();
		if (ch == L'&') {
			next = token_kind::tk_and_then;
			ch = next_char();
		}
		break;
	case L'|':
		next = token_kind::tk_vertical;
		ch = next_char();
		if (ch == L'|') {
			next = token_kind::tk_or_else;
			ch = next_char();
		}
		else if (ch == L')') {
			next = token_kind::tk_close_abs;
			ch = next_char();
		}
		break;
	case L'.':
		ch = next_char();
		if (ch == L'.') {
			next = token_kind::tk_range;
			ch = next_char();
		}
		else {
			std::wstring error = L"Invalid period(.) placement.\r\n";
			throw parser_error(error);
		}
		break;

	case L'\'':
	case L'\"':
	{
		wchar_t q = current_char();
		next = (q == L'\"') ? token_kind::tk_string : token_kind::tk_char;
		ch = next_char();
		wchar_t pre = (wchar_t)next;
		if (encoding == Encoding::UTF16LE) {
			std::wstring s;
			while (true) {
				if (ch == q && pre != L'\\')break;

				if (ch == L'\\') {
					if (pre == L'\\')s += ch;
				}
				else {
					s += ch;
				}

				pre = ch;
				ch = next_char();
			}
			ch = next_char();
			string_value = s;
		}
		else {
			std::string s;
			while (true) {
				if (ch == q && pre != L'\\')break;

				if (ch == L'\\') {
					if (pre == L'\\')s += *current;
				}
				else {
					s += *current;
				}

				pre = ch;

				if (IsDBCSLeadByteEx(Encoding::CP_SHIFT_JIS, ch)) {
					ch = next_char();
					s += ch;
				}
				ch = next_char();
			}
			ch = next_char();
			string_value = StringUtility::ConvertMultiToWide(s);
		}

		if (q == L'\'') {
			if (string_value.size() == 1)
				char_value = string_value[0];
			else
				throw parser_error(L"A value of type char may only be one character long.");
		}
	}
	break;
	case L'\\':
	{
		ch = next_char();
		next = token_kind::tk_char;
		wchar_t c = ch;
		ch = next_char();
		switch (c) {
		case L'0':
			char_value = L'\0';
			break;
		case L'n':
			char_value = L'\n';
			break;
		case L'r':
			char_value = L'\r';
			break;
		case L't':
			char_value = L'\t';
			break;
		case L'x':
			char_value = 0;
			while (std::isxdigit(ch)) {
				char_value = char_value * 16 + (ch >= L'a') ? ch - L'a' + 10 : (ch >= L'A') ?
					ch - L'A' + 10 : ch - L'0';
				ch = next_char();
			}
			break;
		default:
		{
			std::wstring error = L"Invalid character.\r\n";
			throw parser_error(error);
		}
		}
	}
	break;
	default:
		if (std::iswdigit(ch)) {
			next = token_kind::tk_real;
			real_value = 0.0;
			do {
				real_value = real_value * 10. + (ch - L'0');
				ch = next_char();
			} while (std::iswdigit(ch));

			wchar_t ch2 = index_from_current_char(1);
			if (ch == L'.' && std::iswdigit(ch2)) {
				ch = next_char();
				float d = 1;
				while (std::iswdigit(ch)) {
					d = d / 10;
					real_value = real_value + d * (ch - L'0');
					ch = next_char();
				}
			}
		}
		else if (std::iswalpha(ch) || ch == L'_') {
			next = token_kind::tk_word;
			if (encoding == Encoding::UTF16LE) {
				word = "";
				do {
					word += (char)ch;
					ch = next_char();
				} while (std::iswalpha(ch) || ch == '_' || std::iswdigit(ch));
			}
			else {
				char* pStart = (char*)current;
				char* pEnd = pStart;
				do {
					ch = next_char();
					pEnd = (char*)current;
				} while (std::iswalpha(ch) || ch == '_' || std::iswdigit(ch));
				word = std::string(pStart, pEnd);
			}

			/*
			if (word == "alternative")
				next = token_kind::tk_ALTERNATIVE;
			else if (word == "ascent")
				next = token_kind::tk_ASCENT;
			else if (word == "break")
				next = token_kind::tk_BREAK;
			else if (word == "case")
				next = token_kind::tk_CASE;
			else if (word == "descent")
				next = token_kind::tk_DESCENT;
			else if (word == "else")
				next = token_kind::tk_ELSE;
			else if (word == "function")
				next = token_kind::tk_FUNCTION;
			else if (word == "if")
				next = token_kind::tk_IF;
			else if (word == "in")
				next = token_kind::tk_IN;
			else if (word == "let" || word == "var")
				next = token_kind::tk_LET;
			else if (word == "local")
				next = token_kind::tk_LOCAL;
			else if (word == "loop")
				next = token_kind::tk_LOOP;
			else if (word == "others")
				next = token_kind::tk_OTHERS;
			else if (word == "real")
				next = token_kind::tk_REAL;
			else if (word == "return")
				next = token_kind::tk_RETURN;
			else if (word == "sub")
				next = token_kind::tk_SUB;
			else if (word == "task")
				next = token_kind::tk_TASK;
			else if (word == "times")
				next = token_kind::tk_TIMES;
			else if (word == "while")
				next = token_kind::tk_WHILE;
			else if (word == "yield")
				next = token_kind::tk_YIELD;
			else if (word == "wait")
				next = token_kind::tk_WAIT;
			else if (word == "continue")
				next = token_kind::tk_CONTINUE;
			*/

			size_t strHash = std::hash<std::string>{}(word);

			switch (strHash) {
			case 0xc606a268:	//alternative
			case 0x93e05f71:	//switch
				next = token_kind::tk_ALTERNATIVE;
				break;
			case 0x3b698387:	//ascent
				next = token_kind::tk_ASCENT;
				break;
			case 0xc9648178:	//break
				next = token_kind::tk_BREAK;
				break;
			case 0x9b2538b1:	//case
				next = token_kind::tk_CASE;
				break;
			case 0x4fda1135:	//descent
				next = token_kind::tk_DESCENT;
				break;
			case 0xbdbf5bf0:	//else
				next = token_kind::tk_ELSE;
				break;
			case 0x9ed64249:	//function
				next = token_kind::tk_FUNCTION;
				break;
			case 0x39386e06:	//if
				next = token_kind::tk_IF;
				break;
			case 0x41387a9e:	//in
				next = token_kind::tk_IN;
				break;
			case 0x506b03fa:	//let
			case 0x8a25e7be:	//var
				next = token_kind::tk_LET;
				break;
			case 0x9c436708:	//local
				next = token_kind::tk_LOCAL;
				break;
			case 0xddef486b:	//loop
				next = token_kind::tk_LOOP;
				break;
			case 0x23a5c9f2:	//others
			case 0x933b5bde:	//default
				next = token_kind::tk_OTHERS;
				break;
			case 0xd6dfb05d:	//real
				next = token_kind::tk_REAL;
				break;
			case 0x85ee37bf:	//return
				next = token_kind::tk_RETURN;
				break;
			case 0xdc4e3915:	//sub
				next = token_kind::tk_SUB;
				break;
			case 0xf448886c:	//task
				next = token_kind::tk_TASK;
				break;
			case 0x5d68eeb5:	//times
				next = token_kind::tk_TIMES;
				break;
			case 0x0dc628ce:	//while
				next = token_kind::tk_WHILE;
				break;
			case 0x6c96f2ae:	//yield
				next = token_kind::tk_YIELD;
				break;
			case 0x892e4ca0:	//wait
				next = token_kind::tk_WAIT;
				break;
			case 0xb1727e44:	//continue
				next = token_kind::tk_CONTINUE;
				break;
			}
		}
		else {
			next = token_kind::tk_invalid;
		}
	}

}

/* operations */

value add(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::type_kind::tk_array) {
		/*
		if (argv[0].get_type() != argv[1].get_type()) {
			std::wstring error;
			error += L"Variable type mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array()) {
			std::wstring error;
			error += L"Array length mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		*/
		bool rightIsArray = argv[1].get_type()->get_kind() == type_data::type_kind::tk_array;

		size_t countOp = argv[0].length_as_array();
		if (rightIsArray) countOp = std::min(countOp, argv[1].length_as_array());

		value result;
		for (size_t i = 0; i < countOp; ++i) {
			value v[2];
			v[0] = argv[0].index_as_array(i);
			if (rightIsArray) v[1] = argv[1].index_as_array(i);
			else v[1] = argv[1];
			result.append(argv[0].get_type(), add(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() + argv[1].as_real());
}

value subtract(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::type_kind::tk_array) {
		/*
		if (argv[0].get_type() != argv[1].get_type()) {
			std::wstring error;
			error += L"Variable type mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array()) {
			std::wstring error;
			error += L"Array length mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		*/

		bool rightIsArray = argv[1].get_type()->get_kind() == type_data::type_kind::tk_array;

		size_t countOp = argv[0].length_as_array();
		if (rightIsArray) countOp = std::min(countOp, argv[1].length_as_array());

		value result;
		for (size_t i = 0; i < countOp; ++i) {
			value v[2];
			v[0] = argv[0].index_as_array(i);
			if (rightIsArray) v[1] = argv[1].index_as_array(i);
			else v[1] = argv[1];
			result.append(argv[0].get_type(), subtract(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() - argv[1].as_real());
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value multiply(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::type_kind::tk_array) {
		/*
		if (argv[0].get_type() != argv[1].get_type()) {
			std::wstring error;
			error += L"Variable type mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array()) {
			std::wstring error;
			error += L"Array length mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		*/

		bool rightIsArray = argv[1].get_type()->get_kind() == type_data::type_kind::tk_array;

		size_t countOp = argv[0].length_as_array();
		if (rightIsArray) countOp = std::min(countOp, argv[1].length_as_array());

		value result;
		for (size_t i = 0; i < countOp; ++i) {
			value v[2];
			v[0] = argv[0].index_as_array(i);
			if (rightIsArray) v[1] = argv[1].index_as_array(i);
			else v[1] = argv[1];
			result.append(argv[0].get_type(), multiply(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() * argv[1].as_real());
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value divide(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::type_kind::tk_array) {
		/*
		if (argv[0].get_type() != argv[1].get_type()) {
			std::wstring error;
			error += L"Variable type mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array()) {
			std::wstring error;
			error += L"Array length mismatch.\r\n";
			machine->raise_error(error);
			return value();
		}
		*/

		bool rightIsArray = argv[1].get_type()->get_kind() == type_data::type_kind::tk_array;

		size_t countOp = argv[0].length_as_array();
		if (rightIsArray) countOp = std::min(countOp, argv[1].length_as_array());

		value result;
		for (size_t i = 0; i < countOp; ++i) {
			value v[2];
			v[0] = argv[0].index_as_array(i);
			if (rightIsArray) v[1] = argv[1].index_as_array(i);
			else v[1] = argv[1];
			result.append(argv[0].get_type(), divide(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() / argv[1].as_real());
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value remainder(script_machine* machine, int argc, value const* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	return value(machine->get_engine()->get_real_type(), fmodl2(x, y));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value modc(script_machine* machine, int argc, value const* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	return value(machine->get_engine()->get_real_type(), fmod(x, y));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value negative(script_machine* machine, int argc, value const* argv) {
	if (argv[0].get_type()->get_kind() == type_data::type_kind::tk_array) {
		value result;
		for (size_t i = 0; i < argv[0].length_as_array(); ++i) {
			value v = argv[0].index_as_array(i);
			result.append(argv[0].get_type(), negative(machine, 1, &v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), -argv[0].as_real());
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value power(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_real_type(), std::pow(argv[0].as_real(), argv[1].as_real()));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value compare(script_machine* machine, int argc, value const* argv) {
	if (argv[0].get_type() == argv[1].get_type()) {
		int r = 0;

		switch (argv[0].get_type()->get_kind()) {
		case type_data::type_kind::tk_real:
		{
			double a = argv[0].as_real();
			double b = argv[1].as_real();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_char:
		{
			wchar_t a = argv[0].as_char();
			wchar_t b = argv[1].as_char();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_boolean:
		{
			bool a = argv[0].as_boolean();
			bool b = argv[1].as_boolean();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::type_kind::tk_array:
		{
			for (size_t i = 0; i < argv[0].length_as_array(); ++i) {
				if (i >= argv[1].length_as_array()) {
					r = +1;	//"123" > "12"
					break;
				}

				value v[2];
				v[0] = argv[0].index_as_array(i);
				v[1] = argv[1].index_as_array(i);
				r = compare(machine, 2, v).as_real();
				if (r != 0)
					break;
			}
			if (r == 0 && argv[0].length_as_array() < argv[1].length_as_array()) {
				r = -1;	//"12" < "123"
			}
		}
		break;

		default:
			assert(false);
		}
		return value(machine->get_engine()->get_real_type(), static_cast<double>(r));
	}
	else {
		std::wstring error = L"Values of different types are being compared.\r\n";
		machine->raise_error(error);
		return value();
	}
}

value predecessor(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	assert(argv[0].has_data());
	switch (argv[0].get_type()->get_kind()) {
	case type_data::type_kind::tk_real:
		return value(argv[0].get_type(), argv[0].as_real() - 1);

	case type_data::type_kind::tk_char:
	{
		wchar_t c = argv[0].as_char();
		--c;
		return value(argv[0].get_type(), c);
	}
	case type_data::type_kind::tk_boolean:
		return value(argv[0].get_type(), false);
	default:
	{
		std::wstring error = L"This value type does not support the predecessor operation.\r\n";
		machine->raise_error(error);
		return value();
	}
	}
}

value successor(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	assert(argv[0].has_data());
	switch (argv[0].get_type()->get_kind()) {
	case type_data::type_kind::tk_real:
		return value(argv[0].get_type(), argv[0].as_real() + 1);

	case type_data::type_kind::tk_char:
	{
		wchar_t c = argv[0].as_char();
		++c;
		return value(argv[0].get_type(), c);
	}
	case type_data::type_kind::tk_boolean:
		return value(argv[0].get_type(), true);
	default:
	{
		std::wstring error = L"This value type does not support the successor operation.\r\n";
		machine->raise_error(error);
		return value();
	}
	}
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value true_(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_boolean_type(), true);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value false_(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_boolean_type(), false);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value not_(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_boolean_type(), !argv[0].as_boolean());
}

value length(script_machine* machine, int argc, value const* argv) {
	assert(argc == 1);
	return value(machine->get_engine()->get_real_type(), (double)argv[0].length_as_array());
}

value index(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array index operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	double index = argv[1].as_real();

	if (index != (int)index) {
		std::wstring error = L"Array index must be an integer.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (index < 0 || index >= argv[0].length_as_array()) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	const value& result = argv[0].index_as_array(index);
	return const_cast<value&>(result);
}

value index_writable(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array index operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	double index = argv[1].as_real();

	if (index != (int)index) {
		std::wstring error = L"Array index must be an integer.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (index < 0 || index >= argv[0].length_as_array()) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	const value& result = argv[0].index_as_array(index);
	result.unique();
	return result;
}

value slice(script_machine* machine, int argc, value const* argv) {
	assert(argc == 3);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array slice operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	double index_1 = argv[1].as_real();

	if (index_1 != (int)index_1) {
		std::wstring error = L"Array slice indices must be integers.\r\n";
		machine->raise_error(error);
		return value();
	}

	double index_2 = argv[2].as_real();

	if (index_2 != (int)index_2) {
		std::wstring error = L"Array slice indices must be integers.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (index_1 < 0 || index_1 > index_2 || index_2 > argv[0].length_as_array()) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result(argv[0].get_type(), std::wstring());

	for (int i = index_1; i < index_2; ++i) {
		result.append(result.get_type(), argv[0].index_as_array(i));
	}

	return result;
}

value erase(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array erase operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	double index_1 = argv[1].as_real();
	double length = argv[0].length_as_array();

	if (index_1 != (int)index_1) {
		std::wstring error = L"Array erase index must be an integer.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (index_1 < 0 || index_1 >= argv[0].length_as_array()) {
		std::wstring error = L"Array index out of bounds.\r\n";
		machine->raise_error(error);
		return value();
	}

	const value* arg0 = &(argv[0]);

	value result(arg0->get_type(), std::wstring());

	for (int i = 0; i < index_1; ++i) {
		result.append(result.get_type(), arg0->index_as_array(i));
	}
	for (int i = index_1 + 1; i < length; ++i) {
		result.append(result.get_type(), arg0->index_as_array(i));
	}
	return result;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value append(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array append operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (argv[0].length_as_array() > 0 && argv[0].get_type()->get_element() != argv[1].get_type()) {
		std::wstring error = L"Variable type mismatch.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result = argv[0];
	result.append(machine->get_engine()->get_array_type(argv[1].get_type()), argv[1]);
	return result;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value concatenate(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::type_kind::tk_array || argv[1].get_type()->get_kind() != type_data::type_kind::tk_array) {
		std::wstring error = L"This value type does not support the array concatenate operation.\r\n";
		machine->raise_error(error);
		return value();
	}

	if (argv[0].length_as_array() > 0 && argv[1].length_as_array() > 0 && argv[0].get_type() != argv[1].get_type()) {
		std::wstring error = L"Variable type mismatch.\r\n";
		machine->raise_error(error);
		return value();
	}

	value result = argv[0];
	result.concatenate(argv[1]);
	return result;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value round(script_machine* machine, int argc, value const* argv) {
	double r = std::floorl(argv[0].as_real() + 0.5);
	return value(machine->get_engine()->get_real_type(), r);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value truncate(script_machine* machine, int argc, value const* argv) {
	double r = argv[0].as_real();
	r = (r > 0) ? std::floorl(r) : std::ceill(r);
	return value(machine->get_engine()->get_real_type(), r);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value ceil(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_real_type(), std::ceil(argv[0].as_real()));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value floor(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_real_type(), std::floor(argv[0].as_real()));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value absolute(script_machine* machine, int argc, value const* argv) {
	double r = std::fabsl(argv[0].as_real());
	return value(machine->get_engine()->get_real_type(), r);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value assert_(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);
	if (!argv[0].as_boolean()) {
		machine->raise_error(argv[1].as_string());
	}
	return value();
}

int64_t bitDoubleToInt(double val) {
	if (val >= INT64_MAX)
		return INT64_MAX;
	if (val <= INT64_MIN)
		return INT64_MIN;
	return static_cast<int64_t>(val);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseNot(script_machine* machine, int argc, value const* argv) {
	int64_t val = bitDoubleToInt(argv[0].as_real());
	return value(machine->get_engine()->get_real_type(), (double)(~val));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseAnd(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(machine->get_engine()->get_real_type(), (double)(val1 & val2));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseOr(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(machine->get_engine()->get_real_type(), (double)(val1 | val2));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseXor(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	int64_t val2 = bitDoubleToInt(argv[1].as_real());
	return value(machine->get_engine()->get_real_type(), (double)(val1 ^ val2));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseLeft(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	double val2 = argv[1].as_real();

	if (val2 != (int)val2) {
		std::wstring error = L"Shifting factor must be an integer.\r\n";
		machine->raise_error(error);
		return value();
	}

	return value(machine->get_engine()->get_real_type(), (double)(val1 << (int)val2));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value bitwiseRight(script_machine* machine, int argc, value const* argv) {
	int64_t val1 = bitDoubleToInt(argv[0].as_real());
	double val2 = argv[1].as_real();

	if (val2 != (int)val2) {
		std::wstring error = L"Shifting factor must be an integer.\r\n";
		machine->raise_error(error);
		return value();
	}

	return value(machine->get_engine()->get_real_type(), (double)(val1 >> (int)val2));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value isNull(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_boolean_type(), !(argv[0].has_data()));
}

#ifdef __BORLANDC__
#pragma argsused
#endif
value script_debugBreak(script_machine* machine, int argc, value const* argv) {
	DebugBreak();
	return value();
}

function const operations[] = {
	{ "true", true_, 0 },
	{ "false", false_, 0 },
	{ "length", length, 1 },
	{ "not", not_, 1 },
	{ "negative", negative, 1 },
	{ "predecessor", predecessor, 1 },
	{ "successor", successor, 1 },
	{ "round", round, 1 },
	{ "trunc", truncate, 1 },
	{ "truncate", truncate, 1 },
	{ "ceil", ceil, 1 },
	{ "floor", floor, 1 },
	{ "absolute", absolute, 1 },
	{ "add", add, 2 },
	{ "subtract", subtract, 2 },
	{ "multiply", multiply, 2 },
	{ "divide", divide, 2 },
	{ "remainder", remainder, 2 },
	{ "modc", modc, 2 },
	{ "power", power, 2 },
	{ "index_", index, 2 },
	{ "index!", index_writable, 2 },
	{ "slice", slice, 3 },
	{ "erase", erase, 2 },
	{ "append", append, 2 },
	{ "concatenate", concatenate, 2 },
	{ "compare", compare, 2 },
	{ "assert", assert_, 2 },
	{ "bit_not", bitwiseNot, 1 },
	{ "bit_and", bitwiseAnd, 2 },
	{ "bit_or", bitwiseOr, 2 },
	{ "bit_xor", bitwiseXor, 2 },
	{ "bit_left", bitwiseLeft, 2 },
	{ "bit_right", bitwiseRight, 2 },
	{ "isNull", isNull, 1 },
	{ "__DEBUG_BREAK", script_debugBreak, 0 },
};

/* parser */

class gstd::parser {
public:
	struct symbol {
		int level;
		script_engine::block* sub;
		int variable;
		bool can_overload = true;
	};

	struct scope_t : public std::multimap<std::string, symbol> {
		script_engine::block_kind kind;

		scope_t(script_engine::block_kind the_kind) : kind(the_kind) {}

		void singular_insert(std::string name, symbol& s, int argc = 0) {
			bool exists = this->find(name) != this->end();
			auto itr = this->equal_range(name);

			if (exists) {
				symbol* sPrev = &(itr.first->second);

				if (!sPrev->can_overload) {
					std::wstring error = StringUtility::FormatToWide("Default functions cannot be overloaded. (%s)",
						sPrev->sub->name.c_str());
				}
				else {
					for (auto itrPair = itr.first; itrPair != itr.second; ++itrPair) {
						if (argc == itrPair->second.sub->arguments) {
							itrPair->second = s;
							return;
						}
					}
				}
			}

			this->insert(std::make_pair(name, s));
		}
	};

	std::vector<scope_t> frame;
	scanner* lex;
	script_engine* engine;
	bool error;
	std::wstring error_message;
	int error_line;
	std::map<std::string, script_engine::block*> events;

	parser(script_engine* e, scanner* s, int funcc, function const* funcv);

	virtual ~parser() {}

	void parse_parentheses(script_engine::block* block);
	void parse_clause(script_engine::block* block);
	void parse_prefix(script_engine::block* block);
	void parse_suffix(script_engine::block* block);
	void parse_product(script_engine::block* block);
	void parse_sum(script_engine::block* block);
	void parse_comparison(script_engine::block* block);
	void parse_logic(script_engine::block* block);
	void parse_expression(script_engine::block* block);
	int parse_arguments(script_engine::block* block);
	void parse_statements(script_engine::block* block);
	void parse_inline_block(script_engine::block* block, script_engine::block_kind kind);
	void parse_block(script_engine::block* block, std::vector<std::string> const* args, bool adding_result);
private:
	void register_function(function const& func);
	symbol* search(std::string const& name, scope_t** ptrScope = nullptr);
	symbol* search(std::string const& name, int argc, scope_t** ptrScope = nullptr);
	symbol* search_in(scope_t* scope, std::string const& name);
	symbol* search_in(scope_t* scope, std::string const& name, int argc);
	symbol* search_result();
	void scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result);
	void write_operation(script_engine::block* block, char const* name, int clauses);

	typedef script_engine::code code;
};

parser::parser(script_engine* e, scanner* s, int funcc, function const* funcv) : engine(e), lex(s), frame(), error(false) {
	frame.push_back(scope_t(script_engine::block_kind::bk_normal));

	for (size_t i = 0; i < sizeof(operations) / sizeof(function); ++i)
		register_function(operations[i]);

	for (size_t i = 0; i < funcc; ++i)
		register_function(funcv[i]);

	try {
		scan_current_scope(0, nullptr, false);
		parse_statements(engine->main_block);
		if (lex->next != token_kind::tk_end) {
			std::wstring error = L"Unexpected end-of-file while parsing. (Did you forget a semicolon?)\r\n";
			throw parser_error(error);
		}
	}
	catch (parser_error& e) {
		error = true;
		error_message = e.what();
		error_line = lex->line;
	}
}

void parser::register_function(function const& func) {
	symbol s;
	s.level = 0;
	s.sub = engine->new_block(0, script_engine::block_kind::bk_function);
	s.sub->arguments = func.arguments;
	s.sub->name = func.name;
	s.sub->func = func.func;
	s.variable = -1;
	s.can_overload = false;
	frame[0].singular_insert(func.name, s, func.arguments);
}

parser::symbol* parser::search(std::string const& name, scope_t** ptrScope) {
	for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
		scope_t* scope = &*itr;
		if (ptrScope) *ptrScope = scope;

		auto itrSymbol = scope->find(name);
		if (itrSymbol != scope->end()) {
			return &(itrSymbol->second);
		}
	}
	return nullptr;
}
parser::symbol* parser::search(std::string const& name, int argc, scope_t** ptrScope) {
	for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
		scope_t* scope = &*itr;
		if (ptrScope) *ptrScope = scope;

		if (scope->find(name) == scope->end()) continue;

		auto itrSymbol = scope->equal_range(name);
		if (itrSymbol.first != scope->end()) {
			for (auto itrPair = itrSymbol.first; itrPair != itrSymbol.second; ++itrPair) {
				if (itrPair->second.sub) {
					if (argc == itrPair->second.sub->arguments)
						return &(itrPair->second);
				}
				else {
					return &(itrPair->second);
				}
			}
			return nullptr;
		}
	}
	return nullptr;
}
parser::symbol* parser::search_in(scope_t* scope, std::string const& name) {
	auto itrSymbol = scope->find(name);
	if (itrSymbol != scope->end())
		return &(itrSymbol->second);
	return nullptr;
}
parser::symbol* parser::search_in(scope_t* scope, std::string const& name, int argc) {
	if (scope->find(name) == scope->end()) return nullptr;

	auto itrSymbol = scope->equal_range(name);
	if (itrSymbol.first != scope->end()) {
		for (auto itrPair = itrSymbol.first; itrPair != itrSymbol.second; ++itrPair) {
			if (itrPair->second.sub) {
				if (argc == itrPair->second.sub->arguments)
					return &(itrPair->second);
			}
			else {
				return &(itrPair->second);
			}
		}
	}

	return nullptr;
}
parser::symbol* parser::search_result() {
	for (auto itr = frame.rbegin(); itr != frame.rend(); ++itr) {
		if (itr->kind == script_engine::block_kind::bk_sub || itr->kind == script_engine::block_kind::bk_microthread)
			return nullptr;

		auto itrSymbol = itr->find("\x01");
		if (itrSymbol != itr->end())
			return &(itrSymbol->second);
	}
	return nullptr;
}

void parser::scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result) {
	//先読みして識別子を登録する
	scanner lex2(*lex);
	try {
		scope_t* current_frame = &frame[frame.size() - 1];
		int cur = 0;
		int var = 0;

		if (adding_result) {
			symbol s;
			s.level = level;
			s.sub = nullptr;
			s.variable = var;
			++var;
			current_frame->singular_insert("\x01", s);
		}

		if (args != nullptr) {
			for (size_t i = 0; i < args->size(); ++i) {
				symbol s;
				s.level = level;
				s.sub = nullptr;
				s.variable = var;
				++var;
				current_frame->singular_insert((*args)[i], s);
			}
		}

		while (cur >= 0 && lex2.next != token_kind::tk_end && lex2.next != token_kind::tk_invalid) {
			switch (lex2.next) {
			case token_kind::tk_open_cur:
				++cur;
				lex2.advance();
				break;
			case token_kind::tk_close_cur:
				--cur;
				lex2.advance();
				break;
			case token_kind::tk_at:
			case token_kind::tk_SUB:
			case token_kind::tk_FUNCTION:
			case token_kind::tk_TASK:
			{
				token_kind type = lex2.next;
				lex2.advance();
				if (cur == 0) {
					size_t countArgs = 0;

					script_engine::block_kind kind = (type == token_kind::tk_SUB || type == token_kind::tk_at) ?
						script_engine::block_kind::bk_sub : (type == token_kind::tk_FUNCTION) ?
						script_engine::block_kind::bk_function : script_engine::block_kind::bk_microthread;

					std::string name = lex2.word;

					lex2.advance();
					if (lex2.next == token_kind::tk_open_par) {
						if (kind == script_engine::block_kind::bk_sub) {
							std::wstring error = L"A parameter list in not allowed here.\r\n";
							throw parser_error(error);
						}
						else {
							lex2.advance();
							while (lex2.next == token_kind::tk_word || lex2.next == token_kind::tk_LET ||
								lex2.next == token_kind::tk_REAL) {
								++countArgs;
								if (lex2.next == token_kind::tk_LET || lex2.next == token_kind::tk_REAL) lex2.advance();
								if (lex2.next == token_kind::tk_word) lex2.advance();
								if (lex2.next != token_kind::tk_comma)
									break;
								lex2.advance();
							}
						}
					}

					{
						symbol* dup = search_in(current_frame, name, countArgs);
						if (dup != nullptr) {	//Woohoo for detailed error messages.
							std::wstring typeSub;
							switch (dup->sub->kind) {
							case script_engine::block_kind::bk_function:
								typeSub = L"function";
								return;
							case script_engine::block_kind::bk_microthread:
								typeSub = L"task";
								return;
							case script_engine::block_kind::bk_sub:
								typeSub = L"sub or an \'@\' block";
								return;
							default:
								typeSub = L"block";
								return;
							}

							std::wstring error = L"A ";
							error += typeSub;
							error += L" of the same name was already defined in the current scope.\r\n";
							if (dup->can_overload)
								error += L"**You may overload functions and tasks by giving them different argument counts.\r\n";
							else
								error += StringUtility::FormatToWide("**\'%s\' cannot be overloaded.\r\n", name.c_str());
							throw parser_error(error);
						}
					}

					symbol s;
					s.level = level;
					s.sub = engine->new_block(level + 1, kind);
					s.sub->name = name;
					s.sub->func = nullptr;
					s.sub->arguments = countArgs;
					s.variable = -1;
					s.can_overload = kind != script_engine::block_kind::bk_sub;
					current_frame->singular_insert(name, s, countArgs);
				}
			}
			break;
			case token_kind::tk_REAL:
			case token_kind::tk_LET:
				lex2.advance();
				if (cur == 0) {
#ifdef __SCRIPT_H__NO_CHECK_DUPLICATED
					if (lex2.word == "\x01") {
#endif
						if (current_frame->find(lex2.word) != current_frame->end()) {
							std::wstring error = L"A variable of the same name was already declared in the current scope.\r\n";
							throw parser_error(error);
						}
#ifdef __SCRIPT_H__NO_CHECK_DUPLICATED
					}
#endif
					symbol s;
					s.level = level;
					s.sub = nullptr;
					s.variable = var;
					s.can_overload = false;
					++var;
					current_frame->singular_insert(lex2.word, s);

					lex2.advance();
				}
				break;
			default:
				lex2.advance();
				break;
			}
		}
	}
	catch (...) {
		lex->line = lex2.line;
		throw;
	}
}

void parser::write_operation(script_engine::block* block, char const* name, int clauses) {
	symbol* s = search(name);
	assert(s != nullptr);
	if (s->sub->arguments != clauses) {
		std::wstring error = L"Invalid argument count for the default function. (expected " + 
			std::to_wstring(clauses) + L")\r\n";
		throw parser_error(error);
	}

	block->codes.push_back(script_engine::code(lex->line, script_engine::command_kind::pc_call_and_push_result, s->sub, clauses));
}

void parser::parse_parentheses(script_engine::block* block) {
	if (lex->next != token_kind::tk_open_par) {
		std::wstring error = L"\"(\" is required.\r\n";
		throw parser_error(error);
	}
	lex->advance();

	parse_expression(block);

	if (lex->next != token_kind::tk_close_par) {
		std::wstring error = L"\")\" is required.\r\n";
		throw parser_error(error);
	}
	lex->advance();
}

void parser::parse_clause(script_engine::block* block) {
	if (lex->next == token_kind::tk_real) {
		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_value, 
			value(engine->get_real_type(), lex->real_value)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_char) {
		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_value, 
			value(engine->get_char_type(), lex->char_value)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_string) {
		std::wstring str = lex->string_value;
		lex->advance();
		while (lex->next == token_kind::tk_string || lex->next == token_kind::tk_char) {
			str += (lex->next == token_kind::tk_string) ? lex->string_value : (std::wstring() + lex->char_value);
			lex->advance();
		}

		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_value, 
			value(engine->get_string_type(), str)));
	}
	else if (lex->next == token_kind::tk_word) {
		std::string name = lex->word;
		lex->advance();
		int argc = parse_arguments(block);

		symbol* s = search(name, argc);

		if (s == nullptr) {
			std::wstring error;
			if (search(name) != nullptr)
				error = StringUtility::FormatToWide("No matching overload for %s with %d arguments was found.\r\n",
					lex->word.c_str(), argc);
			error = StringUtility::FormatToWide("%s is not defined.\r\n",
				lex->word.c_str());
			throw parser_error(error);
		}

		if (s->sub != nullptr) {
			if (s->sub->kind != script_engine::block_kind::bk_function) {
				std::wstring error = L"Tasks and subs cannot return values.\r\n";
				throw parser_error(error);
			}

			/*
			if (argc != s->sub->arguments) {
				std::wstring error;
				error += StringUtility::FormatToWide("\'%s\' has an incorrect number of arguments.\r\n", s->sub->name.c_str());
				throw parser_error(error);
			}
			*/

			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_call_and_push_result, s->sub, argc));
		}
		else {
			//変数
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_variable, s->level, s->variable));
		}
	}
	else if (lex->next == token_kind::tk_open_bra) {
		lex->advance();
		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_value, 
			value(engine->get_string_type(), std::wstring())));
		while (lex->next != token_kind::tk_close_bra) {
			parse_expression(block);
			write_operation(block, "append", 2);
			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}
		if (lex->next != token_kind::tk_close_bra) {
			std::wstring error = L"\"]\" is required.\r\n";
			throw parser_error(error);
		}
		lex->advance();
	}
	else if (lex->next == token_kind::tk_open_abs) {
		lex->advance();
		parse_expression(block);
		write_operation(block, "absolute", 1);
		if (lex->next != token_kind::tk_close_abs) {
			std::wstring error = L"\"|)\" is required.\r\n";
			throw parser_error(error);
		}
		lex->advance();
	}
	else if (lex->next == token_kind::tk_open_par) {
		parse_parentheses(block);
	}
	else {
		std::wstring error = L"Invalid expression.\r\n";
		throw parser_error(error);
	}
}

void parser::parse_suffix(script_engine::block* block) {
	parse_clause(block);
	if (lex->next == token_kind::tk_caret) {
		lex->advance();
		parse_suffix(block); //再帰
		write_operation(block, "power", 2);
	}
	else {
		while (lex->next == token_kind::tk_open_bra) {
			lex->advance();
			parse_expression(block);

			if (lex->next == token_kind::tk_range) {
				lex->advance();
				parse_expression(block);
				write_operation(block, "slice", 3);
			}
			else {
				write_operation(block, "index_", 2);
			}

			if (lex->next != token_kind::tk_close_bra) {
				std::wstring error = L"\"]\" is required.\r\n";
				throw parser_error(error);
			}
			lex->advance();
		}
	}
}

void parser::parse_prefix(script_engine::block* block) {
	if (lex->next == token_kind::tk_plus) {
		lex->advance();
		parse_prefix(block);	//再帰
	}
	else if (lex->next == token_kind::tk_minus) {
		lex->advance();
		parse_prefix(block);	//再帰
		write_operation(block, "negative", 1);
	}
	else if (lex->next == token_kind::tk_exclamation) {
		lex->advance();
		parse_prefix(block);	//再帰
		write_operation(block, "not", 1);
	}
	else {
		parse_suffix(block);
	}
}

void parser::parse_product(script_engine::block* block) {
	parse_prefix(block);
	while (lex->next == token_kind::tk_asterisk || lex->next == token_kind::tk_slash || lex->next == token_kind::tk_percent) {
		char const* name = (lex->next == token_kind::tk_asterisk) ? "multiply" : 
			(lex->next == token_kind::tk_slash) ? "divide" : "remainder";
		lex->advance();
		parse_prefix(block);
		write_operation(block, name, 2);
	}
}

void parser::parse_sum(script_engine::block* block) {
	parse_product(block);
	while (lex->next == token_kind::tk_tilde || lex->next == token_kind::tk_plus || lex->next == token_kind::tk_minus) {
		char const* name = (lex->next == token_kind::tk_tilde) ? "concatenate" : 
			(lex->next == token_kind::tk_plus) ? "add" : "subtract";
		lex->advance();
		parse_product(block);
		write_operation(block, name, 2);
	}
}

void parser::parse_comparison(script_engine::block* block) {
	parse_sum(block);
	switch (lex->next) {
	case token_kind::tk_assign:
	{
		std::wstring error = L"Did you mean to write \"==\"?\r\n";
		throw parser_error(error);
	}

	case token_kind::tk_e:
	case token_kind::tk_g:
	case token_kind::tk_ge:
	case token_kind::tk_l:
	case token_kind::tk_le:
	case token_kind::tk_ne:
		token_kind op = lex->next;
		lex->advance();
		parse_sum(block);
		write_operation(block, "compare", 2);
		switch (op) {
		case token_kind::tk_e:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_e));
			break;
		case token_kind::tk_g:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_g));
			break;
		case token_kind::tk_ge:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_ge));
			break;
		case token_kind::tk_l:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_l));
			break;
		case token_kind::tk_le:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_le));
			break;
		case token_kind::tk_ne:
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_ne));
			break;
		}
		break;
	}
}

void parser::parse_logic(script_engine::block* block) {
	parse_comparison(block);
	while (lex->next == token_kind::tk_and_then || lex->next == token_kind::tk_or_else) {
		script_engine::command_kind cmd = (lex->next == token_kind::tk_and_then) ? 
			script_engine::command_kind::pc_case_if_not : script_engine::command_kind::pc_case_if;
		lex->advance();

		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_dup));
		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_begin));
		block->codes.push_back(code(lex->line, cmd));
		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));

		parse_comparison(block);

		block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_end));
	}
}

void parser::parse_expression(script_engine::block* block) {
	parse_logic(block);
}

int parser::parse_arguments(script_engine::block* block) {
	int result = 0;
	if (lex->next == token_kind::tk_open_par) {
		lex->advance();
		while (lex->next != token_kind::tk_close_par) {
			++result;
			parse_expression(block);
			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}
		if (lex->next != token_kind::tk_close_par) {
			std::wstring error = L"\")\" is required.\r\n";
			throw parser_error(error);
		}
		lex->advance();
	}
	return result;
}

void parser::parse_statements(script_engine::block* block) {
	for (; ; ) {
		bool need_semicolon = true;

		if (lex->next == token_kind::tk_word) {
			std::string name = lex->word;

			scope_t* resScope = nullptr;
			symbol* s = search(name, &resScope);
			if (s == nullptr) {
				std::wstring error = StringUtility::FormatToWide("%s is not defined.\r\n", lex->word.c_str());
				throw parser_error(error);
			}

			lex->advance();
			switch (lex->next) {
			case token_kind::tk_assign:
				lex->advance();
				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
				break;

			case token_kind::tk_open_bra:
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_variable_writable, s->level, s->variable));
				while (lex->next == token_kind::tk_open_bra) {
					lex->advance();
					parse_expression(block);
					if (lex->next != token_kind::tk_close_bra) {
						std::wstring error = L"\"]\" is required.\r\n";
						throw parser_error(error);
					}
					lex->advance();
					write_operation(block, "index!", 2);
				}
				if (lex->next != token_kind::tk_assign) {
					std::wstring error = L"\"=\" is required.\r\n";
					throw parser_error(error);
				}
				lex->advance();
				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign_writable));
				break;

			case token_kind::tk_add_assign:
			case token_kind::tk_subtract_assign:
			case token_kind::tk_multiply_assign:
			case token_kind::tk_divide_assign:
			case token_kind::tk_remainder_assign:
			case token_kind::tk_power_assign:
			{
				char const* f;
				switch (lex->next) {
				case token_kind::tk_add_assign:
					f = "add";
					break;
				case token_kind::tk_subtract_assign:
					f = "subtract";
					break;
				case token_kind::tk_multiply_assign:
					f = "multiply";
					break;
				case token_kind::tk_divide_assign:
					f = "divide";
					break;
				case token_kind::tk_remainder_assign:
					f = "remainder";
					break;
				case token_kind::tk_power_assign:
					f = "power";
					break;
				}
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_variable, s->level, s->variable));

				parse_expression(block);
				write_operation(block, f, 2);

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
			}
			break;

			case token_kind::tk_inc:
			case token_kind::tk_dec:
			{
				char const* f = (lex->next == token_kind::tk_inc) ? "successor" : "predecessor";
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_variable, s->level, s->variable));
				write_operation(block, f, 1);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
			}
			break;
			default:
				if (s->sub == nullptr) {
					std::wstring error = L"You cannot call a variable as if it was a function or a sub.\r\n";
					throw parser_error(error);
				}

				int argc = parse_arguments(block);

				s = search_in(resScope, name, argc);

				if (s == nullptr) {
					std::wstring error = StringUtility::FormatToWide("No matching overload for %s with %d arguments was found.\r\n", 
						s->sub->name.c_str(), argc);
					throw parser_error(error);
				}

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_call, s->sub, argc));
			}
		}
		else if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) {
			lex->advance();

			if (lex->next != token_kind::tk_word) {
				std::wstring error = L"Variable name is required.\r\n";
				throw parser_error(error);
			}

			symbol* s = search(lex->word);

			lex->advance();
			if (lex->next == token_kind::tk_assign) {
				lex->advance();
				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
			}
		}
		else if (lex->next == token_kind::tk_LOCAL) {
			lex->advance();
			parse_inline_block(block, script_engine::block_kind::bk_normal);
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_LOOP) {
			lex->advance();
			if (lex->next == token_kind::tk_open_par) {
				parse_parentheses(block);
				int ip = block->codes.size();
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_count));
				parse_inline_block(block, script_engine::block_kind::bk_loop);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_continue_marker));
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_back, ip));
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
			}
			else {
				int ip = block->codes.size();
				parse_inline_block(block, script_engine::block_kind::bk_loop);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_continue_marker));
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_back, ip));
			}
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_TIMES) {
			lex->advance();
			parse_parentheses(block);
			int ip = block->codes.size();
			if (lex->next == token_kind::tk_LOOP) {
				lex->advance();
			}
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_count));
			parse_inline_block(block, script_engine::block_kind::bk_loop);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_continue_marker));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_back, ip));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_WHILE) {
			lex->advance();
			int ip = block->codes.size();
			parse_parentheses(block);
			if (lex->next == token_kind::tk_LOOP) {
				lex->advance();
			}
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_if));
			parse_inline_block(block, script_engine::block_kind::bk_loop);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_continue_marker));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_back, ip));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_ASCENT || lex->next == token_kind::tk_DESCENT) {
			bool back = lex->next == token_kind::tk_DESCENT;
			lex->advance();

			if (lex->next != token_kind::tk_open_par) {
				std::wstring error = L"\"(\" is required.\r\n";
				throw parser_error(error);
			}
			lex->advance();

			if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) {
				lex->advance();
			}

			if (lex->next != token_kind::tk_word) {
				std::wstring error = L"Variable name is required.\r\n";
				throw parser_error(error);
			}

			std::string s = lex->word;

			lex->advance();

			if (lex->next != token_kind::tk_IN) {
				std::wstring error = L"\"in\" is required.\r\n";
				throw parser_error(error);
			}
			lex->advance();

			parse_expression(block);

			if (lex->next != token_kind::tk_range) {
				std::wstring error = L"\"..\" is required.\r\n";
				throw parser_error(error);
			}
			lex->advance();

			parse_expression(block);

			if (lex->next != token_kind::tk_close_par) {
				std::wstring error = L"\")\" is required.\r\n";
				throw parser_error(error);
			}
			lex->advance();

			if (lex->next == token_kind::tk_LOOP) {
				lex->advance();
			}

			if (!back) {
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_swap));
			}

			int ip = block->codes.size();

			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_dup2));
			write_operation(block, "compare", 2);

			block->codes.push_back(code(lex->line, back ? script_engine::command_kind::pc_loop_descent : 
				script_engine::command_kind::pc_loop_ascent));

			if (back) {
				write_operation(block, "predecessor", 1);
			}

			script_engine::block* b = engine->new_block(block->level + 1, script_engine::block_kind::bk_loop);
			std::vector < std::string > counter;
			counter.push_back(s);
			parse_block(b, &counter, false);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_dup));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_call, b, 1));

			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_continue_marker));

			if (!back) {
				write_operation(block, "successor", 1);
			}

			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_back, ip));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_IF) {
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_begin));

			parse_parentheses(block);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_if_not));
			parse_inline_block(block, script_engine::block_kind::bk_normal);
			while (lex->next == token_kind::tk_ELSE) {
				lex->advance();
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_next));
				if (lex->next == token_kind::tk_IF) {
					lex->advance();
					parse_parentheses(block);
					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_if_not));
					parse_inline_block(block, script_engine::block_kind::bk_normal);
				}
				else {
					parse_inline_block(block, script_engine::block_kind::bk_normal);
					break;
				}
			}

			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_ALTERNATIVE) {
			lex->advance();
			parse_parentheses(block);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_begin));
			while (lex->next == token_kind::tk_CASE) {
				lex->advance();

				if (lex->next != token_kind::tk_open_par) {
					std::wstring error = L"\"(\" is required.\r\n";
					throw parser_error(error);
				}
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_begin));
				do {
					lex->advance();

					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_dup));
					parse_expression(block);
					write_operation(block, "compare", 2);
					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_compare_e));
					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_dup));
					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_if));
					block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));

				} while (lex->next == token_kind::tk_comma);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_push_value, 
					value(engine->get_boolean_type(), false)));
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_end));
				if (lex->next != token_kind::tk_close_par) {
					std::wstring error = L"\")\" is required.\r\n";
					throw parser_error(error);
				}
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_if_not));
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
				parse_inline_block(block, script_engine::block_kind::bk_normal);
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_next));
			}
			if (lex->next == token_kind::tk_OTHERS) {
				lex->advance();
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
				parse_inline_block(block, script_engine::block_kind::bk_normal);
			}
			else {
				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_pop));
			}
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_BREAK) {
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_break_loop));
		}
		else if (lex->next == token_kind::tk_CONTINUE) {
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_loop_continue));
		}
		else if (lex->next == token_kind::tk_RETURN) {
			lex->advance();
			switch (lex->next) {
			case token_kind::tk_end:
			case token_kind::tk_invalid:
			case token_kind::tk_semicolon:
			case token_kind::tk_close_cur:
				break;
			default:
				parse_expression(block);
				symbol* s = search_result();
				if (s == nullptr) {
					std::wstring error = L"Only functions may return values.\r\n";
					throw parser_error(error);
				}

				block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
			}
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_break_routine));
		}
		else if (lex->next == token_kind::tk_YIELD) {
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_yield));
		}
		else if (lex->next == token_kind::tk_WAIT) {
			lex->advance();
			parse_parentheses(block);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_wait));
		}
		else if (lex->next == token_kind::tk_at || lex->next == token_kind::tk_SUB || 
			lex->next == token_kind::tk_FUNCTION || lex->next == token_kind::tk_TASK) 
		{
			bool is_event = lex->next == token_kind::tk_at;

			lex->advance();
			if (lex->next != token_kind::tk_word) {
				std::wstring error = L"Function name is necessary.\r\n";
				throw parser_error(error);
			}

			std::string funcName = lex->word;
			scope_t* pScope = nullptr;

			symbol* s = search(funcName, &pScope);

			if (is_event) {
				if (s->sub->level > 1) {
					std::wstring error = L"A \'@\' block cannot exist here.\r\n";
					throw parser_error(error);
				}
				events[funcName] = s->sub;
			}

			lex->advance();

			std::vector<std::string> args;
			if (s->sub->kind != script_engine::block_kind::bk_sub) {
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					while (lex->next == token_kind::tk_word || lex->next == token_kind::tk_LET || 
						lex->next == token_kind::tk_REAL) 
					{
						if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) {
							lex->advance();
							if (lex->next != token_kind::tk_word) {
								std::wstring error = L"Parameter name is required.\r\n";
								throw parser_error(error);
							}
						}
						args.push_back(lex->word);
						lex->advance();
						if (lex->next != token_kind::tk_comma)
							break;
						lex->advance();
					}
					if (lex->next != token_kind::tk_close_par) {
						std::wstring error = L"\")\" is required.\r\n";
						throw parser_error(error);
					}
					lex->advance();
				}
			}
			else {
				//互換性のため空の括弧だけ許す
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					if (lex->next != token_kind::tk_close_par) {
						std::wstring error = L"Only an empty parameter list is allowed here.\r\n";
						throw parser_error(error);
					}
					lex->advance();
				}
			}

			s = search_in(pScope, funcName, args.size());

			parse_block(s->sub, &args, s->sub->kind == script_engine::block_kind::bk_function);
			need_semicolon = false;
		}

		//セミコロンが無いと継続しない
		if (need_semicolon && lex->next != token_kind::tk_semicolon)
			break;

		if (lex->next == token_kind::tk_semicolon)
			lex->advance();
	}
}

void parser::parse_inline_block(script_engine::block* block, script_engine::block_kind kind) {
	script_engine::block* b = engine->new_block(block->level + 1, kind);
	parse_block(b, nullptr, false);
	block->codes.push_back(code(lex->line, script_engine::command_kind::pc_call, b, 0));
}

void parser::parse_block(script_engine::block* block, std::vector<std::string> const* args, bool adding_result) {
	if (lex->next != token_kind::tk_open_cur) {
		std::wstring error = L"\"{\" is required.\r\n";
		throw parser_error(error);
	}
	lex->advance();

	frame.push_back(scope_t(block->kind));

	scan_current_scope(block->level, args, adding_result);

	if (args != nullptr) {
		for (size_t i = 0; i < args->size(); ++i) {
			symbol* s = search((*args)[i]);
			block->codes.push_back(code(lex->line, script_engine::command_kind::pc_assign, s->level, s->variable));
		}
	}
	parse_statements(block);

	frame.pop_back();

	if (lex->next != token_kind::tk_close_cur) {
		std::wstring error = L"\"}\" is required.\r\n";
		throw parser_error(error);
	}
	lex->advance();
}

/* script_type_manager */

script_type_manager::script_type_manager() {
	real_type = &*types.insert(types.end(), type_data(type_data::type_kind::tk_real));
	char_type = &*types.insert(types.end(), type_data(type_data::type_kind::tk_char));
	boolean_type = &*types.insert(types.end(), type_data(type_data::type_kind::tk_boolean));
	string_type = &*types.insert(types.end(), type_data(type_data::type_kind::tk_array, char_type));
}

type_data* script_type_manager::get_array_type(type_data* element) {
	for (std::list<type_data>::iterator i = types.begin(); i != types.end(); ++i) {
		if (i->get_kind() == type_data::type_kind::tk_array && i->get_element() == element) {
			return &*i;
		}
	}
	return &*types.insert(types.end(), type_data(type_data::type_kind::tk_array, element));
}

/* script_engine */

script_engine::script_engine(script_type_manager* a_type_manager, std::string const& source, int funcc, function const* funcv) :
	type_manager(a_type_manager) {
	main_block = new_block(0, block_kind::bk_normal);

	const char* end = &source[0] + source.size();
	scanner s(source.c_str(), end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::script_engine(script_type_manager* a_type_manager, std::vector<char> const& source, int funcc, function const* funcv) :
	type_manager(a_type_manager) {
	main_block = new_block(0, block_kind::bk_normal);

	if (false) {
		wchar_t* pStart = (wchar_t*)&source[0];
		wchar_t* pEnd = (wchar_t*)(&source[0] + std::min(source.size(), 64U));
		std::wstring str = std::wstring(pStart, pEnd);
		//Logger::WriteTop(str);
	}
	const char* end = &source[0] + source.size();
	scanner s(&source[0], end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::~script_engine() {
	blocks.clear();
}

/* script_machine */

script_machine::script_machine(script_engine* the_engine) {
	assert(!the_engine->get_error());
	engine = the_engine;

	first_using_environment = nullptr;
	last_using_environment = nullptr;
	first_garbage_environment = nullptr;
	last_garbage_environment = nullptr;

	error = false;
	bTerminate = false;
}

script_machine::~script_machine() {
	while (first_using_environment != nullptr) {
		environment* object = first_using_environment;
		first_using_environment = first_using_environment->succ;
		delete object;
	}

	while (first_garbage_environment != nullptr) {
		environment* object = first_garbage_environment;
		first_garbage_environment = first_garbage_environment->succ;
		delete object;
	}
}

script_machine::environment* script_machine::new_environment(environment* parent, script_engine::block* b) {
	environment* result = nullptr;

	if (first_garbage_environment != nullptr) {
		//ごみ回収
		result = first_garbage_environment;
		first_garbage_environment = result->succ;
		*((result->succ != nullptr) ? &result->succ->pred : &last_garbage_environment) = result->pred;
	}

	if (result == nullptr) {
		result = new environment;
	}

	result->parent = parent;
	result->ref_count = 1;
	result->sub = b;
	result->ip = 0;
	result->variables.clear();
	result->stack.clear();
	result->has_result = false;
	result->waitCount = 0;

	//使用中リストへの追加
	result->pred = last_using_environment;
	result->succ = nullptr;
	*((result->pred != nullptr) ? &result->pred->succ : &first_using_environment) = result;
	last_using_environment = result;

	return result;
}

void script_machine::dispose_environment(environment* object) {
	assert(object->ref_count == 0);

	//使用中リストからの削除
	*((object->pred != nullptr) ? &object->pred->succ : &first_using_environment) = object->succ;
	*((object->succ != nullptr) ? &object->succ->pred : &last_using_environment) = object->pred;

	//ごみリストへの追加
	object->pred = last_garbage_environment;
	object->succ = nullptr;
	*((object->pred != nullptr) ? &object->pred->succ : &first_garbage_environment) = object;
	last_garbage_environment = object;
}

void script_machine::run() {
	if (bTerminate) return;

	assert(!error);
	if (first_using_environment == nullptr) {
		error_line = -1;
		threads.clear();
		threads.insert(threads.begin(), new_environment(nullptr, engine->main_block));
		current_thread_index = threads.begin();
		finished = false;
		stopped = false;
		resuming = false;

		while (!finished && !bTerminate) {
			advance();
		}
	}
}

void script_machine::resume() {
	if (bTerminate)return;

	assert(!error);
	assert(stopped);
	stopped = false;
	finished = false;
	resuming = true;
	while (!finished && !bTerminate) {
		advance();
	}
}

void script_machine::call(std::string event_name) {
	if (bTerminate)return;

	assert(!error);
	assert(!stopped);
	if (engine->events.find(event_name) != engine->events.end()) {
		run();	//念のため

		auto env = threads.begin();

		script_engine::block* event = engine->events[event_name];
		++((*env)->ref_count);
		*env = new_environment(*env, event);

		finished = false;
		environment* epp = (*env)->parent->parent;
		call_start_parent_environment_list.push_back(epp);
		while (!finished && !bTerminate) {
			advance();
		}
		call_start_parent_environment_list.pop_back();
		finished = false;
	}
}

bool script_machine::has_event(std::string event_name) {
	assert(!error);
	return engine->events.find(event_name) != engine->events.end();
}

int script_machine::get_current_line() {
	environment* current = *current_thread_index;
	script_engine::code* c = &(current->sub->codes[current->ip]);
	return c->line;
}

void script_machine::advance() {
	//assert(current_thread_index < threads.end());
	environment* current = *current_thread_index;

	if (current->waitCount > 0) {
		yield();
		--current->waitCount;
		return;
	}

	if (current->ip >= current->sub->codes.size()) {
		environment* removing = current;
		current = current->parent;

		bool bFinish = false;
		if (current == nullptr)
			bFinish = true;
		else {
			if (call_start_parent_environment_list.size() > 1) {
				environment* env = *call_start_parent_environment_list.rbegin();
				if (current == env)
					bFinish = true;
			}
		}

		if (bFinish) {
			finished = true;
		}
		else {
			*current_thread_index = current;

			if (removing->has_result) {
				assert(current != nullptr && removing->variables.size() > 0);
				current->stack.push_back(removing->variables[0]);
			}
			else if (removing->sub->kind == script_engine::block_kind::bk_microthread) {
				current_thread_index = threads.erase(current_thread_index);
				yield();
			}

/*
#ifndef NDEBUG
			assert(removing->stack.length == 0);
			if (removing->stack.length > 0) {
				for (int i = 0; i < removing->stack.length; ++i) {
					removing->stack.pop_back();
				}
			}
#endif
*/
			for (; ; ) {
				--(removing->ref_count);
				if (removing->ref_count > 0)
					break;
				environment* next = removing->parent;
				dispose_environment(removing);
				removing = next;
			}
		}
	}
	else {
		script_engine::code* c = &(current->sub->codes[current->ip]);
		error_line = c->line;
		++(current->ip);

		switch (c->command) {
		case script_engine::command_kind::pc_assign:
		{
			stack_t& stack = current->stack;
			assert(stack.size() > 0);

			for (environment* i = current; i != nullptr; i = i->parent) {
				if (i->sub->level == c->level) {
					variables_t& vars = i->variables;

					if (vars.size() <= c->variable)
						vars.resize(c->variable + 4);

					value* dest = &(vars[c->variable]);
					value* src = &stack.back();

					if (dest->has_data() && dest->get_type() != src->get_type()
						&& !(dest->get_type()->get_kind() == type_data::type_kind::tk_array
							&& src->get_type()->get_kind() == type_data::type_kind::tk_array
							&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
						std::wstring error = L"A variable cannot change its value type.\r\n";
						raise_error(error);
					}

					//dest->overwrite(*src);
					*dest = *src;
					stack.pop_back();

					break;
				}
			}
		}
		break;

		case script_engine::command_kind::pc_assign_writable:
		{
			stack_t& stack = current->stack;
			assert(stack.size() >= 2);

			value* dest = &stack[stack.size() - 2];
			value* src = &stack[stack.size() - 1];

			if (dest->has_data() && dest->get_type() != src->get_type()
				&& !(dest->get_type()->get_kind() == type_data::type_kind::tk_array && src->get_type()->get_kind() == type_data::type_kind::tk_array
					&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
				std::wstring error = L"A variable cannot change its value type.\r\n";
				raise_error(error);
			}
			else {
				dest->overwrite(*src);
				stack.pop_back();
				stack.pop_back();
				//stack->length -= 2;
			}
		}
		break;

		case script_engine::command_kind::pc_continue_marker:	//Dummy token for pc_loop_continue
			break;
		case script_engine::command_kind::pc_loop_continue:
		case script_engine::command_kind::pc_break_loop:
		case script_engine::command_kind::pc_break_routine:
			for (environment* i = current; i != nullptr; i = i->parent) {
				i->ip = i->sub->codes.size();

				if (c->command == script_engine::command_kind::pc_break_loop || c->command == script_engine::command_kind::pc_loop_continue) {
					bool exit = false;
					switch (i->sub->kind) {
					case script_engine::block_kind::bk_loop:
					{
						script_engine::command_kind targetCommand = script_engine::command_kind::pc_loop_back;
						if (c->command == script_engine::command_kind::pc_loop_continue) 
							targetCommand = script_engine::command_kind::pc_continue_marker;

						environment* e = i->parent;
						assert(e != nullptr);
						do
							++(e->ip);
						while (e->sub->codes[e->ip - 1].command != targetCommand);
					}
					case script_engine::block_kind::bk_microthread:	//Prevents catastrophes.
					case script_engine::block_kind::bk_function:
						exit = true;
					default:
						break;
					}
					if (exit) break;
				}
				else {
					if (i->sub->kind == script_engine::block_kind::bk_sub || i->sub->kind == script_engine::block_kind::bk_function
						|| i->sub->kind == script_engine::block_kind::bk_microthread)
						break;
					else if (i->sub->kind == script_engine::block_kind::bk_loop)
						i->parent->stack.clear();
				}
			}
			break;
		case script_engine::command_kind::pc_call:
		case script_engine::command_kind::pc_call_and_push_result:
		{
			stack_t& current_stack = current->stack;
			//assert(current_stack.size() >= c->arguments);

			if (c->sub->func != nullptr) {
				//ネイティブ呼び出し
				
				size_t sizePrev = current_stack.size();

				value* argv = current_stack.data() + (sizePrev - c->arguments);

				value ret = c->sub->func(this, c->arguments, argv);

				if (stopped) {
					--(current->ip);
				}
				else {
					resuming = false;
					//詰まれた引数を削除
					for (size_t i = 0; i < c->arguments; ++i) {
						if (current_stack.size() == 0) break;
						current_stack.pop_back();
					}
					//current_stack->length -= c->arguments;
					//戻り値
					if (c->command == script_engine::command_kind::pc_call_and_push_result)
						current_stack.push_back(value(ret));
				}
			}
			else if (c->sub->kind == script_engine::block_kind::bk_microthread) {
				//マイクロスレッド起動
				++(current->ref_count);

				environment* e = new_environment(current, c->sub);
				threads.insert(++current_thread_index, e);
				--current_thread_index;

				//引数の積み替え
				for (size_t i = 0; i < c->arguments; ++i) {
					if (current_stack.size() == 0) break;
					value v = std::move(current_stack.back());
					e->stack.push_back(v);
					current_stack.pop_back();
				}
			}
			else {
				//スクリプト間の呼び出し
				++(current->ref_count);

				environment* e = new_environment(current, c->sub);
				e->has_result = c->command == script_engine::command_kind::pc_call_and_push_result;
				*current_thread_index = e;

				//引数の積み替え
				for (size_t i = 0; i < c->arguments; ++i) {
					if (current_stack.size() == 0) break;
					value v = std::move(current_stack.back());
					e->stack.push_back(v);
					current_stack.pop_back();
				}
			}
		}
		break;

		case script_engine::command_kind::pc_case_begin:
		case script_engine::command_kind::pc_case_end:
			break;

		case script_engine::command_kind::pc_case_if:
		case script_engine::command_kind::pc_case_if_not:
		case script_engine::command_kind::pc_case_next:
		{
			bool exit = true;
			if (c->command != script_engine::command_kind::pc_case_next) {
				stack_t& current_stack = current->stack;
				exit = current_stack.back().as_boolean();
				if (c->command == script_engine::command_kind::pc_case_if_not)
					exit = !exit;
				current_stack.pop_back();
			}
			if (exit) {
				int nested = 0;
				for (; ; ) {
					switch (current->sub->codes[current->ip].command) {
					case script_engine::command_kind::pc_case_begin:
						++nested;
						break;
					case script_engine::command_kind::pc_case_end:
						--nested;
						if (nested < 0)
							goto next;
						break;
					case script_engine::command_kind::pc_case_next:
						if (nested == 0 && c->command != script_engine::command_kind::pc_case_next) {
							++(current->ip);
							goto next;
						}
						break;
					}
					++(current->ip);
				}
next:
#ifdef _MSC_VER
				;
#endif
			}
		}
		break;

		case script_engine::command_kind::pc_compare_e:
		case script_engine::command_kind::pc_compare_g:
		case script_engine::command_kind::pc_compare_ge:
		case script_engine::command_kind::pc_compare_l:
		case script_engine::command_kind::pc_compare_le:
		case script_engine::command_kind::pc_compare_ne:
		{
			stack_t& stack = current->stack;
			value& t = stack.back();
			float r = t.as_real();
			bool b = false;
			switch (c->command) {
			case script_engine::command_kind::pc_compare_e:
				b = r == 0;
				break;
			case script_engine::command_kind::pc_compare_g:
				b = r > 0;
				break;
			case script_engine::command_kind::pc_compare_ge:
				b = r >= 0;
				break;
			case script_engine::command_kind::pc_compare_l:
				b = r < 0;
				break;
			case script_engine::command_kind::pc_compare_le:
				b = r <= 0;
				break;
			case script_engine::command_kind::pc_compare_ne:
				b = r != 0;
				break;
			}
			t.set(engine->get_boolean_type(), b);
		}
		break;

		case script_engine::command_kind::pc_dup:
		{
			stack_t& stack = current->stack;
			assert(stack.size() > 0);
			value v = stack.back();
			stack.push_back(value(v));
		}
		break;

		case script_engine::command_kind::pc_dup2:
		{
			stack_t& stack = current->stack;
			size_t len = stack.size();
			value v1 = stack[len - 2];
			value v2 = stack[len - 1];
			stack.push_back(value(v1));
			stack.push_back(value(v2));
		}
		break;

		case script_engine::command_kind::pc_loop_back:
			current->ip = c->ip;
			break;

		case script_engine::command_kind::pc_loop_ascent:
		{
			stack_t& stack = current->stack;
			value* i = &stack.back();
			if (i->as_real() <= 0) {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != script_engine::command_kind::pc_loop_back);
			}
			current->stack.pop_back();
		}
		break;

		case script_engine::command_kind::pc_loop_descent:
		{
			stack_t& stack = current->stack;
			value* i = &stack.back();
			if (i->as_real() >= 0) {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != script_engine::command_kind::pc_loop_back);
			}
			current->stack.pop_back();
		}
		break;

		case script_engine::command_kind::pc_loop_count:
		{
			stack_t& stack = current->stack;
			value* i = &stack.back();
			assert(i->get_type()->get_kind() == type_data::type_kind::tk_real);
			float r = i->as_real();
			if (r > 0)
				i->set(engine->get_real_type(), r - 1);
			else {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != script_engine::command_kind::pc_loop_back);
			}
		}
		break;

		case script_engine::command_kind::pc_loop_if:
		{
			stack_t& stack = current->stack;
			bool c = stack.back().as_boolean();
			current->stack.pop_back();
			if (!c) {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != script_engine::command_kind::pc_loop_back);
			}
		}
		break;

		case script_engine::command_kind::pc_pop:
			assert(current->stack.size() > 0);
			current->stack.pop_back();
			break;

		case script_engine::command_kind::pc_push_value:
			current->stack.push_back(value(c->data));
			break;

		case script_engine::command_kind::pc_push_variable:
		case script_engine::command_kind::pc_push_variable_writable:
			for (environment* i = current; i != nullptr; i = i->parent) {
				if (i->sub->level == c->level) {
					bool callingIsNull = i->sub->func == isNull;

					variables_t& vars = i->variables;

					if (!callingIsNull && (vars.size() <= c->variable || !(vars[c->variable].has_data()))) {
						std::wstring error = L"Variable hasn't been initialized.\r\n";
						raise_error(error);
					}
					else {
						value* var = &vars[c->variable];
						if (c->command == script_engine::command_kind::pc_push_variable_writable)
							var->unique();
						current->stack.push_back(value(*var));
					}
					break;
				}
			}
			break;

		case script_engine::command_kind::pc_swap:
		{
			size_t len = current->stack.size();
			assert(len >= 2);
			std::swap(current->stack[len - 1], current->stack[len - 2]);
		}
		break;

		
		case script_engine::command_kind::pc_wait:
		{
			stack_t& stack = current->stack;
			value* t = &(stack.back());
			current->waitCount = (int)(t->as_real() + 0.01) - 1;
			if (current->waitCount < 0) break;
		}
		//Fallthrough
		case script_engine::command_kind::pc_yield:
			yield();
			break;

		default:
			assert(false);
		}
	}
}
