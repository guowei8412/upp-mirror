#include "DockCont.h"
#include "Docking.h"

// ImgButton
void ImgButton::Paint(Draw &w)
{
	Size sz = GetSize();
	if (look)
		ChPaint(w, sz, look[Pusher::GetVisualState()]);
	int dx = IsPush() * -1;
	int dy = IsPush();
	if (!img.IsEmpty()) {
		Size isz = img.GetSize();
		w.DrawImage((sz.cx - isz.cx) / 2 + dx, (sz.cy - isz.cy) / 2 + dy, img);	
	}
}

// DockCont (Dockable Container)
#if	defined(PLATFORM_WIN32)
LRESULT DockCont::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCRBUTTONDOWN) {
		WindowMenu();
		return 1L;
	}
	if (message == WM_NCLBUTTONDOWN && IsFloating() && base)
		dragging = 1;
	if (message == WM_NCLBUTTONUP)
		dragging = 0;
	if (message == WM_MOVE && IsFloating() && base && GetMouseLeft() && dragging) {
		if (dragging == 1)	{
			MoveBegin();
			dragging++;
		}
		Moving();
		dragging = true;
	}
	else if (message == WM_EXITSIZEMOVE && IsFloating() && base && !GetMouseLeft() && dragging) {
		MoveEnd();
		dragging = false;
	}
		
	return TopWindow::WindowProc(message, wParam, lParam);
}

void DockCont::StartMouseDrag(const Point &p)
{
	SendMessage(GetHWND(), WM_NCLBUTTONDOWN, 2, MAKELONG(p.x, p.y));	
}
#elif defined(PLATFORM_X11)
void DockCont::EventProc(XWindow& w, XEvent *event)
{
	
	if (IsOpen()) {
		switch(event->type) {
		case ConfigureNotify:{
			XConfigureEvent& e = event->xconfigure;
			if (Point(e.x, e.y) != GetScreenRect().TopLeft()) {
				if (!dragging)
					MoveBegin();
				Moving();				
				SetFocus();
				dragging = true;
			}
			}
			break;
		case FocusIn:
			XFocusChangeEvent &e = event->xfocus;
			if (e.mode == NotifyUngrab && dragging) {
				dragging = false;
				MoveEnd();
//				SetFocus();
				return;
			}
			break;
		}
	}
	TopWindow::EventProc(w, event);	
}

void DockCont::StartMouseDrag(const Point &p)
{
	Atom xwndDrag = XAtom("_NET_WM_MOVERESIZE");
	XEvent e;
	Zero(e);
	e.xclient.type = ClientMessage;
	e.xclient.message_type = xwndDrag;
	e.xclient.window = GetWindow();
	e.xclient.format = 32;
	e.xclient.display = Xdisplay;
	e.xclient.send_event = XTrue;
	e.xclient.data.l[0] = p.x;
	e.xclient.data.l[1] = p.y;
	e.xclient.data.l[2] = 8;
	e.xclient.data.l[3] = 1;
	e.xclient.data.l[4] = 0;	
	
	XUngrabPointer( Xdisplay, CurrentTime );
	XSendEvent(Xdisplay, RootWindow(Xdisplay, Xscreenno), XFalse, SubstructureNotifyMask, &e);
	XFlush(Xdisplay);
}
#endif

void DockCont::Paint(Draw &w)
{
	TopWindow::Paint(w);
	if (!IsFloating() && !IsTabbed()) {
		DockableCtrl *dc = GetCurrent0();
		if (!dc) return;
		const DockableCtrl::Style &s = *style;
		//bool focus = HasFocusDeep() ? 1 : 0;
		Rect r = GetSize();
		const Rect &m = s.handle_margins;
		Point p;
		
		if (s.frame) {
			s.frame->FramePaint(w, r);
			s.frame->FrameLayout(r);
		}
		
		int hsz = GetHandleSize(s);
		if (s.handle_vert) {
			r.right = r.left + hsz;	
			p = Point(r.left-1 + m.left, r.bottom - m.bottom);
		}
		else {
			r.bottom = r.top + hsz;
			p = Point(r.left + m.left, r.top + m.top);
		}
		ChPaint(w, r, s.handle[focus]);
		
		Image img = dc->GetIcon();
		if (!img.IsEmpty()) {
			int isz = hsz;
			if (s.handle_vert) {
				isz -= (m.left + m.right);
				p.y -= hsz;
				ChPaint(w, max(p.x+m.left, r.left), p.y, isz, isz, img);
				p.y -= 2;
			}
			else {
				isz -= (m.top + m.bottom);
				ChPaint(w, p.x, max(p.y, r.top), isz, isz, img);
				p.x += isz + 2;
			}
		}
		if (!s.title_font.IsNull()) {
			if (s.handle_vert)
				r.top = GetClipWidth(r).y + m.top;
			else
			 	r.right = GetClipWidth(r).x - m.right;
			w.Clip(r);
			WString text = IsNull(dc->GetGroup()) ? dc->GetTitle() : (WString)Format("%s (%s)", dc->GetTitle(), dc->GetGroup());
			w.DrawText(p.x, p.y, s.handle_vert ? 900 : 0, text, s.title_font, s.title_ink[focus]);
			w.End();
		}
	}
}

