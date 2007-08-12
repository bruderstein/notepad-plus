/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef WORD_STYLE_H
#define WORD_STYLE_H

#include "Window.h"
#include "ColourPicker.h"
#include "StaticDialog.h"
#include "WordStyleDlgRes.h"
#include "TabBar.h"
#include "Parameters.h"
#include "resource.h"

#define WM_UPDATESCINTILLAS  (WORDSTYLE_USER + 1) //GlobalStyleDlg's msg 2 send 2 its parent
#define WM_DATA_MODIFIED     (WORDSTYLE_USER + 2) //WordStyleDlg's msg 2 send 2 its parent(GlobalStyleDlg)

enum fontStyleType {BOLD_STATUS, ITALIC_STATUS, UNDERLINE_STATUS};

const bool C_FOREGROUND = false;
const bool C_BACKGROUND = true;


class ColourStaticTextHooker {
public :
	ColourStaticTextHooker() : _colour(RGB(0x00, 0x00, 0x00))/*, _hFont(NULL)*/ {};

	COLORREF setColour(COLORREF colour2Set) {
		COLORREF oldColour = _colour;
		_colour = colour2Set;
		return oldColour;
	};
	void hookOn(HWND staticHandle) {
		::SetWindowLong(staticHandle, GWL_USERDATA, (LONG)this);
		_oldProc = (WNDPROC)::SetWindowLong(staticHandle, GWL_WNDPROC, (LONG)staticProc);
	};
private :
	COLORREF _colour;
	WNDPROC _oldProc;

	static BOOL CALLBACK staticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
		ColourStaticTextHooker *pColourStaticTextHooker = reinterpret_cast<ColourStaticTextHooker *>(::GetWindowLong(hwnd, GWL_USERDATA));
		return pColourStaticTextHooker->colourStaticProc(hwnd, message, wParam, lParam);
	}; 
	BOOL CALLBACK colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

class WordStyleDlg : public StaticDialog
{
public :
	WordStyleDlg():_isDirty(false), _isSync(true){_originalWarning[0] = '\0';};

    void init(HINSTANCE hInst, HWND parent)	{
        Window::init(hInst, parent);
	};

	virtual void create(int dialogID, bool isRTL = false);

    void doDialog(bool isRTL = false) {
    	if (!isCreated())
			create(IDD_STYLER_DLG, isRTL);
	    display();
    };

    virtual void redraw() const {
        _pFgColour->redraw();
        _pBgColour->redraw();
		//StaticDialog::redraw();
		::InvalidateRect(_hStyleInfoStaticText, NULL, TRUE);
		::UpdateWindow(_hStyleInfoStaticText);
    };



private :
    ColourPicker *_pFgColour;
    ColourPicker *_pBgColour;

    int _currentLexerIndex;

    HWND _hCheckBold;
    HWND _hCheckItalic;
	HWND _hCheckUnderline;
    HWND _hFontNameCombo;
    HWND _hFontSizeCombo;

	HWND _hFgColourStaticText;
	HWND _hBgColourStaticText;
	HWND _hFontNameStaticText;
	HWND _hFontSizeStaticText;
	HWND _hStyleInfoStaticText;
	char _originalWarning[256];

	LexerStylerArray _lsArray;
    StyleArray _globalStyles;

	ColourStaticTextHooker colourHooker;

	bool _isDirty;
    bool _isSync;

	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);


	Style & getCurrentStyler() {
		int styleIndex = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
        if (_currentLexerIndex == 0)
            return _globalStyles.getStyler(styleIndex);
        else
        {
		    LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(_currentLexerIndex - 1);
		    return lexerStyler.getStyler(styleIndex);
        }
	};

	void updateColour(bool which);
	void updateFontStyleStatus(fontStyleType whitchStyle);
	void updateExtension();
	void updateFontName();
	void updateFontSize();
	void updateUserKeywords();

	void enableFg(bool isEnable) {
		::EnableWindow(_pFgColour->getHSelf(), isEnable);
		::EnableWindow(_hFgColourStaticText, isEnable);
	};

	void enableBg(bool isEnable) {
		::EnableWindow(_pBgColour->getHSelf(), isEnable);
		::EnableWindow(_hBgColourStaticText, isEnable);
	};

	void enableFontName(bool isEnable) {
		::EnableWindow(_hFontNameCombo, isEnable);
		::EnableWindow(_hFontNameStaticText, isEnable);
	};

	void enableFontSize(bool isEnable) {
		::EnableWindow(_hFontSizeCombo, isEnable);
		::EnableWindow(_hFontSizeStaticText, isEnable);
	};

	void enableFontStyle(bool isEnable) {
		::EnableWindow(_hCheckBold, isEnable);
		::EnableWindow(_hCheckItalic, isEnable);
		::EnableWindow(_hCheckUnderline, isEnable);
	};
    long notifyDataModified() {
		_isDirty = true;
		::EnableWindow(::GetDlgItem(_hSelf, IDOK), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), TRUE);
		return TRUE;
    }
	void setStyleListFromLexer(int index);
    void setVisualFromStyleList();
};

#endif //WORD_STYLE_H
