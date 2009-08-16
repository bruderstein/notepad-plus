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

#include "precompiled_headers.h"
#include "PluginsManager.h"

#include "tinyxml.h"

#include "Parameters.h"
#include "resource.h"


const TCHAR * USERMSG = TEXT("This plugin is not compatible with current version of Notepad++.\n\n\
Do you want to remove this plugin from plugins directory to prevent this message from the next launch time?");

bool PluginsManager::unloadPlugin(int index, HWND nppHandle)
{
    SCNotification scnN;
	scnN.nmhdr.code = NPPN_SHUTDOWN;
	scnN.nmhdr.hwndFrom = nppHandle;
	scnN.nmhdr.idFrom = 0;
	_pluginInfos[index]->_pBeNotified(&scnN);

    //::DestroyMenu(_pluginInfos[index]->_pluginMenu);
    //_pluginInfos[index]->_pluginMenu = NULL;

    if (::FreeLibrary(_pluginInfos[index]->_hLib))
        _pluginInfos[index]->_hLib = NULL;
    else
        printStr(TEXT("not ok"));
    //delete _pluginInfos[index];
//      printInt(index);
    //vector<PluginInfo *>::iterator it = _pluginInfos.begin() + index;
    //_pluginInfos.erase(it);
    //printStr(TEXT("remove"));
    return true;
}

