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

#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"
#include "UniConversion.h"
#include "Buffer.h"

int Searching::convertExtendedToString(const TCHAR * query, TCHAR * result, int length) {	//query may equal to result, since it always gets smaller
	int i = 0, j = 0;
	int charLeft = length;
	bool isGood = true;
	TCHAR current;
	while(i < length) {	//because the backslash escape quences always reduce the size of the generic_string, no overflow checks have to be made for target, assuming parameters are correct
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
					} else if (current == 'o') {	//377
						size = 3, base = 8;
					} else if (current == 'd') {	//255
						size = 3, base = 10;
					} else if (current == 'x') {	//0xFF
						size = 2, base = 16;
					}
					if (charLeft >= size) {
						int res = 0;
						if (Searching::readBase(query+(i+1), &res, base, size)) {
							result[j] = (TCHAR)res;
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

bool Searching::readBase(const TCHAR * str, int * value, int base, int size) {
	int i = 0, temp = 0;
	*value = 0;
	TCHAR max = '0' + TCHAR(base - 1);
	TCHAR current;
	while(i < size) {
		current = str[i];
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

	pEditView->execute(SCI_SETANCHOR, posStart);	
}

LONG FindReplaceDlg::originalFinderProc = NULL;

void FindReplaceDlg::destroy()
{
	_tab.destroy();
	StaticDialog::destroy();
}

#ifdef UNICODE
void FindReplaceDlg::addText2Combo(const TCHAR * txt2add, HWND hCombo, bool /*isUTF8*/)
{	
	if (!hCombo) return;
	if (!lstrcmp(txt2add, TEXT(""))) return;

	int i = 0;

	i = ::SendMessage(hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2add);
	if (i != CB_ERR) // found
	{
		::SendMessage(hCombo, CB_DELETESTRING, i, 0);
	}

	i = ::SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)txt2add);

	::SendMessage(hCombo, CB_SETCURSEL, i, 0);
}
#else
void FindReplaceDlg::addText2Combo(const TCHAR * txt2add, HWND hCombo, bool isUTF8)
{	
	if (!hCombo) return;
	if (!lstrcmp(txt2add, TEXT(""))) return;

	int i = 0;

	TCHAR text[FINDREPLACE_MAXLENGTH];
	bool isWin9x = _winVer <= WV_ME;
	wchar_t wchars2Add[FINDREPLACE_MAXLENGTH];
	wchar_t textW[FINDREPLACE_MAXLENGTH];
	int count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);

	if (isUTF8)
		::MultiByteToWideChar(CP_UTF8, 0, txt2add, -1, wchars2Add, FINDREPLACE_MAXLENGTH - 1);

	for ( ; i < count ; i++)
	{
		if (isUTF8)
		{
			if (!isWin9x)
				::SendMessageW(hCombo, CB_GETLBTEXT, i, (LPARAM)textW);

			else
			{
				::SendMessageA(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
				::MultiByteToWideChar(CP_ACP, 0, text, -1, textW, FINDREPLACE_MAXLENGTH - 1);
			}

			if (!wcscmp(wchars2Add, textW))
			{
				::SendMessage(hCombo, CB_DELETESTRING, i, 0);
				break;
			}
		}
		else
		{
			::SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
			if (!strcmp(txt2add, text))
			{
				::SendMessage(hCombo, CB_DELETESTRING, i, 0);
				break;
			}
		}
	}

	if (!isUTF8)
		i = ::SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)txt2add);

	else
	{
		if (!isWin9x)
			i = ::SendMessageW(hCombo, CB_INSERTSTRING, 0, (LPARAM)wchars2Add);
		else
		{
			::WideCharToMultiByte(CP_ACP, 0, wchars2Add, -1, text, FINDREPLACE_MAXLENGTH - 1, NULL, NULL);
			i = ::SendMessageA(hCombo, CB_INSERTSTRING, 0, (LPARAM)text);
		}
	}
	::SendMessage(hCombo, CB_SETCURSEL, i, 0);
}
#endif


#ifdef UNICODE
generic_string FindReplaceDlg::getTextFromCombo(HWND hCombo, bool /*isUnicode*/) const
{	
	TCHAR str[FINDREPLACE_MAXLENGTH];
	::SendMessage(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)str);
	return generic_string(str);
}
#else
generic_string FindReplaceDlg::getTextFromCombo(HWND hCombo, bool isUnicode) const
{	
	TCHAR str[FINDREPLACE_MAXLENGTH];
	bool isWin9x = _winVer <= WV_ME;
	if (isUnicode)
	{
		wchar_t wchars[FINDREPLACE_MAXLENGTH];
		if ( !isWin9x )
		{
			::SendMessageW(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)wchars);
		}
		else
		{
			char achars[FINDREPLACE_MAXLENGTH];
			::SendMessageA(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)achars);
			::MultiByteToWideChar(CP_ACP, 0, achars, -1, wchars, FINDREPLACE_MAXLENGTH - 1);
		}
		::WideCharToMultiByte(CP_UTF8, 0, wchars, -1, str, FINDREPLACE_MAXLENGTH - 1, NULL, NULL);
	}
	else
	{
		::SendMessage(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)str);
	}

	return generic_string(str);
}
#endif

// important : to activate all styles
const int STYLING_MASK = 255;

