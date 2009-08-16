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

#include "precompiled_headers.h"
#include "WordStyleDlg.h"

#include "ColourPicker.h"
#include "WordStyleDlgRes.h"
#include "TabBar.h"
#include "Parameters.h"


class ColourStaticTextHooker {
public :
	ColourStaticTextHooker() : 
		_colour(RGB(0x00, 0x00, 0x00)),
		_oldProc(NULL)
	{}

	COLORREF setColour(COLORREF colour2Set)
	{
		COLORREF oldColour = _colour;
		_colour = colour2Set;
		return oldColour;
	}
	
	void hookOn(HWND staticHandle)
	{
		::SetWindowLongPtr(staticHandle, GWL_USERDATA, (LONG)this);
		_oldProc = (WNDPROC)::SetWindowLongPtr(staticHandle, GWL_WNDPROC, (LONG)staticProc);
	}
private :
	COLORREF _colour;
	WNDPROC _oldProc;

	static BOOL CALLBACK staticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		ColourStaticTextHooker *pColourStaticTextHooker = reinterpret_cast<ColourStaticTextHooker *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
		return pColourStaticTextHooker->colourStaticProc(hwnd, message, wParam, lParam);
	};
	BOOL CALLBACK colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

WordStyleDlg::WordStyleDlg():
	_pFgColour(NULL), _pBgColour(NULL),
	_currentLexerIndex(0), _currentThemeIndex(0),
	_hCheckBold(NULL), _hCheckItalic(NULL), _hCheckUnderline(NULL),
	_hFontNameCombo(NULL), _hFontSizeCombo(NULL), _hSwitch2ThemeCombo(NULL),
	_hFgColourStaticText(NULL),_hBgColourStaticText(NULL),
	_hFontNameStaticText(NULL), _hFontSizeStaticText(NULL),
	_hStyleInfoStaticText(NULL),
	_restoreInvalid(false), 
	_colourHooker(new ColourStaticTextHooker),
	_isDirty(false), 
	_isThemeDirty(false), 
	_isShownGOCtrls(false)
{}

WordStyleDlg::~WordStyleDlg()
{
	delete _colourHooker;
}

void WordStyleDlg::doDialog(bool isRTL)
{
	if (!isCreated())
	{
		create(IDD_STYLER_DLG, isRTL);
		prepare2Cancel();
	}

	if (!::IsWindowVisible(_hSelf))
	{
		prepare2Cancel();
	}
	display();
}

void WordStyleDlg::prepare2Cancel()
{
	_styles2restored = (NppParameters::getInstance())->getLStylerArray();
	_gstyles2restored = (NppParameters::getInstance())->getGlobalStylers();
	_gOverride2restored = (NppParameters::getInstance())->getGlobalOverrideStyle();
};

void WordStyleDlg::redraw() const
{
	assert(_pFgColour);
	assert(_pBgColour);
	_pFgColour->redraw();
	_pBgColour->redraw();
	::InvalidateRect(_hStyleInfoStaticText, NULL, TRUE);
	::UpdateWindow(_hStyleInfoStaticText);
}

void WordStyleDlg::restoreGlobalOverrideValues()
{
	GlobalOverride & gOverride = (NppParameters::getInstance())->getGlobalOverrideStyle();
	gOverride = _gOverride2restored;
}

Style & WordStyleDlg::getCurrentStyler()
{
	int styleIndex = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (styleIndex == LB_ERR)
	{
		styleIndex = 0;
	}

	if (_currentLexerIndex == 0)
	{
		return _globalStyles.getStyler(styleIndex);
	}
	else
	{
		LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(_currentLexerIndex - 1);
		return lexerStyler.getStyler(styleIndex);
	}
}

