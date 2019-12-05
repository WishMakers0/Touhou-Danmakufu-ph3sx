#pragma once

#include "GstdConstant.hpp"
#include "LightweightVector.hpp"

namespace gstd {
	class type_data {
	public:
		enum type_kind {
			tk_real, tk_char, tk_boolean, tk_array
		};

		type_data(type_kind k, type_data* t = nullptr) : kind(k), element(t) {}

		type_data(type_data const & source) : kind(source.kind), element(source.element) {}

		//�f�X�g���N�^�̓f�t�H���g�ɔC����

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

	class value {
	public:
		value() : data(nullptr) {}
		value(type_data* t, double v) {
			data = std::make_shared<body>();
			data->type = t;
			data->real_value = v;
		}
		value(type_data* t, wchar_t v) {
			data = std::make_shared<body>();
			data->type = t;
			data->char_value = v;
		}
		value(type_data* t, bool v) {
			data = std::make_shared<body>();
			data->type = t;
			data->boolean_value = v;
		}
		value(type_data* t, std::wstring v);
		value(const value& source) {
			data = source.data;
		}
		~value() {
			release();
		}

		value& operator=(value const& source);

		bool has_data() const {
			return data.get() != nullptr;
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

		void unique(bool forceCreate = false) const;

		void overwrite(value const& source);
	private:
		inline void release() {
			if (data == nullptr) {
				data.reset((body*)nullptr);
			}
		}

		struct body {
			std::vector<value> array_value;
			type_data* type = nullptr;

			union {
				double real_value = 0.0;
				wchar_t char_value;
				bool boolean_value;
			};
		};

		mutable std::shared_ptr<body> data = nullptr;
	};
}