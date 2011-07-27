/*
 *    Written using CGI 10 Jun 1987 by Ron Record
 *    Rewritten using X11 07 Apr 1993 by Ron Record (rr@ronrecord.com)
 */
/*************************************************************************
 *                                                                       *
 * Copyright (c) 1987-1993 Ronald Joe Record                            *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/


#include "spore.h"

#define BMAX 1024

typedef struct {
    int initial_x,initial_y;
    int now_x, now_y;
    int min_x, min_y;
    int max_x, max_y;
    int dx, dy;
} BALL;

BALL balls[BMAX];
int col_duration=350, nballs=4, usedefault=0;
int  num, move;
int stop=0;
int spore_hue;

int width, height;
int nummaps=1, delay=0, eternal=0;
int demo=0, useroot=0, full=0, oflag=0, spin=0, center=0;
int maxcolor, next;
int **xy;
char *outname;

/* routines declared in this file */
void event_loop(), usage(), init_contexts();
void Clear(), freemem(), setupmem(), init_pts(), print_help();
void redisplay(), resize(), save(), aggregate(), parseargs(), Getkey(); 
void Redraw();
int checkit(), walk();
/* external routines used in this file */
extern long lrand48();
extern void FlushBuffer(), BufferPoint(), InitBuffer();

void
usage()
{
    printf("Usage: spore [-o file][-dsuFRTV]");
    printf("[-m #][-w width][-h height]\n");
    printf("\t-d indicates demo mode\n");
    printf("\t-s indicates spin color wheel when done computing\n");
    printf("\t-R indicates use the root window\n");
    printf("\t-m # indicates a minimum color index of # (0-255)\n");
    printf("\t-o file will save the output as 'file' in PPM format\n");
    printf("\t-w # indicates a window width of #\n");
    printf("\t-h # indicates a window height of #\n");
    printf("\t-u produces this message\n");
    printf("During display :\n");
    printf("\t'f' or 'F' will save the picture as a PPM file\n");
    printf("\t'+' will increment and '-' decrement the minimum color index\n");
    printf("\t'r' or 's' will spin the color wheel forwards or backwards\n");
    printf("\t'W' will increment and 'w' decrement the color map selection\n");
    printf("\t'?' or 'h' will display the usage message\n");
    printf("\t'q' or 'Q' will quit\n");
}

void
init_contexts()
{
    static int i;

    /*
     * create default, writable, graphics contexts for the canvas.
     */
    Data_GC[0] = XCreateGC(dpy, DefaultRootWindow(dpy),
        (unsigned long) NULL, (XGCValues *) NULL);
    /* set the background to black */
    XSetBackground(dpy,Data_GC[0],BlackPixel(dpy, screen));
    /* set the foreground of the 0th context to black */
    XSetForeground(dpy, Data_GC[0], BlackPixel(dpy, screen));
    Data_GC[1] = XCreateGC(dpy, DefaultRootWindow(dpy),
        (unsigned long) NULL, (XGCValues *) NULL);
    /* set the background to black */
    XSetBackground(dpy,Data_GC[1],BlackPixel(dpy, screen));
    /* set the foreground of the 1st context to white */
    XSetForeground(dpy, Data_GC[1], WhitePixel(dpy,  screen));
    for (i=2; i<maxcolor; i++) {
        Data_GC[i] = XCreateGC(dpy, DefaultRootWindow(dpy),
            (unsigned long) NULL, (XGCValues *) NULL);
        /* set the background to black */
        XSetBackground(dpy,Data_GC[i],BlackPixel(dpy, screen));
        /* set the foreground of the ith context to i */
        XSetForeground(dpy, Data_GC[i], i);
    }
}

void
Clear()
{
    XFillRectangle(dpy, pixmap, Data_GC[0], 0, 0, width, height);
    XCopyArea(dpy, pixmap, canvas, Data_GC[0], 0, 0, width, height, 0, 0);
}