int WordStyleDlg::whichTabColourIndex()
{
	int i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return -1;
	TCHAR styleName[128];
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, (LPARAM)styleName);

	if (lstrcmp(styleName, TABBAR_ACTIVEFOCUSEDINDCATOR) == 0)
		return (int)TabBarPlus::activeFocusedTop;

	if (lstrcmp(styleName, TABBAR_ACTIVEUNFOCUSEDINDCATOR) == 0)
		return (int)TabBarPlus::activeUnfocusedTop;

	if (lstrcmp(styleName, TABBAR_ACTIVETEXT) == 0)
		return (int)TabBarPlus::activeText;

	if (lstrcmp(styleName, TABBAR_INACTIVETEXT) == 0)
		return (int)TabBarPlus::inactiveText;

	return -1;
}

void WordStyleDlg::enableFg(bool isEnable)
{
	assert(_pFgColour);
	::EnableWindow(_pFgColour->getHSelf(), isEnable);
	::EnableWindow(_hFgColourStaticText, isEnable);
}

void WordStyleDlg::enableBg(bool isEnable)
{
	assert(_pBgColour);
	::EnableWindow(_pBgColour->getHSelf(), isEnable);
	::EnableWindow(_hBgColourStaticText, isEnable);
}

void WordStyleDlg::enableFontName(bool isEnable)
{
	::EnableWindow(_hFontNameCombo, isEnable);
	::EnableWindow(_hFontNameStaticText, isEnable);
}

void WordStyleDlg::enableFontSize(bool isEnable)
{
	::EnableWindow(_hFontSizeCombo, isEnable);
	::EnableWindow(_hFontSizeStaticText, isEnable);
}

void WordStyleDlg::enableFontStyle(bool isEnable)
{
	::EnableWindow(_hCheckBold, isEnable);
	::EnableWindow(_hCheckItalic, isEnable);
	::EnableWindow(_hCheckUnderline, isEnable);
}

long WordStyleDlg::notifyDataModified()
{
	_isDirty = true;
	_isThemeDirty = true;
	::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), TRUE);
	return TRUE;
}

void WordStyleDlg::showGlobalOverrideCtrls(bool show)
{
	if (show)
	{
		updateGlobalOverrideCtrls();
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FG_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BG_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONT_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_BOLD_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_ITALIC_CHECK), show?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK), show?SW_SHOW:SW_HIDE);
	_isShownGOCtrls = show;
}


BOOL CALLBACK ColourStaticTextHooker::colourStaticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_PAINT:
        {
	        RECT rect;
            ::GetClientRect(hwnd, &rect);

            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);
    		
            ::SetTextColor(hdc, _colour);

		    // Get the default GUI font
            HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

			HANDLE hOld = SelectObject(hdc, hf);

		    // Draw the text!
            TCHAR text[MAX_PATH];
            ::GetWindowText(hwnd, text, MAX_PATH);
            ::DrawText(hdc, text, -1, &rect, DT_LEFT);
    		
            ::SelectObject(hdc, hOld);

            ::EndPaint(hwnd, &ps);

		    return TRUE;
        }

		default:
		break;
    }
    return ::CallWindowProc(_oldProc, hwnd, Message, wParam, lParam);
}
void WordStyleDlg::updateGlobalOverrideCtrls()
{
	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BG_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBg, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONT_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFont, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_FONTSIZE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableFontSize, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_BOLD_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableBold, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_ITALIC_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableItalic, 0);
	::SendDlgItemMessage(_hSelf, IDC_GLOBAL_UNDERLINE_CHECK, BM_SETCHECK, nppGUI._globalOverride.enableUnderLine, 0);
}

