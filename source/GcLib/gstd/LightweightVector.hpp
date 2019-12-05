#pragma once

//Currently not used, somehow broken.

#pragma warning (disable:4786)
#pragma warning (disable:4018)
#pragma warning (disable:4244)

#include <string>

namespace gstd {
	/*
	template<typename T> 
	class lightweight_vector_const_iterator : public std::_Iterator_base {
	public:
		using iterator_category = std::random_access_iterator_tag;

		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = const T*;
		using reference = const value_type&;
		using _TPtr = pointer;

		lightweight_vector_const_iterator() {
			_Ptr = nullptr;
		}
		lightweight_vector_const_iterator(_TPtr _PArg) {
			_Ptr = _PArg;
		}

		_NODISCARD reference operator*() const {
			return *_Ptr;
		}
		_NODISCARD pointer operator->() const {
			return _Ptr;
		}

		lightweight_vector_const_iterator& operator++() {
			++_Ptr;
			return *this;
		}
		lightweight_vector_const_iterator operator++(int) {
			lightweight_vector_const_iterator _Tmp = *this;
			++(*this);
			return _Tmp;
		}
		lightweight_vector_const_iterator& operator--() {
			--_Ptr;
			return *this;
		}
		lightweight_vector_const_iterator operator--(int) {
			lightweight_vector_const_iterator _Tmp = *this;
			--(*this);
			return _Tmp;
		}

		lightweight_vector_const_iterator& operator+=(const difference_type _Off) {
			_Ptr += _Off;
			return *this;
		}
		_NODISCARD lightweight_vector_const_iterator operator+(const difference_type _Off) const {
			lightweight_vector_const_iterator _Tmp = *this;
			return (_Tmp += _Off);
		}
		lightweight_vector_const_iterator& operator-=(const difference_type _Off) {
			_Ptr -= _Off;
			return *this;
		}
		_NODISCARD lightweight_vector_const_iterator operator-(const difference_type _Off) const {
			lightweight_vector_const_iterator _Tmp = *this;
			return (_Tmp -= _Off);
		}
		_NODISCARD difference_type operator-(const lightweight_vector_const_iterator& _Right) const {
			return (_Ptr - _Right._Ptr);
		}

		_NODISCARD reference operator[](const difference_type _Off) const {
			return (*((*this) + _Off));
		}

		_NODISCARD bool operator==(const lightweight_vector_const_iterator& _Right) const {
			return (_Ptr == _Right._Ptr);
		}
		_NODISCARD bool operator!=(const lightweight_vector_const_iterator& _Right) const {
			return !((*this) == _Right);
		}

		_NODISCARD bool operator<(const lightweight_vector_const_iterator& _Right) const {
			return (_Ptr < _Right._Ptr);
		}
		_NODISCARD bool operator>(const lightweight_vector_const_iterator& _Right) const {
			return (_Right < (*this));
		}
		_NODISCARD bool operator<=(const lightweight_vector_const_iterator& _Right) const {
			return !(_Right < (*this));
		}
		_NODISCARD bool operator>=(const lightweight_vector_const_iterator& _Right) const {
			return !((*this) < _Right);
		}

		_NODISCARD pointer _Unwrapped() const {
			return _Ptr;
		}
		void _Seek_to(pointer _It) {
			_Ptr = const_cast<pointer>(_It);
		}

		_TPtr _Ptr;
	};
	template<typename T> 
	class lightweight_vector_iterator : public lightweight_vector_const_iterator<T> {
	public:
		using _base = lightweight_vector_const_iterator<T>;
		using iterator_category = std::random_access_iterator_tag;

		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = value_type & ;

		lightweight_vector_iterator() {

		}
		lightweight_vector_iterator(pointer _Parg) : _base(_Parg) {

		}

		_NODISCARD reference operator*() const {
			return const_cast<reference>(_base::operator*());
		}
		_NODISCARD pointer operator->() const {
			return const_cast<pointer>(_base::operator->());
		}

		lightweight_vector_iterator& operator++() {
			++(*(_base*)this);
			return *this;
		}
		lightweight_vector_iterator operator++(int) {
			lightweight_vector_iterator _Tmp = *this;
			++(*this);
			return _Tmp;
		}
		lightweight_vector_iterator& operator--() {
			--(*(_base*)this);
			return *this;
		}
		lightweight_vector_iterator operator--(int) {
			lightweight_vector_iterator _Tmp = *this;
			--(*this);
			return _Tmp;
		}

		lightweight_vector_iterator& operator+=(const difference_type _Off) {
			(*(_base*)this) += _Off;
			return *this;
		}
		_NODISCARD lightweight_vector_iterator operator+(const difference_type _Off) const {
			lightweight_vector_iterator _Tmp = *this;
			return (_Tmp += _Off);
		}
		lightweight_vector_iterator& operator-=(const difference_type _Off) {
			return ((*this) += -_Off);
		}
		_NODISCARD lightweight_vector_iterator operator-(const difference_type _Off) const {
			lightweight_vector_iterator _Tmp = *this;
			return (_Tmp -= _Off);
		}
		_NODISCARD difference_type operator-(const _base& _Right) const {
			return ((*(_base*)this) - _Right);
		}

		_NODISCARD reference operator[](const difference_type _Off) const {
			return (*((*this) + _Off));
		}
		_NODISCARD pointer _Unwrapped() const {
			return this->_Ptr;
		}
	};
	*/

