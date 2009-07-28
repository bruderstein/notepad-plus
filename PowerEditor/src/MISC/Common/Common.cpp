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

WcharMbcsConvertor * WcharMbcsConvertor::_pSelf = new WcharMbcsConvertor;

// Set a call back with the handle after init to set the path.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/callbackfunctions/browsecallbackproc.asp
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
};

void systemMessage(const TCHAR *title)
{
  LPVOID lpMsgBuf;
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
				 ::GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL );// Process any inserts in lpMsgBuf.
  MessageBox( NULL, (LPTSTR)lpMsgBuf, title, MB_OK | MB_ICONSTOP);
  ::LocalFree(lpMsgBuf);
}

void printInt(int int2print) 
{
	TCHAR str[32];
	wsprintf(str, TEXT("%d"), int2print);
	::MessageBox(NULL, str, TEXT(""), MB_OK);
};

void printStr(const TCHAR *str2print) 
{
	::MessageBox(NULL, str2print, TEXT(""), MB_OK);
};

void writeLog(const TCHAR *logFileName, const char *log2write)
{	
	FILE *f = NULL;
	generic_fopen(f, logFileName, TEXT("a+"));
	fwrite(log2write, sizeof(log2write[0]), strlen(log2write), f);
	fputc('\n', f);
	fflush(f);
	fclose(f);
}

void folderBrowser(HWND parent, int outputCtrlID, const TCHAR *defaultStr)
{
	// This code was copied and slightly modifed from:
	// http://www.bcbdev.com/faqs/faq62.htm

	// SHBrowseForFolder returns a PIDL. The memory for the PIDL is
	// allocated by the shell. Eventually, we will need to free this
	// memory, so we need to get a pointer to the shell malloc COM
	// object that will free the PIDL later on.
	LPMALLOC pShellMalloc = 0;
	if (::SHGetMalloc(&pShellMalloc) == NO_ERROR)
	{
		// If we were able to get the shell malloc object,
		// then proceed by initializing the BROWSEINFO stuct
		BROWSEINFO info;
		memset(&info, 0, sizeof(info));
		info.hwndOwner = parent;
		info.pidlRoot = NULL;
		TCHAR szDisplayName[MAX_PATH];
		info.pszDisplayName = szDisplayName;
		info.lpszTitle = TEXT("Select a folder to search from");
		info.ulFlags = 0;
		info.lpfn = BrowseCallbackProc;
		TCHAR directory[MAX_PATH];
		::GetDlgItemText(parent, outputCtrlID, directory, sizeof(directory));
		if (!directory[0] && defaultStr)
			info.lParam = reinterpret_cast<LPARAM>(defaultStr);
		else
			info.lParam = reinterpret_cast<LPARAM>(directory);

		// Execute the browsing dialog.
		LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

		// pidl will be null if they cancel the browse dialog.
		// pidl will be not null when they select a folder.
		if (pidl) 
		{
			// Try to convert the pidl to a display generic_string.
			// Return is true if success.
			TCHAR szDir[MAX_PATH];
			if (::SHGetPathFromIDList(pidl, szDir))
				// Set edit control to the directory path.
				::SetDlgItemText(parent, outputCtrlID, szDir);
			pShellMalloc->Free(pidl);
		}
		pShellMalloc->Release();
	}
}

void ClientRectToScreenRect(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};

