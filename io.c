#ifdef SERGEEV
#define _GNU_SOURCE
#endif /* SERGEEV */
#include <stdio.h>
#ifdef SERGEEV
#include <unistd.h>
#endif /* SERGEEV */
#include <termios.h>
#include <curses.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef SERGEEV
#include <conio.h>
#endif /* SERGEEV */
#ifdef MSDOS
#include <dos.h>
#endif
#include <time.h>

#ifdef SERGEEV
#include "sstlinux.h"
#endif /* SERGEEV */
#include "sst.h"

#ifndef SERGEEV
static int linecount;	/* for paging */
#endif /* SERGEEV */
static int screenheight = 24, screenwidth = 80;
#ifndef SERGEEV
static int curses = FALSE;
#else /* SERGEEV */
static int curses = TRUE;
#endif /* SERGEEV */

#ifdef SERGEEV
wnd wnds[6]={{1,1,80,25},	/* FULLSCREEN_WINDOW */
	     {1,1,25,12},	/* LEFTUPPER_WINDOW */
	     {26,2,80,12},	/* SRSCAN_WINDOW */
	     {65,1,80,10},	/* LRSCAN_WINDOW */
	     {1,13,80,23},	/* LOWER_WINDOW */
	     {1,24,80,25},	/* BOTTOM_WINDOW */
};
short curwnd;
#endif /* SERGEEV */

static void outro(int sig) {
/* wrap up, either normally or due to signal */
    if (curses) {
	clear();
	(void)refresh();
	(void)resetterm();
	//(void)echo();
	(void)endwin();
    }
}

static void fastexit(int sig) {
    outro(sig);
    putchar('\n');
    exit(0);
}

void iostart(int usecurses) {
	(void) signal(SIGINT, fastexit);
	(void) signal(SIGINT, fastexit);
#ifdef SIGIOT
	(void) signal(SIGIOT,fastexit);		/* for assert(3) */
#endif /* SIGIOT */
	if(signal(SIGQUIT,SIG_IGN) != SIG_IGN)
	    (void)signal(SIGQUIT,fastexit);

	if ((curses = usecurses)) {
		(void)initscr();
#ifdef KEY_MIN
		keypad(stdscr, TRUE);
#endif /* KEY_MIN */
		(void)saveterm();
		(void)nonl();
		(void)cbreak();
		//(void)noecho();
		scrollok(stdscr, TRUE);
		getmaxyx(stdscr, screenheight, screenwidth);
	} else {
		char *LINES = getenv("LINES");
		if (LINES)
		    screenheight = atoi(LINES);
	}
}

void ioend(void) {
    outro(0);
}

void waitfor(void) {
/* wait for user action -- OK to do nothing if on a TTY */
#ifdef SERGEEV
	getche();
#endif /* SERGEEV */
}

void pause_game(int i) {
	char *prompt;
#ifndef SERGEEV
        char buf[BUFSIZ];
#else /* SERGEEV */
        drawmaps(0);
        setwnd(BOTTOM_WINDOW);
#endif /* SERGEEV */
	if (i==1) {
		if (skill > 2)
			prompt = "[ANOUNCEMENT ARRIVING...]";
		else
			prompt = "[IMPORTANT ANNOUNCEMENT ARRIVING -- PRESS ENTER TO CONTINUE]";
	}
	else {
	    	if (skill > 2)
	    		prompt = "[CONTINUE?]";
	    	else
			prompt = "[PRESS ENTER TO CONTINUE]";

	}
#ifndef SERGEEV
	if (curses) {
	    waddch(stdscr, '\n');
		waddstr(stdscr, prompt);
		wgetnstr(stdscr, buf, sizeof(buf));
		wclear(stdscr);
		wrefresh(stdscr);
	} else {
		putchar('\n');
		proutn(prompt);
		fgets(buf, sizeof(buf), stdin);
		if (i != 0) {
		    /* much more in that old-TTY spirit to throw linefeeds */
		    int j;
		    for (j = 0; j < screenheight; j++)
			putchar('\n');
		}
		linecount = 0;
	}
#else /* SERGEEV */
	proutn(prompt);
	getche();
        clrscr();
        setwnd(LOWER_WINDOW);
        clrscr();
#endif /* SERGEEV */
}