int PluginsManager::loadPlugin(const generic_string& pluginFilePath, std::vector<generic_string> & dll2Remove)
{
	PluginInfo *pi = new PluginInfo;
	try {
		pi->_moduleName = PathFindFileName(pluginFilePath.c_str());
		
		pi->_hLib = ::LoadLibrary(pluginFilePath.c_str());
		if (!pi->_hLib)
			throw generic_string(TEXT("Load Library is failed.\nMake \"Runtime Library\" setting of this project as \"Multi-threaded(/MT)\" may cure this problem."));

		pi->_pFuncIsUnicode = (PFUNCISUNICODE)GetProcAddress(pi->_hLib, "isUnicode");
#ifdef UNICODE
		if (!pi->_pFuncIsUnicode || !pi->_pFuncIsUnicode())
			throw generic_string(TEXT("This ANSI plugin is not compatible with your Unicode Notepad++."));
#else
		if (pi->_pFuncIsUnicode)
			throw generic_string(TEXT("This Unicode plugin is not compatible with your ANSI mode Notepad++."));
#endif

		pi->_pFuncSetInfo = (PFUNCSETINFO)GetProcAddress(pi->_hLib, "setInfo");
					
		if (!pi->_pFuncSetInfo)
			throw generic_string(TEXT("Missing \"setInfo\" function"));

		pi->_pFuncGetName = (PFUNCGETNAME)GetProcAddress(pi->_hLib, "getName");
		if (!pi->_pFuncGetName)
			throw generic_string(TEXT("Missing \"getName\" function"));

		pi->_pBeNotified = (PBENOTIFIED)GetProcAddress(pi->_hLib, "beNotified");
		if (!pi->_pBeNotified)
			throw generic_string(TEXT("Missing \"beNotified\" function"));

		pi->_pMessageProc = (PMESSAGEPROC)GetProcAddress(pi->_hLib, "messageProc");
		if (!pi->_pMessageProc)
			throw generic_string(TEXT("Missing \"messageProc\" function"));
		
		pi->_pFuncSetInfo(_nppData);

		pi->_pFuncGetFuncsArray = (PFUNCGETFUNCSARRAY)GetProcAddress(pi->_hLib, "getFuncsArray");
		if (!pi->_pFuncGetFuncsArray) 
			throw generic_string(TEXT("Missing \"getFuncsArray\" function"));

		pi->_funcItems = pi->_pFuncGetFuncsArray(&pi->_nbFuncItem);

		if ((!pi->_funcItems) || (pi->_nbFuncItem <= 0))
			throw generic_string(TEXT("Missing \"FuncItems\" array, or the nb of Function Item is not set correctly"));

		pi->_pluginMenu = ::CreateMenu();
		
		GetLexerCountFn GetLexerCount = (GetLexerCountFn)::GetProcAddress(pi->_hLib, "GetLexerCount");
		// it's a lexer plugin
		if (GetLexerCount)
		{
			GetLexerNameFn GetLexerName = (GetLexerNameFn)::GetProcAddress(pi->_hLib, "GetLexerName");
			if (!GetLexerName)
				throw generic_string(TEXT("Loading GetLexerName function failed."));

			GetLexerStatusTextFn GetLexerStatusText = (GetLexerStatusTextFn)::GetProcAddress(pi->_hLib, "GetLexerStatusText");

			if (!GetLexerStatusText)
				throw generic_string(TEXT("Loading GetLexerStatusText function failed."));

			// Assign a buffer for the lexer name.
			char lexName[MAX_EXTERNAL_LEXER_NAME_LEN];
			lexName[0] = '\0';
			TCHAR lexDesc[MAX_EXTERNAL_LEXER_DESC_LEN];
			lexDesc[0] = '\0';

			int numLexers = GetLexerCount();

			NppParameters * nppParams = NppParameters::getInstance();
			
			ExternalLangContainer *containers[30];
#ifdef UNICODE
			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif
			for (int x = 0; x < numLexers; x++)
			{
				GetLexerName(x, lexName, MAX_EXTERNAL_LEXER_NAME_LEN);
				GetLexerStatusText(x, lexDesc, MAX_EXTERNAL_LEXER_DESC_LEN);
#ifdef UNICODE
				const TCHAR *pLexerName = wmc->char2wchar(lexName, CP_ACP);
#else
				const TCHAR *pLexerName = lexName;
#endif
				if (!nppParams->isExistingExternalLangName(pLexerName) && nppParams->ExternalLangHasRoom())
					containers[x] = new ExternalLangContainer(pLexerName, lexDesc);
				else
					containers[x] = NULL;
			}

			TCHAR xmlPath[MAX_PATH];
            lstrcpy(xmlPath, nppParams->getNppPath().c_str());
			PathAppend(xmlPath, TEXT("plugins\\Config"));
            PathAppend(xmlPath, pi->_moduleName.c_str());
			PathRemoveExtension(xmlPath);
			PathAddExtension(xmlPath, TEXT(".xml"));

			if (!PathFileExists(xmlPath))
			{
				memset(xmlPath, 0, MAX_PATH * sizeof(TCHAR));
				lstrcpy(xmlPath, nppParams->getAppDataNppDir() );
				PathAppend(xmlPath, TEXT("plugins\\Config"));
                PathAppend(xmlPath, pi->_moduleName.c_str());
				PathRemoveExtension( xmlPath );
				PathAddExtension( xmlPath, TEXT(".xml") );
				
				if (! PathFileExists( xmlPath ) )
				{
					throw generic_string(generic_string(xmlPath) + TEXT(" is missing."));
				}
			}

			TiXmlDocument *_pXmlDoc = new TiXmlDocument(xmlPath);

			if (!_pXmlDoc->LoadFile())
			{
				delete _pXmlDoc;
				_pXmlDoc = NULL;
				throw generic_string(generic_string(xmlPath) + TEXT(" failed to load."));
			}
			
			for (int x = 0; x < numLexers; x++) // postpone adding in case the xml is missing/corrupt
				if (containers[x] != NULL)
					nppParams->addExternalLangToEnd(containers[x]);

			nppParams->getExternalLexerFromXmlTree(_pXmlDoc);
			nppParams->getExternalLexerDoc()->push_back(_pXmlDoc);
#ifdef UNICODE
			const char *pDllName = wmc->wchar2char(pluginFilePath.c_str(), CP_ACP);
#else
			const char *pDllName = pluginFilePath.c_str();
#endif
			::SendMessage(_nppData._scintillaMainHandle, SCI_LOADLEXERLIBRARY, 0, (LPARAM)pDllName);
		}
        
		_pluginInfos.push_back(pi);
        return (_pluginInfos.size() - 1);
	}
	catch(generic_string s)
	{
		s += TEXT("\n\n");
		s += USERMSG;
		if (::MessageBox(NULL, s.c_str(), pluginFilePath.c_str(), MB_YESNO) == IDYES)
		{
			dll2Remove.push_back(pluginFilePath);
		}
		delete pi;
        return -1;
	}
	catch(...)
	{
		generic_string msg = TEXT("Fail loaded");
		msg += TEXT("\n\n");
		msg += USERMSG;
		if (::MessageBox(NULL, msg.c_str(), pluginFilePath.c_str(), MB_YESNO) == IDYES)
		{
			dll2Remove.push_back(pluginFilePath);
		}
		delete pi;
        return -1;
	}
}


