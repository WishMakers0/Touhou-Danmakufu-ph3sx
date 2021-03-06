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
		type_data* elem = data->type->get_element();
		if (elem != nullptr && elem->get_kind() == type_data::type_kind::tk_char) {
			std::wstring result = L"";
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

size_t value::length_as_array() const {
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
	if (data == nullptr) return;
	if (std::addressof(data) == std::addressof(source.data)) return;

	*data = *source.data;
}

value value::new_from(value const& source) {
	value res = source;
	res.unique();
	return res;
}

//--------------------------------------

/* parser_error */

class parser_error : public gstd::wexception {
public:
	parser_error(std::wstring const& the_message) : gstd::wexception(the_message) {}
	parser_error(std::string const& the_message) : gstd::wexception(the_message) {}
private:

};

/* lexical analyzer */

enum class token_kind : uint8_t {
	tk_end, tk_invalid, 
	tk_word, tk_real, tk_char, tk_string, 
	tk_open_par, tk_close_par, tk_open_bra, tk_close_bra, tk_open_cur, tk_close_cur, 
	tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_colon, tk_tilde, tk_assign, tk_plus, tk_minus, 
	tk_asterisk, tk_slash, tk_percent, tk_caret, tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, tk_exclamation, tk_ampersand,
	tk_and_then, tk_vertical, tk_or_else, tk_at, 
	tk_inc, tk_dec, 
	tk_add_assign, tk_subtract_assign, tk_multiply_assign, tk_divide_assign, tk_remainder_assign, tk_power_assign, 
	tk_range, tk_ALTERNATIVE, tk_ASCENT, tk_BREAK, tk_CASE, tk_DESCENT, tk_ELSE,
	tk_FUNCTION, tk_IF, tk_FOR, tk_EACH, tk_IN, tk_LET, tk_LOCAL, tk_LOOP, tk_CONTINUE, tk_OTHERS, 
	tk_REAL, tk_RETURN,
	tk_SUB, tk_TASK, tk_TIMES, tk_WHILE, tk_YIELD, tk_WAIT,
	tk_TRUE, tk_FALSE,
	tk_const,
	/*tk_bitwise_not, tk_bitwise_and, tk_bitwise_or, tk_bitwise_xor, tk_bitwise_left, tk_bitwise_right,*/
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

	void copy_state(const scanner& src) {
		encoding = src.encoding;
		current = src.current;
		endPoint = src.endPoint;
		next = src.next;
		word = src.word;
		real_value = src.real_value;
		char_value = src.char_value;
		string_value = src.string_value;
		line = src.line;
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
	case L':':
		next = token_kind::tk_colon;
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
			throw parser_error("Invalid period(.) placement.\r\n");
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
				if (ch == L'\n') ++line;	//For multiple-lined strings
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
				if (ch == L'\n') ++line;	//For multiple-lined strings
			}
			ch = next_char();
			string_value = StringUtility::ConvertMultiToWide(s);
		}

		if (q == L'\'') {
			if (string_value.size() == 1)
				char_value = string_value[0];
			else
				throw parser_error("A value of type char may only be one character long.");
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
			throw parser_error("Invalid character.\r\n");
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
			static_assert(sizeof(size_t) == 4U, "The script lexer only supports size_t of 4 bytes.");

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
			case 0xacf38390:	//for
				next = token_kind::tk_FOR;
				break;
			case 0x147aa128:	//each
				next = token_kind::tk_EACH;
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
			case 0x4db211e5:	//true
				next = token_kind::tk_TRUE;
				break;
			case 0x0b069958:	//false
				next = token_kind::tk_FALSE;
				break;
			case 0x664fd1d4:	//const
				next = token_kind::tk_const;
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
value remainder_(script_machine* machine, int argc, value const* argv) {
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
	//result.unique();
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
	return const_cast<value&>(result);
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

bool append_check(script_machine* machine, size_t arg0_size, type_data* arg0_type, type_data* arg1_type) {
	if (arg0_type->get_kind() != type_data::type_kind::tk_array) {
		machine->raise_error(L"This value type does not support the array append operation.\r\n");
		return false;
	}
	if (arg0_size > 0U && arg0_type->get_element() != arg1_type) {
		machine->raise_error(L"Variable type mismatch.\r\n");
		return false;
	}
	return true;
}
value append(script_machine* machine, int argc, value const* argv) {
	assert(argc == 2);

	append_check(machine, argv[0].length_as_array(), argv[0].get_type(), argv[1].get_type());

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

value script_debugBreak(script_machine* machine, int argc, value const* argv) {
	DebugBreak();
	return value();
}

value conv_as_real(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_real_type(), argv[0].as_real());
}
value conv_as_bool(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_boolean_type(), argv[0].as_boolean());
}
value conv_as_char(script_machine* machine, int argc, value const* argv) {
	return value(machine->get_engine()->get_char_type(), argv[0].as_char());
}

function const operations[] = {
	//{ "true", true_, 0 },
	//{ "false", false_, 0 },
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
	{ "remainder", remainder_, 2 },
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
	{ "as_real", conv_as_real, 1 },
	{ "as_bool", conv_as_bool, 1 },
	{ "as_char", conv_as_char, 1 },
	{ "__DEBUG_BREAK", script_debugBreak, 0 },
};

/* parser */

class gstd::parser {
public:
	using command_kind = script_engine::command_kind;
	using block_kind = script_engine::block_kind;

	struct symbol {
		int level;
		script_engine::block* sub;
		int variable;
		bool can_overload = true;
		bool can_modify = true;		//Applies to the scripter, not the engine
	};

	struct scope_t : public std::multimap<std::string, symbol> {
		block_kind kind;

		scope_t(block_kind the_kind) : kind(the_kind) {}

		void singular_insert(std::string name, symbol& s, int argc = 0);
	};

	std::vector<scope_t> frame;
	scanner* _lex;
	script_engine* engine;
	bool error;
	std::wstring error_message;
	int error_line;
	std::map<std::string, script_engine::block*> events;

	parser(script_engine* e, scanner* s, int funcc, function const* funcv);

	virtual ~parser() {}

	void parse_parentheses(script_engine::block* block, scanner* scanner);
	void parse_clause(script_engine::block* block, scanner* scanner);
	void parse_prefix(script_engine::block* block, scanner* scanner);
	void parse_suffix(script_engine::block* block, scanner* scanner);
	void parse_product(script_engine::block* block, scanner* scanner);
	void parse_sum(script_engine::block* block, scanner* scanner);
	void parse_comparison(script_engine::block* block, scanner* scanner);
	void parse_logic(script_engine::block* block, scanner* scanner);
	void parse_expression(script_engine::block* block, scanner* scanner);
	int parse_arguments(script_engine::block* block, scanner* scanner);
	void parse_statements(script_engine::block* block, scanner* scanner, 
		token_kind statement_terminator = token_kind::tk_semicolon, bool single_parse = false);
	void parse_inline_block(script_engine::block* block, scanner* scanner, block_kind kind);
	void parse_block(script_engine::block* block, scanner* scanner, std::vector<std::string> const* args, bool adding_result);

	void parse_parentheses(script_engine::block* block) {
		parse_parentheses(block, _lex);
	}
	void parse_clause(script_engine::block* block) {
		parse_clause(block, _lex);
	}
	void parse_prefix(script_engine::block* block) {
		parse_prefix(block, _lex);
	}
	void parse_suffix(script_engine::block* block) {
		parse_suffix(block, _lex);
	}
	void parse_product(script_engine::block* block) {
		parse_product(block, _lex);
	}
	void parse_sum(script_engine::block* block) {
		parse_sum(block, _lex);
	}
	void parse_comparison(script_engine::block* block) {
		parse_comparison(block, _lex);
	}
	void parse_logic(script_engine::block* block) {
		parse_logic(block, _lex);
	}
	void parse_expression(script_engine::block* block) {
		parse_expression(block, _lex);
	}
	int parse_arguments(script_engine::block* block) {
		return parse_arguments(block, _lex);
	}
	void parse_statements(script_engine::block* block, bool single_parse = false) {
		parse_statements(block, _lex, token_kind::tk_semicolon, single_parse);
	}
	void parse_inline_block(script_engine::block* block, block_kind kind) {
		parse_inline_block(block, _lex, kind);
	}
	void parse_block(script_engine::block* block, std::vector<std::string> const* args, bool adding_result) {
		parse_block(block, _lex, args, adding_result);
	}
private:
	void register_function(function const& func);
	symbol* search(std::string const& name, scope_t** ptrScope = nullptr);
	symbol* search(std::string const& name, int argc, scope_t** ptrScope = nullptr);
	symbol* search_in(scope_t* scope, std::string const& name);
	symbol* search_in(scope_t* scope, std::string const& name, int argc);
	symbol* search_result();
	int scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result, int initVar = 0);
	void write_operation(script_engine::block* block, scanner* lex, char const* name, int clauses);
	void write_operation(script_engine::block* block, scanner* lex, const symbol* s, int clauses);

	inline static void parser_assert(bool expr, std::wstring error) {
		if (!expr) throw parser_error(error);
	}
	inline static void parser_assert(bool expr, std::string error) {
		if (!expr) throw parser_error(error);
	}

	typedef script_engine::code code;
};

void parser::scope_t::singular_insert(std::string name, symbol& s, int argc) {
	bool exists = this->find(name) != this->end();
	auto itr = this->equal_range(name);

	if (exists) {
		symbol* sPrev = &(itr.first->second);

		if (!sPrev->can_overload) {
			std::string error = StringUtility::Format("Default functions cannot be overloaded. (%s)",
				sPrev->sub->name.c_str());
			parser_assert(false, error);
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

parser::parser(script_engine* e, scanner* s, int funcc, function const* funcv) : engine(e), _lex(s), frame(), error(false) {
	frame.push_back(scope_t(block_kind::bk_normal));

	for (size_t i = 0; i < sizeof(operations) / sizeof(function); ++i)
		register_function(operations[i]);

	for (size_t i = 0; i < funcc; ++i)
		register_function(funcv[i]);

	try {
		int countVar = scan_current_scope(0, nullptr, false);
		engine->main_block->codes.push_back(code(_lex->line, command_kind::pc_var_alloc, countVar));
		parse_statements(engine->main_block, _lex);

		parser_assert(_lex->next == token_kind::tk_end, 
			"Unexpected end-of-file while parsing. (Did you forget a semicolon after a string?)\r\n");
	}
	catch (parser_error& e) {
		error = true;
		error_message = e.what();
		error_line = _lex->line;
	}
}

inline void parser::register_function(function const& func) {
	symbol s;
	s.level = 0;
	s.sub = engine->new_block(0, block_kind::bk_function);
	s.sub->arguments = func.arguments;
	s.sub->name = func.name;
	s.sub->func = func.func;
	s.variable = -1;
	s.can_overload = false;
	s.can_modify = false;
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
		if (itr->kind == block_kind::bk_sub || itr->kind == block_kind::bk_microthread)
			return nullptr;

		auto itrSymbol = itr->find("\x01");
		if (itrSymbol != itr->end())
			return &(itrSymbol->second);
	}
	return nullptr;
}

int parser::scan_current_scope(int level, std::vector<std::string> const* args, bool adding_result, int initVar) {
	//先読みして識別子を登録する
	scanner lex2(*_lex);

	int var = initVar;

	try {
		scope_t* current_frame = &frame[frame.size() - 1];
		int cur = 0;

		if (adding_result) {
			symbol s;
			s.level = level;
			s.sub = nullptr;
			s.variable = var;
			s.can_overload = false;
			s.can_modify = false;
			++var;
			current_frame->singular_insert("\x01", s);
		}

		if (args) {
			for (size_t i = 0; i < args->size(); ++i) {
				symbol s;
				s.level = level;
				s.sub = nullptr;
				s.variable = var;
				s.can_overload = false;
				s.can_modify = true;
				++var;
				current_frame->singular_insert((*args)[i], s);
			}
		}

		while (cur >= 0 && lex2.next != token_kind::tk_end && lex2.next != token_kind::tk_invalid) {
			bool is_const = false;

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

					block_kind kind = (type == token_kind::tk_SUB || type == token_kind::tk_at) ?
						block_kind::bk_sub : (type == token_kind::tk_FUNCTION) ?
						block_kind::bk_function : block_kind::bk_microthread;

					std::string name = lex2.word;

					lex2.advance();
					if (lex2.next == token_kind::tk_open_par) {
						lex2.advance();

						//I honestly should just throw out support for subs
						if (lex2.next != token_kind::tk_close_par && kind == block_kind::bk_sub) {
							parser_assert(false, "A parameter list in not allowed here.\r\n");
						}
						else {
							while (lex2.next == token_kind::tk_word || lex2.next == token_kind::tk_LET ||
								lex2.next == token_kind::tk_REAL) 
							{
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
							std::string typeSub;
							switch (dup->sub->kind) {
							case block_kind::bk_function:
								typeSub = "function";
								break;
							case block_kind::bk_microthread:
								typeSub = "task";
								break;
							case block_kind::bk_sub:
								typeSub = "sub or an \'@\' block";
								break;
							default:
								typeSub = "block";
								break;
							}

							std::string error = "A ";
							error += typeSub;
							error += " of the same name was already defined in the current scope.\r\n";
							if (dup->can_overload)
								error += "**You may overload functions and tasks by giving them different argument counts.\r\n";
							else
								error += StringUtility::Format("**\'%s\' cannot be overloaded.\r\n", name.c_str());
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
					s.can_overload = kind != block_kind::bk_sub;
					s.can_modify = false;
					current_frame->singular_insert(name, s, countArgs);
				}
			}
			break;
			case token_kind::tk_const:
				is_const = true;
			case token_kind::tk_REAL:
			case token_kind::tk_LET:
			{
				lex2.advance();
				if (cur == 0) {
#ifdef __SCRIPT_H__CHECK_NO_DUPLICATED
					parser_assert(current_frame->find(lex2.word) == current_frame->end(),
						"A variable of the same name was already declared in the current scope.\r\n");
#endif
					symbol s;
					s.level = level;
					s.sub = nullptr;
					s.variable = var;
					s.can_overload = false;
					s.can_modify = !is_const;
					++var;
					current_frame->singular_insert(lex2.word, s);

					lex2.advance();
				}
				break;
			}
			default:
				lex2.advance();
				break;
			}
		}
	}
	catch (...) {
		_lex->line = lex2.line;
		throw;
	}

	return (var - initVar);
}

void parser::write_operation(script_engine::block* block, scanner* lex, char const* name, int clauses) {
	symbol* s = search(name);
	assert(s != nullptr);
	{
		std::string error = "Invalid argument count for the default function. (expected " +
			std::to_string(clauses) + ")\r\n";
		parser_assert(s->sub->arguments == clauses, error);
	}

	block->codes.push_back(script_engine::code(lex->line, command_kind::pc_call_and_push_result, s->sub, clauses));
}
void parser::write_operation(script_engine::block* block, scanner* lex, const symbol* s, int clauses) {
	assert(s != nullptr);
	{
		std::string error = "Invalid argument count for the default function. (expected " +
			std::to_string(clauses) + ")\r\n";
		parser_assert(s->sub->arguments == clauses, error);
	}
	block->codes.push_back(script_engine::code(lex->line, command_kind::pc_call_and_push_result, s->sub, clauses));
}

void parser::parse_parentheses(script_engine::block* block, scanner* lex) {
	parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");
	lex->advance();

	parse_expression(block, lex);

	parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
	lex->advance();
}

void parser::parse_clause(script_engine::block* block, scanner* lex) {
	if (lex->next == token_kind::tk_real) {
		block->codes.push_back(code(lex->line, command_kind::pc_push_value, 
			value(engine->get_real_type(), lex->real_value)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_char) {
		block->codes.push_back(code(lex->line, command_kind::pc_push_value, 
			value(engine->get_char_type(), lex->char_value)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_TRUE) {
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_boolean_type(), true)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_FALSE) {
		block->codes.push_back(code(lex->line, command_kind::pc_push_value,
			value(engine->get_boolean_type(), false)));
		lex->advance();
	}
	else if (lex->next == token_kind::tk_string) {
		std::wstring str = lex->string_value;
		lex->advance();
		while (lex->next == token_kind::tk_string || lex->next == token_kind::tk_char) {
			str += (lex->next == token_kind::tk_string) ? lex->string_value : (std::wstring() + lex->char_value);
			lex->advance();
		}

		block->codes.push_back(code(lex->line, command_kind::pc_push_value, 
			value(engine->get_string_type(), str)));
	}
	else if (lex->next == token_kind::tk_word) {
		std::string name = lex->word;
		lex->advance();
		int argc = parse_arguments(block, lex);

		symbol* s = search(name, argc);

		if (s == nullptr) {
			std::string error;
			if (search(name))
				error = StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
					lex->word.c_str(), argc);
			else
				error = StringUtility::Format("%s is not defined.\r\n", lex->word.c_str());
			throw parser_error(error);
		}

		if (s->sub) {
			parser_assert(s->sub->kind == block_kind::bk_function, "Tasks and subs cannot return values.\r\n");
			block->codes.push_back(code(lex->line, command_kind::pc_call_and_push_result, s->sub, argc));
		}
		else {
			//変数
			block->codes.push_back(code(lex->line, command_kind::pc_push_variable, s->level, s->variable, name));
		}
	}
	else if (lex->next == token_kind::tk_open_bra) {
		lex->advance();

		size_t count_member = 0U;
		while (lex->next != token_kind::tk_close_bra) {
			parse_expression(block, lex);
			++count_member;

			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}

		if (count_member > 0U)
			block->codes.push_back(code(lex->line, command_kind::pc_construct_array, count_member));
		else
			block->codes.push_back(code(lex->line, command_kind::pc_push_value,
				value(engine->get_string_type(), std::wstring())));

		parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
		lex->advance();
	}
	else if (lex->next == token_kind::tk_open_abs) {
		lex->advance();
		parse_expression(block, lex);
		
#ifndef __SCRIPT_H__INLINE_OPERATION
		write_operation(block, lex, "absolute", 1);
#else
		block->codes.push_back(code(lex->line, command_kind::pc_inline_abs));
#endif

		parser_assert(lex->next == token_kind::tk_close_abs, "\"|)\" is required.\r\n");
		lex->advance();
	}
	else if (lex->next == token_kind::tk_open_par) {
		parse_parentheses(block, lex);
	}
	else {
		parser_assert(false, "Invalid expression.\r\n");
	}
}

void parser::parse_suffix(script_engine::block* block, scanner* lex) {
	parse_clause(block, lex);
	if (lex->next == token_kind::tk_caret) {
		lex->advance();
		parse_suffix(block, lex); //再帰
		
#ifndef __SCRIPT_H__INLINE_OPERATION
		write_operation(block, lex, "power", 2);
#else
		block->codes.push_back(code(lex->line, command_kind::pc_inline_pow));
#endif
	}
	else {
		while (lex->next == token_kind::tk_open_bra) {
			lex->advance();
			parse_expression(block, lex);

			if (lex->next == token_kind::tk_range) {
				lex->advance();
				parse_expression(block, lex);
				write_operation(block, lex, "slice", 3);
			}
			else {
				write_operation(block, lex, "index_", 2);
			}

			parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
			lex->advance();
		}
	}
}

void parser::parse_prefix(script_engine::block* block, scanner* lex) {
	if (lex->next == token_kind::tk_plus) {
		lex->advance();
		parse_prefix(block, lex);	//再帰
	}
	else if (lex->next == token_kind::tk_minus) {
		lex->advance();
		parse_prefix(block, lex);	//再帰

#ifndef __SCRIPT_H__INLINE_OPERATION
		write_operation(block, lex, "negative", 1);
#else
		block->codes.push_back(code(lex->line, command_kind::pc_inline_neg));
#endif
	}
	else if (lex->next == token_kind::tk_exclamation) {
		lex->advance();
		parse_prefix(block, lex);	//再帰

#ifndef __SCRIPT_H__INLINE_OPERATION
		write_operation(block, lex, "not", 1);
#else
		block->codes.push_back(code(lex->line, command_kind::pc_inline_not));
#endif
	}
	else {
		parse_suffix(block, lex);
	}
}

void parser::parse_product(script_engine::block* block, scanner* lex) {
	parse_prefix(block, lex);
	while (lex->next == token_kind::tk_asterisk || lex->next == token_kind::tk_slash || lex->next == token_kind::tk_percent) {
#ifndef __SCRIPT_H__INLINE_OPERATION
		char const* name = (lex->next == token_kind::tk_asterisk) ? "multiply" :
			(lex->next == token_kind::tk_slash) ? "divide" : "remainder";
		lex->advance();
		parse_prefix(block, lex);
		write_operation(block, lex, name, 2);
#else
		command_kind f = (lex->next == token_kind::tk_asterisk) ? command_kind::pc_inline_mul :
			(lex->next == token_kind::tk_slash) ? command_kind::pc_inline_div : command_kind::pc_inline_mod;
		lex->advance();
		parse_prefix(block, lex);
		block->codes.push_back(code(lex->line, f));
#endif
	}
}

void parser::parse_sum(script_engine::block* block, scanner* lex) {
	parse_product(block, lex);
	while (lex->next == token_kind::tk_tilde || lex->next == token_kind::tk_plus || lex->next == token_kind::tk_minus) {
#ifndef __SCRIPT_H__INLINE_OPERATION
		char const* name = (lex->next == token_kind::tk_tilde) ? "concatenate" : 
			(lex->next == token_kind::tk_plus) ? "add" : "subtract";
		lex->advance();
		parse_product(block, lex);
		write_operation(block, lex, name, 2);
#else
		command_kind f = (lex->next == token_kind::tk_tilde) ? command_kind::pc_inline_cat :
			(lex->next == token_kind::tk_plus) ? command_kind::pc_inline_add : command_kind::pc_inline_sub;
		lex->advance();
		parse_product(block, lex);
		block->codes.push_back(code(lex->line, f));
#endif
	}
}

void parser::parse_comparison(script_engine::block* block, scanner* lex) {
	parse_sum(block, lex);
	switch (lex->next) {
	case token_kind::tk_assign:
		parser_assert(false, "Did you intend to write \"==\"?\r\n");
		break;

	case token_kind::tk_e:
	case token_kind::tk_g:
	case token_kind::tk_ge:
	case token_kind::tk_l:
	case token_kind::tk_le:
	case token_kind::tk_ne:
		token_kind op = lex->next;
		lex->advance();
		parse_sum(block, lex);

#ifndef __SCRIPT_H__INLINE_OPERATION
		write_operation(block, lex, "compare", 2);
		switch (op) {
		case token_kind::tk_e:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_e));
			break;
		case token_kind::tk_g:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_g));
			break;
		case token_kind::tk_ge:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_ge));
			break;
		case token_kind::tk_l:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_l));
			break;
		case token_kind::tk_le:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_le));
			break;
		case token_kind::tk_ne:
			block->codes.push_back(code(lex->line, command_kind::pc_compare_ne));
			break;
		}
