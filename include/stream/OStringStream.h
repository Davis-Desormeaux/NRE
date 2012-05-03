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

#include <stream/OStream.h>
#include <cstring>

namespace nul {

class OStringStream : public OStream {
public:
	static void format(char *dst,size_t max,const char *fmt,...) {
		OStringStream os(dst,max);
		va_list ap;
		va_start(ap,fmt);
		os.vwritef(fmt,ap);
		va_end(ap);
	}

	explicit OStringStream(char *dst,size_t max) : OStream(),
			_dst(dst), _max(max), _pos() {
	}

	virtual void write(char c) {
		if(_pos < _max)
			_dst[_pos++] = c;
	}

private:
	char *_dst;
	size_t _max;
	size_t _pos;
};

}