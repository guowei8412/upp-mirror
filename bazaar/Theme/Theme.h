#ifndef _Skulpture_Theme_h_
#define _Skulpture_Theme_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

class Theme
{
private:
	TextSettings ini;
	String folder;
	
	bool useButton;
	bool useHeader;
	bool useProgress;
	bool useTab;
	bool useDropC;
	bool useDropL;
	
	void LoadEditField(EditField::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadButton(Button::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadToolButton(ToolButton::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadMenuBar(MenuBar::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadToolBar(ToolBar::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadOption0(const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadOption1(const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadOption2(const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadSwitch0(const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadSwitch1(const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadScrollBar(ScrollBar::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadMultiButton(MultiButton::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadTabCtrl(TabCtrl::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadProgress(ProgressIndicator::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	void LoadHeader(HeaderCtrl::Style& d, const VectorMap<String, String>& set, const String& dir, const String& file);
	
	Vector<String> Fill(const VectorMap<String, String>& set, String val, int count = 4);
	
	void Load();
	int GetGroup(const String& group);
	
public:
	Theme& Load(const String& fileName);
	Theme& Apply();
	
	bool   HasButton()						{ return GetGroup("button") >= 0; }
	Theme& UseButton(bool use)				{ useButton = use; return *this; }
	bool   HasHeaderCtrl()					{ return GetGroup("headerctrl") >= 0; }
	Theme& UseHeaderCtrl(bool use)			{ useHeader = use; return *this; }
	bool   HasProgressIndicator()			{ return GetGroup("progress") >= 0; }
	Theme& UseProgressIndicator(bool use)	{ useProgress = use; return *this; }
	bool   HasTabCtrl()						{ return GetGroup("tabctrl") >= 0; }
	Theme& UseTabCtrl(bool use)				{ useTab = use; return *this; }
	bool   HasDropChoice()					{ return GetGroup("dropchoice") >= 0; }
	Theme& UseDropChoice(bool use)			{ useDropC = use; return *this; }
	bool   HasDropList()					{ return GetGroup("droplist") >= 0; }
	Theme& UseDropList(bool use)			{ useDropL = use; return *this; }	
		
	Theme(): useButton(true), useHeader(true), useProgress(true), useTab(true),
				useDropC(true), useDropL(true)			{}
};

#endif