#else
		switch (op) {
		case token_kind::tk_e:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_e));
			break;
		case token_kind::tk_g:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_g));
			break;
		case token_kind::tk_ge:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_ge));
			break;
		case token_kind::tk_l:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_l));
			break;
		case token_kind::tk_le:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_le));
			break;
		case token_kind::tk_ne:
			block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_ne));
			break;
		}
#endif

		break;
	}
}

void parser::parse_logic(script_engine::block* block, scanner* lex) {
	parse_comparison(block, lex);
	while (lex->next == token_kind::tk_and_then || lex->next == token_kind::tk_or_else) {
		script_engine::command_kind cmd = (lex->next == token_kind::tk_and_then) ? 
			command_kind::pc_case_if_not : command_kind::pc_case_if;
		lex->advance();

		block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
		block->codes.push_back(code(lex->line, command_kind::pc_case_begin));
		block->codes.push_back(code(lex->line, cmd));
		block->codes.push_back(code(lex->line, command_kind::pc_pop));

		parse_comparison(block, lex);

		block->codes.push_back(code(lex->line, command_kind::pc_case_end));
	}
}

inline void parser::parse_expression(script_engine::block* block, scanner* lex) {
	parse_logic(block, lex);
}

int parser::parse_arguments(script_engine::block* block, scanner* lex) {
	int result = 0;
	if (lex->next == token_kind::tk_open_par) {
		lex->advance();

		while (lex->next != token_kind::tk_close_par) {
			++result;
			parse_expression(block, lex);
			if (lex->next != token_kind::tk_comma) break;
			lex->advance();
		}

		parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
		lex->advance();
	}
	return result;
}