void skip(int i) {
#ifndef SERGEEV
    while (i-- > 0) {
	if (curses) {
	    int y, x;
	    getyx(stdscr, y, x);
	    if (y == screenheight-1)
		pause_game(0);
	    else
		waddch(stdscr, '\n');
	} else {
	    linecount++;
	    if (linecount >= screenheight)
		pause_game(0);
	    else
		putchar('\n');
	}
#else /* SERGEEV */
        while (i-- > 0) proutn("\n\r");
}

static void vproutn(char *fmt, va_list ap) {
    char *s, *p;
    vasprintf(&s, fmt, ap);
    p=s;
    if ((curwnd==LOWER_WINDOW)&&(wherey()==wnds[curwnd].wndbottom-wnds[curwnd].wndtop)){
       if (strchr(s,'\n')) {
          p=strchr(s,'\n');
          p[0]=0;
          cprintf("%s",s);
          p++;
          pause_game(0);
       }
#endif /* SERGEEV */
    }
#ifdef SERGEEV
    if ((curwnd==LOWER_WINDOW)&&(wherey()>wnds[curwnd].wndbottom-wnds[curwnd].wndtop+1))
       cprintf("\r");
//        setwnd(curwnd);
    if (strchr(s,'\n') || strchr(s,'\r')) clreol();
    cprintf("%s",p);
    free(s);
#endif /* SERGEEV */
}

void proutn(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
#ifndef SERGEEV
    if (curses) {
	vw_printw(stdscr, fmt, ap);
        wrefresh(stdscr);
    } else
	vprintf(fmt, ap);
#else /* SERGEEV */
    vproutn(fmt, ap);
#endif /* SERGEEV */
    va_end(ap);
}

void prout(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
#ifndef SERGEEV
    if (curses) {
	vw_printw(stdscr, fmt, ap);
	wrefresh(stdscr);
    } else
	vprintf(fmt, ap);
#else /* SERGEEV */
    vproutn(fmt, ap);
#endif /* SERGEEV */
    va_end(ap);
    skip(1);
}

void proutc(char *line) {
    line[strlen(line)-1] = '\0';
#ifndef SERGEEV
    if (curses)
	waddstr(stdscr, line);
    else
	fputs(line, stdout);
#else /* SERGEEV */
    cputs(line);
#endif /* SERGEEV */
    skip(1);
}

#ifdef SERGEEV
static void prchr(char *s){
     char str[2];
     strncpy(str,s,1);
     str[1]=0;
     proutn(str);
}

static void vprouts(char *fmt, va_list ap) {
    char *s, *p;
    vasprintf(&s, fmt, ap);
    p=s;
    while (*p) {
        prchr(p++);
        delay(30);
    }
    free(s);
}

#endif /* SERGEEV */
void prouts(char *fmt, ...) {
#ifndef SERGEEV
	clock_t endTime;
	char *s, buf[BUFSIZ];
	/* print slowly! */
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	skip(1);
	for (s = buf; *s; s++) {
		endTime = clock() + CLOCKS_PER_SEC*0.05;
		while (clock() < endTime) continue;
		if (curses) {
		    waddch(stdscr, *s);
		    wrefresh(stdscr);
		}
		else {
		    putchar(*s);
		    fflush(stdout);
		}
	}
#else /* SERGEEV */
    va_list ap;
    va_start(ap, fmt);
    vprouts(fmt, ap);
    va_end(ap);
#endif /* SERGEEV */
}

void c_printf (char *format, ... )
{
    char buffer[BUFSIZ]; /* Well, BUFSIZ is from ncurses...  */
    va_list argp;
    va_start(argp,format);
    vsprintf(buffer,format,argp);
    va_end(argp);
#ifdef SERGEEV
    waddstr(conio_scr,buffer);
#else
    proutn(buffer);
#endif /* SERGEEV */
}

void warble(void)
/* sound and visual effects for teleportation */
{
#ifdef SERGEEV
    int posx, posy;
    posx=wherex();
    posy=wherey();
    drawmaps(1);
    setwnd(LOWER_WINDOW);
    gotoxy(posx,posy);
    sound(50);
    delay(1000);
    nosound();
#else
    prouts(" . . . . . ");
#endif /* SERGEEV */
}

void setpassword(void) {
#ifndef SERGEEV
	while (TRUE) {
		scan();
		strcpy(game.passwd, citem);
		chew();
		if (*game.passwd != 0) break;
		proutn("Please type in a secret password-");
	}
#else
	int i;
        for(i=0;i<3;i++) game.passwd[i]=(char)(97+(int)(Rand()*25));
        game.passwd[3]=0;
#endif /* SERGEEV */
}

void getline(char *line, int max) {
    if (curses) {
#ifndef SERGEEV
	wgetnstr(stdscr, line, max);
	wrefresh(stdscr);
#else /* SERGEEV */
	line[0]=max-1;
	cgets(line);
	memmove(line,&line[2],max-3);
#endif /* SERGEEV */
    } else {
	fgets(line, max, stdin);
        line[strlen(line)-1] = '\0';
    }
}

void setwnd(short wndnum){
/* change windows -- OK for this to be a no-op in tty mode */
#ifdef SERGEEV
     int cury;
     cury=wherey()+wnds[curwnd].wndtop-wnds[wndnum].wndtop;
     if ((curwnd==FULLSCREEN_WINDOW)&&(wndnum!=FULLSCREEN_WINDOW)) clrscr();
     window(wnds[wndnum].wndleft, wnds[wndnum].wndtop, wnds[wndnum].wndright, wnds[wndnum].wndbottom);
     if ((curwnd==wndnum)&&(cury>wnds[wndnum].wndbottom-wnds[wndnum].wndtop+1)){
        gotoxy(wnds[wndnum].wndright-wnds[wndnum].wndleft+1,wnds[wndnum].wndbottom-wnds[wndnum].wndtop+1);
	skip(1);
     }
     curwnd=wndnum;
     gotoxy(1,cury);
#endif /* SERGEEV */
}

void commandhook(char *cmd, int before) {
}