BOOL CALLBACK WordStyleDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			NppParameters *nppParamInst = NppParameters::getInstance();

            _hCheckBold = ::GetDlgItem(_hSelf, IDC_BOLD_CHECK);
            _hCheckItalic = ::GetDlgItem(_hSelf, IDC_ITALIC_CHECK);
			_hCheckUnderline = ::GetDlgItem(_hSelf, IDC_UNDERLINE_CHECK);
			_hFontNameCombo = ::GetDlgItem(_hSelf, IDC_FONT_COMBO);
			_hFontSizeCombo = ::GetDlgItem(_hSelf, IDC_FONTSIZE_COMBO);
			_hSwitch2ThemeCombo = ::GetDlgItem(_hSelf, IDC_SWITCH2THEME_COMBO);

			_hFgColourStaticText = ::GetDlgItem(_hSelf, IDC_FG_STATIC);
			_hBgColourStaticText = ::GetDlgItem(_hSelf, IDC_BG_STATIC);
			_hFontNameStaticText = ::GetDlgItem(_hSelf, IDC_FONTNAME_STATIC);
			_hFontSizeStaticText = ::GetDlgItem(_hSelf, IDC_FONTSIZE_STATIC);
			_hStyleInfoStaticText = ::GetDlgItem(_hSelf, IDC_STYLEDESCRIPTION_STATIC);

			_colourHooker->setColour(RGB(0xFF, 0x00, 0x00));
			_colourHooker->hookOn(_hStyleInfoStaticText);

			_currentThemeIndex = -1;
			int defaultThemeIndex = 0;
			ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
			for(size_t i = 0 ; i < themeSwitcher.size() ; i++)
			{
				std::pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(i);
				int j = ::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, (LPARAM)themeInfo.first.c_str());
				if (! themeInfo.second.compare( nppParamInst->getNppGUI()._themeName ) ) 
				{
					_currentThemeIndex = j;
				}
				if (! themeInfo.first.compare(TEXT("Default")) ) 
				{
					defaultThemeIndex = j;
				}
			}
			if (_currentThemeIndex == -1)
			{
				_currentThemeIndex = defaultThemeIndex;
			}
			::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);

			for(int i = 0 ; i < sizeof(fontSizeStrs)/(3*sizeof(TCHAR)) ; i++)
				::SendMessage(_hFontSizeCombo, CB_ADDSTRING, 0, (LPARAM)fontSizeStrs[i]);

			const std::vector<generic_string> & fontlist = (NppParameters::getInstance())->getFontList();
			for (size_t i = 0 ; i < fontlist.size() ; i++)
			{
				int j = ::SendMessage(_hFontNameCombo, CB_ADDSTRING, 0, (LPARAM)fontlist[i].c_str());
				::SendMessage(_hFontNameCombo, CB_SETITEMDATA, j, (LPARAM)fontlist[i].c_str());
			}

			_pFgColour = new ColourPicker;
			_pBgColour = new ColourPicker;
			_pFgColour->init(_hInst, _hSelf);
			_pBgColour->init(_hInst, _hSelf);

            POINT p1, p2;

            alignWith(_hFgColourStaticText, _pFgColour->getHSelf(), ALIGNPOS_RIGHT, p1);
            alignWith(_hBgColourStaticText, _pBgColour->getHSelf(), ALIGNPOS_RIGHT, p2);

            p1.x = p2.x = ((p1.x > p2.x)?p1.x:p2.x) + 10;
            p1.y -= 4; p2.y -= 4;

            ::MoveWindow((HWND)_pFgColour->getHSelf(), p1.x, p1.y, 25, 25, TRUE);
            ::MoveWindow((HWND)_pBgColour->getHSelf(), p2.x, p2.y, 25, 25, TRUE);
			_pFgColour->display();
			_pBgColour->display();


			::EnableWindow(::GetDlgItem(_hSelf, IDOK), _isDirty);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE/*!_isSync*/);

			ETDTProc enableDlgTheme = (ETDTProc)nppParamInst->getEnableThemeDlgTexture();
			if (enableDlgTheme)
			{
				enableDlgTheme(_hSelf, ETDT_ENABLETAB);
				redraw();
			}

			updateGlobalOverrideCtrls();
			loadLangListFromNppParam();
			setVisualFromStyleList();
			goToCenter();


			return TRUE;
		}

		case WM_DESTROY:
		{
			if (_pFgColour)
			{
				_pFgColour->destroy();
				delete _pFgColour;
				_pFgColour = NULL;
			}
			
			if (_pBgColour)
			{
				_pBgColour->destroy();
				delete _pBgColour;
				_pBgColour = NULL;
			}

			return TRUE;
		}

		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER))
			{
				int percent = ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
				(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
			}
			return TRUE;
		}

		case WM_COMMAND : 
		{
			if (HIWORD(wParam) == EN_CHANGE)
            {
				int editID = LOWORD(wParam);
				if (editID == IDC_USER_KEYWORDS_EDIT)
				{
					updateUserKeywords();
					notifyDataModified();
					apply();
				}
				else if (editID == IDC_USER_EXT_EDIT)
				{
					updateExtension();
					notifyDataModified();
					apply();
				}
			}
			else
			{
				switch (wParam)
				{
					case IDC_BOLD_CHECK :
						updateFontStyleStatus(BOLD_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDC_ITALIC_CHECK :
						updateFontStyleStatus(ITALIC_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDC_UNDERLINE_CHECK :
						updateFontStyleStatus(UNDERLINE_STATUS);
						notifyDataModified();
						apply();
						break;

					case IDCANCEL :
						//::MessageBox(NULL, TEXT("cancel"), TEXT(""), MB_OK);
						if (_isDirty)
						{
							NppParameters *nppParamInst = NppParameters::getInstance();
							if (_restoreInvalid) 
							{	
								generic_string str( nppParamInst->getNppGUI()._themeName );
								nppParamInst->reloadStylers( &str[0] );
							}

							LexerStylerArray & lsArray = nppParamInst->getLStylerArray();
							StyleArray & globalStyles = nppParamInst->getGlobalStylers();
							
							if (_restoreInvalid) 
							{
								_lsArray = _styles2restored = lsArray;
								_globalStyles = _gstyles2restored = globalStyles;
							}
							else 
							{
								globalStyles = _globalStyles = _gstyles2restored;
								lsArray = _lsArray = _styles2restored;
							}

							restoreGlobalOverrideValues();

							_restoreInvalid = false;
							_isDirty = false;
							_isThemeDirty = false;
							setVisualFromStyleList();

							
							//(nppParamInst->getNppGUI())._themeName
							::SendMessage(_hSwitch2ThemeCombo, CB_SETCURSEL, _currentThemeIndex, 0);
							::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
						}
						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE/*!_isSync*/);
						display(false);
						return TRUE;

					case IDC_SAVECLOSE_BUTTON :
					{
						if (_isDirty)
						{
							LexerStylerArray & lsa = (NppParameters::getInstance())->getLStylerArray();
							StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();

							_lsArray = lsa;
							_globalStyles = globalStyles;
							updateThemeName(_themeName);
							_restoreInvalid = false;

							_currentThemeIndex = ::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0);
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
							_isDirty = false;
						}
						_isThemeDirty = false;
						(NppParameters::getInstance())->writeStyles(_lsArray, _globalStyles);
						::EnableWindow(::GetDlgItem(_hSelf, IDC_SAVECLOSE_BUTTON), FALSE);
						//_isSync = true;
						display(false);
						::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
						return TRUE;
					}
					
					case IDC_SC_TRANSPARENT_CHECK :
					{
						bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
						if (isChecked)
						{
							int percent = ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
							(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
						}
						else
							(NppParameters::getInstance())->removeTransparent(_hSelf);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), isChecked);
						return TRUE;
					}

					case IDC_GLOBAL_FG_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					case  IDC_GLOBAL_BG_CHECK:
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableBg = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					case IDC_GLOBAL_FONT_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFont = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_FONTSIZE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableFontSize = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_BOLD_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableBold = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					
					case IDC_GLOBAL_ITALIC_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableItalic = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}
					case IDC_GLOBAL_UNDERLINE_CHECK :
					{
						GlobalOverride & glo = (NppParameters::getInstance())->getGlobalOverrideStyle();
						glo.enableUnderLine = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0));
						notifyDataModified();
						apply();
						return TRUE;
					}

					default:
					{
						switch (HIWORD(wParam))
						{
							case CBN_SELCHANGE : // == case LBN_SELCHANGE :
							{
								switch (LOWORD(wParam))
								{
									case IDC_FONT_COMBO :
										updateFontName();
										notifyDataModified();
										apply();
										break;
									case IDC_FONTSIZE_COMBO :
										updateFontSize();
										notifyDataModified();
										apply();
										break;
									case IDC_LANGUAGES_LIST :
									{
										int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
										if (i != LB_ERR)
										{
											bool prevThemeState = _isThemeDirty;
											setStyleListFromLexer(i);
											_isThemeDirty = prevThemeState;
										}
										break;
									}
									case IDC_STYLES_LIST :
										setVisualFromStyleList();
										break;

									case IDC_SWITCH2THEME_COMBO :
										switchToTheme();
										setVisualFromStyleList();
										notifyDataModified();
										_isThemeDirty = false;
										apply();
										break;

									NO_DEFAULT_CASE;
								}
								return TRUE;
							}

							case CPN_COLOURPICKED:	
							{
								assert(_pFgColour);
								assert(_pBgColour);
								if ((HWND)lParam == _pFgColour->getHSelf())
								{
									updateColour(C_FOREGROUND);
									notifyDataModified();
									int tabColourIndex;
									if ((tabColourIndex = whichTabColourIndex()) != -1)
									{
										//::SendMessage(_hParent, WM_UPDATETABBARCOLOUR, tabColourIndex, _pFgColour->getColour());
										TabBarPlus::setColour(_pFgColour->getColour(), (TabBarPlus::tabColourIndex)tabColourIndex);
										return TRUE;
									}
									apply();
									return TRUE;
								}
								else if ((HWND)lParam == _pBgColour->getHSelf())
								{
									updateColour(C_BACKGROUND);
									notifyDataModified();
									int tabColourIndex;
									if ((tabColourIndex = whichTabColourIndex()) != -1)
									{
										tabColourIndex = (int)tabColourIndex == TabBarPlus::inactiveText? TabBarPlus::inactiveBg : tabColourIndex;
										TabBarPlus::setColour(_pBgColour->getColour(), (TabBarPlus::tabColourIndex)tabColourIndex);
										return TRUE;
									}

									apply();
									return TRUE;
								}
								else
									return FALSE;
							}

							default :
							{
								return FALSE;
							}
						}
					}
				}
			}
		}
		break;
		
		default :
		break;
	}
	return FALSE;
}

