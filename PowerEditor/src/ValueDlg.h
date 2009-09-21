//this file is part of notepad++
//Copyright (C)2003 Don HO < donho@altern.org >
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

#ifndef VALUE_DLG_H
#define VALUE_DLG_H

#ifndef WINDOW_CONTROL_H
#include "StaticDialog.h"
#endif

#ifndef NUMBERTYPES_H
#include "NumberTypes.h"
#endif

#define DEFAULT_NB_NUMBER 2

class ValueDlg : public StaticDialog
{
public :
        ValueDlg() : 
			_nbNumber(DEFAULT_NB_NUMBER),
			_defaultValue(0)
		{
			memset(&_p, 0, sizeof(POINT));
		}
		//(Warning -- Member with different signature hides virtual member 'Window::init(struct HINSTANCE__ *, struct HWND__ *)'
		//lint -e1411
        void init(HINSTANCE hInst, HWND parent, int valueToSet, const TCHAR *text);
		//lint +e1411
        INT_PTR doDialog(POINT p, bool isRTL = false);
		void setNBNumber(int nbNumber);
		int reSizeValueBox();

protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/);

private :
	int _nbNumber;
    int _defaultValue;
	generic_string _name;
	POINT _p;

};

#endif //TABSIZE_DLG_H
