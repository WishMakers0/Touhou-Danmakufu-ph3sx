#pragma once

#include "../pch.h"

/*
#include "LightweightVector.hpp"

namespace gstd {
	class type_data {
	public:
		enum type_kind : uint8_t {
			tk_real, tk_char, tk_boolean, tk_array
		};

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}

		type_data(type_data const & source) : kind(source.kind), element(source.element) {}

		//デストラクタはデフォルトに任せる

		type_kind get_kind() {
			return kind;
		}

		type_data * get_element() {
			return element;
		}

	private:
		type_kind kind;
		type_data* element;

		type_data& operator=(type_data const & source);
	};

	//TODO(?): turn the value class into many value classes for each value types
	//actual TODO: use std::shared_ptr without breaking
	class value {
	public:
		value() : data(nullptr) {}
		value(type_data* t, double v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->real_value = v;
		}
		value(type_data* t, wchar_t v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->char_value = v;
		}
		value(type_data* t, bool v) {
			data = new body();
			data->ref_count = 1;
			data->type = t;
			data->boolean_value = v;
		}
		value(type_data* t, std::wstring v);
		value(const value& source) {
			data = source.data;
			if (data != nullptr)
				++(data->ref_count);
		}
		~value() {
			release();
		}

		value& operator=(value const& source);

		bool has_data() const {
			return data != nullptr;
		}

		void set(type_data* t, double v);
		void set(type_data* t, bool v);

		void append(type_data* t, value const& x);
		void concatenate(value const& x);

		double as_real() const;
		wchar_t as_char() const;
		bool as_boolean() const;
		std::wstring as_string() const;
		size_t length_as_array() const;
		value const& index_as_array(size_t i);
		value& index_as_array(size_t i) const;
		type_data* get_type() const;

		void unique() const;

		void overwrite(value const& source);
	private:
		inline void release() {
			if (data != nullptr) {
				--(data->ref_count);
				if (data->ref_count == 0) {
					delete data;
				}
			}
		}

		struct body {
			size_t ref_count;
			std::vector<value> array_value;
			type_data* type = nullptr;

			union {
				double real_value = 0.0;
				wchar_t char_value;
				bool boolean_value;
			};
		};

		mutable body* data = nullptr;
	};
}
*/