void WordStyleDlg::loadLangListFromNppParam()
{
	NppParameters *nppParamInst = NppParameters::getInstance();
	_lsArray = nppParamInst->getLStylerArray();
    _globalStyles = nppParamInst->getGlobalStylers();

	// Clean up Language List
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_RESETCONTENT, 0, 0);

	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_ADDSTRING, 0, (LPARAM)TEXT("Global Styles"));
	// All the lexers
    for (int i = 0 ; i < _lsArray.getNbLexer() ; i++)
    {
		::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_ADDSTRING, 0, (LPARAM)_lsArray.getLexerDescFromIndex(i));
    }

	const int index2Begin = 0;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_SETCURSEL, 0, index2Begin);
	setStyleListFromLexer(index2Begin);
}

void WordStyleDlg::updateThemeName(generic_string themeName)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();
	nppGUI._themeName.assign( themeName );
}

void WordStyleDlg::updateColour(bool which)
{
	Style & style = getCurrentStyler();
	if (which == C_FOREGROUND)
	{
		assert(_pFgColour);
		style._fgColor = _pFgColour->getColour();
		if (_pFgColour->isEnabled())
			style._colorStyle |= COLORSTYLE_FOREGROUND;
		else
			style._colorStyle &= ~COLORSTYLE_FOREGROUND;
	}
	else //(which == C_BACKGROUND)
	{
		assert(_pBgColour);
		style._bgColor = _pBgColour->getColour();
		if (_pBgColour->isEnabled())
			style._colorStyle |= COLORSTYLE_BACKGROUND;
		else
			style._colorStyle &= ~COLORSTYLE_BACKGROUND;
	}
}