void
freemem()
{
    static int i;

    for (i=0;i<=width;i++)
        free(xy[i]);
    free(xy);
}

void
setupmem()
{
    static int i;

    if ((xy=(int **)malloc((width+1)*sizeof(int *))) == (int **)NULL) {
        printf("Error malloc'ing xy.\n");
        exit (-1);
    }
    for (i=0;i<width+1;i++) {
        if ((xy[i]=(int *)malloc((height+1)*sizeof(int)))==(int *)NULL){
            printf("Error malloc'ing xy[%d].\n", i);
            exit (-1);
        }
    }
}

void 
init_ball(k)
int k;
{
	static int i, j, outside;

	if (center) {
		i = ((k/2) % 2) ? 1 : -1;
		j = (k % 2) ? 1 : -1;
		balls[k].initial_x=(width/2) + (j * (k*width)/(4*nballs));
		balls[k].now_x=balls[k].initial_x;
		balls[k].initial_y=(height/2) + (i * (k*height)/(4*nballs));
		balls[k].now_y = balls[k].initial_y;
		balls[k].min_x = balls[k].now_x - 2; 
		balls[k].min_y = balls[k].now_y - 2;
		balls[k].max_x = balls[k].now_x + 2; 
		balls[k].max_y = balls[k].now_y + 2;
		balls[k].dx = 4; balls[k].dy = 4;
	}
	else {
		for(i=0;i<MAX_ATTEMPTS;i++) {
			balls[k].initial_x = (lrand48()%(width/2))+(width/4);
			balls[k].initial_y = (lrand48() % (height/2)) + (height/4);
			outside = 1;
			for (j=0; j<nballs; j++) {
			  if (!(stop || (k == j)))
				if ((balls[k].initial_x > balls[j].min_x) ||
					(balls[k].initial_x < balls[j].max_x) ||
					(balls[k].initial_y < balls[j].max_y) ||
					(balls[k].initial_y > balls[j].min_y))
					outside = 0;
			}
			if (outside)
				break;
		}
		balls[k].now_x = balls[k].initial_x;
		balls[k].now_y = balls[k].initial_y;
		balls[k].min_x = balls[k].now_x - 2; 
		balls[k].min_y = balls[k].now_y - 2;
		balls[k].max_x = balls[k].now_x + 2; 
		balls[k].max_y = balls[k].now_y + 2;
		balls[k].dx = 4; balls[k].dy = 4;
	}
	xy[balls[k].now_x][balls[k].now_y] = spore_hue;
	BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, spore_hue, 
					balls[k].now_x, height - balls[k].now_y - 1);
}

void
init_pts()
{
	static int i, j;

	for (i=0; i<width+1; i++)
		for (j=0; j<height+1; j++)
			xy[i][j] = 0;
	for (i=0;i<nballs;i++)
		init_ball(i);
}

initialize()
{
    InitBuffer(&Points, maxcolor);
    Clear();
    init_pts();
	num = 1;
}

#define x_str 10

void
print_help() 
{
    static char str[80];
    static int y_str, spacing;
    static int ascent, descent, dir;
    static XCharStruct overall;
    static GC gc;

    gc = Data_GC[1];
    XClearWindow(dpy, help);
    y_str = 60;
    sprintf(str,"During run-time, interactive control can be exerted via : ");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    XQueryTextExtents(dpy,(XID)XGContextFromGC(gc),"Hey!",
			4,&dir,&ascent,&descent,&overall);
    spacing = ascent + descent + 5;
    y_str += 2 * spacing;
    sprintf(str,"        - lowers the value of mincolindex, + raises it");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        c deletes the last spore unless there's only one");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        C creates a new spore");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        d decreases the delay, D increases it");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        e toggles continuous creation of new spores");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        f or F saves spore to a PPM file");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        h or H or ? displays this message");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        n goes on to the next spore");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        N goes on to the next spore without increment");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        r or s spins the colorwheel, R or S stops the spin");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        w decrements, W increments the color wheel index");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        <ctrl>-W reads the color palette in $HOME/.sporemap");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        q or Q exits");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += 2*spacing;
    sprintf(str,"Press 'h', 'H', or '?' to unmap the help window");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
}

