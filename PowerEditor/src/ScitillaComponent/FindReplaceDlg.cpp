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

#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"
#include "constant.h"
#include "common_func.h"
#include "UniConversion.h"

int Searching::convertExtendedToString(const char * query, char * result, int length) {	//query may equal to result, since it always gets smaller
	int i = 0, j = 0;
	int charLeft = length;
	bool isGood = true;
	char current;
	while(i < length) {	//because the backslash escape quences always reduce the size of the string, no overflow checks have to be made for target, assuming parameters are correct
		current = query[i];
		charLeft--;
		if (current == '\\' && charLeft) {	//possible escape sequence
			i++;
			charLeft--;
			current = query[i];
			switch(current) {
				case 'r':
					result[j] = '\r';
					break;
				case 'n':
					result[j] = '\n';
					break;
				case '0':
					result[j] = '\0';
					break;
				case 't':
					result[j] = '\t';
					break;
				case '\\':
					result[j] = '\\';
					break;
				case 'b':
				case 'd':
				case 'o':
				case 'x': {
					int size = 0, base = 0;
					if (current == 'b') {			//11111111
						size = 8, base = 2;
					} else if (current == '0') {	//377
						size = 3, base = 8;
					} else if (current == 'd') {	//255
						size = 3, base = 10;
					} else if (current == 'x') {	//0xFF
						size = 2, base = 16;
					}
					if (charLeft >= size) {
						int res = 0;
						if (Searching::readBase(query+(i+1), &res, base, size)) {
							result[j] = (char)res;
							i+=size;
							break;
						}
					}
					//not enough chars to make parameter, use default method as fallback
					}
				default: {	//unknown sequence, treat as regular text
					result[j] = '\\';
					j++;
					result[j] = current;
					isGood = false;
					break;
				}
			}
		} else {
			result[j] = query[i];
		}
		i++;
		j++;
	}
	result[j] = 0;
	return j;
}

bool Searching::readBase(const char * string, int * value, int base, int size) {
	int i = 0, temp = 0;
	*value = 0;
	char max = '0' + base - 1;
	char current;
	while(i < size) {
		current = string[i];
		if (current >= '0' && current <= max) {
			temp *= base;
			temp += (current - '0');
		} else {
			return false;
		}
		i++;
	}
	*value = temp;
	return true;
}

void Searching::displaySectionCentered(int posStart, int posEnd, ScintillaEditView * pEditView, bool isDownwards) 
{
	// to make sure the found result is visible
	//When searching up, the beginning of the (possible multiline) result is important, when scrolling down the end
	int testPos = (isDownwards)?posEnd:posStart;
	pEditView->execute(SCI_SETCURRENTPOS, testPos);
	int currentlineNumberDoc = (int)pEditView->execute(SCI_LINEFROMPOSITION, testPos);
	int currentlineNumberVis = (int)pEditView->execute(SCI_VISIBLEFROMDOCLINE, currentlineNumberDoc);
	pEditView->execute(SCI_ENSUREVISIBLE, currentlineNumberDoc);	// make sure target line is unfolded

	int firstVisibleLineVis =	(int)pEditView->execute(SCI_GETFIRSTVISIBLELINE);
	int linesVisible =			(int)pEditView->execute(SCI_LINESONSCREEN) - 1;	//-1 for the scrollbar
	int lastVisibleLineVis =	(int)linesVisible + firstVisibleLineVis;
	
	//if out of view vertically, scroll line into (center of) view
	int linesToScroll = 0;
	if (currentlineNumberVis < firstVisibleLineVis)
	{
		linesToScroll = currentlineNumberVis - firstVisibleLineVis;
		//use center
		linesToScroll -= linesVisible/2;		
	}
	else if (currentlineNumberVis > lastVisibleLineVis)
	{
		linesToScroll = currentlineNumberVis - lastVisibleLineVis;
		//use center
		linesToScroll += linesVisible/2;
	}
	pEditView->scroll(0, linesToScroll);

	//Make sure the caret is visible, scroll horizontally (this will also fix wrapping problems)
	pEditView->execute(SCI_GOTOPOS, posStart);
	pEditView->execute(SCI_GOTOPOS, posEnd);
	//pEditView->execute(SCI_SETSEL, start, posEnd);	
	//pEditView->execute(SCI_SETCURRENTPOS, posEnd);
	pEditView->execute(SCI_SETANCHOR, posStart);	
}

void FindReplaceDlg::addText2Combo(const char * txt2add, HWND hCombo, bool isUTF8)
{	
	if (!hCombo) return;
	if (!strcmp(txt2add, "")) return;

	bool bMustDie9x = _winVer <= WV_ME;
	char text[MAX_PATH];
	WCHAR textW[MAX_PATH];

	int count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	bool hasFound = false;
	int i = 0;

	WCHAR wchars2Add[MAX_PATH];
	if (isUTF8)
		::MultiByteToWideChar(CP_UTF8, 0, txt2add, -1, wchars2Add, MAX_PATH - 1);

	for ( ; i < count ; i++)
	{
		
		if (isUTF8)
		{
			if ( !bMustDie9x )
			{
				::SendMessageW(hCombo, CB_GETLBTEXT, i, (LPARAM)textW);
			}
			else
			{
				::SendMessageA(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
				::MultiByteToWideChar(CP_ACP, 0, text, -1, textW, MAX_PATH - 1);
			}
			if (!wcscmp(wchars2Add, textW))
			{
				hasFound = true;
				break;
			}
		}
		else
		{
			::SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
			if (!strcmp(txt2add, text))
			{
				hasFound = true;
				break;
			}
		}
	}

	if (!hasFound)
	{
		if (!isUTF8)
			i = ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)txt2add);
		else
		{
			if ( !bMustDie9x )
			{
				i = ::SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)wchars2Add);
			}
			else
			{
				::WideCharToMultiByte(CP_ACP, 0, wchars2Add, -1, text, MAX_PATH - 1, NULL, NULL);
				i = ::SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)text);
			}
		}
	}

	::SendMessage(hCombo, CB_SETCURSEL, i, 0);
}

