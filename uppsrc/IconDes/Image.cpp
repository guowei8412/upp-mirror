#include "IconDes.h"

void IconDes::Interpolate()
{
	if(!IsCurrent())
		return;
	FinishPaste();
	SaveUndo();
	Slot& c = Current();
	c.base_image = c.image;
	::InterpolateImage(c.image, c.image.GetSize());
	MaskSelection();
}

bool IconDes::BeginTransform()
{
	SaveUndo();
	Refresh();
	SyncShow();
	if(!IsPasting()) {
		if(SelectionRect() == GetImageSize())
			return false;
		Move();
	}
	return true;
}

void IconDes::KeyMove(int dx, int dy)
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		c.pastepos.x += dx;
		c.pastepos.y += dy;
		MakePaste();
	}
	else {
		Image h = c.image;
		c.image = CreateImage(h.GetSize(), Null);
		::Copy(c.image, Point(dx, dy), h, h.GetSize());
	}
	Sync();
}

void IconDes::MirrorX()
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		MirrorHorz(c.paste_image, c.paste_image.GetSize());
		MakePaste();
	}
	else
		MirrorHorz(c.image, c.image.GetSize());
	SyncShow();
}

void IconDes::SymmX()
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		if(c.paste_image.GetLength() > 1024 * 1024)
			return;
		Image m = c.paste_image;
		Size sz = m.GetSize();
		MirrorHorz(m, m.GetSize());
		Image h = CreateImage(Size(2 * sz.cx, sz.cy), Null);
		::Copy(h, Point(0, 0), c.paste_image, sz);
		::Copy(h, Point(sz.cx, 0), m, sz);
		c.paste_image = h;
		MakePaste();
	}
	else {
		Size sz = c.image.GetSize();
		if(sz.cx < 2)
			return;
		::Copy(c.image, Point(sz.cx - sz.cx / 2, 0), c.image, Size(sz.cx / 2, sz.cy));
		MirrorHorz(c.image, RectC(sz.cx - sz.cx / 2, 0, sz.cx / 2, sz.cy));
	}
	SyncShow();
}

void IconDes::MirrorY()
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		MirrorVert(c.paste_image, c.paste_image.GetSize());
		MakePaste();
	}
	else
		MirrorVert(c.image, c.image.GetSize());
	SyncShow();
}

void IconDes::SymmY()
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		if(c.paste_image.GetLength() > 1024 * 1024)
			return;
		Image m = c.paste_image;
		Size sz = m.GetSize();
		MirrorVert(m, m.GetSize());
		Image h = CreateImage(Size(sz.cx, 2 * sz.cy), Null);
		::Copy(h, Point(0, 0), c.paste_image, sz);
		::Copy(h, Point(0, sz.cy), m, sz);
		c.paste_image = h;
		MakePaste();
	}
	else {
		Size sz = c.image.GetSize();
		if(sz.cy < 2)
			return;
		::Copy(c.image, Point(0, sz.cy - sz.cy / 2), c.image, Size(sz.cx, sz.cy / 2));
		MirrorVert(c.image, RectC(0, sz.cy - sz.cy / 2, sz.cx, sz.cy / 2));
	}
	SyncShow();
}

void IconDes::Rotate()
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(BeginTransform()) {
		c.paste_image = RotateClockwise(c.paste_image);
		MakePaste();
	}
	else
		c.image = RotateClockwise(c.image);
	SyncShow();
}

void IconDes::SmoothRescale()
{
	if(!IsCurrent())
		return;
	WithRescaleLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Rescale");
	dlg.cx <<= dlg.cy <<= dlg.Breaker();
	Slot& c = Current();
	BeginTransform();
	Image bk = IsPasting() ? c.paste_image : c.image;
	dlg.cx <<= bk.GetWidth();
	dlg.cy <<= bk.GetHeight();
	dlg.keep <<= true;
	for(;;) {
		Size sz(minmax((int)~dlg.cx, 1, 9999), minmax((int)~dlg.cy, 1, 9999));
		if(IsPasting()) {
			c.paste_image = Rescale(bk, sz);
			MakePaste();
			SyncImage();
		}
		else {
			c.image = Rescale(bk, sz);
			Refresh();
			SyncShow();
		}
		SyncImage();
		SyncShow();
		switch(dlg.Run()) {
		case IDCANCEL:
			if(IsPasting()) {
				c.paste_image = bk;
				MakePaste();
			}
			else {
				c.image = bk;
				Refresh();
				SyncShow();
			}
			return;
		case IDOK:
			return;
		}
		if(dlg.keep) {
			if(dlg.cx.HasFocus() && bk.GetWidth() > 0)
				dlg.cy <<= (int)~dlg.cx * bk.GetHeight() / bk.GetWidth();
			if(dlg.cy.HasFocus() && bk.GetHeight() > 0)
				dlg.cx <<= (int)~dlg.cy * bk.GetWidth() / bk.GetHeight();
		}
	}
}