void
redisplay (event)
XExposeEvent    *event;
{
    if ((event->window == help) && (!useroot))
        print_help();
    else {
        /*
        * Extract the exposed area from the event and copy
        * from the saved pixmap to the window.
        */
        XCopyArea(dpy, pixmap, canvas, Data_GC[0], event->x, event->y, 
            event->width, event->height, event->x, event->y);
    }
}

void
resize()
{
    Window r;
    int j; 
    int x, y;
    unsigned int bw, d, new_w, new_h;

    freemem();
    XGetGeometry(dpy,canvas,&r,&x,&y,&new_w,&new_h,&bw,&d);
    if (((int)new_w == width) && ((int)new_h == height)) {
        setupmem();
        return;
    }
    width = (int)new_w; height = (int)new_h;
    if (pixmap)
        XFreePixmap(dpy, pixmap);
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), 
            width, height, DefaultDepth(dpy, screen));
    setupmem();
	next = 1;
	stop = 0;
	nummaps++;
	initialize();
}

void
Cleanup() {
	freemem();
	XCloseDisplay(dpy);
}

/* Store spore growth in PPM format */
void
save()
{
    FILE *outfile;
    unsigned char c;
    XImage *ximage;
    static int i,j;
    struct Colormap {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    };
    struct Colormap *colormap=NULL;

    if ((colormap=
        (struct Colormap *)malloc(sizeof(struct Colormap)*maxcolor))
            == NULL) {
        fprintf(stderr,"Error malloc'ing colormap array\n");
		Cleanup();
        exit(-1);
    }
    outfile = fopen(outname,"w");
    if(!outfile) {
        perror(outname);
		Cleanup();
        exit(-1);
    }

    ximage=XGetImage(dpy, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);

    for (i=0;i<maxcolor;i++) {
        colormap[i].red=(unsigned char)(Colors[i].red >> 8);
        colormap[i].green=(unsigned char)(Colors[i].green >> 8);
        colormap[i].blue =(unsigned char)(Colors[i].blue >> 8);
    }
    fprintf(outfile,"P%d %d %d\n",6,width,height);
    fprintf(outfile,"%d\n",maxcolor-1);

    for (j=0;j<height;j++)
        for (i=0;i<width;i++) {
            c = (unsigned char)XGetPixel(ximage,i,j);
            fwrite((char *)&colormap[c],sizeof colormap[0],1,outfile);
        }
    fclose(outfile);
    free(colormap);
}

void
Redraw() {
    static int i, j;

    FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
	Clear();
    for (i=0; i < width; i++)
    	for (j=0; j < height; j++)
			if (xy[i][j])
				BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, xy[i][j], 
							i, height - j - 1);
    FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
}