void FindReplaceDlg::create(int dialogID, bool isRTL) 
{
	StaticDialog::create(dialogID, isRTL);
	fillFindHistory();
	_currentStatus = REPLACE_DLG;
	initOptionsFromDlg();

	RECT rect;
	//::GetWindowRect(_hSelf, &rect);
	getClientRect(rect);
	_tab.init(_hInst, _hSelf, false, false, true);
	_tab.setFont(TEXT("Tahoma"), 13);
	
	const TCHAR *find = TEXT("Find");
	const TCHAR *replace = TEXT("Replace");
	const TCHAR *findInFiles = TEXT("Find in files");

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

void FindReplaceDlg::fillFindHistory()
{
	NppParameters *nppParams = NppParameters::getInstance();

	FindHistory& findHistory = nppParams->getFindHistory();

	fillComboHistory(IDD_FINDINFILES_DIR_COMBO,     findHistory._nbFindHistoryPath,    findHistory._pFindHistoryPath);
	fillComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._nbFindHistoryFilter,  findHistory._pFindHistoryFilter);
	fillComboHistory(IDFINDWHAT,                    findHistory._nbFindHistoryFind,    findHistory._pFindHistoryFind);
	fillComboHistory(IDREPLACEWITH,                 findHistory._nbFindHistoryReplace, findHistory._pFindHistoryReplace);

	::SendDlgItemMessage(_hSelf, IDWRAP, BM_SETCHECK, findHistory._isWrap, 0);
	::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, findHistory._isMatchWord, 0);
	::SendDlgItemMessage(_hSelf, IDMATCHCASE, BM_SETCHECK, findHistory._isMatchCase, 0);

	::SendDlgItemMessage(_hSelf, IDDIRECTIONUP, BM_SETCHECK, !findHistory._isDirectionDown, 0);
	::SendDlgItemMessage(_hSelf, IDDIRECTIONDOWN, BM_SETCHECK, findHistory._isDirectionDown, 0);

	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK, BM_SETCHECK, findHistory._isFifInHiddenFolder, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK, BM_SETCHECK, findHistory._isFifRecuisive, 0);
    ::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK, BM_SETCHECK, findHistory._isFolderFollowDoc, 0);

	::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, findHistory._searchMode == FindHistory::normal, 0);
	::SendDlgItemMessage(_hSelf, IDEXTENDED, BM_SETCHECK, findHistory._searchMode == FindHistory::extended, 0);
	::SendDlgItemMessage(_hSelf, IDREGEXP, BM_SETCHECK, findHistory._searchMode == FindHistory::regExpr, 0);
	if (findHistory._searchMode == FindHistory::regExpr)
	{
		//regex doesnt allow wholeword
		::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, BST_UNCHECKED, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD), (BOOL)false);

		//regex doesnt allow upward search
		::SendDlgItemMessage(_hSelf, IDDIRECTIONDOWN, BM_SETCHECK, BST_CHECKED, 0);
		::SendDlgItemMessage(_hSelf, IDDIRECTIONUP, BM_SETCHECK, BST_UNCHECKED, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), (BOOL)false);
	}
	
	if (nppParams->isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_SHOW);
		
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, findHistory._transparency);
		
		if (findHistory._transparencyMode == FindHistory::none)
		{
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), FALSE);
		}
		else
		{
			::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_CHECK, BM_SETCHECK, TRUE, 0);
			
			int id;
			if (findHistory._transparencyMode == FindHistory::onLossingFocus)
			{
				id = IDC_TRANSPARENT_LOSSFOCUS_RADIO;
			}
			else
			{
				id = IDC_TRANSPARENT_ALWAYS_RADIO;
				(NppParameters::getInstance())->SetTransparent(_hSelf, findHistory._transparency);

			}
			::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);
		}
	}
}

void FindReplaceDlg::fillComboHistory(int id, int count, generic_string **pStrings)
{
	int i;
	bool isUnicode = false;
	HWND hCombo;

	hCombo = ::GetDlgItem(_hSelf, id);
	for (i = count -1 ; i >= 0 ; i--)
	{
		addText2Combo(pStrings[i]->c_str(), hCombo, isUnicode);
	}
	::SendMessage(hCombo, CB_SETCURSEL, 0, 0); // select first item
}


void FindReplaceDlg::saveFindHistory()
{
	if (! isCreated()) return;
	FindHistory& findHistory = (NppParameters::getInstance())->getFindHistory();

	saveComboHistory(IDD_FINDINFILES_DIR_COMBO,     findHistory._nbMaxFindHistoryPath,    findHistory._nbFindHistoryPath,    findHistory._pFindHistoryPath);
	saveComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._nbMaxFindHistoryFilter,  findHistory._nbFindHistoryFilter,  findHistory._pFindHistoryFilter);
	saveComboHistory(IDFINDWHAT,                    findHistory._nbMaxFindHistoryFind,    findHistory._nbFindHistoryFind,    findHistory._pFindHistoryFind);
	saveComboHistory(IDREPLACEWITH,                 findHistory._nbMaxFindHistoryReplace, findHistory._nbFindHistoryReplace, findHistory._pFindHistoryReplace);
}

void FindReplaceDlg::saveComboHistory(int id, int maxcount, int & oldcount, generic_string **pStrings)
{
	int i, count;
	HWND hCombo;
	TCHAR text[FINDREPLACE_MAXLENGTH];

	hCombo = ::GetDlgItem(_hSelf, id);
	count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	count = min(count, maxcount);
	for (i = 0; i < count; i++)
	{
		::SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM) text);
		if (i < oldcount)
			*pStrings[i] = text;
		else
			pStrings[i] = new generic_string(text);
	}
	for (; i < oldcount; i++) delete pStrings[i];
	oldcount = count;
}

void FindReplaceDlg::updateCombos()
{
	updateCombo(IDREPLACEWITH);
	updateCombo(IDFINDWHAT);
}

FoundInfo Finder::EmptyFoundInfo(0, 0, TEXT(""));
SearchResultMarking Finder::EmptySearchResultMarking;

bool Finder::notify(SCNotification *notification)
{
	static bool isDoubleClicked = false;

	switch (notification->nmhdr.code)
	{
		case SCN_MARGINCLICK:
			if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
			{
				_scintView.marginClick(notification->position, notification->modifiers);
			}
			break;

		case SCN_DOUBLECLICK:
		{
			// remove selection from the finder
			isDoubleClicked = true;
			int pos = notification->position;
			if (pos == INVALID_POSITION)
				pos = _scintView.execute(SCI_GETLINEENDPOSITION, notification->line);
			_scintView.execute(SCI_SETSEL, pos, pos);

			GotoFoundLine();
		}
		break;

		case SCN_PAINTED :
			if (isDoubleClicked)
			{
				(*_ppEditView)->getFocus();
				isDoubleClicked = false;
			}
			break;
	}
	return false;
}


void Finder::GotoFoundLine()
{
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	int end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);
	if (start + 2 >= end) return; // avoid empty lines

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		_scintView.execute(SCI_TOGGLEFOLD, lno);
		return;
	}

	const FoundInfo fInfo = *(_pMainFoundInfos->begin() + lno);

	// Switch to another document
	::SendMessage(::GetParent(_hParent), WM_DOOPEN, 0, (LPARAM)fInfo._fullPath.c_str());
	Searching::displaySectionCentered(fInfo._start, fInfo._end, *_ppEditView);

	// Then we colourise the double clicked line
	setFinderStyle();
	_scintView.execute(SCI_SETLEXER, SCLEX_NULL);   // yniq - this line causes a bug!!! (last line suddenly belongs to file header level (?) instead of having level=0x400)
													// later it affects DeleteResult and gotoNextFoundResult (assertions)
													// fixed by calling setFinderStyle() in DeleteResult()
	_scintView.execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_HIGHLIGHT_LINE, true);
	_scintView.execute(SCI_STARTSTYLING, start, STYLING_MASK);
	_scintView.execute(SCI_SETSTYLING, end - start + 2, SCE_SEARCHRESULT_HIGHLIGHT_LINE);
	_scintView.execute(SCI_COLOURISE, start, end + 1);
}