Image IconDes::ImageStart()
{
	if(!IsCurrent())
		return CreateImage(Size(1, 1), Black);
	SaveUndo();
	Refresh();
	SyncShow();
	Slot& c = Current();
	if(!IsPasting())
		c.base_image = c.image;
	return IsPasting() ? c.paste_image : c.image;
}

void IconDes::ImageSet(const Image& m)
{
	if(!IsCurrent())
		return;
	Slot& c = Current();
	if(IsPasting()) {
		c.paste_image = m;
		MakePaste();
	}
	else {
		c.image = m;
		MaskSelection();
	}
	Refresh();
	SyncShow();
}

void IconDes::BlurSharpen()
{
	WithSharpenLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Blur/Sharpen");
	PlaceDlg(dlg);
	dlg.level <<= 0;
	dlg.level <<= dlg.Breaker();
	dlg.passes <<= 1;
	dlg.passes <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		Image m = bk;
		for(int q = 0; q < (int)~dlg.passes; q++)
			m = Sharpen(m, -int(256 * (double)~dlg.level));
		ImageSet(m);
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}

void IconDes::Colorize()
{
	WithColorizeLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Colorize");
	PlaceDlg(dlg);
	dlg.level.Max(10);
	dlg.level <<= 1;
	dlg.level <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		ImageSet(::Colorize(bk, CurrentColor(), (int)(minmax((double)~dlg.level, 0.0, 1.0) * 255)));
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}

void IconDes::Chroma()
{
	WithColorizeLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Chroma");
	PlaceDlg(dlg);
	dlg.level.Max(10);
	dlg.level <<= 1;
	dlg.level <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		ImageSet(::Grayscale(bk, 256 - (int)(minmax((double)~dlg.level, 0.0, 4.0) * 255)));
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}

void IconDes::Contrast()
{
	WithColorizeLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Contrast");
	PlaceDlg(dlg);
	dlg.level.Max(10);
	dlg.level <<= 1;
	dlg.level <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		ImageSet(::Contrast(bk, (int)(minmax((double)~dlg.level, 0.0, 4.0) * 255)));
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}

void IconDes::Alpha()
{
	WithColorizeLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Alpha");
	PlaceDlg(dlg);
	dlg.level.Max(4);
	dlg.level <<= 1;
	dlg.level <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		int a = (int)(minmax((double)~dlg.level, 0.0, 4.0) * 255);
		ImageBuffer ib(bk.GetSize());
		RGBA *t = ib;
		const RGBA *s = bk;
		const RGBA *e = bk + bk.GetLength();
		while(s < e) {
			*t = *s;
			t->a = Saturate255((s->a * a) >> 8);
			s++;
			t++;
		}
		ImageSet(ib);
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}

void IconDes::Colors()
{
	WithImgColorLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "Alpha");
	PlaceDlg(dlg);
	dlg.r_mul <<= dlg.g_mul <<= dlg.b_mul <<= dlg.a_mul <<= 1;
	dlg.r_add <<= dlg.g_add <<= dlg.b_add <<= dlg.a_add <<= 0;
	dlg.all = true;
	dlg.r_mul <<= dlg.g_mul <<= dlg.b_mul <<= dlg.a_mul <<=
	dlg.r_add <<= dlg.g_add <<= dlg.b_add <<= dlg.a_add <<=
	dlg.all <<= dlg.Breaker();
	Image bk = ImageStart();
	for(;;) {
		bool all = dlg.all;
		dlg.g_mul.Enable(!all);
		dlg.g_add.Enable(!all);
		dlg.b_mul.Enable(!all);
		dlg.b_add.Enable(!all);
		if(all) {
			dlg.g_mul <<= dlg.b_mul <<= ~dlg.r_mul;
			dlg.g_add <<= dlg.b_add <<= ~dlg.r_add;
		}
		ImageBuffer ib(bk.GetSize());
		RGBA *t = ib;
		const RGBA *s = bk;
		const RGBA *e = bk + bk.GetLength();
		int r_mul = int(256 * (double)~dlg.r_mul);
		int r_add = int(256 * (double)~dlg.r_add);
		int g_mul = int(256 * (double)~dlg.g_mul);
		int g_add = int(256 * (double)~dlg.g_add);
		int b_mul = int(256 * (double)~dlg.b_mul);
		int b_add = int(256 * (double)~dlg.b_add);
		int a_mul = int(256 * (double)~dlg.a_mul);
		int a_add = int(256 * (double)~dlg.a_add);
		while(s < e) {
			t->r = Saturate255(((r_mul * s->r) >> 8) + r_add);
			t->g = Saturate255(((g_mul * s->g) >> 8) + g_add);
			t->b = Saturate255(((b_mul * s->b) >> 8) + b_add);
			t->a = Saturate255(((a_mul * s->a) >> 8) + a_add);
			s++;
			t++;
		}
		ImageSet(ib);
		switch(dlg.Run()) {
		case IDCANCEL:
			ImageSet(bk);
			return;
		case IDOK:
			return;
		}
	}
}