bool PluginsManager::loadPlugins(const TCHAR *dir)
{
	if (_isDisabled)
		return false;

	std::vector<generic_string> dllNames;
	std::vector<generic_string> dll2Remove;
	generic_string nppPath = (NppParameters::getInstance())->getNppPath();

	generic_string pluginsFullPathFilter = (dir && dir[0])?dir:nppPath;

	pluginsFullPathFilter += TEXT("\\plugins\\*.dll");

	WIN32_FIND_DATA foundData;
	HANDLE hFindFile = ::FindFirstFile(pluginsFullPathFilter.c_str(), &foundData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		generic_string plugins1stFullPath = (dir && dir[0])?dir:nppPath;
		plugins1stFullPath += TEXT("\\plugins\\");
		plugins1stFullPath += foundData.cFileName;
		dllNames.push_back(plugins1stFullPath);

		while (::FindNextFile(hFindFile, &foundData))
		{
			generic_string fullPath = (dir && dir[0])?dir:nppPath;
			fullPath += TEXT("\\plugins\\");
			fullPath += foundData.cFileName;
			dllNames.push_back(fullPath);
		}
		::FindClose(hFindFile);

		size_t i = 0;

		for ( ; i < dllNames.size() ; i++)
		{
            loadPlugin(dllNames[i].c_str(),  dll2Remove);
		}
        
	}

	for (size_t j = 0 ; j < dll2Remove.size() ; j++)
		::DeleteFile(dll2Remove[j].c_str());

	return true;
}

// return true if cmdID found and its shortcut is enable
// false otherwise
bool PluginsManager::getShortcutByCmdID(int cmdID, ShortcutKey *sk)
{
	if (cmdID == 0 || !sk)
		return false;

	const std::vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();

	for (size_t i = 0 ; i < pluginCmdSCList.size() ; i++)
	{
		if (pluginCmdSCList[i].getID() == cmdID)
		{
			const KeyCombo & kc = pluginCmdSCList[i].getKeyCombo();
			if (kc._key == 0x00)
				return false;

			sk->_isAlt = kc._isAlt;
			sk->_isCtrl = kc._isCtrl;
			sk->_isShift = kc._isShift;
			sk->_key = kc._key;
			return true;
		}
	}
	return false;
}


void PluginsManager::addInMenuFromPMIndex(int i)
{
    std::vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();
	::InsertMenu(_hPluginsMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_pluginInfos[i]->_pluginMenu, _pluginInfos[i]->_pFuncGetName());

    unsigned short j = 0;
	for ( ; j < _pluginInfos[i]->_nbFuncItem ; j++)
	{
		if (_pluginInfos[i]->_funcItems[j]._pFunc == NULL)
		{
			::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
			continue;
		}
		
        _pluginsCommands.push_back(PluginCommand(_pluginInfos[i]->_moduleName.c_str(), j, _pluginInfos[i]->_funcItems[j]._pFunc));
		
		int cmdID = ID_PLUGINS_CMD + (_pluginsCommands.size() - 1);
		_pluginInfos[i]->_funcItems[j]._cmdID = cmdID;
		generic_string itemName = _pluginInfos[i]->_funcItems[j]._itemName;

		if (_pluginInfos[i]->_funcItems[j]._pShKey)
		{
			ShortcutKey & sKey = *(_pluginInfos[i]->_funcItems[j]._pShKey);
            PluginCmdShortcut pcs(Shortcut(itemName.c_str(), sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key), cmdID, _pluginInfos[i]->_moduleName.c_str(), j);
			pluginCmdSCList.push_back(pcs);
			itemName += TEXT("\t");
			itemName += pcs.toString();
		}
		else
		{	//no ShortcutKey is provided, add an disabled shortcut (so it can still be mapped, Paramaters class can still index any changes and the toolbar wont funk out
            Shortcut sc(itemName.c_str(), false, false, false, 0x00);
            PluginCmdShortcut pcs(sc, cmdID, _pluginInfos[i]->_moduleName.c_str(), j);	//VK_NULL and everything disabled, the menu name is left alone
			pluginCmdSCList.push_back(pcs);
		}
		::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, cmdID, itemName.c_str());

		if (_pluginInfos[i]->_funcItems[j]._init2Check)
			::CheckMenuItem(_hPluginsMenu, cmdID, MF_BYCOMMAND | MF_CHECKED);
	}
	/*UNLOAD
    ::InsertMenu(_pluginInfos[i]->_pluginMenu, j++, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
    ::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, ID_PLUGINS_REMOVING + i, TEXT("Remove this plugin"));
	*/
}

void PluginsManager::setMenu(HMENU hMenu, const TCHAR *menuName)
{
	if (hasPlugins())
	{
		//std::vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();
		const TCHAR *nom_menu = (menuName && menuName[0])?menuName:TEXT("Plugins");

        if (!_hPluginsMenu)
        {
		    _hPluginsMenu = ::CreateMenu();
		    ::InsertMenu(hMenu, 9, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_hPluginsMenu, nom_menu);
        }

		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
            addInMenuFromPMIndex(i);
		}
	}
}