void WordStyleDlg::updateFontSize()
{
	Style & style = getCurrentStyler();
	int iFontSizeSel = ::SendMessage(_hFontSizeCombo, CB_GETCURSEL, 0, 0);

	TCHAR intStr[5];
	if (iFontSizeSel != 0)
	{
		::SendMessage(_hFontSizeCombo, CB_GETLBTEXT, iFontSizeSel, (LPARAM)intStr);
		if ((!intStr) || (!intStr[0])) 
			style._fontSize = -1;
		else
		{
			TCHAR *finStr;
			style._fontSize = generic_strtol(intStr, &finStr, 10);
			if (*finStr != '\0')
				style._fontSize = -1;
		}
	}
	else
		style._fontSize = 0;
}

void WordStyleDlg::updateExtension()
{
	const int NB_MAX = 256;
	TCHAR ext[NB_MAX];
	::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_GETTEXT, NB_MAX, (LPARAM)ext);
	_lsArray.getLexerFromIndex(_currentLexerIndex - 1).setLexerUserExt(ext);
}

void WordStyleDlg::updateUserKeywords()
{
	Style & style = getCurrentStyler();
	//const int NB_MAX = 2048;
	//TCHAR kw[NB_MAX];
	int len = ::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXTLENGTH, 0, 0);
	len +=1;
	TCHAR *kw = new TCHAR[len];
	::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_GETTEXT, len, (LPARAM)kw);
	style.setKeywords(kw);

	delete [] kw;
}