Point DockCont::GetClipWidth(const Rect &r) const
{
	return (windowpos.GetParent() && windowpos.IsShown()) ? windowpos.GetRect().BottomLeft() :
		   (autohide.GetParent() && autohide.IsShown())   ? autohide.GetRect().BottomLeft() : 
		   (close.GetParent() && close.IsShown())   	  ? close.GetRect().BottomLeft() : 
		   r.TopRight();
}

void DockCont::LeftDrag(Point p, dword keyflags)
{
	Rect r = GetSize();
	if (style->frame) style->frame->FrameLayout(r);
	if (r.Contains(p))
		MoveBegin();
}

void DockCont::Layout()
{
	if (waitsync) {
		waitsync = false;
		if (GetCount() == 1) {
			Value v = tabbar.Get(0);
			if (IsDockCont(v)) {
				DockCont *dc = ContCast(v);
				AddFrom(*dc);
				base->CloseContainer(*dc);
				RefreshLayout();
			}
		}	
		if (!tabbar.GetCount())
			base->CloseContainer(*this);
		TabSelected();
	}
}

void DockCont::ChildRemoved(Ctrl *child)
{
	if (child->GetParent() != this || !tabbar.GetCount()) return;
	Ctrl *c = dynamic_cast<DockableCtrl *>(child);
	if (!c) c = dynamic_cast<DockCont *>(child);
	if (c)
		for (int i = 0; i < GetCount(); i++)
			if (c == GetCtrl(i)) {
				tabbar.Close(i);
				waitsync = true;
				break;				
			}
}

void DockCont::ChildAdded(Ctrl *child)
{
	if (child->GetParent() != this) 
		return;
	else if (DockableCtrl *dc = dynamic_cast<DockableCtrl *>(child)) {
		Value v = ValueCast(dc);
		tabbar.Insert(0, v, dc->GetGroup(), true);
		SyncSize(*dc, dc->GetStyle());
	}	
	else if (DockCont *dc = dynamic_cast<DockCont *>(child)) {
		Value v = ValueCast(dc);
		tabbar.Insert(0, v, Null, true);
		SyncSize(*dc, dc->GetCurrent().GetStyle());
	}	
	else 
		return;	
	TabSelected();	
	if (GetCount() >= 2) RefreshLayout();
}

void 	DockCont::MoveBegin()		{ base->ContainerDragStart(*this); }
void 	DockCont::Moving()			{ base->ContainerDragMove(*this); }
void 	DockCont::MoveEnd()			{ base->ContainerDragEnd(*this); }	

void DockCont::WindowMenu()
{
	MenuBar bar;
	DockContMenu menu(base);
	menu.ContainerMenu(bar, this, true);
	bar.Execute();
}

void DockCont::TabSelected() 
{
	int ix = tabbar.GetCursor();
	if (ix >= 0) {
		DockableCtrl *dc = Get0(ix);
		if (!dc) return;
		Ctrl *ctrl = GetCtrl(ix);
		if (ctrl != &(base->GetHighlightCtrl())) {
			Ctrl *first = &tabbar;
			for (Ctrl *c = first->GetNext(); c; c = c->GetNext())
				if (c != ctrl) c->Hide();
		}
		style = &dc->GetStyle();
		Icon(dc->GetIcon()).Title(dc->GetTitle());
		SyncSize(*ctrl, *style);
		SyncButtons(!IsTabbed() && !IsFloating());
		ctrl->Show();
		if (IsTabbed()) {
			DockCont *c = static_cast<DockCont *>(GetParent());
			c->tabbar.SyncRepos();
			c->TabSelected();
			c->RefreshFrame();
		}
		else
			Refresh();
	}
}

void DockCont::TabDragged(int ix) 
{
	if (ix >= 0) {
		// Special code needed
		Value v = tabbar.Get(ix);
		if (IsDockCont(v)) {
			DockCont *c = ContCast(v);
			c->Remove();
			base->FloatFromTab(*this, *c);
		}
		else {
			DockableCtrl *c = DockCast(v);
			c->Remove();
			base->FloatFromTab(*this, *c);
		}
		if (tabbar.IsAutoHide()) 
			RefreshLayout();		
	}
}

