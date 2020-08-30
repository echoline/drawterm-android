#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"

#include <draw.h>
#include <memdraw.h>
#include <keyboard.h>
#include <cursor.h>
#include "screen.h"

Memimage *gscreen;
extern int screenWidth;
extern int screenHeight;
char *snarfbuf = nil;

unsigned char*
screenData()
{
	int x, y, o;
	static unsigned char *ret = NULL;
	if (ret == NULL)
		ret = malloc(sizeof(unsigned char) * screenWidth * screenHeight * 4);
	if (gscreen != NULL && gscreen->data != NULL && gscreen->data->bdata != NULL)
		for (y = 0; y < screenHeight; y++)
			for (x = 0; x < screenWidth; x++) {
				o = (y * screenWidth + x) * 4;
				ret[o+0] = 0xFF;
				ret[o+1] = gscreen->data->bdata[o+2];
				ret[o+2] = gscreen->data->bdata[o+1];
				ret[o+3] = gscreen->data->bdata[o+0];
			}
	return ret;
}

/**
 * TODO: android clipboard
 */
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
	return;
}

void
screeninit(void)
{
	memimageinit();
	gscreen = allocmemimage(Rect(0,0,screenWidth,screenHeight), XRGB32);
	terminit();
	return;
}

void
screensize(Rectangle r, ulong chan)
{
}

Memdata*
attachscreen(Rectangle *r, ulong *chan, int *depth, int *width, int *softscreen)
{
	*r = gscreen->r;
	*depth = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 0;
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