void parser::parse_statements(script_engine::block* block, scanner* lex, token_kind statement_terminator, bool single_parse) {
	auto assert_const = [](symbol* s, std::string& name) {
		if (!s->can_modify) {
			std::string error = StringUtility::Format("\"%s\": ", name.c_str());
			if (s->sub)
				throw parser_error(error + "Functions, tasks, and subs are implicitly const\r\n"
					"and thus cannot be modified in this manner.\r\n");
			else
				throw parser_error(error + "const variables cannot be modified.\r\n");
		}
	};

	for (; ; ) {
		bool need_semicolon = true;

		if (lex->next == token_kind::tk_word) {
			std::string name = lex->word;

			scope_t* resScope = nullptr;
			symbol* s = search(name, &resScope);
			parser_assert(s, StringUtility::Format("%s is not defined.\r\n", name.c_str()));

			lex->advance();
			switch (lex->next) {
			case token_kind::tk_assign:
				assert_const(s, name);

				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
				break;
			case token_kind::tk_open_bra:
				assert_const(s, name);

				block->codes.push_back(code(lex->line, command_kind::pc_push_variable_writable, s->level, s->variable, name));
				while (lex->next == token_kind::tk_open_bra) {
					lex->advance();
					parse_expression(block, lex);
					
					parser_assert(lex->next == token_kind::tk_close_bra, "\"]\" is required.\r\n");
					lex->advance();

					write_operation(block, lex, "index!", 2);
				}

				parser_assert(lex->next == token_kind::tk_assign, "\"=\" is required.\r\n");

				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_assign_writable, 0, 0, name));
				break;

			case token_kind::tk_add_assign:
			case token_kind::tk_subtract_assign:
			case token_kind::tk_multiply_assign:
			case token_kind::tk_divide_assign:
			case token_kind::tk_remainder_assign:
			case token_kind::tk_power_assign:
			{
				assert_const(s, name);

#ifndef __SCRIPT_H__INLINE_OPERATION
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
				
				block->codes.push_back(code(lex->line, command_kind::pc_push_variable, s->level, s->variable, name));
				parse_expression(block, lex);
				write_operation(block, lex, f, 2);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
#else
				command_kind f;
				switch (lex->next) {
				case token_kind::tk_add_assign:
					f = command_kind::pc_inline_add_asi;
					break;
				case token_kind::tk_subtract_assign:
					f = command_kind::pc_inline_sub_asi;
					break;
				case token_kind::tk_multiply_assign:
					f = command_kind::pc_inline_mul_asi;
					break;
				case token_kind::tk_divide_assign:
					f = command_kind::pc_inline_div_asi;
					break;
				case token_kind::tk_remainder_assign:
					f = command_kind::pc_inline_mod_asi;
					break;
				case token_kind::tk_power_assign:
					f = command_kind::pc_inline_pow_asi;
					break;
				}
				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, f, s->level, s->variable, name));
#endif
				break;
			}
			case token_kind::tk_inc:
			case token_kind::tk_dec:
			{
				assert_const(s, name);

#ifndef __SCRIPT_H__INLINE_OPERATION
				char const* f = (lex->next == token_kind::tk_inc) ? "successor" : "predecessor";
				lex->advance();

				block->codes.push_back(code(lex->line, command_kind::pc_push_variable, s->level, s->variable, name));
				write_operation(block, lex, f, 1);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
#else
				command_kind f = (lex->next == token_kind::tk_inc) ? command_kind::pc_inline_inc : command_kind::pc_inline_dec;
				lex->advance();
				block->codes.push_back(code(lex->line, f, s->level, s->variable, name));
#endif
				break;
			}
			default:
			{
				parser_assert(s, "A variable cannot be called as if it was a function, a task, or a sub.\r\n");

				int argc = parse_arguments(block, lex);

				s = search_in(resScope, name, argc);
				parser_assert(s, StringUtility::Format("No matching overload for %s with %d arguments was found.\r\n",
					name.c_str(), argc));

				block->codes.push_back(code(lex->line, command_kind::pc_call, s->sub, argc));

				break;
			}
			}
		}
		else if (lex->next == token_kind::tk_const || lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) {
			lex->advance();
			parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

			std::string name = lex->word;

			lex->advance();
			if (lex->next == token_kind::tk_assign) {
				symbol* s = search(name);

				lex->advance();
				parse_expression(block, lex);
				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, name));
			}
		}
		//"local" can be omitted to make C-style blocks
		else if (lex->next == token_kind::tk_LOCAL || lex->next == token_kind::tk_open_cur) {
			if (lex->next == token_kind::tk_LOCAL) lex->advance();
			parse_inline_block(block, lex, block_kind::bk_normal);

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
			if (block->codes.back().sub->codes.size() == 0U) {
				engine->blocks.pop_back();
				block->codes.pop_back();
			}
#endif

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_LOOP) {
			lex->advance();
			size_t ip_begin = block->codes.size();

			if (lex->next == token_kind::tk_open_par) {
				parse_parentheses(block, lex);
				size_t ip = block->codes.size();
				{
					script_engine::block tmp(block->level, block_kind::bk_normal);

					tmp.codes.push_back(code(lex->line, command_kind::pc_loop_count));
					parse_inline_block(&tmp, lex, block_kind::bk_loop);
					script_engine::block* childBlock = tmp.codes.back().sub;
					tmp.codes.push_back(code(lex->line, command_kind::pc_continue_marker));
					tmp.codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));
					tmp.codes.push_back(code(lex->line, command_kind::pc_pop));

					//Try to optimize looped yield to a wait
					if (childBlock->codes.size() == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
						engine->blocks.pop_back();
						block->codes.push_back(code(lex->line, command_kind::pc_wait));
					}
#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
					else if (childBlock->codes.size() == 0U) {		//Empty loop, discard everything
						engine->blocks.pop_back();
						while (block->codes.size() > ip_begin)
							block->codes.pop_back();
					}
#endif
					else {
						for (code c : tmp.codes) block->codes.push_back(c);
					}
				}
			}
			else {
				size_t ip = block->codes.size();
				parse_inline_block(block, lex, block_kind::bk_loop);
				block->codes.push_back(code(lex->line, command_kind::pc_continue_marker));
				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
				if (block->codes[ip].sub->codes.size() == 0U) {
					engine->blocks.pop_back();
					while (block->codes.size() > ip_begin)
						block->codes.pop_back();
				}
#endif
			}

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_TIMES) {
			lex->advance();
			size_t ip_begin = block->codes.size();

			parse_parentheses(block, lex);
			size_t ip = block->codes.size();
			if (lex->next == token_kind::tk_LOOP) {
				lex->advance();
			}
			{
				script_engine::block tmp(block->level, block_kind::bk_normal);

				tmp.codes.push_back(code(lex->line, command_kind::pc_loop_count));
				parse_inline_block(&tmp, lex, block_kind::bk_loop);
				script_engine::block* childBlock = tmp.codes.back().sub;
				tmp.codes.push_back(code(lex->line, command_kind::pc_continue_marker));
				tmp.codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));
				tmp.codes.push_back(code(lex->line, command_kind::pc_pop));

				//Try to optimize looped yield to a wait
				if (childBlock->codes.size() == 1U && childBlock->codes[0].command == command_kind::pc_yield) {
					engine->blocks.pop_back();
					block->codes.push_back(code(lex->line, command_kind::pc_wait));
				}
				else if (childBlock->codes.size() >= 1U) {
					for (code c : tmp.codes) block->codes.push_back(c);
				}
