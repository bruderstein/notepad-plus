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

#ifndef NPP_SESSION
#define NPP_SESSION

struct Position
{ 
	size_t _firstVisibleLine;
	size_t _startPos;
	size_t _endPos;
	size_t _xOffset;
	int _selMode;
	int _scrollWidth;
	Position() : _firstVisibleLine(0), _startPos(0), _endPos(0), _xOffset(0), _selMode(0), _scrollWidth(1) {}
};

struct sessionFileInfo : public Position {
	sessionFileInfo(const TCHAR *fn);
	sessionFileInfo(const TCHAR *fn, const TCHAR *ln, Position pos);

	sessionFileInfo(generic_string fn) : _fileName(fn){};
	sessionFileInfo(generic_string fn, Position pos) : Position(pos), _fileName(fn){};

	generic_string _fileName;
	generic_string	_langName;
	std::vector<size_t> marks;
};

struct Session {
	int nbMainFiles() const 
	{
		assert(_mainViewFiles.size() == static_cast<int>(_mainViewFiles.size()));
		return static_cast<int>(_mainViewFiles.size());
	};

	int nbSubFiles() const 
	{
		assert(_subViewFiles.size() == static_cast<int>(_subViewFiles.size()));
		return static_cast<int>(_subViewFiles.size());
	};

	int _activeView;
	int _activeMainIndex;
	int _activeSubIndex;
	std::vector<sessionFileInfo> _mainViewFiles;
	std::vector<sessionFileInfo> _subViewFiles;
};

#endif // NPP_SESSION
