#include "StaticDialog.h"

class WINRECT;
class CWinMgr;

class SizeableDlg : public StaticDialog {
	typedef StaticDialog MyBaseClass;
public:
	SizeableDlg(WINRECT* pWinMap);
	~SizeableDlg();

protected:
	CWinMgr* _winMgr;	  // window manager

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL onInitDialog();
	virtual void onSize(UINT nType, int cx, int cy);
	virtual void onGetMinMaxInfo(MINMAXINFO* lpMMI);
	virtual LRESULT onWinMgr(WPARAM wp, LPARAM lp);
};