#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
				else {		//Empty loop, discard everything
					engine->blocks.pop_back();
					while (block->codes.size() > ip_begin)
						block->codes.pop_back();
				}
#endif
			}

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_WHILE) {
			lex->advance();
			size_t ip = block->codes.size();

			parse_parentheses(block, lex);
			if (lex->next == token_kind::tk_LOOP) {
				lex->advance();
			}

			block->codes.push_back(code(lex->line, command_kind::pc_loop_if));

			size_t ip_call = block->codes.size();
			parse_inline_block(block, lex, block_kind::bk_loop);

			block->codes.push_back(code(lex->line, command_kind::pc_continue_marker));
			block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
			if (block->codes[ip_call].sub->codes.size() == 0U) {
				engine->blocks.pop_back();
				while (block->codes.size() > ip)
					block->codes.pop_back();
			}
#endif

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_FOR) {
			lex->advance();

			size_t ip_for_begin = block->codes.size();

			if (lex->next == token_kind::tk_EACH) {				//Foreach loop
				lex->advance();
				parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");

				lex->advance();
				if (lex->next == token_kind::tk_LET) lex->advance();
				else if (lex->next == token_kind::tk_const)
					parser_assert(false, "The counter variable cannot be const.\r\n");

				parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

				std::string feIdentifier = lex->word;

				lex->advance();
				parser_assert(lex->next == token_kind::tk_IN || lex->next == token_kind::tk_colon, 
					"\"in\" or a colon is required.\r\n");
				lex->advance();

				parse_expression(block, lex);

				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				if (lex->next == token_kind::tk_LOOP) lex->advance();

				size_t ip = block->codes.size();

				block->codes.push_back(code(lex->line, command_kind::pc_for_each_and_push_first));

				script_engine::block* b = engine->new_block(block->level + 1, block_kind::bk_loop);
				std::vector<std::string> counter;
				counter.push_back(feIdentifier);
				parse_block(b, lex, &counter, false);

				//block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
				block->codes.push_back(code(lex->line, command_kind::pc_call, b, 1));

				block->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));
				block->codes.push_back(code(lex->line, command_kind::pc_pop));

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
				if (b->codes.size() == 1U) {	//1 for pc_assign
					engine->blocks.pop_back();
					while (block->codes.size() > ip_for_begin)
						block->codes.pop_back();
				}