void DockCont::TabContext(int ix)
{
	MenuBar bar;
	DockContMenu menu(base);
	Value v = tabbar.Get(ix);
	if (IsDockCont(v))
		menu.ContainerMenu(bar, ContCast(v), false);
	else
		menu.WindowMenu(bar, DockCast(v));
	bar.Separator();
	tabbar.ContextMenu(bar);
	bar.Execute();
}

void DockCont::TabClosed(Value v)
{
	CtrlCast(v)->Remove();
	if (IsDockCont(v)) base->CloseContainer(*ContCast(v)); 
	waitsync = true;
	Layout();
	if (tabbar.GetCount() == 1) RefreshLayout();
}

void DockCont::CloseAll()
{
	base->CloseContainer(*this);
}

void DockCont::Float()
{
	base->FloatContainer(*this);
}

void DockCont::Dock(int align)
{
	base->DockContainer(align, *this);
}

void DockCont::AutoHideAlign(int align)
{
	base->AutoHideContainer((align == DockWindow::DOCK_NONE) ? DockWindow::DOCK_TOP : align, *this);
}

void DockCont::AddFrom(DockCont &cont, int except)
{
	bool all = except < 0 || except >= cont.GetCount();
	for (int i = cont.GetCount() - 1; i >= 0; i--)
		if (i != except) {
			Ctrl *c = cont.GetCtrl(i);
			c->Remove();
			Add(*c);				
		}
	if (all)
		cont.tabbar.Clear();
}

void DockCont::Clear()
{
	for (int i = 0; i < tabbar.GetCount(); i++)
		CtrlCast(tabbar.Get(i))->Close();
	tabbar.Clear();
}

int DockCont::GetHandleSize(const DockableCtrl::Style &s) const
{
	return max((s.title_font.IsNull() ? 0 : s.title_font.GetHeight()), s.btnsize)
			 + s.handle_margins.top + s.handle_margins.bottom; 
}

Rect DockCont::GetHandleRect(const DockableCtrl::Style &s) const
{
	Rect r = GetFrameRect(s);
	if (s.handle_vert)
		r.left += GetHandleSize(s);
	else
		r.top += GetHandleSize(s);
	return r;
}

Rect DockCont::GetFrameRect(const DockableCtrl::Style &s) const
{
	Rect r(0, 0, 0, 0);
	if (s.frame) {
		s.frame->FrameLayout(r);
		r.right = -r.right;
		r.bottom = -r.bottom;	
	}
	return r;
}

void DockCont::SyncSize(Ctrl &c, const DockableCtrl::Style &s)
{
	if(!IsFloating() && !IsTabbed()) {
		Rect r = GetHandleRect(s);
		c.HSizePos(r.left, r.right).VSizePos(r.top, r.bottom);	
	}
	else
		c.SizePos();
}

void DockCont::SyncCurrent()
{
	Value v = ~tabbar;
	Ctrl *c = NULL;
	DockableCtrl *dc = NULL;
	if (IsDockCont(v)) {
		DockCont *cont = ContCast(v);
		c = cont;
		dc = &cont->GetCurrent();
	}
	else {
		dc = DockCast(v);
		c = dc;
	}
	SyncSize(*c, dc->GetStyle());
}

void DockCont::StateDocked(DockWindow &dock)
{
	base = &dock;
	SyncButtons(true);
	dockstate = STATE_DOCKED;	
	if (tabbar.HasCursor()) SyncCurrent();
	Show(); 
}

void DockCont::StateFloating(DockWindow &dock)
{
	SyncButtons(false); 
	base = &dock; 
	dockstate = STATE_FLOATING;
	if (tabbar.HasCursor()) SyncCurrent();
	Show(); 
}

void DockCont::SyncButtons(bool show)
{
	if (show && base && style) {
		Ctrl::LogPos lp;
		const DockableCtrl::Style &s = *style;
		Rect r = GetFrameRect(s);
		int btnsize = GetHandleSize(s) - 3;
		
		Logc &inc = s.handle_vert ? lp.y : lp.x;
		lp.x = s.handle_vert ? Ctrl::Logc(ALIGN_LEFT, r.left+1, btnsize) : Ctrl::Logc(ALIGN_RIGHT, r.right+1, btnsize);
		lp.y = Ctrl::Logc(ALIGN_TOP, r.top+1, btnsize);		
		
		if (close.GetParent()) {
			close.SetLook(s.close).SetPos(lp).Show();
			inc.SetA(inc.GetA() + btnsize + 1);
		}
		bool ah = base->IsAutoHide();
		autohide.Show(ah);
		if (ah && autohide.GetParent()) {
			autohide.SetLook(s.autohide).SetPos(lp);
			inc.SetA(inc.GetA() + btnsize + 1);		
		}
		if (windowpos.GetParent())
			windowpos.SetLook(s.windowpos).SetPos(lp).Show();
	}
	else {
		close.Hide();
		autohide.Hide();
		windowpos.Hide();
	}
}

