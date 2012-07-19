/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <services/Console.h>
#include <util/DList.h>

#include "ConsoleService.h"

class ConsoleSessionData : public nre::SessionData {
public:
	ConsoleSessionData(nre::Service *s,size_t id,capsel_t caps,nre::Pt::portal_func func)
			: SessionData(s,id,caps,func), _page(0),  _sm(), _in_ds(), _out_ds(), _prod(), _regs(),
			  _show_pages(true) {
		_regs.offset = nre::Console::TEXT_OFF >> 1;
		_regs.mode = 0;
		_regs.cursor_pos = (nre::Console::ROWS - 1) * nre::Console::COLS + (nre::Console::TEXT_OFF >> 1);
		_regs.cursor_style = 0x0d0e;
	}
	virtual ~ConsoleSessionData() {
		delete _prod;
		delete _in_ds;
		delete _out_ds;
	}

	uint page() const {
		return _page;
	}
	nre::Producer<nre::Console::ReceivePacket> *prod() {
		return _prod;
	}
	nre::DataSpace *out_ds() {
		return _out_ds;
	}

	void prev() {
		if(_show_pages) {
			_page = (_page - 1) % Screen::TEXT_PAGES;
			ConsoleService::get()->switcher().switch_to(this,this);
		}
	}
	void next() {
		if(_show_pages) {
			_page = (_page + 1) % Screen::TEXT_PAGES;
			ConsoleService::get()->switcher().switch_to(this,this);
		}
	}

	void create(nre::DataSpace *in_ds,nre::DataSpace *out_ds,bool show_pages = true);

	void to_front() {
		swap();
		activate();
	}
	void to_back() {
		swap();
	}
	void activate() {
		_regs.offset = (nre::Console::TEXT_OFF >> 1) + (_page << 11);
		set_regs(_regs);
	}

	void set_regs(const nre::Console::Register &regs) {
		_page = (_regs.offset - (nre::Console::TEXT_OFF >> 1)) >> 11;
		_regs = regs;
		if(ConsoleService::get()->active() == this)
			ConsoleService::get()->screen()->set_regs(_regs);
	}

	PORTAL static void portal(capsel_t pid);

private:
	void swap() {
		_out_ds->switch_to(ConsoleService::get()->screen()->mem());
	}

	uint _page;
	nre::UserSm _sm;
	nre::DataSpace *_in_ds;
	nre::DataSpace *_out_ds;
	nre::Producer<nre::Console::ReceivePacket> *_prod;
	nre::Console::Register _regs;
	bool _show_pages;
};