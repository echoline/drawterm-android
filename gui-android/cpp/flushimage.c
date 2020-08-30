#include <u.h>
#include <libc.h>
#include <draw.h>

static
int
doflush(Display *d)
{
	int n;

	n = d->bufp-d->buf;
	if(n <= 0)
		return 1;

	if(write(d->fd, d->buf, n) != n){
		if(0)
			fprint(2, "flushimage fail: d=%p: %r\n", d); /**/
		d->bufp = d->buf;	/* might as well; chance of continuing */
		return -1;
	}
	d->bufp = d->buf;
	return 1;
}

int
flushimage(Display *d, int visible)
{
/*	if(visible == 1 && visibleclicks && mousebuttons && _drawmouse.buttons) {
		Rectangle r, r1;
		int ret;

		r = mousebuttons->r;
		r = rectaddpt(r, _drawmouse.xy);
		r = rectaddpt(r, Pt(-Dx(mousebuttons->r)/2, -Dy(mousebuttons->r)-3));
		drawop(mousesave, mousesave->r, screen, nil, r.min, S);

		r1 = rectaddpt(Rect(0, 0, 22, 22), r.min);
		if(_drawmouse.buttons & 1)
			drawop(screen, r1, mousebuttons, nil, ZP, S);
		r1 = rectaddpt(r1, Pt(21, 0));
		if(_drawmouse.buttons & 2)
			drawop(screen, r1, mousebuttons, nil, Pt(21, 0), S);
		r1 = rectaddpt(r1, Pt(21, 0));
		if(_drawmouse.buttons & 4)
			drawop(screen, r1, mousebuttons, nil, Pt(42, 0), S);
		ret = flushimage(d, 2);
		drawop(screen, r, mousesave, nil, ZP, S);
		return ret;
	}*/

	if(visible){
		*d->bufp++ = 'v';	/* five bytes always reserved for this */
		if(d->_isnewdisplay){
			BPLONG(d->bufp, d->screenimage->id);
			d->bufp += 4;
		}
	}
	return doflush(d);
}

uchar*
bufimage(Display *d, int n)
{
	uchar *p;

	if(n<0 || d == nil || n>d->bufsize){
		werrstr("bad count in bufimage");
		return nil;
	}
	if(d->bufp+n > d->buf+d->bufsize)
		if(doflush(d) < 0)
			return 0;
	p = d->bufp;
	d->bufp += n;
	return p;
}