void Finder::DeleteResult()
{
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS); // yniq - add handling deletion of multiple lines?

	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	int end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);
	if (start + 2 >= end) return; // avoid empty lines

	_scintView.setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, 0); // Restore searchResult lexer in case the lexer was changed to SCLEX_NULL in GotoFoundLine()

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)  // delete a folder
	{
		int endline = _scintView.execute(SCI_GETLASTCHILD, lno, -1) + 1;
		assert((size_t) endline <= _pMainFoundInfos->size());

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno, _pMainFoundInfos->begin() + endline); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno, _pMainMarkings->begin() + endline);

		int end = _scintView.execute(SCI_POSITIONFROMLINE, endline);
		_scintView.execute(SCI_SETSEL, start, end);
		setFinderReadOnly(false);
		_scintView.execute(SCI_CLEAR);
		setFinderReadOnly(true);
	}
	else // delete one line
	{
		assert((size_t) lno < _pMainFoundInfos->size());

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno);

		setFinderReadOnly(false);
		_scintView.execute(SCI_LINEDELETE);
		setFinderReadOnly(true);
	}
	_MarkingsStruct._length = _pMainMarkings->size();

	assert(_pMainFoundInfos->size() == _pMainMarkings->size());
	assert(_scintView.execute(SCI_GETLINECOUNT) == (int)_pMainFoundInfos->size() + 1);
}

void Finder::gotoNextFoundResult(int direction)
{
	int increment = direction < 0 ? -1 : 1;
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int total_lines = _scintView.execute(SCI_GETLINECOUNT);
	if (total_lines <= 1) return;
	
	if (lno == total_lines - 1) lno--; // last line doesn't belong to any search, use last search

	int init_lno = lno;
	int max_lno = _scintView.execute(SCI_GETLASTCHILD, lno, searchHeaderLevel);

	assert(max_lno <= total_lines - 2);

	// get the line number of the current search (searchHeaderLevel)
	int level = _scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELNUMBERMASK;
	int min_lno = lno;
	while (level-- >= fileHeaderLevel)
	{
		min_lno = _scintView.execute(SCI_GETFOLDPARENT, min_lno);
		assert(min_lno >= 0);
	}

	if (min_lno < 0) min_lno = lno; // when lno is a search header line

	assert(min_lno <= max_lno);

	lno += increment;
	
	if      (lno > max_lno) lno = min_lno;
	else if (lno < min_lno) lno = max_lno;

	while (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		lno += increment;
		if      (lno > max_lno) lno = min_lno;
		else if (lno < min_lno) lno = max_lno;
		if (lno == init_lno) break;
	}

	if ((_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG) == 0)
	{
		int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
		_scintView.execute(SCI_SETSEL, start, start);
		_scintView.execute(SCI_ENSUREVISIBLE, lno);
		_scintView.execute(SCI_SCROLLCARET);

		GotoFoundLine();
	}
}