void WordStyleDlg::updateFontName()
{
    Style & style = getCurrentStyler();
	int iFontSel = ::SendMessage(_hFontNameCombo, CB_GETCURSEL, 0, 0);
	TCHAR *fnStr = (TCHAR *)::SendMessage(_hFontNameCombo, CB_GETITEMDATA, iFontSel, 0);
	style._fontName = fnStr;
}

void WordStyleDlg::updateFontStyleStatus(fontStyleType whitchStyle)
{
    Style & style = getCurrentStyler();
	if (style._fontStyle == -1)
		style._fontStyle = 0;

	int fontStyle = FONTSTYLE_UNDERLINE;
	HWND hWnd = _hCheckUnderline;

	if (whitchStyle == BOLD_STATUS)
	{
		fontStyle = FONTSTYLE_BOLD;
		hWnd = _hCheckBold;
	}
	if (whitchStyle == ITALIC_STATUS)
	{
		fontStyle = FONTSTYLE_ITALIC;
		hWnd = _hCheckItalic;
	}

	int isChecked = ::SendMessage(hWnd, BM_GETCHECK, 0, 0);
	if (isChecked != BST_INDETERMINATE)
	{
		if (isChecked == BST_CHECKED)
			style._fontStyle |= fontStyle;
		else
			style._fontStyle &= ~fontStyle;
	}
}

void WordStyleDlg::switchToTheme()
{
	int iSel = ::SendMessage(_hSwitch2ThemeCombo, CB_GETCURSEL, 0, 0);

	generic_string prevThemeName(_themeName);
	_themeName.clear();
	
	NppParameters *nppParamInst = NppParameters::getInstance();
    ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
	std::pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(iSel);
    _themeName = themeInfo.second;

	if (_isThemeDirty)
	{
		TCHAR themeFileName[MAX_PATH];
		lstrcpy(themeFileName, prevThemeName.c_str());
		PathStripPath(themeFileName);
		PathRemoveExtension(themeFileName);
		int mb_response =
			::MessageBox( _hSelf,
				TEXT(" Unsaved changes are about to be discarded!\n") 
				TEXT(" Do you want to save your changes before switching themes?"),
				themeFileName,
				MB_ICONWARNING | MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND );
		if ( mb_response == IDYES )
			(NppParameters::getInstance())->writeStyles(_lsArray, _globalStyles);
	}
	nppParamInst->reloadStylers(&_themeName[0]);

	loadLangListFromNppParam();
	_restoreInvalid = true;

}

