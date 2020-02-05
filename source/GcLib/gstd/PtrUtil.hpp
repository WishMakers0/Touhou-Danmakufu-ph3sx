#pragma once

#include "../pch.h"

namespace gstd {
	template<typename T> static constexpr inline void ptr_delete(T*& ptr) {
		if (ptr) {
			delete ptr;
			ptr = nullptr;
		}
	}
	template<typename T> static constexpr inline void ptr_delete_scalar(T*& ptr) {
		if (ptr) {
			delete[] ptr;
			ptr = nullptr;
		}
	}
	template<typename T> static constexpr inline void ptr_release(T*& ptr) {
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}
}