BOOL CALLBACK FindReplaceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
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

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDREPLACEALL));
			 _findInFilesClosePos.left = p.x;
			 _findInFilesClosePos.top = p.y;

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDCANCEL));
			 _findClosePos.left = p.x;
			 _findClosePos.top = p.y + 10;

			return TRUE;
		}
		
		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER))
			{
				int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
				FindHistory & findHistory = (NppParameters::getInstance())->getFindHistory();
				findHistory._transparency = percent;
				if (isCheckedOrNot(IDC_TRANSPARENT_ALWAYS_RADIO))
				{
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
				}
				return TRUE;
			}
			break;
		}

		case WM_ACTIVATE :
		{
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				CharacterRange cr;
				(*_ppEditView)->getSelection(cr);
				int nbSelected = cr.cpMax - cr.cpMin;

				_isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK)?1:0;
				int checkVal = _isInSelection?BST_CHECKED:BST_UNCHECKED;
				
				if (!_isInSelection)
				{
					if (nbSelected <= 1024)
					{
						checkVal = BST_UNCHECKED;
						_isInSelection = false;
					}
					else
					{
						checkVal = BST_CHECKED;
						_isInSelection = true;
					}
				}
				// Searching/replacing in column selection is not allowed 
				if ((*_ppEditView)->execute(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE)
				{
					checkVal = BST_UNCHECKED;
					_isInSelection = false;
					nbSelected = 0;
				}
				::EnableWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), nbSelected);
                // uncheck if the control is disable
                if (!nbSelected)
                {
					checkVal = BST_UNCHECKED;
					_isInSelection = false;
                }
				::SendDlgItemMessage(_hSelf, IDC_IN_SELECTION_CHECK, BM_SETCHECK, checkVal, 0);
			}
			
			if (isCheckedOrNot(IDC_TRANSPARENT_LOSSFOCUS_RADIO))
			{
				if (LOWORD(wParam) == WA_INACTIVE && isVisible())
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
			NppParameters *nppParamInst = NppParameters::getInstance();
			FindHistory & findHistory = nppParamInst->getFindHistory();
			switch (wParam)
			{
				case IDCANCEL : // Close
					display(false);
					return TRUE;
				
//Single actions
				case IDOK : // Find Next : only for FIND_DLG and REPLACE_DLG
				{
					bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;

					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
					generic_string str2Search = getTextFromCombo(hFindCombo, isUnicode);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					processFindNext(str2Search.c_str());
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDREPLACE :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						generic_string str2Search = getTextFromCombo(hFindCombo, isUnicode);
						generic_string str2Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						processReplace(str2Search.c_str(), str2Replace.c_str());
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;
//Process actions
				case IDC_FINDALL_OPENEDFILES :
				{
					if (_currentStatus == FIND_DLG)
					{
                        combo2ExtendedMode(IDFINDWHAT);
						updateCombo(IDFINDWHAT);

						nppParamInst->_isFindReplacing = true;
						findAllIn(ALL_OPEN_DOCS);
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_FINDALL_CURRENTFILE :
				{
                    combo2ExtendedMode(IDFINDWHAT);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					findAllIn(CURRENT_DOC);
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDD_FINDINFILES_FIND_BUTTON :
				{
					const int filterSize = 256;
					TCHAR filters[filterSize];
					TCHAR directory[MAX_PATH];
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_filters = filters;

					::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
					addText2Combo(directory, ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
					_directory = directory;
					
					if ((lstrlen(directory) > 0) && (directory[lstrlen(directory)-1] != '\\'))
						_directory += TEXT("\\");

                    combo2ExtendedMode(IDFINDWHAT);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					findAllIn(FILES_IN_DIR);
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDD_FINDINFILES_REPLACEINFILES :
				{
					const int filterSize = 256;
					TCHAR filters[filterSize];
					TCHAR directory[MAX_PATH];
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_filters = filters;

					::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
					addText2Combo(directory, ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
					_directory = directory;
					
					if ((lstrlen(directory) > 0) && (directory[lstrlen(directory)-1] != '\\'))
						_directory += TEXT("\\");

					generic_string msg = TEXT("Are you sure you want to replace all occurances in :\r");
					msg += _directory;
					msg += TEXT("\rfor file type : ");
					msg += _filters[0]?_filters:TEXT("*.*");
					
					if (::MessageBox(_hSelf, msg.c_str(), TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						updateCombo(IDFINDWHAT);
						updateCombo(IDREPLACEWITH);

						nppParamInst->_isFindReplacing = true;
						::SendMessage(_hParent, WM_REPLACEINFILES, 0, 0);
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_REPLACE_OPENEDFILES :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						replaceAllInOpenedDocs();
						nppParamInst->_isFindReplacing = false;
					}
				}			
				return TRUE;

				case IDREPLACEALL :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
						int nbReplaced = processAll(ProcessReplaceAll, NULL, NULL);
						(*_ppEditView)->execute(SCI_ENDUNDOACTION);
						nppParamInst->_isFindReplacing = false;

						TCHAR result[64];
						if (nbReplaced < 0)
							lstrcpy(result, TEXT("The regular expression to search is formed badly"));
						else
							wsprintf(result, TEXT("%d occurrences were replaced."), nbReplaced);
						::MessageBox(_hSelf, result, TEXT("Replace All"), MB_OK);
					}
				}
				return TRUE;

				case IDCCOUNTALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						int nbCounted = processAll(ProcessCountAll, NULL, NULL);
						TCHAR result[128];
						if (nbCounted < 0)
							lstrcpy(result, TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?"));
						else
							wsprintf(result, TEXT("%d match(es) to occurrence(s)"), nbCounted);
						::MessageBox(_hSelf, result, TEXT("Count"), MB_OK);
					}
				}
				return TRUE;

				case IDCMARKALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						updateCombo(IDFINDWHAT);

						nppParamInst->_isFindReplacing = true;
						int nbMarked = processAll(ProcessMarkAll, NULL, NULL);
						nppParamInst->_isFindReplacing = false;
						TCHAR result[128];
						if (nbMarked < 0)
							lstrcpy(result, TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?"));
						else
							wsprintf(result, TEXT("%d match(es) to occurrence(s)"), nbMarked);
						::MessageBox(_hSelf, result, TEXT("Mark"), MB_OK);
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
					findHistory._isMatchWord = _options._isWholeWord = isCheckedOrNot(IDWHOLEWORD);
					return TRUE;

				case IDMATCHCASE :
					findHistory._isMatchCase = _options._isMatchCase = isCheckedOrNot(IDMATCHCASE);
					return TRUE;

				case IDNORMAL:
				case IDEXTENDED:
				case IDREGEXP : {
					if (isCheckedOrNot(IDREGEXP))
					{
						_options._searchType = FindRegex;
						findHistory._searchMode = FindHistory::regExpr;
					}
					else if (isCheckedOrNot(IDEXTENDED))
					{
						_options._searchType = FindExtended;
						findHistory._searchMode = FindHistory::extended;
					}
					else
					{
						_options._searchType = FindNormal;
						findHistory._searchMode = FindHistory::normal;					
					}

					bool isRegex = (_options._searchType == FindRegex);
					if (isRegex) 
					{	
						//regex doesnt allow whole word
						_options._isWholeWord = false;
						::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, _options._isWholeWord?BST_CHECKED:BST_UNCHECKED, 0);

						//regex doesnt allow upward search
						::SendDlgItemMessage(_hSelf, IDDIRECTIONDOWN, BM_SETCHECK, BST_CHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDDIRECTIONUP, BM_SETCHECK, BST_UNCHECKED, 0);
						_options._whichDirection = DIR_DOWN;
					}

					::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD), (BOOL)!isRegex);
					::EnableWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), (BOOL)!isRegex);
					return TRUE; }

				case IDWRAP :
					findHistory._isWrap = _options._isWrapAround = isCheckedOrNot(IDWRAP);
					return TRUE;

				case IDDIRECTIONUP :
				case IDDIRECTIONDOWN :
					_options._whichDirection = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), BM_GETCHECK, BST_CHECKED, 0));
					findHistory._isDirectionDown = _options._whichDirection == DIR_DOWN;
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
					if (_currentStatus <= REPLACE_DLG)
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
						findHistory._transparencyMode = FindHistory::onLossingFocus;
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						(NppParameters::getInstance())->removeTransparent(_hSelf);
						findHistory._transparencyMode = FindHistory::none;
					}

					return TRUE;
				}

				case IDC_TRANSPARENT_ALWAYS_RADIO :
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
					findHistory._transparencyMode = FindHistory::persistant;
				}
				return TRUE;

				case IDC_TRANSPARENT_LOSSFOCUS_RADIO :
				{
					(NppParameters::getInstance())->removeTransparent(_hSelf);
					findHistory._transparencyMode = FindHistory::onLossingFocus;
				}
				return TRUE;

				//
				// Find in Files
				//
				case IDD_FINDINFILES_RECURSIVE_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						findHistory._isFifRecuisive = _isRecursive = isCheckedOrNot(IDD_FINDINFILES_RECURSIVE_CHECK);
					
				}
				return TRUE;

				case IDD_FINDINFILES_INHIDDENDIR_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						findHistory._isFifInHiddenFolder = _isInHiddenDir = isCheckedOrNot(IDD_FINDINFILES_INHIDDENDIR_CHECK);
					
				}
				return TRUE;

                case IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
                        findHistory._isFolderFollowDoc = isCheckedOrNot(IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK);

                    if (findHistory._isFolderFollowDoc)
                    {
                        NppParameters *pNppParam = NppParameters::getInstance();
                        const TCHAR * dir = pNppParam->getWorkingDir();
                        ::SetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, dir);
                    }
					
				}
				return TRUE;

				case IDD_FINDINFILES_BROWSE_BUTTON :
				{
					if (_currentStatus == FINDINFILES_DLG)
						folderBrowser(_hSelf, IDD_FINDINFILES_DIR_COMBO, _directory.c_str());	
				}
				return TRUE;

				default :
					break;
			}
			break;
		}
	}
	return FALSE;
}