void
Getkey(event)
XKeyEvent *event;
{
    char key;
	static int spinning=0, spindir=0;
    static XWindowAttributes attr;
	extern void write_cmap(), init_color();

    if (XLookupString(event, (char *)&key, sizeof(key), (KeySym *)0,
       (XComposeStatus *) 0) > 0)
            switch (key) {
            	case '\015': /*write out current colormap to $HOME/.<prog>map*/
        			write_cmap(dpy,cmap,Colors,maxcolor,"spore","Spore");
					break;
                case '+': mincolindex += INDEXINC;
                    if (mincolindex > maxcolor)
                        mincolindex = 1;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
							mincolindex,maxcolor,numwheels,"spore","Spore",0);
                    break;
                case '-': mincolindex -= INDEXINC;
                    if (mincolindex < 1)
                        mincolindex = maxcolor - 1;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
							mincolindex,maxcolor,numwheels,"spore","Spore",0);
                    break;
				case 'c':	/* delete last spore unless there's only one */
						if (nballs > 1)
							nballs--;
						break;
				case 'C':	/* create a new spore, leaving the old */
						center = 0;
						if (stop) { /* finished with current aggregate */
							next = 1;
							nummaps++;
							init_ball(lrand48()%nballs); /* re-do one of them */
						}
						else {					/* still computing current */
							nballs++;			/* increment number of spores */
							init_ball(nballs-1);/* initialize new spore */
						}
						break;
                case 'd': delay -= 25; if (delay < 0) delay = 0; break;
                case 'D': delay += 25; break;
                case 'e': eternal = (!eternal);
			  			if (eternal) {
							center = 0;
							next = 1;
							nummaps++;
							init_ball(lrand48()%nballs); /* re-do one of them */
			  			}
						break;
                case 'f':	/* save in PPM format file */
                case 'F': save(); break;
                case 'n':	/* go on to the next spore */
                    next = 1;
					initialize();
                    break;
                case 'N':	/* go on to the next spore */
                    nummaps++;	/* but don't increment the spore counter */
                    next = 1;
                    break;
				case 'S':
                case 'R': spinning=0;
                    break;
                case 'r': spinning=1; spindir=1; 
					Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 1);
                    break;
                case 's': spinning=1; spindir=0;
					Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 0);
                    break;
            	case '\027': /* (ctrl-W) read palette from $HOME/.sporemap */
                  numwheels = 0;
				  if (!usedefault)
                      init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
							mincolindex,maxcolor,numwheels,"spore","Spore",0);
                  break;
                case 'W': 
                    if (numwheels < MAXWHEELS)
                        numwheels++;
                    else
                        numwheels = 0;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
							mincolindex,maxcolor,numwheels,"spore","Spore",0);
                    break;
                case 'w': 
                    if (numwheels > 0)
                        numwheels--;
                    else
                        numwheels = MAXWHEELS;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
							mincolindex,maxcolor,numwheels,"spore","Spore",0);
                    break;
                case '?':
                case 'h': 
					if (!useroot) {
						XGetWindowAttributes(dpy, help, &attr);
                    	if (attr.map_state != IsUnmapped)
                        	XUnmapWindow(dpy, help);
                    	else {
                        	XMapRaised(dpy, help);
                        	print_help();
                    	}
					}
                    break;
				case 'X':	/* create new spores, erasing the old */
						next = stop = 1;
						nummaps++;
						FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
										0, maxcolor);
						initialize();
						break;
                case 'Q':
                case 'q': Cleanup(); exit(0); break;
            }
			if (spinning)
				Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, spindir);
}

void
parseargs(argc, argv)
int argc;
char *argv[];
{
    int c;
    extern int optind, getopt();
    extern char *optarg;

    outname = "spore.ppm";
    width = 512; height = 480;
    while((c = getopt(argc, argv, "desuCFRTVc:h:k:m:n:o:w:D:N:")) != EOF)
    {    switch(c)
        {
        case 'c':
            numwheels = atoi(optarg);
            if (numwheels > MAXWHEELS)
                numwheels = MAXWHEELS;
            if (numwheels < 0)
                numwheels = 0;
            break;
        case 'd':
            demo++;
            break;
        case 'e':
            eternal++;
            break;
        case 'u':
            usage();
            exit(0);
        case 'h':
            height = atoi(optarg);
            break;
        case 'k':
            col_duration = atoi(optarg);
            break;
        case 'm':
            mincolindex = atoi(optarg);
            break;
        case 'n':
            nummaps = atoi(optarg);
            break;
        case 'o':
            ++oflag;
            outname = optarg;
            break;
        case 's':
            ++spin;
            break;
        case 'w':
            width = atoi(optarg);
            break;
        case 'C':
            center++;
            break;
        case 'F':
            full++;
            break;
        case 'D':
            delay = atoi(optarg);
            break;
        case 'N':
            nballs = atoi(optarg);
            break;
        case 'R':
            useroot++;
            break;
        case '?':
            usage();
            exit(1);
            break;
        }
    }
}