string FindReplaceDlg::getTextFromCombo(HWND hCombo, bool isUnicode) const
{	
	bool bMustDie9x = _winVer <= WV_ME;
	char str[MAX_PATH];
	if (isUnicode)
	{
		WCHAR wchars[MAX_PATH];
		if ( !bMustDie9x )
		{
			::SendMessageW(hCombo, WM_GETTEXT, MAX_PATH - 1, (LPARAM)wchars);
		}
		else
		{
			char achars[MAX_PATH];
			::SendMessageA(hCombo, WM_GETTEXT, MAX_PATH - 1, (LPARAM)achars);
			::MultiByteToWideChar(CP_ACP, 0, achars, -1, wchars, MAX_PATH - 1);
		}
		::WideCharToMultiByte(CP_UTF8, 0, wchars, -1, str, MAX_PATH - 1, NULL, NULL);
	}
	else
	{
		::SendMessage(hCombo, WM_GETTEXT, MAX_PATH - 1, (LPARAM)str);
	}
	return string(str);
}

// important : to activate all styles
const int STYLING_MASK = 255;

void FindReplaceDlg::create(int dialogID, bool isRTL) 
{
	StaticDialog::create(dialogID, isRTL);
	_currentStatus = REPLACE_DLG;

	initOptionsFromDlg();

	if ((NppParameters::getInstance())->isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_SHOW);
		
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
		if (!isCheckedOrNot(IDC_TRANSPARENT_CHECK))
		{
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), FALSE);
		}
	}
	RECT rect;
	//::GetWindowRect(_hSelf, &rect);
	getClientRect(rect);
	_tab.init(_hInst, _hSelf, false, false, true);
	_tab.setFont("Tahoma", 13);
	
	const char *find = "Find";
	const char *replace = "Replace";
	const char *findInFiles = "Find in files";

	NppParameters::FindDlgTabTitiles & fdTitles = NppParameters::getInstance()->getFindDlgTabTitiles();
		
	if (fdTitles.isWellFilled())
	{
		find = fdTitles._find.c_str();
		replace = fdTitles._replace.c_str();
		findInFiles = fdTitles._findInFiles.c_str();
	}
	_tab.insertAtEnd(find);
	_tab.insertAtEnd(replace);
	_tab.insertAtEnd(findInFiles);

	_tab.reSizeTo(rect);
	_tab.display();

	ETDTProc enableDlgTheme = (ETDTProc)::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
	if (enableDlgTheme)
		enableDlgTheme(_hSelf, ETDT_ENABLETAB);

	goToCenter();
}

void FindReplaceDlg::updateCombos()
{
	bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;
	HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
	addText2Combo(getTextFromCombo(hReplaceCombo, isUnicode).c_str(), hReplaceCombo, isUnicode);

	HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
	addText2Combo(getTextFromCombo(hFindCombo, isUnicode).c_str(), hFindCombo, isUnicode);
}

bool Finder::notify(SCNotification *notification)
{
	switch (notification->nmhdr.code) 
	{
		case SCN_MARGINCLICK: 
		{   
			if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
			{
				_scintView.marginClick(notification->position, notification->modifiers);
			}
			break;
		}

		case SCN_DOUBLECLICK :
		{
			try {
				int currentPos = _scintView.execute(SCI_GETCURRENTPOS);
				if (currentPos)
				{
					char prevChar = (char)_scintView.execute(SCI_GETCHARAT, currentPos - 1);
					if (prevChar == 0x0A)
						currentPos -= 2;	
				}
				
				int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
				int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
				int end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);

				if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
				{
					_scintView.execute(SCI_TOGGLEFOLD, lno);
					_scintView.execute(SCI_SETCURRENTPOS, start);
					_scintView.execute(SCI_SETANCHOR, start);
					return false;
				}

				// in getInfo() method the previous line is renew as current for next call
				const FoundInfo &fInfo = getInfo(lno);

				int markedLine = getCurrentMarkedLine();

				// now we clean the previous mark
				if (markedLine != -1)
					(*_ppEditView)->execute(SCI_MARKERDELETE, markedLine, MARK_BOOKMARK);

				// After cleaning the previous mark, we can swich to another document

				int cmd = getMode()==FILES_IN_DIR?WM_DOOPEN:NPPM_SWITCHTOFILE;

				::SendMessage(::GetParent(_hParent), cmd, 0, (LPARAM)fInfo._fullPath.c_str());
				Searching::displaySectionCentered(fInfo._start, fInfo._end, *_ppEditView);

				// we set the current mark here
				int nb = (*_ppEditView)->getCurrentLineNumber();
				setCurrentMarkedLine(nb);
				(*_ppEditView)->execute(SCI_MARKERADD, nb, MARK_BOOKMARK);

				// Then we colourise the double clicked line
				setFinderStyle();
				_scintView.showMargin(ScintillaEditView::_SC_MARGE_FOLDER, true);
				_scintView.execute(SCI_SETLEXER, SCLEX_NULL);
				_scintView.execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_KWORD3, true);
				
				// 
				_scintView.execute(SCI_STARTSTYLING,  start,  STYLING_MASK);
				_scintView.execute(SCI_SETSTYLING,  end - start + 2, SCE_SEARCHRESULT_KWORD3);
				_scintView.execute(SCI_COLOURISE, start, end + 1);
				_scintView.execute(SCI_SETCURRENTPOS, start);
				_scintView.execute(SCI_SETANCHOR, start);
				return true;

			} catch(...){
				printStr("SCN_DOUBLECLICK problem");
			}
			break;
		}

		default :
			break;
	}
	return false;
}