// return value :
// true  : the text2find is found
// false : the text2find is not found
bool FindReplaceDlg::processFindNext(const TCHAR *txt2find, FindOption *options, FindStatus *oFindStatus)
{
	if (oFindStatus)
		*oFindStatus = FSFound;

	if (!txt2find || !txt2find[0])
		return false;

	FindOption *pOptions = options?options:&_options;

	int stringSizeFind = lstrlen(txt2find);
	TCHAR *pText = new TCHAR[stringSizeFind + 1];
	lstrcpy(pText, txt2find);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pText, stringSizeFind);
	}

	int docLength = int((*_ppEditView)->execute(SCI_GETLENGTH));
	CharacterRange cr;
	(*_ppEditView)->getSelection(cr);

	//The search "zone" is relative to the selection, so search happens 'outside'
	int startPosition = cr.cpMax;
	int endPosition = docLength;

	if (pOptions->_whichDirection == DIR_UP)
	{
		//When searching upwards, start is the lower part, end the upper, for backwards search
		startPosition = cr.cpMin - 1;
		endPosition = 0;
	}

	if (FirstIncremental==pOptions->_incrementalType)
	{
		// the text to find is modified so use the current position
		startPosition = cr.cpMin;
		endPosition = docLength;
	}
	else if (NextIncremental==pOptions->_incrementalType)
	{
		// text to find is not modified, so use current position +1
		startPosition = cr.cpMin +1;
		endPosition = docLength;	
		if (pOptions->_whichDirection == DIR_UP)
		{
			//When searching upwards, start is the lower part, end the upper, for backwards search
			startPosition = cr.cpMax - 1;
			endPosition = 0;
		}
	}

	int flags = Searching::buildSearchFlags(pOptions);

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	//::SendMessageA(_hParent, WM_SETTEXT, 0, (LPARAM)pText);
	int posFind = (*_ppEditView)->searchInTarget(pText, startPosition, endPosition);
	if (posFind == -1) //no match found in target, check if a new target should be used
	{
		if (pOptions->_isWrapAround) 
		{
			//when wrapping, use the rest of the document (entire document is usable)
			if (pOptions->_whichDirection == DIR_DOWN)
			{
				startPosition = 0;
				endPosition = docLength;
				if (oFindStatus)
					*oFindStatus = FSEndReached;
			}
			else
			{
				startPosition = docLength;
				endPosition = 0;
				if (oFindStatus)
					*oFindStatus = FSTopReached;
			}

			//new target, search again
			posFind = (*_ppEditView)->searchInTarget(pText, startPosition, endPosition);
		}
		if (posFind == -1)
		{
			if (oFindStatus)
				*oFindStatus = FSNotFound;
			//failed, or failed twice with wrap
			if (NotIncremental==pOptions->_incrementalType) //incremental search doesnt trigger messages
			{	
				generic_string msg = TEXT("Can't find the text:\r\n\"");
				msg += pText;
				msg += TEXT("\"");
				::MessageBox(_hSelf, msg.c_str(), TEXT("Find"), MB_OK);
				// if the dialog is not shown, pass the focus to his parent(ie. Notepad++)
				if (!::IsWindowVisible(_hSelf))
				{
					::SetFocus((*_ppEditView)->getHSelf());
				}
				else
				{
					::SetFocus(::GetDlgItem(_hSelf, IDFINDWHAT));
				}
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
bool FindReplaceDlg::processReplace(const TCHAR *txt2find, const TCHAR *txt2replace, FindOption *options)
{
	if (!txt2find || !txt2find[0] || !txt2replace)
		return false;

	FindOption *pOptions = options?options:&_options;

	if ((*_ppEditView)->getCurrentBuffer()->isReadOnly()) return false;

	int stringSizeFind = lstrlen(txt2find);
	int stringSizeReplace = lstrlen(txt2replace);
	TCHAR *pTextFind = new TCHAR[stringSizeFind + 1];
	TCHAR *pTextReplace = new TCHAR[stringSizeReplace + 1];
	lstrcpy(pTextFind, txt2find);
	lstrcpy(pTextReplace, txt2replace);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pTextFind, stringSizeFind);
		stringSizeReplace = Searching::convertExtendedToString(txt2replace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);
	CharacterRange cr;
	(*_ppEditView)->getSelection(cr);
	
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int posFind = (*_ppEditView)->searchInTarget(pTextFind, cr.cpMin, cr.cpMax);
	if (posFind != -1)
	{
		if (isRegExp)
		{
			//For the rare re exp case. ex: replace ^ by AAA
			int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
			(*_ppEditView)->execute(SCI_GETTARGETEND);

			int replacedLen = (*_ppEditView)->replaceTargetRegExMode(pTextReplace);
			
			//if (!foundTextLen)
			(*_ppEditView)->execute(SCI_SETSEL, start, start + replacedLen);
		}
		else
		{
			int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
			int replacedLen = (*_ppEditView)->replaceTarget(pTextReplace);
			(*_ppEditView)->execute(SCI_SETSEL, start, start + replacedLen);
		}
	}

	delete [] pTextFind;
	delete [] pTextReplace;
	return processFindNext(txt2find);	//after replacing, find the next section for selection
}


int FindReplaceDlg::markAll(const TCHAR *txt2find, int styleID)
{
	_doStyleFoundToken = true;
	FindOption opt;
	opt._isMatchCase = true;
	opt._isWholeWord = false;

	int nbFound = processAll(ProcessMarkAllExt, txt2find, NULL, true, NULL, &opt, styleID);
	return nbFound;
}

int FindReplaceDlg::markAll2(const TCHAR *txt2find)
{
	FindOption opt;
	opt._isMatchCase = false;
	opt._isWholeWord = true;
	int nbFound = processAll(ProcessMarkAll_2, txt2find, NULL, true, NULL, &opt);
	return nbFound;
}



int FindReplaceDlg::markAllInc(const TCHAR *txt2find, FindOption *opt)
{
	int nbFound = processAll(ProcessMarkAll_IncSearch, txt2find, NULL, true, NULL, opt);
	return nbFound;
}

int FindReplaceDlg::processAll(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, bool isEntire, const TCHAR *fileName, FindOption *opt, int colourStyleID)
{
	FindOption *pOptions = opt?opt:&_options;

	CharacterRange cr;
	(*_ppEditView)->getSelection(cr);
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
		startPosition = 0;
		endPosition = cr.cpMax;
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
	}
	
	//then readjust scope if the selection override is active and allowed
	if ((_isInSelection) && ((op == ProcessMarkAll) || ((op == ProcessReplaceAll) && (!isEntire))))	//if selection limiter and either mark all or replace all w/o entire document override
	{
		startPosition = cr.cpMin;
		endPosition = cr.cpMax;
	}

	if ((op == ProcessMarkAllExt) && (colourStyleID != -1))
	{
		startPosition = 0;
		endPosition = docLength;
	}

	return processRange(op, txt2find, txt2replace, startPosition, endPosition, fileName, opt, colourStyleID);
}

int FindReplaceDlg::processRange(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, int startRange, int endRange, const TCHAR *fileName, FindOption *opt, int colourStyleID)
{
	int nbProcessed = 0;
	
	if (!isCreated() && !txt2find)
		return nbProcessed;

	if ((op == ProcessReplaceAll) && (*_ppEditView)->getCurrentBuffer()->isReadOnly())
		return nbProcessed;

	if (startRange == endRange)
		return nbProcessed;

	if (!fileName)
		fileName = TEXT("");

	FindOption *pOptions = opt?opt:&_options;
	//bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
	bool isUnicode = ((*_ppEditView)->execute(SCI_GETCODEPAGE) == SC_CP_UTF8);

	int stringSizeFind = 0;
	int stringSizeReplace = 0;

	TCHAR *pTextFind = NULL;//new TCHAR[stringSizeFind + 1];
	if (!txt2find)
	{
		HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
		generic_string str2Search = getTextFromCombo(hFindCombo, isUnicode);
		stringSizeFind = str2Search.length();
		pTextFind = new TCHAR[stringSizeFind + 1];
		lstrcpy(pTextFind, str2Search.c_str());
	}
	else
	{
		stringSizeFind = lstrlen(txt2find);
		pTextFind = new TCHAR[stringSizeFind + 1];
		lstrcpy(pTextFind, txt2find);
	}

	if (!pTextFind[0]) 
	{
		delete [] pTextFind;
		return nbProcessed;
	}

	TCHAR *pTextReplace = NULL;
	if (op == ProcessReplaceAll)
	{
		if (!txt2replace)
		{
			HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
			generic_string str2Replace = getTextFromCombo(hReplaceCombo, isUnicode);
			stringSizeReplace = str2Replace.length();
			pTextReplace = new TCHAR[stringSizeReplace + 1];
			lstrcpy(pTextReplace, str2Replace.c_str());
		}
		else
		{
			stringSizeReplace = lstrlen(txt2replace);
			pTextReplace = new TCHAR[stringSizeReplace + 1];
			lstrcpy(pTextReplace, txt2replace);
		}
	}	

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(pTextFind, pTextFind, stringSizeFind);
		if (op == ProcessReplaceAll)
			stringSizeReplace = Searching::convertExtendedToString(pTextReplace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);



	if (op == ProcessMarkAll && colourStyleID == -1)	//if marking, check if purging is needed
	{
		if (_doPurge) {
			if (_doMarkLine)
				(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);

			if (_doStyleFoundToken)
				(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
		}
	}

	int targetStart = 0;
	int targetEnd = 0;

	//Initial range for searching
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	targetStart = (*_ppEditView)->searchInTarget(pTextFind, startRange, endRange);
	
	if ((targetStart != -1) && (op == ProcessFindAll))	//add new filetitle if this file results in hits
	{
		_pFinder->addFileNameTitle(fileName);
	}
	while (targetStart != -1)
	{
		//int posFindBefore = posFind;
		targetStart = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
		targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));
		if (targetEnd > endRange) {	//we found a result but outside our range, therefore do not process it
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		int replaceDelta = 0;

		// Search resulted in empty token, possible with RE
		if (!foundTextLen) {
			delete [] pTextFind;
			delete [] pTextReplace;
			return -1;
		}
		
		switch (op)
		{
			case ProcessFindAll: 
			{
				int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, targetStart);
				int lend = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, lineNumber);
				int lstart = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, lineNumber);
				int nbChar = lend - lstart;

				// use the static buffer
				TCHAR lineBuf[1024];

				if (nbChar > 1024 - 3)
					lend = lstart + 1020;

				int start_mark = targetStart - lstart;
				int end_mark = targetEnd - lstart;

				(*_ppEditView)->getGenericText(lineBuf, lstart, lend, &start_mark, &end_mark);
				generic_string line;
#ifdef UNICODE
				line = lineBuf;
#else
				UINT cp = (*_ppEditView)->execute(SCI_GETCODEPAGE);
				if (cp != SC_CP_UTF8)
				{
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *pTextW = wmc->char2wchar(lineBuf, ::GetACP());
					const char *pTextA = wmc->wchar2char(pTextW, SC_CP_UTF8);
					line = pTextA;
				}
				else
					line = lineBuf;
#endif
				line += TEXT("\r\n");
				SearchResultMarking srm;
				srm._start = start_mark;
				srm._end = end_mark;
				_pFinder->add(FoundInfo(targetStart, targetEnd,  fileName), srm, line.c_str(), lineNumber + 1);

				break; 
			}

			case ProcessReplaceAll: 
			{
				int replacedLength;
				if (isRegExp)
					replacedLength = (*_ppEditView)->replaceTargetRegExMode(pTextReplace);
				else
					replacedLength = (*_ppEditView)->replaceTarget(pTextReplace);

				replaceDelta = replacedLength - foundTextLen;
				break; 
			}

			case ProcessMarkAll: 
			{
				if (_doStyleFoundToken)
				{
					(*_ppEditView)->execute(SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_FOUND_STYLE);
					(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				}

				if (_doMarkLine)
				{
					int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, targetStart);
					int state = (*_ppEditView)->execute(SCI_MARKERGET, lineNumber);

					if (!(state & (1 << MARK_BOOKMARK)))
						(*_ppEditView)->execute(SCI_MARKERADD, lineNumber, MARK_BOOKMARK);
				}
				break; 
			}
			
			case ProcessMarkAllExt:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  colourStyleID);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessMarkAll_2:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_2);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessMarkAll_IncSearch:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_INC);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessCountAll: 
			{
				//Nothing to do
				break;
			}

			default:
			{
				delete [] pTextFind;
				delete [] pTextReplace;
				return nbProcessed;
			}
			
		}	

		startRange = targetStart + foundTextLen + replaceDelta;		//search from result onwards
		endRange += replaceDelta;									//adjust end of range in case of replace

		nbProcessed++;
		
		//::SendMessageA(_hParent, WM_SETTEXT, 0, (LPARAM)pTextFind);
		targetStart = (*_ppEditView)->searchInTarget(pTextFind, startRange, endRange);
	}
	delete [] pTextFind;
	delete [] pTextReplace;

	if (nbProcessed > 0 && op == ProcessFindAll) 
		_pFinder->addFileHitCount(nbProcessed);

	return nbProcessed;
}

