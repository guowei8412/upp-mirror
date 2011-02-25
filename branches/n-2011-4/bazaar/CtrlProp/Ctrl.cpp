#include "CtrlPropCommon.h"
#include <CtrlLib/CtrlLib.h>

bool PropSetData(const Value& v, Ctrl& o) { o.SetData(v); return true; }
bool PropGetData(Value& v, const Ctrl& o) { v = o.GetData(); return true; }
bool PropEnable(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.Enable(v); return true; }
bool PropIsEnabled(Value& v, const Ctrl& o) { v = o.IsEnabled(); return true; }
bool PropShow(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.Show(v); return true; }
bool PropIsVisible(Value& v, const Ctrl& o) { v = o.IsVisible(); return true; }
bool PropEditable(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.SetEditable(v); return true; }
bool PropIsEditable(Value& v, const Ctrl& o) { v = o.IsEditable(); return true; }
bool PropFocus(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; bool b = v; if(b) o.SetFocus(); else if(o.GetParent()) o.GetParent()->SetFocus(); return true; }
bool PropHasFocus(Value& v, const Ctrl& o) { v = o.HasFocus(); return true; }
bool PropModify(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; bool b = v; if(b) o.SetModify(); else o.ResetModify(); return true; }
bool PropIsModified(Value& v, const Ctrl& o) { v = o.IsModified(); return true; }
bool PropSetTip(const Value& v, Ctrl& o) { o.Tip(AsString(v)); return true; }
bool PropGetTip(Value& v, const Ctrl& o) { v = o.GetTip(); return true; }
bool PropWantFocus(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.WantFocus(v); return true; }
bool PropIsWantFocus(Value& v, const Ctrl& o) { v = o.IsWantFocus(); return true; }
bool PropInitFocus(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.InitFocus(v); return true; }
bool PropIsInitFocus(Value& v, const Ctrl& o) { v = o.IsInitFocus(); return true; }
bool PropBackPaint(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.BackPaint(((bool(v))?(Ctrl::FULLBACKPAINT):(Ctrl::NOBACKPAINT))); return true; }
bool PropIsBackPaint(Value& v, const Ctrl& o) { v = bool(o.GetBackPaint()!=Ctrl::NOBACKPAINT); return true; }
bool PropTransparent(const Value& v, Ctrl& o) { if(!IsNumber(v)) return false; o.Transparent(v); return true; }
bool PropIsTransparent(Value& v, const Ctrl& o) { v = o.IsTransparent(); return true; }

bool PropRefresh(const Value& v, Ctrl& o) { o.Refresh(); return true; }

bool PropSetLogPos(const Value& v, Ctrl& o) { if(!v.Is<Ctrl::LogPos>()) return false; o.SetPos(RawValue<Ctrl::LogPos>::Extract(v)); return true; }
bool PropGetLogPos(Value& v, const Ctrl& o) { v = RawToValue(o.GetPos()); return true; }

bool PropGetTypeInfo(Value& v, const Ctrl& o) { v = String(typeid(o).name()); return true; }

PROPERTIES(Ctrl, EmptyClass)
PROPERTY("data", PropSetData, PropGetData)
PROPERTY("enable", PropEnable, PropIsEnabled)
PROPERTY("show", PropShow, PropIsVisible)
PROPERTY("editable", PropEditable, PropIsEditable)
PROPERTY("logpos", PropSetLogPos, PropGetLogPos)
PROPERTY("focus", PropFocus, PropHasFocus)
PROPERTY("modify", PropModify, PropIsModified)
PROPERTY("tip", PropSetTip, PropGetTip)
PROPERTY("wantfocus", PropWantFocus, PropIsWantFocus)
PROPERTY("initFocus", PropInitFocus, PropIsInitFocus)
PROPERTY("backpaint", PropBackPaint, PropIsBackPaint)
PROPERTY("transparent", PropTransparent, PropIsTransparent)
//PROPERTY_SET("refresh", PropRefresh)
PROPERTY_GET("type", PropGetTypeInfo)
END_PROPERTIES

GETSETPROP_DUMMY(Ctrl, EmptyClass)
PROPS(Ctrl, Ctrl, EmptyClass)
