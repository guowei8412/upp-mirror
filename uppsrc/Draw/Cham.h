enum LookOp {
	LOOK_PAINT,
	LOOK_PAINTEDGE,
	LOOK_MARGINS,
	LOOK_ISOPAQUE,
};

void  ChLookFn(Value (*fn)(Draw& w, const Rect& r, const Value& look, int lookop));

Image AdjustColors(const Image& img);

void   Override(Iml& target, Iml& source, bool colored = false);
void   ColoredOverride(Iml& target, Iml& source);

void   ChReset();
void   ChFinish();

void   ChPaint(Draw& w, const Rect& r, const Value& look);
void   ChPaint(Draw& w, int x, int y, int cx, int cy, const Value& look);
void   ChPaintEdge(Draw& w, const Rect& r, const Value& look);
void   ChPaintEdge(Draw& w, int x, int y, int cx, int cy, const Value& look);
Rect   ChMargins(const Value& look);
bool   ChIsOpaque(const Value& look);

template <class T>
struct ChStyle {
	byte status;
	byte registered;

	T&   Write() const         { T& x = *(T *)this; x.status = 2; return x; }
	void Assign(const T& src)  { *(T *)this = src; }

	ChStyle()                  { status = 0; registered = 0; }
};

#define CH_STYLE(klass, type, style) \
struct COMBINE5(klass, __, type, __, style) : klass::type { \
	void Init(); \
	static void InitIt(); \
}; \
\
void COMBINE5(klass, __, type, __, style)::InitIt() { \
	klass::style(); \
} \
\
const klass::Style& klass::style() \
{ \
	static COMBINE5(klass, __, type, __, style) b; \
	if(b.status == 0) { \
		ChRegisterStyle__(b.status, b.registered, COMBINE5(klass, __, type, __, style)::InitIt); \
		b.Init(); \
		b.status = 1; \
	} \
	return b; \
} \
\
void COMBINE5(klass, __, type, __, style)::Init()

#define CH_VAR(chtype, type, name, init) \
chtype& COMBINE(ch_var__, name)(); \
void COMBINE(ch_init__, name)() { \
	COMBINE(ch_var__, name)(); \
} \
\
chtype& COMBINE(ch_var__, name)() { \
	static chtype b; \
	if(b.status == 0) { \
		ChRegisterStyle__(b.status, b.registered, COMBINE(ch_init__, name)); \
		b.value = init; \
		b.status = 1; \
	} \
	return b; \
} \
\
type name() { return COMBINE(ch_var__, name)().value; } \
void COMBINE(name, _Write)(type v) { COMBINE(ch_var__, name)().Write().value = v; }

struct ChColor : ChStyle<ChColor> { Color value; };
#define CH_COLOR(name, init) CH_VAR(ChColor, Color, name, init)

struct ChInt : ChStyle<ChInt> { int value; };
#define CH_INT(name, init) CH_VAR(ChInt, int, name, init)

struct ChValue : ChStyle<ChValue> { Value value; };
#define CH_VALUE(name, init) CH_VAR(ChValue, Value, name, init)

struct ChImage : ChStyle<ChImage> { Image value; };
#define CH_IMAGE(name, init) CH_VAR(ChImage, Image, name, init)

void ChPaint(Draw& w, const Rect& r, const Value& element);
void ChPaint(Draw& w, int x, int y, int cx, int cy, const Value& element);

Value ChLookWith(const Value& look, const Image& img);
Value ChLookWith(const Value& look, const Image& img, Color color);
Value ChLookWith(const Value& look, const Image& img, Color (*color)(int i), int i);

//private:
void ChRegisterStyle__(byte& state, byte& registered, void (*init)());