void FindReplaceDlg::replaceAllInOpenedDocs()
{
	::SendMessage(_hParent, WM_REPLACEALL_INOPENEDDOC, 0, 0);
}

void FindReplaceDlg::findAllIn(InWhat op)
{
	//HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("findInFilesEvent"));
	if (!_pFinder)
	{
		_pFinder = new Finder();
		_pFinder->init(_hInst, _hSelf, _ppEditView);
		
		tTbData	data = {0};
		_pFinder->create(&data, false);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pFinder->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO;
		data.hIconTab = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_FIND_RESULT_ICON), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszAddInfo = _findAllResultStr;

		data.pszModuleName = TEXT("dummy");

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		data.dlgID = 0;
		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);

		_pFinder->_scintView.init(_hInst, _pFinder->getHSelf());

		// Subclass the ScintillaEditView for the Finder (Scintilla doesn't notify all key presses)
		originalFinderProc = SetWindowLong( _pFinder->_scintView.getHSelf(), GWL_WNDPROC, (LONG) finderProc);

		_pFinder->setFinderReadOnly(true);
		_pFinder->_scintView.execute(SCI_SETCODEPAGE, SC_CP_UTF8);
		_pFinder->_scintView.execute(SCI_USEPOPUP, FALSE);
		_pFinder->_scintView.execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
		_pFinder->_scintView.execute(SCI_SETCARETLINEVISIBLE, 1);
		_pFinder->_scintView.execute(SCI_SETCARETWIDTH, 0);
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_FOLDER, true);
		//_pFinder->_scintView.execute(SCI_SETEOLMODE, SC_EOL_CRLF); // yniq - needed?

		// Send the address of _MarkingsStruct to the lexer
		char ptrword[sizeof(void*)*2+1];
		sprintf_s(ptrword, sizeof(void*)*2+1, "%p", &_pFinder->_MarkingsStruct);
		_pFinder->_scintView.execute(SCI_SETKEYWORDS, 0, (LPARAM) ptrword);

		// get the width of FindDlg
		RECT findRect;
		::GetWindowRect(_pFinder->getHSelf(), &findRect);

		// overwrite some default settings
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, false);
		_pFinder->_scintView.setMakerStyle(FOLDER_STYLE_SIMPLE);

		_pFinder->_scintView.display();
		_pFinder->display();
	}
	_pFinder->setFinderStyle();
	
	::SendMessage(_pFinder->getHSelf(), WM_SIZE, 0, 0);

	int cmdid = 0;
	if (op == ALL_OPEN_DOCS)
		cmdid = WM_FINDALL_INOPENEDDOC;
	else if (op == FILES_IN_DIR)
		cmdid = WM_FINDINFILES;
	else if (op == CURRENT_DOC)
		cmdid = WM_FINDALL_INCURRENTDOC;

	if (!cmdid) return;

	if (::SendMessage(_hParent, cmdid, 0, 0))
	{
		wsprintf(_findAllResultStr, TEXT("%d hits"), _findAllResult);
		if (_findAllResult) 
		{
			focusOnFinder();
		}
		else
			getFocus(); // no hits
	}
	else // error - search folder doesn't exist
		::SendMessage(_hSelf, WM_NEXTDLGCTL, (WPARAM)::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO), TRUE);
}