void
event_loop()
{
    int n;
    XEvent event;

    n = XEventsQueued(dpy, QueuedAfterFlush);
    while (n-- > 0) {
        XNextEvent(dpy, &event);
        switch(event.type) {
            case KeyPress:
                Getkey(&event);
                break;
            case Expose:
                redisplay(&event);
                break;
            case ConfigureNotify:
                resize();
                break;
        }
    }
}

int
checkit(q) 
int q;
{
    static int i, j;

    for (i=0;i<3;i++)
        for (j=0;j<3;j++)
            if (xy[(balls[q].now_x)+i-1][(balls[q].now_y)+j-1])
                return(0);
    return(1);
}

int
walk(x,y,z)
int x,y,z;
{
    balls[z].now_x = x;
    balls[z].now_y = y;
    if (checkit(z) == 0)
        return(0);
    for(;;) {
        move = lrand48() % 3;
        if (balls[z].now_x > balls[z].initial_x) {
            if (balls[z].now_y > balls[z].initial_y) {
                switch(move) {
                case 0: balls[z].now_x--; break;
                case 1: balls[z].now_x--; balls[z].now_y--; break;
                case 2: balls[z].now_y--; break;
                }
            }
            else if (balls[z].now_y < balls[z].initial_y) {
                switch(move) {
                case 0: balls[z].now_x--; break;
                case 1: balls[z].now_x--; balls[z].now_y++; break;
                case 2: balls[z].now_y++; break;
                }
            }
            else {
                switch(move) {
                case 0: balls[z].now_x--; balls[z].now_y--; break;
                case 1: balls[z].now_x--; break;
                case 2: balls[z].now_x--; balls[z].now_y++; break;
                }
            }
        }
        else if (balls[z].now_x < balls[z].initial_x) {
            if (balls[z].now_y > balls[z].initial_y) {
                switch(move) {
                case 0: balls[z].now_x++; break;
                case 1: balls[z].now_x++; balls[z].now_y--; break;
                case 2: balls[z].now_y--; break;
                }
            }
            else if (balls[z].now_y < balls[z].initial_y) {
                switch(move) {
                case 0: balls[z].now_y++; break;
                case 1: balls[z].now_y++; balls[z].now_x++; break;
                case 2: balls[z].now_x++; break;
                }
            }
            else {
                switch(move) {
                case 0: balls[z].now_x++; balls[z].now_y++; break;
                case 1: balls[z].now_x++; break;
                case 2: balls[z].now_x++; balls[z].now_y--; break;
                }
            }
        }
        else {
            if (balls[z].now_y > balls[z].initial_y) {
                switch(move) {
                case 0: balls[z].now_x--; balls[z].now_y--; break;
                case 1: balls[z].now_y--; break;
                case 2: balls[z].now_x++; balls[z].now_y--; break;
                }
            }
            else {
                switch(move) {
                case 0: balls[z].now_x--; balls[z].now_y++; break;
                case 1: balls[z].now_y++; break;
                case 2: balls[z].now_y++; balls[z].now_x++; break;
                }
            }
        }
        if (checkit(z) == 0)
            return(0);
    }
}

adjust_box(k)
int k;
{
	if ((balls[k].now_y <= balls[k].min_y) && (balls[k].now_y > 1)) {
		balls[k].min_y = balls[k].now_y - 1;
		balls[k].dy = balls[k].max_y - balls[k].min_y;
	}
	if ((balls[k].now_x <= balls[k].min_x) && (balls[k].now_x > 1)) {
		balls[k].min_x = balls[k].now_x - 1;
		balls[k].dx = balls[k].max_x - balls[k].min_x;
	}
	if ((balls[k].now_y >= balls[k].max_y) && (balls[k].now_y < height - 1)) {
		balls[k].max_y = balls[k].now_y + 1;
		balls[k].dy = balls[k].max_y - balls[k].min_y;
	}
	if ((balls[k].now_x >= balls[k].max_x) && (balls[k].now_x < width - 1)) {
		balls[k].max_x = balls[k].now_x + 1;
		balls[k].dx = balls[k].max_x - balls[k].min_x;
	}
}

