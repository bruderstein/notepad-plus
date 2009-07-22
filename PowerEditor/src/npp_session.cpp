// This file is part of notepad++
// Copyright (C)2009 The Notepad++ Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiled_headers.h"
#include "npp_session.h"


sessionFileInfo::sessionFileInfo( const TCHAR *fn )
{
	if (fn) _fileName = fn;
}

sessionFileInfo::sessionFileInfo( const TCHAR *fn, const TCHAR *ln, Position pos ) : 
	Position(pos)
{
	if (fn) _fileName = fn;
	if (ln)	_langName = ln;
}