void FindReplaceDlg::enableReplaceFunc(bool isEnable) 
{
	_currentStatus = isEnable?REPLACE_DLG:FIND_DLG;
	int hideOrShow = isEnable?SW_SHOW:SW_HIDE;
	RECT *pClosePos = isEnable?&_replaceClosePos:&_findClosePos;

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
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_CURRENTFILE),!hideOrShow);

	gotoCorrectTab();

	::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), pClosePos->left, pClosePos->top, pClosePos->right, pClosePos->bottom, TRUE);

	TCHAR label[MAX_PATH];
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);

	setDefaultButton(IDOK);
}

void FindReplaceDlg::enableFindInFilesControls(bool isEnable)
{
	// Hide Items
	::ShowWindow(::GetDlgItem(_hSelf, IDWRAP), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_CURRENTFILE), isEnable?SW_HIDE:SW_SHOW);
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
	if (isEnable)
	{
		::ShowWindow(::GetDlgItem(_hSelf, ID_STATICTEXT_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH), SW_SHOW);
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_REPLACEINFILES), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_BROWSE_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FIND_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_GOBACK_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK), isEnable?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK), isEnable?SW_SHOW:SW_HIDE);
}

void FindReplaceDlg::getPatterns(vector<generic_string> & patternVect)
{
	cutString(_filters.c_str(), patternVect);
}

void FindReplaceDlg::combo2ExtendedMode(int comboID)
{
	HWND hFindCombo = ::GetDlgItem(_hSelf, comboID);
	if (!hFindCombo) return;
	
	generic_string str2transform = getTextFromCombo(hFindCombo);
		
    // Count the number of character '\n' and '\r'
    size_t nbEOL = 0;
    size_t str2transformLen = lstrlen(str2transform.c_str());
    for (size_t i = 0 ; i < str2transformLen ; i++)
    {
        if (str2transform[i] == '\r' || str2transform[i] == '\n')
            nbEOL++;
    }

    if (nbEOL)
    {
		TCHAR * newBuffer = new TCHAR[str2transformLen + nbEOL*2 + 1];
        int j = 0;
        for (size_t i = 0 ; i < str2transformLen ; i++)
        {
            if (str2transform[i] == '\r')
            {
                newBuffer[j++] = '\\';
                newBuffer[j++] = 'r';
            }
            else if (str2transform[i] == '\n')
            {
                newBuffer[j++] = '\\';
                newBuffer[j++] = 'n';
            }
            else
            {
                newBuffer[j++] = str2transform[i];
            }
        }
        newBuffer[j++] = '\0';
		setSearchText(newBuffer);

        _options._searchType = FindExtended;
		::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, FALSE, 0);
		::SendDlgItemMessage(_hSelf, IDEXTENDED, BM_SETCHECK, TRUE, 0);
		::SendDlgItemMessage(_hSelf, IDREGEXP, BM_SETCHECK, FALSE, 0);

		delete [] newBuffer;
    }
}