#endif
			}
			else if (lex->next == token_kind::tk_open_par) {	//Regular for loop
				lex->advance();

				bool isNewVar = false;
				bool isNewVarConst = false;
				std::string newVarName = "";
				scanner lex_s1(*lex);
				if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL || lex->next == token_kind::tk_const) {
					isNewVar = true;
					isNewVarConst = lex->next == token_kind::tk_const;
					lex->advance();
					parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");
					newVarName = lex->word;
				}

				while (lex->next != token_kind::tk_semicolon) lex->advance();
				lex->advance();

				bool hasExpr = true;
				if (lex->next == token_kind::tk_semicolon) hasExpr = false;

				scanner lex_s2(*lex);

				while (lex->next != token_kind::tk_semicolon) lex->advance();
				lex->advance();

				scanner lex_s3(*lex);

				while (lex->next != token_kind::tk_semicolon && lex->next != token_kind::tk_close_par) lex->advance();
				lex->advance();
				if (lex->next == token_kind::tk_close_par) lex->advance();

				script_engine::block* forBlock = engine->new_block(block->level + 1, block_kind::bk_normal);

				block->codes.push_back(code(lex->line, command_kind::pc_call, forBlock, 0));

				size_t ip_code;

				//For block
				frame.push_back(scope_t(forBlock->kind));
				{
					if (isNewVar) {
						symbol s;
						s.level = forBlock->level;
						s.sub = nullptr;
						s.variable = 0;
						s.can_overload = false;
						s.can_modify = !isNewVarConst;
						frame.back().singular_insert(newVarName, s);
					}
					parse_statements(forBlock, &lex_s1, token_kind::tk_semicolon, true);

					//hahaha what
					script_engine::block tmpEvalBlock(block->level + 1, block_kind::bk_normal);
					parse_statements(&tmpEvalBlock, &lex_s3, token_kind::tk_comma, true);
					while (lex_s3.next == token_kind::tk_comma) {
						lex_s3.advance();
						parse_statements(&tmpEvalBlock, &lex_s3, token_kind::tk_comma, true);
					}
					lex->copy_state(lex_s3);
					lex->advance();

					size_t ip_begin = forBlock->codes.size();

					//Code block
					{
						if (hasExpr) {
							parse_expression(forBlock, &lex_s2);
							forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_loop_if));
						}

						//Parse the code contained inside the for loop
						ip_code = forBlock->codes.size();
						parse_inline_block(forBlock, lex, block_kind::bk_loop);
					}

					forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_continue_marker));
					{
						for (auto itr = tmpEvalBlock.codes.begin(); itr != tmpEvalBlock.codes.end(); ++itr)
							forBlock->codes.push_back(*itr);
					}
					forBlock->codes.push_back(code(lex_s2.line, command_kind::pc_loop_back, ip_begin));
				}
				frame.pop_back();

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
				if (forBlock->codes[ip_code].sub->codes.size() == 0U) {
					engine->blocks.pop_back();
					engine->blocks.pop_back();
					while (block->codes.size() > ip_for_begin)
						block->codes.pop_back();
				}
#endif
			}
			else {
				parser_assert(false, "\"(\" is required.\r\n");
			}

			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_ASCENT || lex->next == token_kind::tk_DESCENT) {
			bool isAscent = lex->next == token_kind::tk_ASCENT;
			lex->advance();

			parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");
			lex->advance();

			if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) lex->advance();
			else if (lex->next == token_kind::tk_const)
				parser_assert(false, "The counter variable cannot be const.\r\n");

			parser_assert(lex->next == token_kind::tk_word, "Variable name is required.\r\n");

			std::string counterName = lex->word;

			lex->advance();

			parser_assert(lex->next == token_kind::tk_IN, "\"in\" is required.\r\n");
			lex->advance();

			size_t ip_ascdsc_begin = block->codes.size();

			script_engine::block* containerBlock = engine->new_block(block->level + 1, block_kind::bk_normal);
			block->codes.push_back(code(lex->line, command_kind::pc_call, containerBlock, 0));
			frame.push_back(scope_t(containerBlock->kind));
			{
				auto InsertSymbol = [&](size_t var, std::string name, bool isInternal) {
					symbol s;
					s.level = containerBlock->level;
					s.sub = nullptr;
					s.variable = var;
					s.can_overload = false;
					s.can_modify = !isInternal;
					frame.back().singular_insert(name, s);
				};

				InsertSymbol(0, "!0", true);	//An internal value, inaccessible to scripters (value copied to s_2 in each loop)
				InsertSymbol(1, "!1", true);	//An internal value, inaccessible to scripters
				InsertSymbol(2, counterName, false);	//The actual counter

				parse_expression(containerBlock, lex);	//First value, to s_0 if ascent
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, (int)(!isAscent), "!0"));

				parser_assert(lex->next == token_kind::tk_range, "\"..\" is required.\r\n");
				lex->advance();

				parse_expression(containerBlock, lex);	//Second value, to s_0 if descent
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, (int)(isAscent), "!1"));

				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				if (lex->next == token_kind::tk_LOOP) {
					lex->advance();
				}

				size_t ip = containerBlock->codes.size();

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 1, "!1"));
				containerBlock->codes.push_back(code(lex->line, isAscent ? command_kind::pc_compare_and_loop_ascent :
					command_kind::pc_compare_and_loop_descent));

				if (!isAscent) {
#ifndef __SCRIPT_H__INLINE_OPERATION
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
						containerBlock->level, 0, "!0"));
					write_operation(containerBlock, lex, "predecessor", 1);
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
						containerBlock->level, 0, "!0"));
#else
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_inline_dec,
						containerBlock->level, 0, "!0"));
#endif
				}

				//Copy s_0 to s_2
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
					containerBlock->level, 0, "!0"));
				containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
					containerBlock->level, 2, counterName));

				//Parse the code contained inside the loop
				size_t ip_code = containerBlock->codes.size();
				parse_inline_block(containerBlock, lex, block_kind::bk_loop);

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_continue_marker));

				if (isAscent) {
#ifndef __SCRIPT_H__INLINE_OPERATION
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_push_variable,
						containerBlock->level, 0, "!0"));
					write_operation(containerBlock, lex, "successor", 1);
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_assign,
						containerBlock->level, 0, "!0"));
#else
					containerBlock->codes.push_back(code(lex->line, command_kind::pc_inline_inc,
						containerBlock->level, 0, "!0"));
#endif
				}

				containerBlock->codes.push_back(code(lex->line, command_kind::pc_loop_back, ip));

#ifdef __SCRIPT_H__BLOCK_OPTIMIZE
				if (containerBlock->codes[ip_code].sub->codes.size() == 0U) {
					engine->blocks.pop_back();
					engine->blocks.pop_back();
					while (block->codes.size() > ip_ascdsc_begin)
						block->codes.pop_back();
				}
