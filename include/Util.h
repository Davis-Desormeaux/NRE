/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <Types.h>

namespace nul {

class Util {
public:
	template<typename T>
	static inline T min(T a,T b) {
		return a < b ? a : b;
	}

	template<typename T>
	static inline T max(T a,T b) {
		return a < b ? b : a;
	}

	template<typename T>
	static inline T roundup(T value,T align) {
		return (value + align - 1) & ~(align - 1);
	}

	template<typename T>
	static inline T rounddown(T value,T align) {
		return value & ~(align - 1);
	}

	static inline size_t blockcount(size_t value,size_t blocksize) {
		return (value + blocksize - 1) / blocksize;
	}

	static inline bool overlapped(uintptr_t b1,size_t s1,uintptr_t b2,size_t s2) {
		uintptr_t e1 = b1 + s1;
		uintptr_t e2 = b2 + s2;
		return (b1 >= b2 && b1 < e2) ||			// start in range
				(e1 > b2 && e1 <= e2) ||		// end in range
				(b1 < b2 && e1 > e2);			// completely overlapped
	}

	/**
	 * Finds the position of the most significant "1" bit
	 */
	template<typename T>
	static inline T bsr(T value) {
		return __builtin_clz(value) ^ 0x1F;
	}
	/**
	 * Finds the position of the least significant "1" bit
	 */
	template<typename T>
	static inline T bsf(T value) {
		return __builtin_ctz(value);
	}

	/**
	 * Calculates the order (log2 of the size) of the largest naturally
	 * aligned block that starts at start and is no larger than size.
	 *
	 * @param minshift The largest value returned by this function.
	 *
	 * @return The calculated order or the minshift parameter is it is
	 * smaller then the order.
	 */
	static inline uint minshift(uintptr_t start,size_t size) {
		uint basealign = static_cast<uint>(bsf(start | (1ul << (8 * sizeof(uintptr_t) - 1))));
		uint shiftalign = static_cast<uint>(bsr(size | 1));
		return min(basealign,shiftalign);
	}

	static inline uint64_t tsc() {
		uint32_t u,l;
		asm volatile("rdtsc" : "=a"(l), "=d"(u));
		return (uint64_t)u << 32 | l;
	}
};

}