std::vector<std::generic_string> tokenizeString(const std::generic_string & tokenString, const char delim) {
	//Vector is created on stack and copied on return
	std::vector<std::generic_string> tokens;

    // Skip delimiters at beginning.
	std::string::size_type lastPos = tokenString.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = tokenString.find_first_of(delim, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(tokenString.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = tokenString.find_first_not_of(delim, pos);
        // Find next "non-delimiter"
        pos = tokenString.find_first_of(delim, lastPos);
    }
	return tokens;
}

void ScreenRectToClientRect(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};

int filter(unsigned int code) 
{
   if (code == EXCEPTION_ACCESS_VIOLATION)
      return EXCEPTION_EXECUTE_HANDLER;

   return EXCEPTION_CONTINUE_SEARCH;
}

int getCpFromStringValue(const char * encodingStr)
{
	if (!encodingStr)
		return CP_ACP;

	if (_stricmp("windows-1250", encodingStr) == 0)
		return 1250;
	if (_stricmp("windows-1251", encodingStr) == 0)
		return 1251;
	if (_stricmp("windows-1252", encodingStr) == 0)
		return 1252;
	if (_stricmp("windows-1253", encodingStr) == 0)
		return 1253;
	if (_stricmp("windows-1254", encodingStr) == 0)
		return 1254;
	if (_stricmp("windows-1255", encodingStr) == 0)
		return 1255;
	if (_stricmp("windows-1256", encodingStr) == 0)
		return 1256;
	if (_stricmp("windows-1257", encodingStr) == 0)
		return 1257;
	if (_stricmp("windows-1258", encodingStr) == 0)
		return 1258;

	if (_stricmp("big5", encodingStr) == 0)
		return 950;
	if (_stricmp("gb2312", encodingStr) == 0)
		return 936;
	if (_stricmp("shift_jis", encodingStr) == 0)
		return 932;
	if (_stricmp("euc-kr", encodingStr) == 0)
		return 51949;
	if (_stricmp("tis-620", encodingStr) == 0)
		return 874;

	if (_stricmp("iso-8859-8", encodingStr) == 0)
		return 28598;

	if (_stricmp("utf-8", encodingStr) == 0)
		return 65001;

	return CP_ACP;
}

std::generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand)
{
	TCHAR cleanedName[64] = TEXT("");
	size_t j = 0;
	size_t menuNameLen = lstrlen(menuItemStr);
	for(size_t k = 0 ; k < menuNameLen ; k++) 
	{
		if (menuItemStr[k] == '\t')
		{
			cleanedName[k] = 0;
			break;
		}
		else if (menuItemStr[k] == '&')
		{
			if (keepAmpersand)
				cleanedName[j++] = menuItemStr[k];
			//else skip
		}
		else
		{
			cleanedName[j++] = menuItemStr[k];
		}
	}
	cleanedName[j] = 0;
	return cleanedName;
};

#define initSize 1024

WcharMbcsConvertor::WcharMbcsConvertor() : 
	_multiByteStr(NULL), 
	_multiByteAllocLen(0), 
	_wideCharStr(NULL), 
	_wideCharAllocLen(0)
{
}

WcharMbcsConvertor::~WcharMbcsConvertor()
{
	if (_multiByteStr)
		delete [] _multiByteStr;
	if (_wideCharStr)
		delete [] _wideCharStr;
}

const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, UINT codepage)
{
	if (!_wideCharStr)
	{
		_wideCharStr = new wchar_t[initSize];
		_wideCharAllocLen = initSize;
	}

	int len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, 0);
	if (len > 0)
	{
		if (len > int(_wideCharAllocLen))
		{
			delete [] _wideCharStr;
			_wideCharAllocLen = len;
			_wideCharStr = new wchar_t[_wideCharAllocLen];
		}
		MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, len);
	}
	else
		_wideCharStr[0] = 0;

	return _wideCharStr;
}

// "mstart" and "mend" are pointers to indexes in mbcs2Convert, 
// which are converted to the corresponding indexes in the returned wchar_t string.
const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, UINT codepage, int *mstart, int *mend)
{
	if (!_wideCharStr)
	{
		_wideCharStr = new wchar_t[initSize];
		_wideCharAllocLen = initSize;
	}

	int len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, 0);
	if (len > 0)
	{
		if (len > int(_wideCharAllocLen))
		{
			delete [] _wideCharStr;
			_wideCharAllocLen = len;
			_wideCharStr = new wchar_t[_wideCharAllocLen];
		}
		len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, len);
		*mstart = MultiByteToWideChar(codepage, 0, mbcs2Convert, *mstart, _wideCharStr, 0);
		*mend   = MultiByteToWideChar(codepage, 0, mbcs2Convert, *mend, _wideCharStr, 0);
		if (*mstart >= len || *mend >= len)
		{
			*mstart = 0;
			*mend = 0;
		}
	}
	else
	{
		_wideCharStr[0] = 0;
		*mstart = 0;
		*mend = 0;
	}
	return _wideCharStr;
}