	template<typename T>
	class lightweight_vector {
	public:
		using pointer = T*;
		using const_pointer = const T*;
		/*
		using iterator = lightweight_vector_iterator<T>;
		using const_iterator = lightweight_vector_const_iterator<T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		*/

		size_t length;
		size_t capacity;
		T* at;

		lightweight_vector();
		lightweight_vector(const lightweight_vector& source);
		~lightweight_vector();

		lightweight_vector& operator=(const lightweight_vector& source);

		void reserve(size_t newSize);
		void expand();

		void push_back(const T& value);
		void pop_back();

		void clear();
		void release();

		size_t size() const {
			return length;
		}

		T& operator[] (size_t i) {
			return at[i];
		}
		const T& operator[] (size_t i) const {
			return at[i];
		}

		T& front() const {
			return *at;
		}
		T& back() const {
			return at[length - 1];
		}

		/*
		iterator begin() {
			return iterator(at);
		}
		const_iterator cbegin() {
			return begin();
		}
		iterator end() {
			return iterator(&at[0] + length);
		}
		const_iterator cend() {
			return end();
		}

		reverse_iterator rbegin() {
			return reverse_iterator(begin());
		}
		const_reverse_iterator crbegin() {
			return rbegin();
		}
		reverse_iterator rend() {
			return reverse_iterator(end());
		}
		const_reverse_iterator crend() {
			return rend();
		}
		*/

		void erase(T* pos);
		void insert(T* pos, const T& value);
	};

	template<typename T> inline lightweight_vector<T>::lightweight_vector() {
		length = 0U;
		capacity = 0U;
		at = nullptr;
	}
	template<typename T> inline lightweight_vector<T>::lightweight_vector(const lightweight_vector& source) {
		length = source.length;
		capacity = source.capacity;

		if (source.capacity > 0U) {
			at = new T[source.capacity];
			for (size_t i = 0U; i < length; ++i)
				at[i] = source.at[i];
		}
		else {
			at = nullptr;
		}
	}
	template<typename T> inline lightweight_vector<T>::~lightweight_vector() {
		if (at != nullptr)
			delete[] at;
		at = nullptr;
	}

	template<typename T> lightweight_vector<T>& lightweight_vector<T>::operator=(const lightweight_vector& source) {
		release();

		length = source.length;
		capacity = source.capacity;

		if (source.capacity > 0U) {
			at = new T[source.capacity];
			for (size_t i = 0U; i < length; ++i)
				at[i] = source.at[i];
		}
		else {
			at = nullptr;
		}

		return *this;
	}

	template<typename T> void lightweight_vector<T>::reserve(size_t newSize) {
		while (capacity < newSize)
			expand();
	}
	template<typename T> void lightweight_vector<T>::expand() {
		if (capacity == 0U) {
			capacity = 0x4u;
			at = new T[capacity];
		}
		else {
			if (capacity != 0xffffffffu) {
				if (capacity != 0x80000000u) capacity = capacity * 2;
				else capacity = 0xffffffffu;
			}
			else throw std::bad_alloc();

			T* n = new T[capacity];
			for (size_t i = 0U; i < length; ++i)
				n[i] = at[i];
			delete[] at;
			at = n;
		}
	}

	template<typename T> void lightweight_vector<T>::push_back(const T& value) {
		if (length == capacity) expand();
		at[length] = value;
		++length;
	}
	template<typename T> void lightweight_vector<T>::pop_back() {
		if (length > 0) --length;
		T temp;
		at[length] = temp;
		//memset(&at[length], 0x00, sizeof(T));
	}

	template<typename T> void lightweight_vector<T>::clear() {
		length = 0U;
	}
	template<typename T> void lightweight_vector<T>::release() {
		clear();
		if (at != nullptr) {
			delete[] at;
			at = nullptr;
			capacity = 0U;
		}
	}

	template<typename T> void lightweight_vector<T>::erase(T* pos) {
		if (length == 0U) return;
		//if ((pos < &front()) || (pos > &back())) return;

		--length;
		T* endPos = at + length - 1U;
		for (T* i = pos; i < endPos; ++i)
			*i = *(i + 1);
	}
	template<typename T> void lightweight_vector<T>::insert(T* pos, const T& value) {
		if (length == capacity) {
			size_t pos_index = (ptrdiff_t)(pos - at);
			expand();
			pos = at + pos_index;
		}
		for (T* i = at + length; i > pos; --i)
			*i = *(i - 1);
		*pos = value;
		++length;
	}
}