BOOL CALLBACK FindReplaceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			// Wrap arround active by default
			::SendDlgItemMessage(_hSelf, IDWRAP, BM_SETCHECK, BST_CHECKED, 0);
			// Normal search active by default
			::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, BST_CHECKED, 0);

			if (_isRecursive)
				::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK, BM_SETCHECK, BST_CHECKED, 0);

			RECT arc;
			::GetWindowRect(::GetDlgItem(_hSelf, IDCANCEL), &arc);
			_findInFilesClosePos.bottom = _replaceClosePos.bottom = _findClosePos.bottom = arc.bottom - arc.top;
			_findInFilesClosePos.right = _replaceClosePos.right = _findClosePos.right = arc.right - arc.left;

			POINT p;
			p.x = arc.left;
			p.y = arc.top;
			::ScreenToClient(_hSelf, &p);

			_replaceClosePos.left = p.x;
			_replaceClosePos.top = p.y;

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDREPLACE));
			 _findInFilesClosePos.left = p.x;
			 _findInFilesClosePos.top = p.y;

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES));
			 _findClosePos.left = p.x;
			 _findClosePos.top = p.y + 10;
			return TRUE;
		}
		
		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER))
			{
				if (isCheckedOrNot(IDC_TRANSPARENT_ALWAYS_RADIO))
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
				}
			}
			return TRUE;
		}

		case WM_NOTIFY:
		{
			NMHDR *nmhdr = (NMHDR *)lParam;
			if (nmhdr->code == TCN_SELCHANGE)
			{
				HWND tabHandle = _tab.getHSelf();
				if (nmhdr->hwndFrom == tabHandle)
				{
					int indexClicked = int(::SendMessage(tabHandle, TCM_GETCURSEL, 0, 0));
					doDialog((DIALOG_TYPE)indexClicked);
					if ((DIALOG_TYPE)indexClicked == FINDINFILES_DLG)
					{
						char currentDir[MAX_PATH];
						::GetCurrentDirectory(MAX_PATH, currentDir);
						setFindInFilesDirFilter(currentDir, NULL);
					}
				}
				return TRUE;
			}
			break;
		}

		case WM_ACTIVATE :
		{
			CharacterRange cr = (*_ppEditView)->getSelection();
			int nbSelected = cr.cpMax - cr.cpMin;

			int checkVal;
			if (nbSelected <= 64)
			{
				checkVal = BST_UNCHECKED;
				_isInSelection = false;
			}
			else
			{
				checkVal = BST_CHECKED;
				_isInSelection = true;
			}
			::SendDlgItemMessage(_hSelf, IDC_IN_SELECTION_CHECK, BM_SETCHECK, checkVal, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), nbSelected);


			if (isCheckedOrNot(IDC_TRANSPARENT_LOSSFOCUS_RADIO))
			{
				if (wParam == WA_INACTIVE)
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
				}
				else
				{
					(NppParameters::getInstance())->removeTransparent(_hSelf);
				}
			}
			return TRUE;
		}

		case NPPM_MODELESSDIALOG :
			return ::SendMessage(_hParent, NPPM_MODELESSDIALOG, wParam, lParam);

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL : // Close
					display(false);
					return TRUE;
				
//Single actions
				case IDOK : // Find Next
				{
					if ((_currentStatus == FIND_DLG) || (_currentStatus == REPLACE_DLG))
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						string str2Search = getTextFromCombo(hFindCombo, isUnicode);
						updateCombo(IDFINDWHAT);
						processFindNext(str2Search.c_str());
					}
					else if (_currentStatus == FINDINFILES_DLG)
					{
						::SendMessage(_hSelf, WM_COMMAND, IDD_FINDINFILES_FIND_BUTTON, (LPARAM)_hSelf);
					}
				}
				return TRUE;

				case IDREPLACE :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						string str2Search = getTextFromCombo(hFindCombo, isUnicode);
						string str2Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombos();
						processReplace(str2Search.c_str(), str2Replace.c_str());
					}
				}
				return TRUE;
//Process actions
				case IDC_FINDALL_OPENEDFILES :
				{
					if (_currentStatus == FIND_DLG)
					{
						updateCombo(IDFINDWHAT);
						findAllIn(ALL_OPEN_DOCS);
					}
				}
				return TRUE;

				case IDD_FINDINFILES_FIND_BUTTON :
				{
					if (_currentStatus == FINDINFILES_DLG)
					{
						char filters[256];
						char directory[MAX_PATH];
						::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, sizeof(filters));
						addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
						_filters = filters;

						::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, sizeof(directory));
						addText2Combo(directory, ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
						_directory = directory;
						
						if ((strlen(directory) > 0) && (directory[strlen(directory)-1] != '\\'))
							_directory += "\\";

						updateCombo(IDFINDWHAT);
						findAllIn(FILES_IN_DIR);
					}
					
				}
				return TRUE;

				case IDC_REPLACE_OPENEDFILES :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						updateCombos();
						replaceAllInOpenedDocs();
					}
				}			
				return TRUE;

				case IDREPLACEALL :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						updateCombos();

						(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
						int nbReplaced = processAll(ProcessReplaceAll, NULL, NULL);
						(*_ppEditView)->execute(SCI_ENDUNDOACTION);

						char result[64];
						if (nbReplaced < 0)
							strcpy(result, "The regular expression to search is formed badly");
						else
						{
							itoa(nbReplaced, result, 10);
							strcat(result, " tokens are replaced.");
						}
						::MessageBox(_hSelf, result, "", MB_OK);
					}
				}
				return TRUE;

				case IDCCOUNTALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						int nbCounted = processAll(ProcessCountAll, NULL, NULL);
						char result[128];
						if (nbCounted < 0)
							strcpy(result, "The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
						else
				{
							itoa(nbCounted, result, 10);
							strcat(result, " tokens are found.");
					}
						::MessageBox(_hSelf, result, "", MB_OK);
				}
				}
				return TRUE;

				case IDCMARKALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						updateCombo(IDFINDWHAT);

						int nbMarked = processAll(ProcessMarkAll, NULL, NULL);
						char result[128];
						if (nbMarked < 0)
							strcpy(result, "The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
						else
						{
							itoa(nbMarked, result, 10);
							strcat(result, " tokens are found.");
						}
						::MessageBox(_hSelf, result, "", MB_OK);
					}
				}
				return TRUE;

				case IDC_CLEAR_ALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
						(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
					}
				}
				return TRUE;
