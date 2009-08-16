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

#ifndef NOTEPAD_PLUS_H
#define NOTEPAD_PLUS_H

#ifndef BUFFERID_H
#include "BufferID.h"
#endif

#ifndef PARAMETERS_DEF_H
#include "Parameters_def.h"
#endif

#ifndef SCINTILLA_REF_H
#include "ScintillaRef.h"
#endif

#ifndef WINDOW_CONTROL_H
#include "Window.h"
#endif

#ifndef NOTEPAD_PLUS_MSGS_H
#include "Notepad_plus_msgs.h"
#endif

// To be removed later.  Need to be included since Parameters.h was removed from ScintillaEditView.h
#ifndef SHORTCUTS_H
#include "shortcut.h"
#endif

#ifndef NPP_STYLES
#include "npp_styles.h"  // For StyleArray, etc.
#endif

#define MENU 0x01
#define TOOLBAR 0x02

enum FileTransferMode {
	TransferClone		= 0x01,
	TransferMove		= 0x02
};

enum WindowStatus {	//bitwise mask
	WindowMainActive	= 0x01,
	WindowSubActive		= 0x02,
	WindowBothActive	= 0x03,	//little helper shortcut
	WindowUserActive	= 0x04,
	WindowMask			= 0x07
};

/*
//Plugins rely on #define's
enum Views {
	MAIN_VIEW			= 0x00,
	SUB_VIEW			= 0x01
};
*/

// Forward declarations.
class DockingManager;

class FindReplaceDlg;
class FindIncrementDlg;
class AboutDlg;
class RunDlg;
class GoToLineDlg;
class ColumnEditorDlg;
class WordStyleDlg;
class PreferenceDlg;
class WindowsMenu;
class RunMacroDlg;

class ContextMenu;
class StatusBar;
class ToolBar;
class ReBar;

class ScintillaEditView;
class DocTabView;
class IconList;
class trayIconControler;
class SplitterContainer;

class LastRecentFileList;
class SmartHighlighter;
class AutoCompletion;
class PluginsManager;
class ShortcutMapper;


struct TaskListInfo;

struct tTbData;

class TiXmlNodeA;
class TiXmlNode;

// Stuff from Parameters.h
struct CmdLineParams;
struct Session;

static TiXmlNodeA * searchDlgNode(TiXmlNodeA *node, const char *dlgTagName);

struct iconLocator {
	int listIndex;
	int iconIndex;
	generic_string iconLocation;

	iconLocator(int iList, int iIcon, const generic_string iconLoc) 
		: listIndex(iList), iconIndex(iIcon), iconLocation(iconLoc){};
};

struct VisibleGUIConf {
	bool isPostIt;
	bool isFullScreen;
	
	//Used by both views
	bool isMenuShown;
	//bool isToolbarShown;	//toolbar forcefully hidden by hiding rebar
	DWORD_PTR preStyle;

	//used by postit only
	bool isTabbarShown;
	bool isAlwaysOnTop;
	bool isStatusbarShown;

	//used by fullscreen only
	WINDOWPLACEMENT _winPlace;

	VisibleGUIConf() : 
		isPostIt(false), isFullScreen(false), isMenuShown(true),
		preStyle(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN), isTabbarShown(true),
		isAlwaysOnTop(false), isStatusbarShown(true)
	{
		_winPlace.length = 0;
	};
};

class FileDialog;

class Notepad_plus : public Window {
	enum comment_mode {cm_comment, cm_uncomment, cm_toggle};
public:
	Notepad_plus();
	virtual ~Notepad_plus();
	//(Warning -- Member with different signature hides virtual member 'Window::init(struct HINSTANCE__ *, struct HWND__ *)'
	//lint -e1411
	void init(HINSTANCE hInst, HWND parent, const TCHAR *cmdLine, CmdLineParams *cmdLineParams);
	//lint +e1411
	void killAllChildren();

    static const TCHAR * getClassName() {
		return _className;
	};
	
	void setTitle();
	void getTaskListInfo(TaskListInfo *tli);