const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, UINT codepage) 
{
	if (!_multiByteStr)
	{
		_multiByteStr = new char[initSize];
		_multiByteAllocLen = initSize;
	}

	int len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, 0, NULL, NULL);
	if (len > 0)
	{
		if (len > int(_multiByteAllocLen))
		{
			delete [] _multiByteStr;
			_multiByteAllocLen = len;
			_multiByteStr = new char[_multiByteAllocLen];
		}
		WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, len, NULL, NULL);
	}
	else
		_multiByteStr[0] = 0;

	return _multiByteStr;
}

const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, UINT codepage, long *mstart, long *mend) 
{
	if (!_multiByteStr)
	{
		_multiByteStr = new char[initSize];
		_multiByteAllocLen = initSize;
	}

	int len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, 0, NULL, NULL);
	if (len > 0)
	{
		if (len > int(_multiByteAllocLen))
		{
			delete [] _multiByteStr;
			_multiByteAllocLen = len;
			_multiByteStr = new char[_multiByteAllocLen];
		}
		len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, len, NULL, NULL); // not needed?
		*mstart = WideCharToMultiByte(codepage, 0, wcharStr2Convert, *mstart, _multiByteStr, 0, NULL, NULL);
		*mend = WideCharToMultiByte(codepage, 0, wcharStr2Convert, *mend, _multiByteStr, 0, NULL, NULL);
		if (*mstart >= len || *mend >= len)
		{
			*mstart = 0;
			*mend = 0;
		}
	}
	else
		_multiByteStr[0] = 0;

	return _multiByteStr;
}

std::wstring string2wstring(const std::string & rString, UINT codepage)
{
	int len = MultiByteToWideChar(codepage, 0, rString.c_str(), -1, NULL, 0);
	if(len > 0)
	{		
		std::vector<wchar_t> vw(len);
		MultiByteToWideChar(codepage, 0, rString.c_str(), -1, &vw[0], len);
		return &vw[0];
	}
	else
		return L"";
}

std::string wstring2string(const std::wstring & rwString, UINT codepage)
{
	int len = WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, NULL, 0, NULL, NULL);
	if(len > 0)
	{		
		std::vector<char> vw(len);
		WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, &vw[0], len, NULL, NULL);
		return &vw[0];
	}
	else
		return "";
}

static TCHAR* convertFileName(TCHAR *buffer, const TCHAR *filename)
{
	TCHAR *b = buffer;
	const TCHAR *p = filename;
	while (*p)
	{
		if (*p == '&') *b++ = '&';
		*b++ = *p++;
	}
	*b = 0;	
	return buffer;
}

TCHAR *BuildMenuFileName(TCHAR *buffer, int len, int pos, const TCHAR *filename)
{
	TCHAR cwd[MAX_PATH];
	buffer[0] = 0;
	GetCurrentDirectory(_countof(cwd), cwd);
	lstrcat(cwd, TEXT("\\"));

	TCHAR *itr = buffer;
	TCHAR *end = buffer + len - 1;
	if (pos < 9)
	{
		*itr++ = '&';
		*itr++ = '1' + (TCHAR)pos;
	}
	else if (pos == 9)
	{
		*itr++ = '1';
		*itr++ = '&';
		*itr++ = '0';
	}
	else
	{
		wsprintf(itr, TEXT("%d"), pos+1);
		itr = itr + lstrlen(itr);
	}
	*itr++ = ':';
	*itr++ = ' ';
	if (0 == generic_strnicmp(filename, cwd, lstrlen(cwd)))
	{
		TCHAR cnvName[MAX_PATH];
		const TCHAR *s1 = PathFindFileName(filename);
		int len = lstrlen(s1);
		if (len < (end-itr))
		{
			lstrcpy(cnvName, s1);
		}
		else
		{
			int n = (len-3-(itr-buffer))/2;
			generic_strncpy(cnvName, s1, n);
			lstrcpy(cnvName+n, TEXT("..."));
			lstrcat(cnvName, s1 + lstrlen(s1) - n);
		}
		convertFileName(itr, cnvName);
	}
	else
	{
		TCHAR cnvName[MAX_PATH*2];
		convertFileName(cnvName, filename);
		PathCompactPathEx(itr, filename, len - (itr-buffer), 0);
	}
	return buffer;
}