#endif
			}
			frame.pop_back();
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_IF) {
			lex->advance();
			block->codes.push_back(code(lex->line, command_kind::pc_case_begin));

			parse_parentheses(block, lex);
			block->codes.push_back(code(lex->line, command_kind::pc_case_if_not));
			parse_inline_block(block, lex, block_kind::bk_normal);
			while (lex->next == token_kind::tk_ELSE) {
				lex->advance();
				block->codes.push_back(code(lex->line, command_kind::pc_case_next));
				if (lex->next == token_kind::tk_IF) {
					lex->advance();
					parse_parentheses(block, lex);
					block->codes.push_back(code(lex->line, command_kind::pc_case_if_not));
					parse_inline_block(block, lex, block_kind::bk_normal);
				}
				else {
					parse_inline_block(block, lex, block_kind::bk_normal);
					break;
				}
			}

			block->codes.push_back(code(lex->line, command_kind::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_ALTERNATIVE) {
			lex->advance();
			parse_parentheses(block, lex);
			block->codes.push_back(code(lex->line, command_kind::pc_case_begin));
			while (lex->next == token_kind::tk_CASE) {
				lex->advance();
				parser_assert(lex->next == token_kind::tk_open_par, "\"(\" is required.\r\n");
				
				block->codes.push_back(code(lex->line, command_kind::pc_case_begin));
				do {
					lex->advance();

					block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
					parse_expression(block, lex);

#ifndef __SCRIPT_H__INLINE_OPERATION
					write_operation(block, lex, "compare", 2);
					block->codes.push_back(code(lex->line, command_kind::pc_compare_e));
#else
					block->codes.push_back(code(lex->line, command_kind::pc_inline_cmp_e));
#endif

					block->codes.push_back(code(lex->line, command_kind::pc_dup_n, 1));
					block->codes.push_back(code(lex->line, command_kind::pc_case_if));
					block->codes.push_back(code(lex->line, command_kind::pc_pop));

				} while (lex->next == token_kind::tk_comma);
				block->codes.push_back(code(lex->line, command_kind::pc_push_value, 
					value(engine->get_boolean_type(), false)));
				block->codes.push_back(code(lex->line, command_kind::pc_case_end));
				
				parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
				lex->advance();

				block->codes.push_back(code(lex->line, command_kind::pc_case_if_not));
				block->codes.push_back(code(lex->line, command_kind::pc_pop));
				parse_inline_block(block, lex, block_kind::bk_normal);
				block->codes.push_back(code(lex->line, command_kind::pc_case_next));
			}
			if (lex->next == token_kind::tk_OTHERS) {
				lex->advance();
				block->codes.push_back(code(lex->line, command_kind::pc_pop));
				parse_inline_block(block, lex, block_kind::bk_normal);
			}
			else {
				block->codes.push_back(code(lex->line, command_kind::pc_pop));
			}
			block->codes.push_back(code(lex->line, command_kind::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == token_kind::tk_BREAK) {
			lex->advance();
			block->codes.push_back(code(lex->line, command_kind::pc_break_loop));
		}
		else if (lex->next == token_kind::tk_CONTINUE) {
			lex->advance();
			block->codes.push_back(code(lex->line, command_kind::pc_loop_continue));
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
				parse_expression(block, lex);
				symbol* s = search_result();
				parser_assert(s, "Only functions may return values.\r\n");

				block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, 
					"[function_result]"));
			}
			block->codes.push_back(code(lex->line, command_kind::pc_break_routine));
		}
		else if (lex->next == token_kind::tk_YIELD) {
			lex->advance();
			block->codes.push_back(code(lex->line, command_kind::pc_yield));
		}
		else if (lex->next == token_kind::tk_WAIT) {
			lex->advance();
			parse_parentheses(block, lex);
			block->codes.push_back(code(lex->line, command_kind::pc_wait));
		}
		else if (lex->next == token_kind::tk_at || lex->next == token_kind::tk_SUB || 
			lex->next == token_kind::tk_FUNCTION || lex->next == token_kind::tk_TASK) 
		{
			token_kind token = lex->next;

			lex->advance();
			if (lex->next != token_kind::tk_word) {
				std::string error = "The ";
				switch (token) {
				case token_kind::tk_at:
					error += "event(\'@\') block";
					break;
				case token_kind::tk_SUB:
					error += "sub";
					break;
				case token_kind::tk_FUNCTION:
					error += "function";
					break;
				case token_kind::tk_TASK:
					error += "task";
					break;
				}
				error += "'s name is required.\r\n";
				throw parser_error(error);
			}

			std::string funcName = lex->word;
			scope_t* pScope = nullptr;

			symbol* s = search(funcName, &pScope);

			if (token == token_kind::tk_at) {
				parser_assert(s->sub->level <= 1, "An event(\'@\') block cannot exist here.\r\n");

				events[funcName] = s->sub;
			}

			lex->advance();

			std::vector<std::string> args;
			if (s->sub->kind != block_kind::bk_sub) {
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					while (lex->next == token_kind::tk_word || lex->next == token_kind::tk_LET || 
						lex->next == token_kind::tk_REAL) 
					{
						if (lex->next == token_kind::tk_LET || lex->next == token_kind::tk_REAL) {
							lex->advance();
							parser_assert(lex->next == token_kind::tk_word, "Parameter name is required.\r\n");
						}
						args.push_back(lex->word);
						lex->advance();
						if (lex->next != token_kind::tk_comma)
							break;
						lex->advance();
					}
					parser_assert(lex->next == token_kind::tk_close_par, "\")\" is required.\r\n");
					lex->advance();
				}
			}
			else {
				//互換性のため空の括弧だけ許す
				if (lex->next == token_kind::tk_open_par) {
					lex->advance();
					parser_assert(lex->next == token_kind::tk_close_par, "Only an empty parameter list is allowed here.\r\n");
					lex->advance();
				}
			}

			s = search_in(pScope, funcName, args.size());

			parse_block(s->sub, lex, &args, s->sub->kind == block_kind::bk_function);
			need_semicolon = false;
		}

		if (single_parse) break;

		//セミコロンが無いと継続しない
		if (need_semicolon && lex->next != statement_terminator)
			break;

		if (lex->next == statement_terminator)
			lex->advance();
	}
}

inline void parser::parse_inline_block(script_engine::block* block, scanner* lex, script_engine::block_kind kind) {
	script_engine::block* b = engine->new_block(block->level + 1, kind);
	parse_block(b, lex, nullptr, false);
	block->codes.push_back(code(lex->line, command_kind::pc_call, b, 0));
}

void parser::parse_block(script_engine::block* block, scanner* lex, std::vector<std::string> const* args, bool adding_result) {
	parser_assert(lex->next == token_kind::tk_open_cur, "\"{\" is required.\r\n");
	lex->advance();

	frame.push_back(scope_t(block->kind));

	int countVar = scan_current_scope(block->level, args, adding_result);
	block->codes.push_back(code(lex->line, command_kind::pc_var_alloc, countVar));
	if (args) {
		for (size_t i = 0; i < args->size(); ++i) {
			symbol* s = search((*args)[i]);
			block->codes.push_back(code(lex->line, command_kind::pc_assign, s->level, s->variable, (*args)[i]));
		}
	}
	parse_statements(block, lex);

	frame.pop_back();

	parser_assert(lex->next == token_kind::tk_close_cur, "\"}\" is required.\r\n");
	lex->advance();
}

/* script_type_manager */

script_type_manager* script_type_manager::base_ = nullptr;
script_type_manager::script_type_manager() {
	if (base_ != nullptr) return;
	base_ = this;

	real_type = types.insert(type_data(type_data::type_kind::tk_real)).first;
	char_type = types.insert(type_data(type_data::type_kind::tk_char)).first;
	boolean_type = types.insert(type_data(type_data::type_kind::tk_boolean)).first;
	string_type = types.insert(type_data(type_data::type_kind::tk_array, const_cast<type_data*>(&*char_type))).first;
	types.insert(type_data(type_data::type_kind::tk_array, const_cast<type_data*>(&*real_type)));		//Real array
	types.insert(type_data(type_data::type_kind::tk_array, const_cast<type_data*>(&*boolean_type)));	//Bool array
}

type_data* script_type_manager::get_array_type(type_data* element) {
	type_data target = type_data(type_data::type_kind::tk_array, element);
	auto itr = types.find(target);
	if (itr == types.end()) {
		itr = types.insert(target).first;
	}
	return const_cast<type_data*>(&*itr);
}

/* script_engine */

