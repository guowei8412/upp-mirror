#include "CtrlLib.h"

NAMESPACE_UPP

int Splitter::ClientToPos(Point p) const
{
	return minmax(vert ? 10000 * p.y / GetSize().cy : 10000 * p.x / GetSize().cx, 0, 9999);
}

int Splitter::PosToClient(int pos) const
{
	return (vert ? GetSize().cy : GetSize().cx) * pos / 10000;
}

int Splitter::GetChildCount() const
{
	int count = 0;
	for(Ctrl *child = GetFirstChild(); child; child = child->GetNext())
		count++;
	return count;
}

void   Splitter::Layout() {
	Size sz = GetSize();
	int count = GetChildCount();
	if(count == 0)
		return;
	count--;
	if(pos.GetCount() < count) {
		pos.SetCount(count, 0);
		for(int i = 0; i < count; i++)
			pos[i] = (i + 1) * 10000 / (count + 1);
	}
	if(style >= 0) {
		int i = 0;
		for(Ctrl *q = GetFirstChild(); q; q = q->GetNext()) {
			if(style == i)
				q->SizePos().Show();
			else
				q->Hide();
			i++;
		}
		return;
	}
	int wd = width >> 1;
	int i = 0;
	for(Ctrl *q = GetFirstChild(); q; q = q->GetNext()) {
		int lo = i > 0 ? PosToClient(pos[i - 1]) + wd : 0;
		int hi = i < count ? PosToClient(pos[i]) - wd : vert ? sz.cy : sz.cx;
		q->Show();
		if(vert)
			q->SetRect(0, lo, sz.cx, hi - lo);
		else
			q->SetRect(lo, 0, hi - lo, sz.cy);
		i++;
	}
}

void   Splitter::Paint(Draw& w) {
	Size sz = GetSize();
	w.Begin();
	if(HasCapture() && mouseindex >= 0 && mouseindex < pos.GetCount()) {
		int p = PosToClient(pos[mouseindex]) - (width >> 1);
		Rect r = vert ? RectC(0, p, sz.cx, width) : RectC(p, 0, width, sz.cy);
		w.DrawRect(r, GUI_GlobalStyle() >= GUISTYLE_XP ? Blend(SColorHighlight, SColorFace) : SColorShadow);
		w.ExcludeClip(r);
	}
	if(!IsTransparent())
		w.DrawRect(sz, SColorFace());
	w.End();
}

void   Splitter::MouseMove(Point p, dword) {
	if(HasCapture() && mouseindex >= 0 && mouseindex < pos.GetCount()) {
		SetPos(ClientToPos(p), mouseindex);
		WhenAction();
	}
}

void   Splitter::LeftDown(Point p, dword) {
	SetCapture();
	Refresh();
	mouseindex = FindIndex(p);
}

int Splitter::FindIndex(Point client) const {
	int best = -1;
	int maxdist = width;
	for(int i = 0; i < pos.GetCount(); i++) {
		int dist = abs((vert ? client.y : client.x) - PosToClient(pos[i]));
		if(dist < maxdist) {
			best = i;
			maxdist = abs(dist);
		}
	}
	return best;
}

void   Splitter::LeftUp(Point p, dword keyflags) {
	Refresh();
}

Image Splitter::CursorImage(Point p, dword) {
	return FindIndex(p) < 0 ? Image::Arrow() : vert ? Image::SizeVert() : Image::SizeHorz();
}

int Splitter::GetMins(int i) const
{
	int min1 = (i < mins.GetCount() ? mins[i] : 0);
	int min2 = 0;
	int cx = GetSize().cx;
	if(cx)
		min2 = (i < minpx.GetCount() ? minpx[i] : 0) * 10000 / cx;
	return max(min1, min2);
}

Splitter& Splitter::SetPos(int p, int i) {
	int l = (i > 0 && i - 1 < pos.GetCount() ? pos[i - 1] : 0) + GetMins(i);
	int h = (i + 1 < pos.GetCount() ? pos[i + 1] : 10000) - GetMins(i + 1);
	pos.At(i) = minmax(p, l, h);
	Layout();
	return *this;
}

void   Splitter::Zoom(int i)
{
	style = i;
	Layout();
}

Splitter& Splitter::BarWidth(int w)
{
	width = w;
	Layout();
	return *this;
}

Splitter& Splitter::Vert(Ctrl& top, Ctrl& bottom)
{
	vert = true;
	Set(top, bottom);
	return *this;
}

Splitter& Splitter::Horz(Ctrl& left, Ctrl& right)
{
	vert = false;
	Set(left, right);
	return *this;
}

void Splitter::Set(Ctrl& l, Ctrl& r)
{
	Clear();
	*this << l << r;
	Layout();
}

void Splitter::Add(Ctrl& pane)
{
	Ctrl::Add(pane);
	Layout();
}

void Splitter::Serialize(Stream& s) {
	int version = 0x02;
	s / version;
	if(version <= 1) {
		int p = GetPos(0);
		s % p;
		SetPos(p, 0);
	}
	else
		s % pos;
	if(version >= 0x01) {
		s / style;
		if(version == 0x01 && style == 3)
			style = -1;
	}
	if(s.IsLoading()) {
		for(int i = 0; i < pos.GetCount(); i++)
			if(pos[i] < 0 || pos[i] >= 10000) {
				pos.Clear();
				s.LoadError();
			}
		if(style >= GetChildCount()) {
			style = -1;
			s.LoadError();
		}
		Layout();
	}
}

void Splitter::Clear() {
	while(GetFirstChild())
		RemoveChild(GetFirstChild());
}

void Splitter::Reset() {
	Clear();
	pos.Clear();
	mins.Clear();
	style = -1;
	width = 4;
	vert = false;
}

Splitter::Splitter() {
	style = -1;
	pos.Add(5000);
	width = 4;
	SetFrame(NullFrame());
	vert = false;
	NoWantFocus();
	VSizePos(0, 0).HSizePos(0, 0);
	mouseindex = -1;
}

Splitter::~Splitter() {}

END_UPP_NAMESPACE