//Option actions
				case IDWHOLEWORD :
					_options._isWholeWord = isCheckedOrNot(IDWHOLEWORD);
					return TRUE;

				case IDMATCHCASE :
					_options._isMatchCase = isCheckedOrNot(IDMATCHCASE);
					return TRUE;

				case IDNORMAL:
				case IDEXTENDED:
				case IDREGEXP : {
					_options._searchType = isCheckedOrNot(IDREGEXP)?FindRegex:isCheckedOrNot(IDEXTENDED)?FindExtended:FindNormal;

					bool isRegex = (_options._searchType == FindRegex);
					if (isRegex) {	//regex doesnt allow wholeword
						_options._isWholeWord = false;
					::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, _options._isWholeWord?BST_CHECKED:BST_UNCHECKED, 0);
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD), (BOOL)!isRegex);

					if (isRegex) {	//regex doesnt allow upward search
						::SendDlgItemMessage(_hSelf, IDDIRECTIONDOWN, BM_SETCHECK, BST_CHECKED, 0);
					::SendDlgItemMessage(_hSelf, IDDIRECTIONUP, BM_SETCHECK, BST_UNCHECKED, 0);
					_options._whichDirection = DIR_DOWN;
					}
					::EnableWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), (BOOL)!isRegex);
					return TRUE; }

				case IDWRAP :
					_options._isWrapAround = isCheckedOrNot(IDWRAP);
					return TRUE;

				case IDDIRECTIONUP :
				case IDDIRECTIONDOWN :
					_options._whichDirection = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), BM_GETCHECK, BST_CHECKED, 0));
					return TRUE;

				case IDC_PURGE_CHECK :
				{
					if (_currentStatus == FIND_DLG)
						_doPurge = isCheckedOrNot(IDC_PURGE_CHECK);
				}
				return TRUE;

				case IDC_MARKLINE_CHECK :
				{
					if (_currentStatus == FIND_DLG)
					{
						_doMarkLine = isCheckedOrNot(IDC_MARKLINE_CHECK);
						::EnableWindow(::GetDlgItem(_hSelf, IDCMARKALL), (_doMarkLine || _doStyleFoundToken));
					}
				}
				return TRUE;

				case IDC_STYLEFOUND_CHECK :
				{
					if (_currentStatus == FIND_DLG)
					{
						_doStyleFoundToken = isCheckedOrNot(IDC_STYLEFOUND_CHECK);
						::EnableWindow(::GetDlgItem(_hSelf, IDCMARKALL), (_doMarkLine || _doStyleFoundToken));
					}
				}
				return TRUE;

				case IDC_IN_SELECTION_CHECK :
				{
					if (_currentStatus == REPLACE_DLG)
						_isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK);
				}
				return TRUE;

				case IDC_TRANSPARENT_CHECK :
				{
					bool isChecked = isCheckedOrNot(IDC_TRANSPARENT_CHECK);

					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), isChecked);

					if (isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_CHECKED, 0);
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						(NppParameters::getInstance())->removeTransparent(_hSelf);
					}

					return TRUE;
				}

				case IDC_TRANSPARENT_ALWAYS_RADIO :
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
				}
				return TRUE;

				case IDC_TRANSPARENT_LOSSFOCUS_RADIO :
				{
					(NppParameters::getInstance())->removeTransparent(_hSelf);
				}
				return TRUE;

				//
				// Find in Files
				//
				case IDD_FINDINFILES_RECURSIVE_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						_isRecursive = isCheckedOrNot(IDD_FINDINFILES_RECURSIVE_CHECK);
					
				}
				return TRUE;

				case IDD_FINDINFILES_INHIDDENDIR_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						_isInHiddenDir = isCheckedOrNot(IDD_FINDINFILES_INHIDDENDIR_CHECK);
					
				}
				return TRUE;
				case IDD_FINDINFILES_BROWSE_BUTTON :
				{
					if (_currentStatus == FINDINFILES_DLG)
						folderBrowser(_hSelf, IDD_FINDINFILES_DIR_COMBO);	
				}
				return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;
}

// return value :
// true  : the text2find is found
// false : the text2find is not found
bool FindReplaceDlg::processFindNext(const char *txt2find, FindOption *options)
{
	if (!txt2find || !txt2find[0])
		return false;

	FindOption *pOptions = options?options:&_options;

	int stringSizeFind = strlen(txt2find);
	char *pText = new char[stringSizeFind + 1];
	strcpy(pText, txt2find);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pText, stringSizeFind);
	}

	int docLength = int((*_ppEditView)->execute(SCI_GETLENGTH));
	CharacterRange cr = (*_ppEditView)->getSelection();

	//The search "zone" is relative to the selection, so search happens 'outside'
	int startPosition = cr.cpMax;
	int endPosition = docLength;

	if (pOptions->_whichDirection == DIR_UP)
	{
		//When searching upwards, start is the lower part, end the upper, for backwards search
		startPosition = cr.cpMin - 1;
		endPosition = 0;
	}

	if (pOptions->_isIncremental)
	{
		startPosition = 0;
		endPosition = docLength;	
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);
	(*_ppEditView)->execute(SCI_SETTARGETSTART, startPosition);
	(*_ppEditView)->execute(SCI_SETTARGETEND, endPosition);
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);

	int posFind =			int((*_ppEditView)->execute(SCI_SEARCHINTARGET, stringSizeFind, (LPARAM)pText));
	if (posFind == -1) //no match found in target, check if a new target should be used
	{
		if (pOptions->_isWrapAround) 
		{
			//when wrapping, use the rest of the document (entire document is usable)
			if (pOptions->_whichDirection == DIR_DOWN)
			{
				startPosition = 0;
				endPosition = docLength;
			}
			else
			{
				startPosition = docLength;
				endPosition = 0;
			}

			//new target, search again
			(*_ppEditView)->execute(SCI_SETTARGETSTART, startPosition);
			(*_ppEditView)->execute(SCI_SETTARGETEND, endPosition);
			posFind = int((*_ppEditView)->execute(SCI_SEARCHINTARGET, stringSizeFind, (LPARAM)pText));
		}
		if (posFind == -1)
		{
			//failed, or failed twice with wrap
			
			if (!pOptions->_isIncremental) {	//incremental search doesnt trigger messages
				const char stringMaxSize = 64;
				char message[30 + stringMaxSize + 4];	//message, string, dots
			strcpy(message, "Can't find the text:\r\n\"");
				strncat(message, pText, stringMaxSize);
				strcat(message, "\"");
				if (strlen(pText) > stringMaxSize) {
				strcat(message, "...");
			}
			::MessageBox(_hSelf, message, "Find", MB_OK);
			// if the dialog is not shown, pass the focus to his parent(ie. Notepad++)
			if (!::IsWindowVisible(_hSelf))
				::SetFocus((*_ppEditView)->getHSelf());
			}
			delete [] pText;
			return false;
		}
	}

	int start =				posFind;//int((*_ppEditView)->execute(SCI_GETTARGETSTART));
	int end =				int((*_ppEditView)->execute(SCI_GETTARGETEND));

	// to make sure the found result is visible
	Searching::displaySectionCentered(start, end, *_ppEditView, pOptions->_whichDirection == DIR_DOWN);


	delete [] pText;
	return true;
}

