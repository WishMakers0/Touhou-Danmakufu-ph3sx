/*

#include "Value.hpp"
#include "GstdUtility.hpp"

using namespace gstd;

value::value(type_data* t, std::wstring v) {
	data = new body();
	data->ref_count = 1;
	data->type = t;
	for (size_t i = 0U; i < v.size(); ++i)
		data->array_value.push_back(value(t->get_element(), v[i]));
}
value& value::operator=(value const& source) {
	if (source.data != nullptr) {
		++(source.data->ref_count);
	}
	release();
	data = source.data;
	return *this;
}

void value::set(type_data* t, double v) {
	unique();
	data->type = t;
	data->real_value = v;
}
void value::set(type_data* t, bool v) {
	unique();
	data->type = t;
	data->boolean_value = v;
}

void value::append(type_data* t, value const& x) {
	unique();
	data->type = t;
	data->array_value.push_back(x);
}
void value::concatenate(value const& x) {
	unique();
	if (length_as_array() == 0U)
		data->type = x.data->type;
	for (size_t i = 0U; i < x.data->array_value.size(); ++i)
		data->array_value.push_back(x.data->array_value[i]);
}

//Turns the value into a unique copy.
void value::unique() const {
	if (data == nullptr) {
		data = new body();
		data->ref_count = 1;
		data->type = nullptr;
	}
	else if (data->ref_count > 1U) {
		--(data->ref_count);
		data = new body(*data);
		data->ref_count = 1;
	}
}

void value::overwrite(value const& source) {
	assert(data != NULL);
	if (data == source.data)return;

	release();
	*data = *source.data;
	data->ref_count = 2;
}

size_t value::length_as_array() const {
	assert(data != nullptr && data->type->get_kind() == type_data::tk_array);
	return data->array_value.size();
}
value const& value::index_as_array(size_t i) {
	assert(data != nullptr && data->type->get_kind() == type_data::tk_array);
	assert(i < data->array_value.size());
	return data->array_value[i];
}
value& value::index_as_array(size_t i) const {
	assert(data != nullptr && data->type->get_kind() == type_data::tk_array);
	assert(i < data->array_value.size());
	return data->array_value[i];
}
type_data* value::get_type() const {
	assert(data != nullptr);
	return data->type;
}

double value::as_real() const {
	if (data == nullptr)
		return 0.0L;
	switch (data->type->get_kind()) {
	case type_data::tk_real:
		return data->real_value;
	case type_data::tk_char:
		return static_cast<float>(data->char_value);
	case type_data::tk_boolean:
		return (data->boolean_value) ? 1.0L : 0.0L;
	case type_data::tk_array:
		if (data->type->get_element()->get_kind() == type_data::tk_char)
			return std::atof(StringUtility::ConvertWideToMulti(as_string()).c_str());
		else
			return 0.0L;
	default:
		assert(false);
		return 0.0L;
	}
}

wchar_t value::as_char() const {
	if (data == nullptr)
		return 0.0L;
	switch (data->type->get_kind()) {
	case type_data::tk_real:
		return data->real_value;
	case type_data::tk_char:
		return data->char_value;
	case type_data::tk_boolean:
		return (data->boolean_value) ? L'1' : L'0';
	case type_data::tk_array:
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
	case type_data::tk_real:
		return data->real_value != 0.0L;
	case type_data::tk_char:
		return data->char_value != L'\0';
	case type_data::tk_boolean:
		return data->boolean_value;
	case type_data::tk_array:
		return data->array_value.size() > 0U;
	default:
		assert(false);
		return false;
	}
}

std::wstring value::as_string() const {
	if (data == nullptr)
		return L"(VOID)";
	switch (data->type->get_kind()) {
	case type_data::tk_real:
	{
		wchar_t buffer[128];
		std::swprintf(buffer, L"%Lf", data->real_value);
		return std::wstring(buffer);
	}
	case type_data::tk_char:
	{
		std::wstring result;
		result += data->char_value;
		return result;
	}
	case type_data::tk_boolean:
		return (data->boolean_value) ? L"true" : L"false";
	case type_data::tk_array:
	{
		if (data->type->get_element()->get_kind() == type_data::tk_char) {
			std::wstring result;
			for (size_t i = 0; i < data->array_value.size(); ++i)
				result += data->array_value[i].as_char();
			return result;
		}
		else {
			std::wstring result = L"[";
			for (size_t i = 0; i < data->array_value.size(); ++i) {
				result += data->array_value[i].as_string();
				if (i != data->array_value.size() - 1U)
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
*/