void WordStyleDlg::setStyleListFromLexer(int index)
{
    _currentLexerIndex = index;

    // Fill out Styles listbox
    // Before filling out, we clean it
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_RESETCONTENT, 0, 0);

	if (index)
	{
		const TCHAR *langName = _lsArray.getLexerNameFromIndex(index - 1);
		const TCHAR *ext = NppParameters::getInstance()->getLangExtFromName(langName);
		const TCHAR *userExt = (_lsArray.getLexerStylerByName(langName))->getLexerUserExt();
		::SendDlgItemMessage(_hSelf, IDC_DEF_EXT_EDIT, WM_SETTEXT, 0, (LPARAM)(ext));
		::SendDlgItemMessage(_hSelf, IDC_USER_EXT_EDIT, WM_SETTEXT, 0, (LPARAM)(userExt));
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_EDIT), index?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_EDIT), index?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_EXT_STATIC), index?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL2_STATIC), index?SW_SHOW:SW_HIDE);

	StyleArray & lexerStyler = index?_lsArray.getLexerFromIndex(index-1):_globalStyles;

    for (int i = 0 ; i < lexerStyler.getNbStyler() ; i++)
    {
        Style & style = lexerStyler.getStyler(i);
		::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_ADDSTRING, 0, (LPARAM)style._styleDesc.c_str());
    }
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_SETCURSEL, 0, 0);
    setVisualFromStyleList();
}