// return value :
// true  : the text is replaced, and find the next occurrence
// false : the text2find is not found, so the text is NOT replace
//      || the text is replaced, and do NOT find the next occurrence
bool FindReplaceDlg::processReplace(const char *txt2find, const char *txt2replace, FindOption *options)
{
	if (!txt2find || !txt2find[0] || !txt2replace)
		return false;

	FindOption *pOptions = options?options:&_options;

	if ((*_ppEditView)->getCurrentBuffer().isReadOnly()) return false;

	int stringSizeFind = strlen(txt2find);
	int stringSizeReplace = strlen(txt2replace);
	char *pTextFind = new char[stringSizeFind + 1];
	char *pTextReplace = new char[stringSizeReplace + 1];
	strcpy(pTextFind, txt2find);
	strcpy(pTextReplace, txt2replace);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pTextFind, stringSizeFind);
		stringSizeReplace = Searching::convertExtendedToString(txt2replace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);
	CharacterRange cr = (*_ppEditView)->getSelection();
	
	(*_ppEditView)->execute(SCI_SETTARGETSTART, cr.cpMin);
	(*_ppEditView)->execute(SCI_SETTARGETEND, cr.cpMax);
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);

	int posFind = int((*_ppEditView)->execute(SCI_SEARCHINTARGET, (WPARAM)stringSizeFind, (LPARAM)pTextFind));

	if (posFind != -1)
	{
		if (isRegExp)
		{
			//For the rare re exp case. ex: replace ^ by AAA
			int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
			int end = int((*_ppEditView)->execute(SCI_GETTARGETEND));
			int foundTextLen = (end >= start)?end - start:start - end;

			int replacedLen = (*_ppEditView)->execute(SCI_REPLACETARGETRE, stringSizeReplace, (LPARAM)pTextReplace);
			
			if (!foundTextLen)
				(*_ppEditView)->execute(SCI_SETSEL, start, start + replacedLen);
		}
		else
		{
			(*_ppEditView)->execute(SCI_REPLACETARGET, stringSizeReplace, (LPARAM)pTextReplace);
		}
	}

	delete [] pTextFind;
	delete [] pTextReplace;
	return processFindNext(txt2find);	//after replacing, find the next section for selection
}

int FindReplaceDlg::markAll(const char *txt2find)
{
	_doStyleFoundToken = true;
	int nbFound = processAll(ProcessMarkAll, txt2find, NULL, true, NULL);
	return nbFound;
}

int FindReplaceDlg::markAll2(const char *txt2find)
{
	FindOption opt;
	opt._isMatchCase = false;
	opt._isWholeWord = true;
	int nbFound = processAll(ProcessMarkAll_2, txt2find, NULL, true, NULL, &opt);
	return nbFound;
}

int FindReplaceDlg::markAllInc(const char *txt2find, FindOption *opt)
{
	int nbFound = processAll(ProcessMarkAll_IncSearch, txt2find, NULL, true, NULL, opt);
	return nbFound;
}

