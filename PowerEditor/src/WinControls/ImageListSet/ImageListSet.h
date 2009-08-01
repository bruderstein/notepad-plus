//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef IMAGE_LIST_H
#define IMAGE_LIST_H

#define NB_MAX_COMMANDS 45
#define	IDI_SEPARATOR_ICON -1

class IconList
{
public :
	IconList() : _hImglst(NULL) {}
	~IconList()
	{
		IconList::destroy();
	}

	void create(HINSTANCE hInst, int iconSize);
	void create(int iconSize, HINSTANCE hInst, int *iconIDArray, int iconIDArraySize);

	void destroy() {
		if (_hImglst)
		{
			ImageList_Destroy(_hImglst);
			_hImglst = NULL;
		}
	}

	HIMAGELIST getHandle() const {return _hImglst;}

	void addIcon(int iconID) const;
	bool changeIcon(int index, const TCHAR *iconLocation) const;
	void setIconSize(int size) const;

private :
	HIMAGELIST _hImglst;
	HINSTANCE _hInst;
	int *_pIconIDArray;
	int _iconIDArraySize;
	int _iconSize;
};

struct ToolBarButtonUnit
{	
	int _cmdID;

	int _defaultIcon;
	int _hotIcon;
	int _grayIcon;

	int _stdIcon;
};

typedef std::vector<ToolBarButtonUnit> ToolBarIconIDs;

typedef std::vector<IconList> IconListVector;

class IconLists
{
public :
	IconLists() {};
	HIMAGELIST getImageListHandle(int index) const {
		return _iconListVector[index].getHandle();
	};

protected :
	IconListVector _iconListVector;
};

#define HLIST_DEFAULT 0
#define HLIST_HOT 1
#define HLIST_DISABLE 2

class ToolBarIcons : public IconLists
{
public :
	ToolBarIcons() : _nbCmd(0) {};
	~ToolBarIcons();

	void init(ToolBarButtonUnit *buttonUnitArray, int arraySize);
	void create(HINSTANCE hInst, int iconSize);
	void destroy();

	HIMAGELIST getDefaultLst() const {
		return IconLists::getImageListHandle(HLIST_DEFAULT);
	};

	HIMAGELIST getHotLst() const {
		return IconLists::getImageListHandle(HLIST_HOT);
	};

	HIMAGELIST getDisableLst() const {
		return IconLists::getImageListHandle(HLIST_DISABLE);
	};

	unsigned int getNbCommand() const {return _nbCmd;};
	int getCommandAt(int index) const {return _cmdArray[index];};
	void resizeIcon(int size) {
		reInit(size);
	};

	void reInit(int size);

	int getNbIcon() const {
		return int(_tbiis.size());
	};

	int getStdIconAt(int i) const {
		return _tbiis[i]._stdIcon;
	};

	bool replaceIcon(int witchList, int iconIndex, const TCHAR *iconLocation) const;

private :
	ToolBarIconIDs _tbiis;
	int _cmdArray[NB_MAX_COMMANDS];
	unsigned int _nbCmd;
};

#endif //IMAGE_LIST_H