	// For filtering the modeless Dialog message
	bool isDlgsMsg(MSG *msg, bool unicodeSupported) const;

// fileOperations
	//The doXXX functions apply to a single buffer and dont need to worry about views, with the excpetion of doClose, since closing one view doesnt have to mean the document is gone
    BufferID doOpen(const TCHAR *fileName, bool isReadOnly = false);
	bool doReload(BufferID id, bool alert = true);
	bool doSave(BufferID, const TCHAR * filename, bool isSaveCopy = false);
	void doClose(BufferID, int whichOne);
	//bool doDelete(const TCHAR *fileName) const {return ::DeleteFile(fileName) != 0;};

	void fileNew();
	void fileOpen();
	bool fileReload();
	bool fileClose(BufferID id = BUFFER_INVALID, int curView = -1);	//use curView to override view to close from
	bool fileCloseAll();
	bool fileCloseAllButCurrent();
	bool fileSave(BufferID id = BUFFER_INVALID);
	bool fileSaveAll();
	bool fileSaveAs(BufferID id = BUFFER_INVALID, bool isSaveCopy = false);
	bool fileDelete(BufferID id = BUFFER_INVALID);
	bool fileRename(BufferID id = BUFFER_INVALID);

	bool addBufferToView(BufferID id, int whichOne);
	bool moveBuffer(BufferID id, int whereTo);	//assumes whereFrom is otherView(whereTo)
	bool switchToFile(BufferID buffer);			//find buffer in active view then in other view.
// end fileOperations

	bool isFileSession(const TCHAR * filename);
	void filePrint(bool showDialog);
	bool saveScintillaParams(bool whichOne);

	bool saveGUIParams();
	void saveDockingParams();
	void saveUserDefineLangs();
	void saveShortcuts();
	void saveSession(const Session & session);
	void saveFindHistory();

	void getCurrentOpenedFiles(Session & session);

	bool fileLoadSession(const TCHAR *fn = NULL);
	const TCHAR * fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save);
	const TCHAR * fileSaveSession(size_t nbFile = 0, TCHAR ** fileNames = NULL);

	bool changeDlgLang(HWND hDlg, const char *dlgTagName, char *title = NULL, int titleBufLen = 0);
	void changeFindReplaceDlgLang();
	void changeConfigLang();
	void changeUserDefineLang();
	void changeMenuLang(generic_string & pluginsTrans, generic_string & windowTrans);
	void changeLangTabContextMenu();
	void changeLangTabDrapContextMenu();
	void changePrefereceDlgLang();
	void changeShortcutLang();
	void changeShortcutmapperLang(ShortcutMapper * sm);

	const TCHAR * getNativeTip(int btnID);
	void changeToolBarIcons();

	bool doBlockComment(comment_mode currCommentMode);
	bool doStreamComment();
	void doTrimTrailing();

	HACCEL getAccTable() const;

	bool addCurrentMacro();
	void loadLastSession();
	bool loadSession(Session* session);

	bool emergency(generic_string emergencySavedDir);

	void notifyBufferChanged(Buffer * buffer, int mask);
	bool findInFiles();
	bool replaceInFiles();
	void setFindReplaceFolderFilter(const TCHAR *dir, const TCHAR *filters);
	std::vector<generic_string> addNppComponents(const TCHAR *destDir, const TCHAR *extFilterName, const TCHAR *extFilter);

	static HWND gNppHWND;	//static handle to Notepad++ window, NULL if non-existant