int FindReplaceDlg::processAll(ProcessOperation op, const char *txt2find, const char *txt2replace, bool isEntire, const char *fileName, FindOption *opt)
{
	int nbReplaced = 0;
	
	if (!isCreated() && !txt2find)
		return nbReplaced;

	if ((op == ProcessReplaceAll) && (*_ppEditView)->getCurrentBuffer().isReadOnly())
		return nbReplaced;

	if (!fileName)
		fileName = "";

	FindOption *pOptions = opt?opt:&_options;
	bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;

	int stringSizeFind = 0;
	int stringSizeReplace = 0;

	char *pTextFind = NULL;//new char[stringSizeFind + 1];
	if (!txt2find) {
		HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
		string str2Search = getTextFromCombo(hFindCombo, isUnicode);
		stringSizeFind = str2Search.length();
		pTextFind = new char[stringSizeFind + 1];
		strcpy(pTextFind, str2Search.c_str());
	} else {
		stringSizeFind = strlen(txt2find);
		pTextFind = new char[stringSizeFind + 1];
		strcpy(pTextFind, txt2find);
	}

	if (!pTextFind[0]) {
		delete [] pTextFind;
		return nbReplaced;
	}

	char *pTextReplace = NULL;
	if (op == ProcessReplaceAll) {
		if (!txt2replace) {
			HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
			string str2Replace = getTextFromCombo(hReplaceCombo, isUnicode);
			stringSizeReplace = str2Replace.length();
			pTextReplace = new char[stringSizeReplace + 1];
			strcpy(pTextReplace, str2Replace.c_str());
		} else {
			stringSizeReplace = strlen(txt2replace);
			pTextReplace = new char[stringSizeReplace + 1];
			strcpy(pTextReplace, txt2replace);
		}
	}	

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(pTextFind, pTextFind, stringSizeFind);
		if (op == ProcessReplaceAll)
			stringSizeReplace = Searching::convertExtendedToString(pTextReplace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);
/*
	if (op == ProcessMarkAll_2)
		flags = SCFIND_WHOLEWORD;
	else if (op == ProcessMarkAll_IncSearch)
		flags ^= SCFIND_WHOLEWORD;
*/
	CharacterRange cr = (*_ppEditView)->getSelection();
	int docLength = int((*_ppEditView)->execute(SCI_GETLENGTH));

	// Default : 
	//        direction : down
	//        begin at : 0
	//        end at : end of doc
	int startPosition = 0;
	int endPosition = docLength;

	bool direction = pOptions->_whichDirection;

	//first try limiting scope by direction
	if (direction == DIR_UP)	
	{
		startPosition = cr.cpMax;
		endPosition = 0;
	}
	else
	{
		startPosition = cr.cpMin;
		endPosition = docLength;
	}

	//then adjust scope if the full document needs to be changed
	if (pOptions->_isWrapAround || isEntire || (op == ProcessCountAll))	//entire document needs to be scanned
	{		
		startPosition = 0;
		endPosition = docLength;
		direction = DIR_DOWN;
	}
	
	//then readjust scope if the selection override is active and allowed
	if ((_isInSelection) && ((op == ProcessMarkAll) || ((op == ProcessReplaceAll) && (!isEntire))))	//if selection limiter and either mark all or replace all w/o entire document override
	{
		startPosition = cr.cpMin;
		endPosition = cr.cpMax;
	}

	(*_ppEditView)->execute(SCI_SETTARGETSTART, startPosition);
	(*_ppEditView)->execute(SCI_SETTARGETEND, endPosition);
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);

	if (op == ProcessMarkAll)	//if marking, check if purging is needed
	{
		if (_doPurge) {
			if (_doMarkLine)
				(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);

			if (_doStyleFoundToken)
				(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
		}
	}

	int posFind = int((*_ppEditView)->execute(SCI_SEARCHINTARGET, (WPARAM)stringSizeFind, (LPARAM)pTextFind));
	
	if ((posFind != -1) && (op == ProcessFindAll))	//add new filetitle if this file results in hits
	{
		const int fileNameLen = strlen(fileName);

		if (fileNameLen > _fileNameLenMax)
		{
			_fileNameLenMax = fileNameLen;

			delete [] _uniFileName;
			_uniFileName = new char[(fileNameLen + 3) * 2 + 1];
		}
		ascii_to_utf8(fileName, fileNameLen, _uniFileName);
		_pFinder->addFileNameTitle(_uniFileName);
	}
	while (posFind != -1)
	{
		//int posFindBefore = posFind;
		int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
		int end = int((*_ppEditView)->execute(SCI_GETTARGETEND));
		int foundTextLen = (end >= start)?end - start:start - end;

		// Search resulted in empty token, problematic (can this happen?)!!!
		if (!foundTextLen) {
			delete [] pTextFind;
			delete [] pTextReplace;
			return -1;
		}
		
		switch (op) {
			case ProcessFindAll: {
				int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, posFind);
				int lend = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, lineNumber);
				int lstart = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, lineNumber);
				int nbChar = lend - lstart;
				bool isRealloc = false;

				if (_maxNbCharAllocated < nbChar)	//line longer than buffer, resize buffer
				{
					isRealloc = true;
					_maxNbCharAllocated = nbChar;
					delete [] _line;
					_line = new char[_maxNbCharAllocated + 3];
					if (isUnicode)					//if unicode, also resize unicode buffer
					{
						const int uniCharLen = (_maxNbCharAllocated + 3) * 2 + 1;
						delete [] _uniCharLine;
						_uniCharLine = new char[uniCharLen];
					}
				}
				(*_ppEditView)->execute(SCI_GETLINE, lineNumber, (LPARAM)_line);

				_line[nbChar] = 0x0D;
				_line[nbChar+1] = 0x0A;
				_line[nbChar+2] = '\0';

				const char *pLine;
				if (isUnicode)
				{
					ascii_to_utf8(_line, (nbChar + 3), _uniCharLine);
					pLine = _uniCharLine;
				}
				else
				{
					pLine = _line;
				}
				_pFinder->add(FoundInfo(start, end, pLine, fileName, _pFinder->_lineCounter), lineNumber + 1);

				startPosition = posFind + foundTextLen;
				break; }

			case ProcessReplaceAll: {
				(*_ppEditView)->execute(SCI_SETTARGETSTART, start);
				(*_ppEditView)->execute(SCI_SETTARGETEND, end);
				int replacedLength = (*_ppEditView)->execute(isRegExp?SCI_REPLACETARGETRE:SCI_REPLACETARGET, (WPARAM)stringSizeReplace, (LPARAM)pTextReplace);

				startPosition = (direction == DIR_UP)?posFind - replacedLength:posFind + replacedLength;
				if ((_isInSelection) && (!isEntire))
				{
					endPosition = endPosition - foundTextLen + replacedLength;
				}
				else
				{
					if (direction == DIR_DOWN)
						endPosition = docLength = docLength - foundTextLen + replacedLength;
				}
				break; }

			case ProcessMarkAll: {
				if (_doStyleFoundToken)
				{
					(*_ppEditView)->execute(SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_FOUND_STYLE);
					(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  start, end - start);
				}

				if (_doMarkLine)
				{
					int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, posFind);
					int state = (*_ppEditView)->execute(SCI_MARKERGET, lineNumber);

					if (!(state & (1 << MARK_BOOKMARK)))
						(*_ppEditView)->execute(SCI_MARKERADD, lineNumber, MARK_BOOKMARK);
				}
				startPosition = (direction == DIR_UP)?posFind - foundTextLen:posFind + foundTextLen;
				break; }

			case ProcessMarkAll_2: {
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_2);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  start, end - start);
				
				startPosition = (direction == DIR_UP)?posFind - foundTextLen:posFind + foundTextLen;
				break; }

			case ProcessMarkAll_IncSearch: {
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_INC);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  start, end - start);
				
				startPosition = (direction == DIR_UP)?posFind - foundTextLen:posFind + foundTextLen;
				break; }

			case ProcessCountAll: {
				startPosition = posFind + foundTextLen;
				break; }

			default: {
				delete [] pTextFind;
				delete [] pTextReplace;
				return nbReplaced;
				break; }
			
		}	

		(*_ppEditView)->execute(SCI_SETTARGETSTART, startPosition);
		(*_ppEditView)->execute(SCI_SETTARGETEND, endPosition);

		posFind = int((*_ppEditView)->execute(SCI_SEARCHINTARGET, (WPARAM)stringSizeFind, (LPARAM)pTextFind));
		nbReplaced++;
	}

	delete [] pTextFind;
	delete [] pTextReplace;

	return nbReplaced;
}