void DockCont::GroupRefresh()
{
	for (int i = 0; i < tabbar.GetCount(); i++)
		if (!IsDockCont(tabbar.Get(i)))
			tabbar.SetTabGroup(i, DockCast(tabbar.Get(i))->GetGroup());
	Refresh();
}

void DockCont::GetGroups(Vector<String> &groups)
{
	for (int i = 0; i < tabbar.GetCount(); i++) {
		Value v = tabbar.Get(i);
		if (IsDockCont(v))
			ContCast(v)->GetGroups(groups);
		else {
			DockableCtrl *dc = DockCast(v);
			String g = dc->GetGroup();
			if (!g.IsEmpty()) {
				bool found = false;
				for (int j = 0;	j < groups.GetCount(); j++)
					if (groups[j] == g) {
						found = true;
						break;
					}
				if (!found)
					groups.Add(g);
			}				
		}
	}
}

void DockCont::WindowButtons(bool menu, bool hide, bool _close)
{
	AddRemoveButton(windowpos, menu);
	AddRemoveButton(autohide, hide);
	AddRemoveButton(close, _close);
	SyncButtons(!IsTabbed() && !IsFloating());
}

void DockCont::AddRemoveButton(Ctrl &c, bool state)
{
	if (state && !c.GetParent()) 
		Add(c); 
	else if (!state) 
		c.Remove();	
}

void DockCont::Highlight()
{
	if (!style || (!IsOpen() && !IsPopUp() && !GetParent())) return;
	ViewDraw v(this); 
	Rect h = GetHandleRect(*style);
	Rect r = GetSize();
	r.left += h.left;
	r.top += h.top;
	r.right -= h.right;
	r.bottom -= h.bottom; 
	ChPaint(v, r, style->highlight);
}

void DockCont::AddContainerSize(Size &sz) const
{
	if (style) {
		int d = (tabbar.IsAutoHide() ? 0 : tabbar.GetFrameSize()) + ((!IsFloating() || dragging) ? GetHandleSize(*style) : 0);
		style->handle_vert ? sz.cx += d : sz.cy += d;
	}	
}

Size DockCont::GetMinSize() const
{ 
	Size sz = tabbar.GetCount() ? GetCurrent().GetMinSize() : Size(0, 0); 
	AddContainerSize(sz);
	return sz;
}

Size DockCont::GetMaxSize() const	
{ 
	Size sz = tabbar.GetCount() ? GetCurrent().GetMaxSize() : Size(0, 0);
	AddContainerSize(sz);
	return sz;
}

Size DockCont::GetStdSize() const
{
	Size sz = usersize;
	if (IsNull(sz.cx) || IsNull(sz.cy)) {
		Size std = GetCurrent().GetStdSize();
		if (IsNull(sz.cx)) sz.cx = std.cx;
		if (IsNull(sz.cy)) sz.cy = std.cy;
	}
	AddContainerSize(sz);
	return sz;
}

void DockCont::SyncUserSize(bool h, bool v)
{
	Size sz = GetSize();
	if (h) usersize.cx = sz.cx;
	if (v) usersize.cy = sz.cy;
}

int DockCont::GetDockAlign() const
{
	return base->GetDockAlign(*this); 	
}

bool DockCont::IsDockAllowed(int align, int dc_ix) const
{
	if (dc_ix >= 0) return IsDockAllowed0(align, tabbar.Get(dc_ix));
	else if (!base->IsDockAllowed(align)) return false;
	
	for (int i = 0; i < tabbar.GetCount(); i++)
		if (!IsDockAllowed0(align, tabbar.Get(i))) return false;
	return true;
}

bool DockCont::IsDockAllowed0(int align, const Value &v) const
{
	return IsDockCont(v) ? ContCast(v)->IsDockAllowed(align, -1) : base->IsDockAllowed(align, *DockCast(v));
}

DockableCtrl * DockCont::Get0(int ix) const
{ 
	if (ix < 0 || ix > tabbar.GetCount()) return NULL;
	Value v = tabbar.Get(ix); 
	return IsDockCont(v) ? ContCast(v)->GetCurrent0() : DockCast(v); 
}