void
aggregate() 
{
	static int k, enter_x, enter_y;

	for (k=0;k<nballs;k++) {
		move = lrand48() % 4;
		switch(move) {
		case 0:	/* entry on top border */
			enter_x = (lrand48() % balls[k].dx) + balls[k].min_x;
			enter_y = balls[k].min_y;
			break;
		case 1:	/* entry on bottom border */
			enter_x = (lrand48() % balls[k].dx) + balls[k].min_x;
			enter_y = balls[k].max_y;
			break;
		case 2:	/* entry on left border */
			enter_y = (lrand48() % balls[k].dy) + balls[k].min_y;
			enter_x = balls[k].min_x;
			break;
		case 3:	/* entry on right border */
			enter_y = (lrand48() % balls[k].dy) + balls[k].min_y;
			enter_x = balls[k].max_x;
			break;
		}
		if (xy[enter_x][enter_y])
			continue;
		walk(enter_x,enter_y,k);
		if (xy[balls[k].now_x][balls[k].now_y])
			continue;
		adjust_box(k);
		xy[balls[k].now_x][balls[k].now_y] = spore_hue;
		BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, spore_hue, 
					balls[k].now_x, height - balls[k].now_y - 1);
		if ((balls[k].now_y >= height-1) || (balls[k].now_x >= width-1)) {
			num = 1;
			stop=1;
		}
		if ((balls[k].now_y <= 1) || (balls[k].now_x <= 1)) {
			num = 1;
			stop=1;
		}
	}
}