void FindReplaceDlg::replaceAllInOpenedDocs()
{
	::SendMessage(_hParent, WM_REPLACEALL_INOPENEDDOC, 0, 0);
}

void FindReplaceDlg::findAllIn(InWhat op)
{
	//HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, "findInFilesEvent");
	if (!_pFinder)
	{
		_pFinder = new Finder;
		_pFinder->init(_hInst, _hSelf, _ppEditView);
		
		tTbData	data = {0};
		_pFinder->create(&data);
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO;
		data.hIconTab = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_FIND_RESULT_ICON), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszAddInfo = _findAllResultStr;

		data.pszModuleName = "dummy";

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		data.dlgID = 0;
		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);

		_pFinder->_scintView.init(_hInst, _pFinder->getHSelf());
		_pFinder->_scintView.execute(SCI_SETCODEPAGE, SC_CP_UTF8);
		_pFinder->_scintView.execute(SCI_USEPOPUP, FALSE);

		//_statusBar.init(_hInst, _hSelf, 0);
		RECT findRect;

		//const int scintEditInitHeight = 130;

		// get the width of FindDlg
		::GetWindowRect(_pFinder->getHSelf(), &findRect);

		// overwrite some default settings
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, false);

		_pFinder->_scintView.setMakerStyle(FOLDER_STYLE_SIMPLE);

		//_pFinder->_scintView.execute(SCI_SETZOOM, _pFinder->_scintView.execute(SCI_GETZOOM) - 2);
		_pFinder->_scintView.display();
		_pFinder->display();
	}
	_pFinder->setFinderStyle();
	_pFinder->setMode(op);
	
	::SendMessage(_pFinder->getHSelf(), WM_SIZE, 0, 0);
	
	int finInFileOpt = _isRecursive?FIND_RECURSIVE:0;
	if (_isRecursive)
		finInFileOpt |= _isInHiddenDir?FIND_INHIDDENDIR:0;
	
	::SendMessage(_hParent, (op==ALL_OPEN_DOCS)?WM_FINDALL_INOPENEDDOC:WM_FINDINFILES, 0, (op!=ALL_OPEN_DOCS)?finInFileOpt:0);
	//char *pDataToWrite = _findAllResultStr + strlen(FIND_RESULT_DEFAULT_TITLE);
	sprintf(_findAllResultStr, "%d hits", _findAllResult);
	::SendMessage(_hParent, NPPM_DMMSHOW, 0, (LPARAM)_pFinder->getHSelf());
}

void FindReplaceDlg::enableReplaceFunc(bool isEnable) 
{
	_currentStatus = isEnable?REPLACE_DLG:FIND_DLG;
	int hideOrShow = isEnable?SW_SHOW:SW_HIDE;
	RECT *pClosePos = isEnable?&_replaceClosePos:&_findClosePos;

	//::EnableWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FIND_BUTTON), FALSE);
	//::EnableWindow(::GetDlgItem(_hSelf, IDOK), TRUE);
	enableFindInFilesControls(false);

	// replce controls
	::ShowWindow(::GetDlgItem(_hSelf, ID_STATICTEXT_REPLACE),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACE),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEALL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEINSEL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION),hideOrShow);

	// find controls
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), !hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_STATIC),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDCMARKALL),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_MARKLINE_CHECK),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STYLEFOUND_CHECK),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PURGE_CHECK),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CLEAR_ALL),!hideOrShow);
//::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDINFILES),!hideOrShow);

	gotoCorrectTab();

	::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), pClosePos->left, pClosePos->top, pClosePos->right, pClosePos->bottom, TRUE);

	char label[MAX_PATH];
	_tab.getCurrentTitle(label, sizeof(label));
	::SetWindowText(_hSelf, label);
}

void FindReplaceDlg::enableFindInFilesControls(bool isEnable)
{
	// Hide Items
	::ShowWindow(::GetDlgItem(_hSelf, IDWRAP), isEnable?SW_HIDE:SW_SHOW);
	//::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDINFILES), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDOK), isEnable?SW_HIDE:SW_SHOW);

	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_STATIC), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_MARKLINE_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STYLEFOUND_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PURGE_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CLEAR_ALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDCMARKALL), isEnable?SW_HIDE:SW_SHOW);

	::ShowWindow(::GetDlgItem(_hSelf, IDC_DIR_STATIC), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACE), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES), isEnable?SW_HIDE:SW_SHOW);

	// Show Items
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_BROWSE_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FIND_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_GOBACK_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK), isEnable?SW_SHOW:SW_HIDE);

	char label[MAX_PATH];
	_tab.getCurrentTitle(label, sizeof(label));
	::SetWindowText(_hSelf, label);
}

void FindReplaceDlg::getPatterns(vector<string> & patternVect)
{
	cutString(_filters.c_str(), patternVect);
}

