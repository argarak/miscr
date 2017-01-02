#include <ncurses.h>
#include <ctype.h>

int main(void)
{
	WINDOW *left,*right;
	int maxx,maxy,halfx;
	int ch;
		
	initscr();
	start_color();
	init_pair(1,COLOR_BLACK,COLOR_BLUE);
	init_pair(2,COLOR_BLACK,COLOR_RED);

/* calculate window sizes and locations */
	getmaxyx(stdscr,maxy,maxx);
	halfx = maxx >> 1;						/* half the screen width */

/* create the two side-by-side windows */
	if( (left = newwin(maxy,halfx,0,0)) == NULL)
	{
		endwin();
		puts("Unable to create 'left' window");
		return 1;
	}
	if( (right = newwin(maxy,halfx,0,halfx)) == NULL)
	{
		endwin();
		puts("Unable to create 'right' window");
		return 1;
	}
	
/* Set up each window */
	mvwaddstr(left,0,0,"Left window (type ~ to end)\n");
	wbkgd(left,COLOR_PAIR(1));
	wrefresh(left);
	mvwaddstr(right,0,0,"Right window\n");
	wbkgd(right,COLOR_PAIR(2));
	wrefresh(right);

/* Read keyboard and update each window */
	do
	{
		ch = wgetch(left);					/* read/refresh left window */
		if(isalpha(ch))
		{
			if(toupper(ch)>='A' && toupper(ch)<='M')
				ch += 13;
			else
				ch -= 13;
		}
		waddch(right,ch);					/* write/refresh right window */
		wrefresh(right);
	} while(ch != '~');
	
	endwin();
	return 0;
}
