#include "CtrlPropCommon.h"
#include <CtrlLib/CtrlLib.h>

bool PropSetBackground(const Value& v, StaticRect& o) { if(!v.Is<Color>()) return false; o.Background(v); return true; }
bool PropGetBackground(Value& v, const StaticRect& o) { v = o.GetBackground(); return true; }

PROPERTIES(StaticRect, Ctrl)
PROPERTY("background", PropSetBackground, PropGetBackground)
END_PROPERTIES

PROPS(Ctrl, StaticRect, Ctrl)