void WordStyleDlg::setVisualFromStyleList() 
{
	showGlobalOverrideCtrls(false);

    Style & style = getCurrentStyler();

	// Global override style
	if (style._styleDesc == TEXT("Global override"))
	{
		showGlobalOverrideCtrls(true);
	}

    //--Warning text
    //bool showWarning = ((_currentLexerIndex == 0) && (style._styleID == STYLE_DEFAULT));//?SW_SHOW:SW_HIDE;

	COLORREF color = RGB(0x00, 0x00, 0xFF);
	TCHAR str[256];

	str[0] = '\0';
	
	int i = ::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	::SendDlgItemMessage(_hSelf, IDC_LANGUAGES_LIST, LB_GETTEXT, i, (LPARAM)str);

	i = ::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR)
		return;
	TCHAR styleName[64];
	::SendDlgItemMessage(_hSelf, IDC_STYLES_LIST, LB_GETTEXT, i, (LPARAM)styleName);

	lstrcat(lstrcat(str, TEXT(" : ")), styleName);

	// PAD for fix a display glitch
	lstrcat(str, TEXT("          "));
	_colourHooker->setColour(color);
	::SetWindowText(_hStyleInfoStaticText, str);

	//-- 2 couleurs : fg et bg
	bool isEnable = false;
	if (HIBYTE(HIWORD(style._fgColor)) != 0xFF)
	{
		assert(_pFgColour);
		_pFgColour->setColour(style._fgColor);
		_pFgColour->setEnabled((style._colorStyle & COLORSTYLE_FOREGROUND) != 0);
		isEnable = true;
	}
	enableFg(isEnable);

	isEnable = false;
	if (HIBYTE(HIWORD(style._bgColor)) != 0xFF)
	{
		assert(_pBgColour);
		_pBgColour->setColour(style._bgColor);
		_pBgColour->setEnabled((style._colorStyle & COLORSTYLE_BACKGROUND) != 0);
		isEnable = true;
	}
	enableBg(isEnable);

	//-- font name
	isEnable = true;
	int iFontName = ::SendMessage(_hFontNameCombo, CB_FINDSTRING, 1, (LPARAM)style._fontName.c_str());
	if (iFontName == CB_ERR)
		iFontName = 0;
	::SendMessage(_hFontNameCombo, CB_SETCURSEL, iFontName, 0);
	enableFontName(isEnable);

	//-- font size
	isEnable = false;
	TCHAR intStr[5] = TEXT("");
	int iFontSize = 0;
	if (style._fontSize != -1)
	{
		wsprintf(intStr, TEXT("%d"), style._fontSize);
		iFontSize = ::SendMessage(_hFontSizeCombo, CB_FINDSTRING, 1, (LPARAM)intStr);
		isEnable = true;
	}
	::SendMessage(_hFontSizeCombo, CB_SETCURSEL, iFontSize, 0);
	enableFontSize(isEnable);

	//-- font style : bold et italic
	isEnable = false;
	int isBold, isItalic, isUnderline;
	if (style._fontStyle != -1)
	{
		isBold = (style._fontStyle & FONTSTYLE_BOLD)?BST_CHECKED:BST_UNCHECKED;
		isItalic = (style._fontStyle & FONTSTYLE_ITALIC)?BST_CHECKED:BST_UNCHECKED;
		isUnderline = (style._fontStyle & FONTSTYLE_UNDERLINE)?BST_CHECKED:BST_UNCHECKED;
		::SendMessage(_hCheckBold, BM_SETCHECK, isBold, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, isItalic, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, isUnderline, 0);
		isEnable = true;
	}
	else // -1 est comme 0
	{
		::SendMessage(_hCheckBold, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckItalic, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendMessage(_hCheckUnderline, BM_SETCHECK, BST_UNCHECKED, 0);
	}

    enableFontStyle(isEnable);


	//-- Default Keywords
	bool shouldBeDisplayed = style._keywordClass != -1;
	if (shouldBeDisplayed)
	{
		LexerStyler & lexerStyler = _lsArray.getLexerFromIndex(_currentLexerIndex - 1);

		LangType lType = NppParameters::getLangIDFromStr(lexerStyler.getLexerName());
		if (lType == L_TXT)
		{
			generic_string str = lexerStyler.getLexerName();
			str += TEXT(" is not defined in NppParameters::getLangIDFromStr()");
				printStr(str.c_str());
		}
		NppParameters *pNppParams = NppParameters::getInstance();
		const TCHAR *kws = pNppParams->getWordList(lType, style._keywordClass);
		if (!kws)
			kws = TEXT("");
		::SendDlgItemMessage(_hSelf, IDC_DEF_KEYWORDS_EDIT, WM_SETTEXT, 0, (LPARAM)(kws));

		const TCHAR *ckwStr = (style._keywords)?style._keywords->c_str():TEXT("");
		::SendDlgItemMessage(_hSelf, IDC_USER_KEYWORDS_EDIT, WM_SETTEXT, 0, (LPARAM)(ckwStr));
	}
	int showOption = shouldBeDisplayed?SW_SHOW:SW_HIDE;
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_EDIT), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_EDIT),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_DEF_KEYWORDS_STATIC), showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_USER_KEYWORDS_STATIC),showOption);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PLUSSYMBOL_STATIC),showOption);

    redraw();
}

void WordStyleDlg::create(int dialogID, bool isRTL)
{
	StaticDialog::create(dialogID, isRTL);

	if ((NppParameters::getInstance())->isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), SW_SHOW);
		
		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
		if (!(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_SC_PERCENTAGE_SLIDER, BM_GETCHECK, 0, 0)))
			::EnableWindow(::GetDlgItem(_hSelf, IDC_SC_PERCENTAGE_SLIDER), FALSE);
	}
}

void WordStyleDlg::apply()
{
	LexerStylerArray & lsa = (NppParameters::getInstance())->getLStylerArray();
	StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();
	lsa = _lsArray;
	globalStyles = _globalStyles; 
	
	::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);
	::SendMessage(_hParent, WM_UPDATESCINTILLAS, 0, 0);
}

void WordStyleDlg::addLastThemeEntry()
{
	NppParameters *nppParamInst = NppParameters::getInstance();
	ThemeSwitcher & themeSwitcher = nppParamInst->getThemeSwitcher();
	std::pair<generic_string, generic_string> & themeInfo = themeSwitcher.getElementFromIndex(themeSwitcher.size() - 1);
	::SendMessage(_hSwitch2ThemeCombo, CB_ADDSTRING, 0, (LPARAM)themeInfo.first.c_str());
};