private:
	static const TCHAR _className[32];
    Window *_pMainWindow;
	DockingManager* _dockingManager;

	SmartHighlighter* _smartHighlighter;

	TiXmlNode *_toolIcons;
	TiXmlNodeA *_nativeLangA;

	int _nativeLangEncoding;

    DocTabView* _mainDocTab;
    DocTabView* _subDocTab;
    DocTabView* _pDocTab;
	DocTabView* _pNonDocTab;

    ScintillaEditView* _subEditView;
    ScintillaEditView* _mainEditView;
	ScintillaEditView* _invisibleEditView;	//for searches
	ScintillaEditView* _fileEditView;		//for FileManager

    ScintillaEditView* _pEditView;
	ScintillaEditView* _pNonEditView;

    SplitterContainer* _pMainSplitter;
    SplitterContainer* _subSplitter;

	// AutoCompletions need to be after ScintillaEditViews, since their constructor depends on them. 
	// If you shuffle them, you'll get a crash on startup.
	AutoCompletion* _autoCompleteMain;
	AutoCompletion* _autoCompleteSub;	//each Scintilla has its own autoComplete

    ContextMenu* _tabPopupMenu;
    ContextMenu* _tabPopupDropMenu;

	ToolBar*	_toolBar;
	IconList* _docTabIconList;
	
    StatusBar* _statusBar;
	bool _toReduceTabBar;
	ReBar* _rebarTop;
	ReBar* _rebarBottom;

	// Dialog
	FindReplaceDlg* _findReplaceDlg;
	FindIncrementDlg* _incrementFindDlg;
    AboutDlg* _aboutDlg;
	RunDlg* _runDlg;
    GoToLineDlg* _goToLineDlg;
	ColumnEditorDlg* _colEditorDlg;
	WordStyleDlg* _configStyleDlg;
	PreferenceDlg* _preferenceDlg;
	
	// a handle list of all the Notepad++ dialogs
	std::vector<HWND> _hModelessDlgs;

	LastRecentFileList* _lastRecentFileList;

	std::vector<iconLocator> _customIconVect;

	WindowsMenu* _windowsMenu;
	HMENU _mainMenuHandle;

	bool _sysMenuEntering;

	// For FullScreen/PostIt features
	VisibleGUIConf	_beforeSpecialView;
	void fullScreenToggle();
	void postItToggle();

	// Keystroke macro recording and playback
	Macro _macro;
	bool _recordingMacro;
	RunMacroDlg* _runMacroDlg;

	// For hotspot
	bool _linkTriggered;
	bool _isDocModifing;
	bool _isHotspotDblClicked;

	//For Dynamic selection highlight
	CharacterRange _prevSelectedRange;

	struct ActivateAppInfo {
		bool _isActivated;
		int _x;
		int _y;
		ActivateAppInfo() : _isActivated(false), _x(0), _y(0){}
	} _activeAppInf;

	//Synchronized Scolling
	
	struct SyncInfo {
		int _line;
		int _column;
		bool _isSynScollV;
		bool _isSynScollH;
		SyncInfo():_line(0), _column(0), _isSynScollV(false), _isSynScollH(false){}
		bool doSync() const {return (_isSynScollV || _isSynScollH); }
	} _syncInfo;

	bool _isUDDocked;

	trayIconControler *_pTrayIco;
	int _zoomOriginalValue;

	// JOCE: move that to pointers...  That will save us another include in this header.
	Accelerator _accelerator;
	ScintillaAccelerator _scintaccelerator;

	PluginsManager* _pluginsManager;

	bool _isRTL;

	bool _isFileOpening;

	class ScintillaCtrls {
	public :
		//ScintillaCtrls();
		void init(HINSTANCE hInst, HWND hNpp);
		HWND createSintilla(HWND hParent);
		bool destroyScintilla(HWND handle2Destroy);
		void destroy();
	private:
		std::vector<ScintillaEditView *> _scintVector;
		HINSTANCE _hInst;
		HWND _hParent;
	} _scintillaCtrls4Plugins;

	std::vector<std::pair<int, int> > _hideLinesMarks;
	StyleArray _hotspotStyles;

	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL notify(SCNotification *notification);
	void specialCmd(int id, int param);
	void command(int id);