void Finder::setFinderStyle()
{
	// Set global styles for the finder
	_scintView.performGlobalStyles();
	
	// Set current line background color for the finder
	const TCHAR * lexerName = ScintillaEditView::langNames[L_SEARCHRESULT].lexerName;
	LexerStyler *pStyler = (_scintView._pParameter->getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		int i = pStyler->getStylerIndexByID(SCE_SEARCHRESULT_CURRENT_LINE);
		if (i != -1)
		{
			Style & style = pStyler->getStyler(i);
			_scintView.execute(SCI_SETCARETLINEBACK, style._bgColor);
		}
	}

	_scintView.setSearchResultLexer();
	_scintView.execute(SCI_COLOURISE, 0, -1);
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

				case NPPM_INTERNAL_SCINTILLAFINFERCOPY :
				{
					_scintView.execute(SCI_COPY);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERSELECTALL :
				{
					_scintView.execute(SCI_SELECTALL);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERCLEARALL:
				{
					removeAll();
					return TRUE;
				}

				default :
				{
					return FALSE;
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
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE, TEXT("Collapse all")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE, TEXT("Uncollapse all")));
				tmp.push_back(MenuItemUnit(0, TEXT("Separator")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCOPY, TEXT("Copy")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERSELECTALL, TEXT("Select All")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCLEARALL, TEXT("Clear All")));

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

void FindIncrementDlg::setSearchText(const TCHAR * txt2find, bool isUTF8 ) {
#ifdef UNICODE
	::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
	isUTF8 = isUTF8;
#else
	if (!isUTF8)
	{
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
		return;
	}
	const int wideBufferSize = 256;
	WCHAR wchars[wideBufferSize];
	::MultiByteToWideChar(CP_UTF8, 0, txt2find, -1, wchars, wideBufferSize);
	winVer winVersion = NppParameters::getInstance()->getWinVersion();
	if (winVersion <= WV_ME) {
		//Cannot simply take txt2find since its UTF8
		char ansiBuffer[wideBufferSize];	//Assuming no more than 2 bytes for each wchar (SBCS or DBCS, no UTF8 and sorts)
		::WideCharToMultiByte(CP_ACP, 0, wchars, -1, ansiBuffer, wideBufferSize, NULL, NULL);
		::SendDlgItemMessageA(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)ansiBuffer);
	} else {
		::SendDlgItemMessageW(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)wchars);
	}
#endif
}

void FindIncrementDlg::destroy()
{
	if (_pRebar) 
	{
		_pRebar->removeBand(_rbBand.wID);
		_pRebar = NULL;
	}
	StaticDialog::destroy();
}

void FindIncrementDlg::display(bool toShow) const
{
	if (!_pRebar)
	{
		Window::display(toShow);
		return;
	}
	if (toShow)
	{
		::SetFocus(::GetDlgItem(_hSelf, IDC_INCFINDTEXT));
		// select the whole find editor text
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, EM_SETSEL, 0, -1);
	}
	_pRebar->setIDVisible(_rbBand.wID, toShow);
}

#define SHIFTED 0x8000
BOOL CALLBACK FindIncrementDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			bool isUnicode = (*(_pFRDlg->_ppEditView))->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
			FindStatus findStatus = FSFound;
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
					(*(_pFRDlg->_ppEditView))->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_INC);
					::SetFocus((*(_pFRDlg->_ppEditView))->getHSelf());

					display(false);
					return TRUE;

				case IDC_INCFINDPREVOK :
				case IDC_INCFINDNXTOK :
				case IDOK :
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._incrementalType = NextIncremental;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));
					if (LOWORD(wParam) == IDC_INCFINDPREVOK)
						fo._whichDirection = DIR_UP;
					else if (LOWORD(wParam) == IDOK)
					{
						SHORT nVirtKey = GetKeyState(VK_SHIFT);
						if (nVirtKey & SHIFTED)
							fo._whichDirection = DIR_UP;
					}
					
					generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					_pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
					setFindStatus(findStatus);
				}
				return TRUE;

				case IDC_INCFINDTEXT :
				{
					switch(HIWORD(wParam))
					{
						case EN_CHANGE :
						{
							FindOption fo;
							fo._isWholeWord = false;
							fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));
							fo._incrementalType = FirstIncremental;
							
							generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
							_pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
							setFindStatus(findStatus);
						}
					return TRUE;
					case EN_KILLFOCUS :
					case EN_SETFOCUS :
						break;
					}
				}
				return TRUE;

				case IDC_INCFINDMATCHCASE:
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._incrementalType = FirstIncremental;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

					generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					bool isFound = _pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
					setFindStatus(findStatus);
					if (!isFound)
					{
						CharacterRange range;
						(*(_pFRDlg->_ppEditView))->getSelection(range);
						(*(_pFRDlg->_ppEditView))->execute(SCI_SETSEL, (WPARAM)-1, range.cpMin);
					}
				}

				case IDC_INCFINDHILITEALL :
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._incrementalType = FirstIncremental;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

					generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					bool isHiLieAll = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDHILITEALL, BM_GETCHECK, 0, 0));
					if (str2Search == TEXT(""))
						isHiLieAll = false;
					markSelectedTextInc(isHiLieAll, &fo);
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
	CharacterRange range;
	(*(_pFRDlg->_ppEditView))->getSelection(range);

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
		return;

	TCHAR text2Find[FINDREPLACE_MAXLENGTH];
	(*(_pFRDlg->_ppEditView))->getGenericSelectedText(text2Find, FINDREPLACE_MAXLENGTH, false);	//do not expand selection (false)
	_pFRDlg->markAllInc(text2Find, opt);
}

void FindIncrementDlg::addToRebar(ReBar * rebar) 
{
	if(_pRebar)
		return;
	_pRebar = rebar;
	RECT client;
	getClientRect(client);

	winVer winVersion = (NppParameters::getInstance())->getWinVersion();
	if (winVersion <= WV_W2K)
	{
		ZeroMemory(&_rbBand, sizeof(REBARBANDINFO));
		_rbBand.cbSize  = sizeof(REBARBANDINFO);
	}
	else
	{
		ZeroMemory(&_rbBand, REBARBANDINFO_V3_SIZE);
		_rbBand.cbSize  = REBARBANDINFO_V3_SIZE;
	}

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