main(argc,argv)
int argc;
char *argv[];
{
    static int i, j;
	static int count;
    XSizeHints hint;
    Atom __SWM_VROOT = None;
    Window rootReturn, parentReturn, *children;
	XVisualInfo *visual_list, visual_template;
    unsigned int numChildren;
    extern void srand48(), init_color();
    
    parseargs(argc,argv);
    srand48((long)time(0));
    dpy = XOpenDisplay("");
    screen = DefaultScreen(dpy);
    if (full || useroot) {
        width = XDisplayWidth(dpy, screen);
        height = XDisplayHeight(dpy, screen);
    }
    maxcolor  = (int)XDisplayCells(dpy, screen);
	if (maxcolor <= 16) {
		STARTCOLOR = 2; delay = 100;
		INDEXINC = 1; mincolindex = 5;
	}
	maxcolor = Min(maxcolor, MAXCOLOR);
	spore_hue = STARTCOLOR;
    /*
    * Create the pixmap to hold the spore growth
    */
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width, height, 
                           DefaultDepth(dpy, screen));
    /*
    * Create the window to display the fractal topographic map
    */
    hint.x = 0;
    hint.y = 0;
    hint.width = width;
    hint.height = height;
    hint.flags = PPosition | PSize;
    if (useroot) {
        canvas = DefaultRootWindow(dpy);
        /* search for virtual root (from ssetroot by Tom LaStrange) */
        __SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
        XQueryTree(dpy,canvas,&rootReturn,&parentReturn,&children,&numChildren);
        for (j = 0; j < numChildren; j++) {
            Atom actual_type;
            int actual_format;
            long nitems, bytesafter;
            Window *newRoot = NULL;

            if (XGetWindowProperty (dpy, children[j], __SWM_VROOT,0,1, False, 
                XA_WINDOW, &actual_type, &actual_format, &nitems, &bytesafter,
                (unsigned char **) &newRoot) == Success && newRoot) {
                canvas = *newRoot;
                break;
            }
        }
    }
    else {
        canvas = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
            0, 0, width, height, 5, 0, 1);
        XSetStandardProperties(dpy, canvas, "Spore by Ron Record",
            "Spore", None, argv, argc, &hint);
        XMapRaised(dpy, canvas);
    }
    XChangeProperty(dpy, canvas, XA_WM_CLASS, XA_STRING, 8, PropModeReplace, 
					"spore", strlen("spore"));
    /*
    * Create the window used to display the help info (if not running on root)
    */
	if (!useroot) {
    	hint.x = XDisplayWidth(dpy, screen) / 4;
    	hint.y = XDisplayHeight(dpy, screen) / 4;
    	hint.width = hint.x * 2;
    	hint.height = hint.y * 2;
    	help = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
            	hint.x, hint.y, hint.width, hint.height, 5, 0, 1);
		XSetWindowBackground(dpy, help, BlackPixel(dpy, screen));
    	/* Title */
    	XSetStandardProperties(dpy,help,"Help","Help",None,argv,argc,&hint);
    	/* Try to write into a new color map */
	    visual_template.class = PseudoColor;
	    visual_list = XGetVisualInfo(dpy,VisualClassMask,&visual_template, &i);
	    if (! visual_list)
		    usedefault = 1;
		if (usedefault)
		  cmap = DefaultColormap(dpy, screen);
		else {
    	  cmap = XCreateColormap(dpy,canvas,DefaultVisual(dpy,screen),AllocAll);
    	  init_color(dpy, canvas, cmap, Colors, STARTCOLOR, mincolindex, 
		      maxcolor, numwheels,"spore", "Spore", 0);
    	  /* install new color map */
          XSetWindowColormap(dpy, canvas, cmap);
	    }
	}
    init_contexts();
    setupmem();
	if (useroot)
    	XSelectInput(dpy,canvas,ExposureMask);
	else {
    	XSelectInput(dpy,canvas,KeyPressMask|ExposureMask|StructureNotifyMask);
    	XSelectInput(dpy,help,KeyPressMask|ExposureMask);
	}
    for (i=0; i!=nummaps; i++) {
        next = 0;
		if (!stop)	/* true 1st time thru */
			initialize();
		stop=0;
		FlushBuffer(dpy,canvas,pixmap,Data_GC,&Points,spore_hue,spore_hue+1);
		for (;;) {
			event_loop();
			aggregate();
			if (stop) {
			  if (eternal) {
				center = 0;
				next = 1;
				nummaps++;
				init_ball(lrand48()%nballs); /* re-do one of them */
			  }
			  break;
			}
			if ((num % col_duration) == 0) {
			  FlushBuffer(dpy,canvas,pixmap,Data_GC,&Points,
							spore_hue,spore_hue+1);
			  spore_hue++; 
			  num=0;
			  if (spore_hue >= maxcolor)
				spore_hue = STARTCOLOR;
			}
			num++;
		}
        FlushBuffer(dpy,canvas,pixmap,Data_GC,&Points,0,maxcolor);
        if (oflag)
            save();
        if (demo) {
            event_loop();
			DemoSpin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 4);
            event_loop();
			for (j=0; j<=MAXWHEELS; j++) {
				if (!usedefault)
				    init_color(dpy, canvas, cmap, Colors, STARTCOLOR, 
					    mincolindex, maxcolor, i, "spore", "Spore", 0);
				event_loop();
				sleep(1);
			}
        }
		else if (useroot) {
        	XSetWindowBackgroundPixmap(dpy, canvas, pixmap);
			for (i=0; i<maxcolor; i++)
				XFreeGC(dpy, Data_GC[i]);
			XFreePixmap(dpy, pixmap);
			XClearWindow(dpy, canvas);
			XFlush(dpy);
			Cleanup(); 
			exit(0);
		}
        else {
            XSync(dpy, True);
            if (spin)
				Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 0);
            for (;;) {
                event_loop();
                if (next) break;
            }
        }
    }
	Cleanup();
    exit(0);
}