//Document management
	UCHAR _mainWindowStatus;	//For 2 views and user dialog if docked
	int _activeView;

	//User dialog docking
	void dockUserDlg();
    void undockUserDlg();

	//View visibility
	void showView(int whichOne);
	bool viewVisible(int whichOne);
	void hideView(int whichOne);
	void hideCurrentView();
	bool bothActive() { return (_mainWindowStatus & WindowBothActive) == WindowBothActive; };
	bool reloadLang();
	bool loadStyles();

	int currentView(){
		return _activeView;
	};
	int otherView(){
		return (_activeView == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	};
	int otherFromView(int whichOne){
		return (whichOne == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	};
	bool canHideView(int whichOne);	//true if view can safely be hidden (no open docs etc)

	int switchEditViewTo(int gid);	//activate other view (set focus etc)

	void docGotoAnotherEditView(FileTransferMode mode);	//TransferMode
	void docOpenInNewInstance(FileTransferMode mode, int x = 0, int y = 0);

	void loadBufferIntoView(BufferID id, int whichOne, bool dontClose = false);		//Doesnt _activate_ the buffer
	void removeBufferFromView(BufferID id, int whichOne);	//Activates alternative of possible, or creates clean document if not clean already

	bool activateBuffer(BufferID id, int whichOne);			//activate buffer in that view if found
	void notifyBufferActivated(BufferID bufid, int view);
	void performPostReload(int whichOne);
//END: Document management

	int doSaveOrNot(const TCHAR *fn);
	int doReloadOrNot(const TCHAR *fn, bool dirty);
	int doCloseOrNot(const TCHAR *fn);
	int doDeleteOrNot(const TCHAR *fn);
	int doActionOrNot(const TCHAR *title, const TCHAR *displayText, int type);
	void enableMenu(int cmdID, bool doEnable) const;
	void enableCommand(int cmdID, bool doEnable, int which) const;
	void checkClipboard();
	void checkDocState();
	void checkUndoState();
	void checkMacroState();
	void checkSyncState();
	void dropFiles(HDROP hdrop);
	void checkModifiedDocument();

    void getMainClientRect(RECT & rc) const;

	void dynamicCheckMenuAndTB() const;

	void enableConvertMenuItems(formatType f) const;

	void checkUnicodeMenuItems(UniMode um) const;

	generic_string getLangDesc(LangType langType, bool shortDesc = false);

	void setLangStatus(LangType langType);

	void setDisplayFormat(formatType f);

	void setUniModeText(UniMode um);
	
	void checkLangsMenu(int id) const ;

    void setLanguage(LangType langType);

	enum LangType menuID2LangType(int cmdID);

    int getFolderMarginStyle() const;

	void checkFolderMarginStyleMenu(int id2Check) const;
    int getFolderMaginStyleIDFrom(folderStyle fStyle) const;
	void checkMenuItem(int itemID, bool willBeChecked) const;
	void charAdded(TCHAR chAdded);
	void MaintainIndentation(TCHAR ch);
	
	void addHotSpot(bool docIsModifing = false);

    void bookmarkAdd(int lineno) const;
    void bookmarkDelete(int lineno) const;
    bool bookmarkPresent(int lineno) const;
    void bookmarkToggle(int lineno) const;
    void bookmarkNext(bool forwardScan);
	void bookmarkClearAll() const;

	void copyMarkedLines();
	void cutMarkedLines();
	void deleteMarkedLines();
	void pasteToMarkedLines();
	void deleteMarkedline(int ln);
	void replaceMarkedline(int ln, const TCHAR *str);
	generic_string getMarkedLine(int ln);

    void findMatchingBracePos(int & braceAtCaret, int & braceOpposite);
    void braceMatch();

    void activateNextDoc(bool direction);
	void activateDoc(int pos);

	void updateStatusBar();

	void showAutoComp();
	void autoCompFromCurrentFile(bool autoInsert = true);
	void showFunctionComp();

	void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
	bool replaceAllFiles();
	bool findInOpenedFiles();
	bool findInCurrentFile();

	bool matchInList(const TCHAR *fileName, const std::vector<generic_string> & patterns);
	void getMatchedFileNames(const TCHAR *dir, const std::vector<generic_string> & patterns, std::vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir);

	void doSynScorll(HWND hW);
	void setWorkingDir(const TCHAR *dir);
	bool str2Cliboard(const TCHAR *str2cpy);
	bool bin2Cliboard(const UCHAR *uchar2cpy, size_t length);

	bool getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible);
	
	int getLangFromMenuName(const TCHAR * langName);

	generic_string getLangFromMenu(const Buffer * buf);

	void setFileOpenSaveDlgFilters(FileDialog & fDlg);
	void markSelectedTextInc(bool enable);

	Style * getStyleFromName(const TCHAR *styleName);

	bool dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix = TEXT(""));	//helper func
	void drawTabbarColoursFromStylerArray();

	void loadCommandlineParams(const TCHAR * commandLine, CmdLineParams * pCmdParams);

	bool noOpenedDoc() const;
	void EnableMouseWheelZoom(bool enable);
};

#endif //NOTEPAD_PLUS_H
