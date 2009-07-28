//this file is part of docking functionality for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef DOCKINGSPLITTER_H
#define DOCKINGSPLITTER_H

#ifndef WINDOW_CONTROL_H
#include "window.h"
#endif

#define	DMS_VERTICAL		0x00000001
#define	DMS_HORIZONTAL		0x00000002

class DockingSplitter : public Window
{
public :
	DockingSplitter() : _hMessage(NULL), _isLeftButtonDown(FALSE), _flags(0) 
	{
		_ptOldPos.x = 0;
		_ptOldPos.y = 0;
	}
	~DockingSplitter(){}

	virtual void destroy() {}

public:
	void init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags);

protected:

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND				_hMessage;

	BOOL				_isLeftButtonDown;
	POINT				_ptOldPos;
	UINT				_flags;

	static BOOL			_isVertReg;
	static BOOL			_isHoriReg;
};

#endif // DOCKINGSPLITTER_H