void Finder::setFinderStyle()
{
    StyleArray & stylers = (_scintView.getParameter())->getMiscStylerArray();
    int iStyleDefault = stylers.getStylerIndexByID(STYLE_DEFAULT);
    if (iStyleDefault != -1)
    {
        Style & styleDefault = stylers.getStyler(iStyleDefault);
		styleDefault._colorStyle = COLORSTYLE_ALL;	//All colors set
	    _scintView.setStyle(styleDefault);
    }

    _scintView.execute(SCI_STYLECLEARALL);
	_scintView.execute(SCI_SETSTYLEBITS, 5);
	_scintView.setSearchResultLexer();
	_scintView.execute(SCI_COLOURISE, 0, -1);
	_scintView.execute(SCI_SETEOLMODE, SC_EOL_LF);
}

BOOL CALLBACK Finder::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE :
				{
					_scintView.foldAll(fold_collapse);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE :
				{
					_scintView.foldAll(fold_uncollapse);
					return TRUE;
				}
				default :
				{
					break;
				}
			}
		}
		
		case WM_CONTEXTMENU :
		{
			if (HWND(wParam) == _scintView.getHSelf())
			{
				POINT p;
				::GetCursorPos(&p);
				ContextMenu scintillaContextmenu;
				vector<MenuItemUnit> tmp;
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE, "Collapse all"));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE, "Uncollapse all"));

				scintillaContextmenu.create(_hSelf, tmp);

				scintillaContextmenu.display(p);
				return TRUE;
			}
			return ::DefWindowProc(_hSelf, message, wParam, lParam);
		}

		case WM_SIZE :
		{
			RECT rc;
			getClientRect(rc);
			_scintView.reSizeTo(rc);
			break;
		}

		case WM_NOTIFY:
		{
			notify(reinterpret_cast<SCNotification *>(lParam));
			return FALSE;
		}
		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return FALSE;
}

void FindIncrementDlg::destroy() {
	if (_pRebar) {
		_pRebar->removeBand(_rbBand.wID);
		_pRebar = NULL;
	}
}

void FindIncrementDlg::display(bool toShow) const {
	if (!_pRebar) {
		Window::display(toShow);
		return;
	}
	if (toShow)
		::SetFocus(::GetDlgItem(_hSelf, IDC_INCFINDTEXT));
	_pRebar->setIDVisible(_rbBand.wID, toShow);
}

BOOL CALLBACK FindIncrementDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			bool isUnicode = (*(_pFRDlg->_ppEditView))->getCurrentBuffer().getUnicodeMode() != uni8Bit;
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
					::SetFocus((*(_pFRDlg->_ppEditView))->getHSelf());
					display(false);
					return TRUE;

				case IDC_INCFINDPREVOK :
				case IDC_INCFINDNXTOK :
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));
					if (LOWORD(wParam) == IDC_INCFINDPREVOK)
						fo._whichDirection = DIR_UP;
					
					string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					_pFRDlg->processFindNext(str2Search.c_str(), &fo);
				}
				return TRUE;

				case IDC_INCFINDMATCHCASE:
				case IDC_INCFINDTEXT :
				case IDC_INCFINDHILITEALL :
				{
					if (_doSearchFromBegin)
					{
						FindOption fo;
						fo._isWholeWord = false;
						fo._isIncremental = true;
						fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

						string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
						bool isFound = _pFRDlg->processFindNext(str2Search.c_str(), &fo);
						if (!isFound)
						{
							CharacterRange range = (*(_pFRDlg->_ppEditView))->getSelection();
							(*(_pFRDlg->_ppEditView))->execute(SCI_SETSEL, -1, range.cpMin);
						}

						bool isHiLieAll = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDHILITEALL, BM_GETCHECK, 0, 0));
						if (str2Search == "")
							isHiLieAll = false;

						markSelectedTextInc(isHiLieAll, &fo);
					}
					else
						_doSearchFromBegin = true;
				}
				return TRUE;

			}
		}

		case WM_ERASEBKGND:
		{
			HWND hParent = ::GetParent(_hSelf);
			HDC winDC = (HDC)wParam;
			//RTL handling
			POINT pt = {0, 0}, ptOrig = {0, 0};
			::MapWindowPoints(_hSelf, hParent, &pt, 1);
			::OffsetWindowOrgEx((HDC)wParam, pt.x, pt.y, &ptOrig);
			LRESULT lResult = SendMessage(hParent, WM_ERASEBKGND,(WPARAM)winDC, 0);
			::SetWindowOrgEx(winDC, ptOrig.x, ptOrig.y, NULL);
			return (BOOL)lResult;
			break; 
		}

		case WM_MOVE:
		{
			::InvalidateRect(_hSelf, NULL, TRUE);	//when moving, force background redraw
			return FALSE;
			break;
		}
	}
	return FALSE;
}

void FindIncrementDlg::markSelectedTextInc(bool enable, FindOption *opt)
{
	(*(_pFRDlg->_ppEditView))->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_INC);

	if (!enable)
		return;

	//Get selection
	CharacterRange range = (*(_pFRDlg->_ppEditView))->getSelection();

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
		return;

	char text2Find[MAX_PATH];
	(*(_pFRDlg->_ppEditView))->getSelectedText(text2Find, sizeof(text2Find), false);	//do not expand selection (false)
	_pFRDlg->markAllInc(text2Find, opt);
}

void FindIncrementDlg::addToRebar(ReBar * rebar) {
	if(_pRebar)
		return;
	HWND hRebar = rebar->getHSelf();
	_pRebar = rebar;
	RECT client;
	getClientRect(client);

	ZeroMemory(&_rbBand, sizeof(REBARBANDINFO));
	_rbBand.cbSize  = sizeof(REBARBANDINFO);
	_rbBand.fMask   = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
					  RBBIM_SIZE | RBBIM_ID;

	_rbBand.fStyle		= RBBS_HIDDEN;
	_rbBand.hwndChild	= getHSelf();
	_rbBand.wID			= REBAR_BAR_SEARCH;	//ID REBAR_BAR_SEARCH for search dialog
	_rbBand.cxMinChild	= 0;
	_rbBand.cyIntegral	= 1;
	_rbBand.cyMinChild	= _rbBand.cyMaxChild	= client.bottom-client.top;
	_rbBand.cxIdeal		= _rbBand.cx			= client.right-client.left;

	_pRebar->addBand(&_rbBand, true);
}
