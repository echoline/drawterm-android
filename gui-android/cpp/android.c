#include <android/native_window.h>
#include <android/log.h>

#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"

#include <draw.h>
#include <memdraw.h>
#include <keyboard.h>
#include <cursor.h>
#include "screen.h"

Memimage *gscreen = nil;
extern int screenWidth;
extern int screenHeight;
char *snarfbuf = nil;
extern ANativeWindow *window;

char*
clipread(void)
{
	return strdup(snarfbuf == nil? "": snarfbuf);
}

int
clipwrite(char *buf)
{
	if (snarfbuf != nil)
		free(snarfbuf);
	snarfbuf = strdup(buf);
	return 0;
}

void
setcolor(ulong i, ulong r, ulong g, ulong b)
{
	return;
}

void
getcolor(ulong v, ulong *r, ulong *g, ulong *b)
{
	*r = (v>>16)&0xFF;
	*g = (v>>8)&0xFF;
	*b = v&0xFF;
}

void
flushmemscreen(Rectangle r)
{
	ANativeWindow_Buffer buffer;
	uint8_t *pixels;
	int x, y, o, b;
	ARect bounds;

	if (window == NULL)
		return;

	memset(&buffer, 0, sizeof(buffer));

	bounds.left = r.min.x;
	bounds.top = r.min.y;
	bounds.right = r.max.x;
	bounds.bottom = r.max.y;

	if (ANativeWindow_lock(window, &buffer, &bounds) != 0) {
		__android_log_print(ANDROID_LOG_WARN, "drawterm", "Unable to lock window buffer");
		return;
	}

	r.min.x = bounds.left;
	r.min.y = bounds.top;
	r.max.x = bounds.right;
	r.max.y = bounds.bottom;

	pixels = (uint8_t*)buffer.bits;
	for (y = r.min.y; y < r.max.y; y++)
		for (x = r.min.x; x < r.max.x; x++) {
			o = (y * screenWidth + x) * 4;
			b = (y * buffer.stride + x) * 4;
			pixels[b+3] = 0xFF;
			pixels[b+2] = gscreen->data->bdata[o+0];
			pixels[b+1] = gscreen->data->bdata[o+1];
			pixels[b+0] = gscreen->data->bdata[o+2];
		}

	if (ANativeWindow_unlockAndPost(window) != 0) {
		__android_log_print(ANDROID_LOG_WARN, "drawterm", "Unable to unlock and post window buffer");
	}
	return;
}

void
screeninit(void)
{
	Rectangle r = Rect(0,0,screenWidth,screenHeight);
	memimageinit();
	screensize(r, XRGB32);
	if (gscreen == nil)
		panic("screensize failed");
	gscreen->clipr = r;
	terminit();
	qlock(&drawlock);
	flushmemscreen(r);
	qunlock(&drawlock);
	return;
}

void
screensize(Rectangle r, ulong chan)
{
	Memimage *mi;

	mi = allocmemimage(r, chan);
	if (mi == nil)
		return;

	if (gscreen != nil)
		freememimage(gscreen);

	gscreen = mi;
	gscreen->clipr = ZR;
}

Memdata*
attachscreen(Rectangle *r, ulong *chan, int *depth, int *width, int *softscreen)
{
	*r = gscreen->clipr;
	*depth = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 1;

	gscreen->data->ref++;
	return gscreen->data;
}

void
setcursor(void)
{
	return;
}

void
mouseset(Point xy)
{
	return;
}

void
guimain(void)
{
	cpubody();
}