WString DockCont::GetTitle(bool force_count) const
{
	if ((IsTabbed() || force_count) && tabbar.GetCount() > 1) 
		return (WString)Format("%s (%d/%d)", GetCurrent().GetTitle(), tabbar.GetCursor()+1, tabbar.GetCount()); 
	return GetCurrent().GetTitle();	
}

DockCont::DockCont()
{
	dragging = false;
	dockstate = STATE_NONE;
	base = NULL;
	focus = false;
	waitsync = false;	
	style = NULL;
	usersize.cx = usersize.cy = Null;
	BackPaint();
#ifdef PLATFORM_WIN32
	ToolWindow();
#endif
	NoCenter().Sizeable(true).MaximizeBox(false).MinimizeBox(false);

	*this << close << autohide << windowpos;

	AddFrame(FieldFrame());	
	tabbar.AutoHideMin(1);
	tabbar.WhenCursor 		= THISBACK(TabSelected);
	tabbar.WhenDrag 		= THISBACK(TabDragged);
	tabbar.WhenContext 		= THISBACK(TabContext);
	tabbar.WhenClose 		= THISBACK(TabClosed);
	AddFrame(tabbar);
	tabbar.SetBottom();	

	close.Tip(t_("Close")) 					<<= THISBACK(CloseAll);
	autohide.Tip(t_("Auto-Hide")) 			<<= THISBACK(AutoHide);
	windowpos.Tip(t_("Window Menu")) 	<<= THISBACK(WindowMenu);		
	WhenClose 							  = THISBACK(CloseAll);
}

void DockCont::Serialize(Stream& s)
{
	int ctrl = 'D';
	int cont = 'C';
	const Vector<DockableCtrl *> &dcs = base->GetDockableCtrls();
	
	if (s.IsStoring()) {		
		if (GetCount() == 1 && IsDockCont(tabbar.Get(0)))
			return ContCast(tabbar.Get(0))->Serialize(s);

		int cnt = GetCount();
		s / cont / cnt;
		for (int i = GetCount() - 1; i >= 0; i--) {
			if (IsDockCont(tabbar.Get(i)))
				ContCast(tabbar.Get(i))->Serialize(s);
			else {
				DockableCtrl *dc = DockCast(tabbar.Get(i));
				int ix = base->FindDocker(dc);
				s / ctrl / ix;					
			}									
		}
		int cursor = tabbar.GetCursor();
		int groupix = tabbar.GetGroup();
		s / cursor / groupix;
	}
	else {
		int cnt;
		int type;
		
		s / type / cnt;
		ASSERT(type == cont);
		for (int i = 0; i < cnt; i++) {
			int64 pos = s.GetPos();
			s / type;
			if (type == cont) {
				s.Seek(pos);
				DockCont *dc = base->CreateContainer();
				dc->Serialize(s);
				base->DockContainerAsTab(*this, *dc, true);
			}
			else if (type == ctrl) {
				int ix;
				s / ix;
				if (ix >= 0 && ix <= dcs.GetCount())
					Add(*dcs[ix]);
			}
			else
				ASSERT(false);								
		}
		int cursor, groupix;
		s / cursor / groupix;
		tabbar.SetGroup(groupix);
		tabbar.SetCursor(min(tabbar.GetCount()-1, cursor));
		TabSelected();
	}	
}

void DockCont::DockContMenu::ContainerMenu(Bar &bar, DockCont *c, bool withgroups)
{
	DockableCtrl *dc = &c->GetCurrent();
	cont = c;
		
	// TODO: Need correct group filtering
	withgroups = false;
	
	// If grouping, find all groups
	DockMenu::WindowMenu(bar, dc);	
	if (withgroups && dock->IsGrouping()) {
		Vector<String> groups;
		cont->GetGroups(groups);
		if (groups.GetCount()) {
			bar.Separator();
			Sort(groups);
			for (int i = 0; i < groups.GetCount(); i++)
				bar.Add(groups[i], THISBACK1(GroupWindowsMenu, groups[i]));
			bar.Add(t_("All"), THISBACK1(GroupWindowsMenu, String(Null)));	
		}
	}	
}

void DockCont::DockContMenu::MenuDock(int align, DockableCtrl *dc)
{
	cont->Dock(align);
}

void DockCont::DockContMenu::MenuFloat(DockableCtrl *dc)
{
	cont->Float();
}

void DockCont::DockContMenu::MenuAutoHide(int align, DockableCtrl *dc)
{
	cont->AutoHideAlign(align);
}

void DockCont::DockContMenu::MenuClose(DockableCtrl *dc)
{
	cont->CloseAll();
}