script_engine::script_engine(std::string const& source, int funcc, function const* funcv) {
	main_block = new_block(0, block_kind::bk_normal);

	const char* end = &source[0] + source.size();
	scanner s(source.c_str(), end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::script_engine(std::vector<char> const& source, int funcc, function const* funcv) {
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

	error = false;
	bTerminate = false;
}

script_machine::~script_machine() {
}

script_machine::environment::environment(std::shared_ptr<environment> parent, script_engine::block* b) {
	this->parent = parent;
	this->sub = b;
	this->ip = 0;
	this->variables.clear();
	this->stack.clear();
	this->has_result = false;
	this->waitCount = 0;
}
script_machine::environment::~environment() {
	this->variables.clear();
	this->stack.clear();
}

void script_machine::run() {
	if (bTerminate) return;

	assert(!error);
	if (threads.size() == 0) {
		error_line = -1;
		threads.push_back(std::make_shared<environment>(nullptr, engine->main_block));
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
		*env = std::make_shared<environment>(*env, event);

		finished = false;

		environment_ptr epp = (*env)->parent->parent;
		call_start_parent_environment_list.push_back(epp);
		while (!finished && !bTerminate) {
			advance();
		}
		call_start_parent_environment_list.pop_back();
		finished = false;
	}
}

void script_machine::advance() {
	typedef script_engine::command_kind command_kind;
	typedef script_engine::block_kind block_kind;

	environment_ptr current = *current_thread_index;

	if (current->waitCount > 0) {
		yield();
		--current->waitCount;
		return;
	}

	if (current->ip >= current->sub->codes.size()) {
		environment_ptr parent = current->parent;

		bool bFinish = false;
		if (parent == nullptr)
			bFinish = true;
		else {
			if (call_start_parent_environment_list.size() > 1) {
				environment_ptr env = *call_start_parent_environment_list.rbegin();
				if (parent == env)
					bFinish = true;
			}
		}

		if (bFinish) {
			finished = true;
		}
		else {
			if (current->sub->kind == block_kind::bk_microthread) {
				current_thread_index = threads.erase(current_thread_index);
				yield();
			}
			else {
				if (current->has_result && parent != nullptr)
					parent->stack.push_back(current->variables[0]);
				*current_thread_index = parent;
			}
		}
	}
	else {
		script_engine::code* c = &(current->sub->codes[current->ip]);
		error_line = c->line;
		++(current->ip);

		//It's so *bad*, I just can't.

		switch (c->command) {
		case command_kind::pc_var_alloc:
			current->variables.resize(c->ip);
			break;
		case command_kind::pc_assign:
		{
			stack_t& stack = current->stack;
			assert(stack.size() > 0);

			for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
				if (i->sub->level == c->level) {
					variables_t& vars = i->variables;

					if (vars.size() <= c->variable) {	//Shouldn't happen with pc_var_alloc
						vars.resize(c->variable + 4);
						vars.length = c->variable + 1;
					}

					value* dest = &(vars[c->variable]);
					value* src = &stack.back();

					if (dest->has_data() && dest->get_type() != src->get_type()
						&& !(dest->get_type()->get_kind() == type_data::type_kind::tk_array
							&& src->get_type()->get_kind() == type_data::type_kind::tk_array
							&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
						raise_error(L"A variable cannot change its value type.\r\n");
						break;
					}

					*dest = *src;
					stack.pop_back();

					break;
				}
			}

			break;
		}
		case command_kind::pc_assign_writable:
		{
			stack_t& stack = current->stack;
			assert(stack.size() >= 2);

			value* dest = &stack[stack.size() - 2];
			value* src = &stack[stack.size() - 1];

			if (dest->has_data() && dest->get_type() != src->get_type()
				&& !(dest->get_type()->get_kind() == type_data::type_kind::tk_array && src->get_type()->get_kind() == type_data::type_kind::tk_array
					&& (dest->length_as_array() > 0 || src->length_as_array() > 0))) {
				raise_error(L"A variable cannot change its value type.\r\n");
			}
			else {
				dest->overwrite(*src);
				stack.pop_back(2U);
			}

			break;
		}
		case command_kind::pc_continue_marker:	//Dummy token for pc_loop_continue
			break;
		case command_kind::pc_loop_continue:
		case command_kind::pc_break_loop:
		case command_kind::pc_break_routine:
			for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
				i->ip = i->sub->codes.size();

				if (c->command == command_kind::pc_break_loop || c->command == command_kind::pc_loop_continue) {
					bool exit = false;
					switch (i->sub->kind) {
					case block_kind::bk_loop:
					{
						command_kind targetCommand = command_kind::pc_loop_back;
						if (c->command == command_kind::pc_loop_continue) 
							targetCommand = command_kind::pc_continue_marker;

						environment* e = (i->parent).get();
						assert(e != nullptr);
						do
							++(e->ip);
						while (e->sub->codes[e->ip - 1].command != targetCommand);
					}
					case block_kind::bk_microthread:	//Prevents catastrophes.
					case block_kind::bk_function:
					//case block_kind::bk_normal:
						exit = true;
					default:
						break;
					}
					if (exit) break;
				}
				else {	//pc_break_routine
					if (i->sub->kind == block_kind::bk_sub || i->sub->kind == block_kind::bk_function
						|| i->sub->kind == block_kind::bk_microthread)
						break;
					else if (i->sub->kind == block_kind::bk_loop)
						i->parent->stack.clear();
				}
			}
			break;
		case command_kind::pc_call:
		case command_kind::pc_call_and_push_result:
		{
			stack_t& current_stack = current->stack;
			//assert(current_stack.size() >= c->arguments);

			if (current_stack.size() < c->arguments) {
				std::wstring error = StringUtility::FormatToWide(
					"Unexpected script error: Stack size[%d] is less than the number of arguments[%d].\r\n",
					current_stack.size(), c->arguments);
				raise_error(error);
				break;
			}

			if (c->sub->func) {
				//Default functions

				size_t sizePrev = current_stack.size();

				value* argv = nullptr;
				if (sizePrev > 0 && c->arguments > 0)
					argv = current_stack.at + (sizePrev - c->arguments);
				value ret = c->sub->func(this, c->arguments, argv);

				if (stopped) {
					--(current->ip);
				}
				else {
					resuming = false;

					//Erase the passed arguments from the stack
					current_stack.pop_back(c->arguments);

					//Return value
					if (c->command == command_kind::pc_call_and_push_result)
						current_stack.push_back(ret);
				}
			}
			else if (c->sub->kind == block_kind::bk_microthread) {
				//Tasks
				environment_ptr e = std::make_shared<environment>(current, c->sub);
				threads.insert(++current_thread_index, e);
				--current_thread_index;

				//Pass the arguments
				for (size_t i = 0; i < c->arguments; ++i) {
					e->stack.push_back(current_stack.back());
					current_stack.pop_back();
				}
			}
			else {
				//User-defined functions or internal blocks
				environment_ptr e = std::make_shared<environment>(current, c->sub);
				e->has_result = c->command == command_kind::pc_call_and_push_result;
				*current_thread_index = e;

				//Pass the arguments
				for (size_t i = 0; i < c->arguments; ++i) {
					e->stack.push_back(current_stack.back());
					current_stack.pop_back();
				}
			}

			break;
		}
		case command_kind::pc_case_begin:
		case command_kind::pc_case_end:
			break;
		case command_kind::pc_case_if:
		case command_kind::pc_case_if_not:
		case command_kind::pc_case_next:
		{
			bool exit = true;
			if (c->command != command_kind::pc_case_next) {
				stack_t& current_stack = current->stack;
				exit = current_stack.back().as_boolean();
				if (c->command == command_kind::pc_case_if_not)
					exit = !exit;
				current_stack.pop_back();
			}
			if (exit) {
				int nested = 0;
				for (; ; ) {
					switch (current->sub->codes[current->ip].command) {
					case command_kind::pc_case_begin:
						++nested;
						break;
					case command_kind::pc_case_end:
						--nested;
						if (nested < 0)
							goto next;
						break;
					case command_kind::pc_case_next:
						if (nested == 0 && c->command != command_kind::pc_case_next) {
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

			break;
		}
		case command_kind::pc_compare_e:
		case command_kind::pc_compare_g:
		case command_kind::pc_compare_ge:
		case command_kind::pc_compare_l:
		case command_kind::pc_compare_le:
		case command_kind::pc_compare_ne:
		{
			stack_t& stack = current->stack;
			value& t = stack.back();
			double r = t.as_real();
			bool b = false;
			switch (c->command) {
			case command_kind::pc_compare_e:
				b = r == 0;
				break;
			case command_kind::pc_compare_g:
				b = r > 0;
				break;
			case command_kind::pc_compare_ge:
				b = r >= 0;
				break;
			case command_kind::pc_compare_l:
				b = r < 0;
				break;
			case command_kind::pc_compare_le:
				b = r <= 0;
				break;
			case command_kind::pc_compare_ne:
				b = r != 0;
				break;
			}
			t.set(engine->get_boolean_type(), b);

			break;
		}
		case command_kind::pc_dup_n:
		{
			stack_t& stack = current->stack;
			size_t len = stack.size();
			assert(c->ip <= len);
			stack.push_back(stack[len - c->ip]);
			break;
		}
		case command_kind::pc_swap:
		{
			size_t len = current->stack.size();
			assert(len >= 2);
			value tmp = current->stack[len - 1];
			current->stack[len - 1] = current->stack[len - 2];
			current->stack[len - 2] = tmp;

			break;
		}
		case command_kind::pc_loop_back:
			current->ip = c->ip;
			break;
		case command_kind::pc_compare_and_loop_ascent:
		case command_kind::pc_compare_and_loop_descent:
		{
			stack_t& stack = current->stack;

			value* cmp_arg = (value*)(&stack.back() - 1);
			value cmp_res = compare(this, 2, cmp_arg);

			bool is_skip = c->command == command_kind::pc_compare_and_loop_ascent ? 
				(cmp_res.as_real() >= 0) : (cmp_res.as_real() <= 0);
			if (is_skip) {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
			}

			current->stack.pop_back(2U);
			break;
		}
		case command_kind::pc_loop_count:
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
				while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
			}

			break;
		}
		case command_kind::pc_loop_if:
		{
			stack_t& stack = current->stack;
			bool c = stack.back().as_boolean();
			current->stack.pop_back();
			if (!c) {
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
			}

			break;
		}
		case command_kind::pc_for_each_and_push_first:
		{
			stack_t& stack = current->stack;
			value* i = &stack.back();
			if (i->get_type()->get_kind() != type_data::type_kind::tk_array)
				raise_error(L"Value must be an array or a string.");		

			if (i->length_as_array() == 0U)
			{
				do
					++(current->ip);
				while (current->sub->codes[current->ip - 1].command != command_kind::pc_loop_back);
			}
			else {
				value iArrayNew = value(i->get_type(), std::wstring());
				current->stack.push_back(i->index_as_array(0U));
				for (size_t iElem = 1U; iElem < i->length_as_array(); ++iElem) {
					iArrayNew.append(i->get_type(), i->index_as_array(iElem));
				}
				*i = iArrayNew;
			}
			break;
		}
		
		case command_kind::pc_construct_array:
		{
			if (c->ip == 0U) break;

			std::vector<value> res_arr;
			res_arr.resize(c->ip);

			value* val_ptr = &current->stack.back() - c->ip + 1;

			type_data* type_arr = engine->get_array_type(val_ptr->get_type());
			value res = value(type_arr, 0.0);

			for (size_t iVal = 0U; iVal < c->ip; ++iVal, ++val_ptr) {
				append_check(this, iVal, type_arr, val_ptr->get_type());
				res_arr[iVal] = *val_ptr;
			}
			res.set(type_arr, res_arr);

			current->stack.pop_back(c->ip);
			current->stack.push_back(res);
			break;
		}

		case command_kind::pc_pop:
			current->stack.pop_back();
			break;
		case command_kind::pc_push_value:
			current->stack.push_back(c->data);
			break;
		case command_kind::pc_push_variable:
		case command_kind::pc_push_variable_writable:
		{
			value* var = find_variable_symbol(current.get(), c);
			if (var == nullptr) break;
			if (c->command == command_kind::pc_push_variable_writable)
				var->unique();
			current->stack.push_back(*var);
			break;
		}
		case command_kind::pc_wait:
		{
			stack_t& stack = current->stack;
			value* t = &(stack.back());
			current->waitCount = (int)(t->as_real() + 0.01) - 1;
			stack.pop_back();
			if (current->waitCount < 0) break;
		}
		//Fallthrough
		case command_kind::pc_yield:
			yield();
			break;

#ifdef __SCRIPT_H__INLINE_OPERATION
		//----------------------------------Inline operations----------------------------------
		case command_kind::pc_inline_inc:
		case command_kind::pc_inline_dec:
		{
			value* var = find_variable_symbol(current.get(), c);
			if (var == nullptr) break;

			value res = (c->command == command_kind::pc_inline_inc) ? successor(this, 1, var) : predecessor(this, 1, var);
			*var = res;

			break;
		}
		case command_kind::pc_inline_add_asi:
		case command_kind::pc_inline_sub_asi:
		case command_kind::pc_inline_mul_asi:
		case command_kind::pc_inline_div_asi:
		case command_kind::pc_inline_mod_asi:
		case command_kind::pc_inline_pow_asi:
		{
			value* var = find_variable_symbol(current.get(), c);
			if (var == nullptr) break;

			value tmp[2] = { *var, current->stack.back() };
			value res;

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 2, tmp); break;
			switch (c->command) {
				DEF_CASE(command_kind::pc_inline_add_asi, add);
				DEF_CASE(command_kind::pc_inline_sub_asi, subtract);
				DEF_CASE(command_kind::pc_inline_mul_asi, multiply);
				DEF_CASE(command_kind::pc_inline_div_asi, divide);
				DEF_CASE(command_kind::pc_inline_mod_asi, remainder_);
				DEF_CASE(command_kind::pc_inline_pow_asi, power);
			}
#undef DEF_CASE

			current->stack.pop_back();
			*var = res;
			break;
		}
		case command_kind::pc_inline_neg:
		case command_kind::pc_inline_not:
		case command_kind::pc_inline_abs:
		{
			value res;

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 1, &current->stack.back()); break;
			switch (c->command) {
				DEF_CASE(command_kind::pc_inline_neg, negative);
				DEF_CASE(command_kind::pc_inline_not, not_);
				DEF_CASE(command_kind::pc_inline_abs, absolute);
			}
#undef DEF_CASE

			current->stack.pop_back();
			current->stack.push_back(res);
			break;
		}
		case command_kind::pc_inline_add:
		case command_kind::pc_inline_sub:
		case command_kind::pc_inline_mul:
		case command_kind::pc_inline_div:
		case command_kind::pc_inline_mod:
		case command_kind::pc_inline_pow:
		case command_kind::pc_inline_app:
		case command_kind::pc_inline_cat:
		{
			value res;
			value* args = (value*)(&current->stack.back() - 1);

#define DEF_CASE(cmd, fn) case cmd: res = fn(this, 2, args); break;
			switch (c->command) {
				DEF_CASE(command_kind::pc_inline_add, add);
				DEF_CASE(command_kind::pc_inline_sub, subtract);
				DEF_CASE(command_kind::pc_inline_mul, multiply);
				DEF_CASE(command_kind::pc_inline_div, divide);
				DEF_CASE(command_kind::pc_inline_mod, remainder_);
				DEF_CASE(command_kind::pc_inline_pow, power);
				DEF_CASE(command_kind::pc_inline_app, append);
				DEF_CASE(command_kind::pc_inline_cat, concatenate);
			}
#undef DEF_CASE

			current->stack.pop_back(2U);
			current->stack.push_back(res);
			break;
		}
		case command_kind::pc_inline_cmp_e:
		case command_kind::pc_inline_cmp_g:
		case command_kind::pc_inline_cmp_ge:
		case command_kind::pc_inline_cmp_l:
		case command_kind::pc_inline_cmp_le:
		case command_kind::pc_inline_cmp_ne:
		{
			value* args = (value*)(&current->stack.back() - 1);
			value cmp_res = compare(this, 2, args);
			double cmp_r = cmp_res.as_real();

#define DEF_CASE(cmd, expr) case cmd: cmp_res.set(engine->get_boolean_type(), expr); break;
			switch (c->command) {
				DEF_CASE(command_kind::pc_inline_cmp_e, cmp_r == 0);
				DEF_CASE(command_kind::pc_inline_cmp_g, cmp_r > 0);
				DEF_CASE(command_kind::pc_inline_cmp_ge, cmp_r >= 0);
				DEF_CASE(command_kind::pc_inline_cmp_l, cmp_r < 0);
				DEF_CASE(command_kind::pc_inline_cmp_le, cmp_r <= 0);
				DEF_CASE(command_kind::pc_inline_cmp_ne, cmp_r != 0);
			}
#undef DEF_CASE

			current->stack.pop_back(2U);
			current->stack.push_back(cmp_res);
			break;
		}
#endif

		default:
			assert(false);
		}
	}
}
value* script_machine::find_variable_symbol(environment* current_env, script_engine::code* var_data) {
	/*
	auto err_uninitialized = [&]() {
#ifdef _DEBUG
		std::wstring error = L"Variable hasn't been initialized";
		error += StringUtility::FormatToWide(": %s\r\n", var_data->var_name.c_str());
		raise_error(error);
#else
		raise_error(L"Variable hasn't been initialized.\r\n");
#endif	
	};
	*/

	for (environment* i = current_env; i != nullptr; i = (i->parent).get()) {
		if (i->sub->level == var_data->level) {
			variables_t& vars = i->variables;

			if (vars.size() <= var_data->variable || !(vars[var_data->variable].has_data())) 
				raise_error(L"Variable hasn't been initialized.\r\n");
			else return &vars[var_data->variable];

			break;
		}
	}

	raise_error(L"Variable hasn't been initialized.\r\n");
	return nullptr;
}