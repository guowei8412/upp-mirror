#include "CtrlLib.h"

NAMESPACE_UPP

void SplitterFrame::FrameAdd(Ctrl& parent)
{
	parent.Add(*this);
}

void SplitterFrame::FrameRemove()
{
	this->Ctrl::Remove();
}

void SplitterFrame::FrameAddSize(Size& sz)
{
	(type == LEFT || type == RIGHT ? sz.cx : sz.cy) += size;
}

int  SplitterFrame::BoundSize()
{
	int maxsize = max(0, (type == LEFT || type == RIGHT ? parentsize.cx : parentsize.cy) - sizemin);
	return max(4, minmax(size, minsize - 4, maxsize));
}

void SplitterFrame::FrameLayout(Rect& r)
{
	Rect rr = r;
	parentsize = r.GetSize();
	int sz = BoundSize();
	switch(type) {
	case LEFT:
		r.left += sz;
		rr.right = r.left;
		break;
	case RIGHT:
		r.right -= sz;
		rr.left = r.right;
		break;
	case TOP:
		r.top += sz;
		rr.bottom = r.top;
		break;
	case BOTTOM:
		r.bottom -= sz;
		rr.top = r.bottom;
		break;
	}
	SetFrameRect(rr);
}

void SplitterFrame::Paint(Draw& draw)
{
	draw.DrawRect(
		Ctrl::GetSize(),
		HasCapture() ? GUI_GlobalStyle() >= GUISTYLE_XP ? Blend(SColorHighlight, SColorFace) : SColorShadow
		             : SColorFace()
	);
}

void SplitterFrame::LeftDown(Point p, dword)
{
	SetCapture();
	Refresh();
	ref = GetMousePos();
	size0 = BoundSize();
}

void SplitterFrame::MouseMove(Point p, dword keyflags)
{
	if(!HasCapture())
		return;
	p = GetMousePos();
	switch(type) {
	case LEFT: size = size0 + p.x - ref.x; break;
	case RIGHT: size = size0 + ref.x - p.x; break;
	case TOP: size = size0 + p.y - ref.y; break;
	case BOTTOM: size = size0 + ref.y - p.y; break;
	}
	RefreshParentLayout();
}

void SplitterFrame::LeftUp(Point p, dword keyflags)
{
	Refresh();
}

Image SplitterFrame::CursorImage(Point p, dword keyflags)
{
	return type == LEFT || type == RIGHT ? Image::SizeHorz() : Image::SizeVert();
}

SplitterFrame& SplitterFrame::Set(Ctrl& c, int _size, int _type)
{
	while(GetFirstChild())
		GetFirstChild()->Remove();
	type = _type;
	size = _size;
	Add(c.SizePos());
	switch(type) {
	case LEFT: c.HSizePos(0, 4); break;
	case RIGHT: c.HSizePos(4, 0); break;
	case TOP: c.VSizePos(0, 4); break;
	case BOTTOM: c.VSizePos(4, 0); break;
	}
	RefreshParentLayout();
	return *this;
}

void SplitterFrame::Serialize(Stream& s)
{
	int version = 0;
	s / version;
	s % size;
}

SplitterFrame::SplitterFrame()
{
	type = LEFT;
	size = size0 = 4;
	minsize = 0;
	sizemin = 0;
}

END_UPP_NAMESPACE
