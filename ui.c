/*
 *  PWMan - password management application
 *
 *  Copyright (C) 2002  Ivan Kelly <ivan@ivankelly.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pwman.h>
#include <ui.h>
#include <help.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

char * statusline_ask_str(char *, char*, int);
Pw *get_highlighted_item();

int should_resize = FALSE;
int can_resize = FALSE;


WINDOW *top = NULL, *bottom = NULL;
extern int curitem, lines;
extern WINDOW *list;

int
ui_draw_top()
{
	werase(top);
	mvwhline(top, 1, 0, ACS_HLINE, COLS);
	if( !options->readonly) {
		mvwprintw(top, 0, 0, "%s | %s", PACKAGE " " VERSION,
			  MAIN_HELPLINE);
	} else {
		mvwprintw(top, 0, 0, "%s | %s | %s", PACKAGE " " VERSION,
			  READONLY_MSG, MAIN_HELPLINE);
	}
	
	wrefresh(top);
}

int
ui_draw_bottom()
{
	werase(bottom);
	mvwhline(bottom, 0, 0, ACS_HLINE, COLS);
	mvwhline(bottom, 2, 0, ACS_HLINE, COLS);

	wrefresh(bottom);
}

int 
ui_refresh_windows()
{
	ui_draw_top();
	ui_draw_bottom();
	uilist_refresh();

	refresh();
}

int
ui_resize_windows()
{
	wresize(top, 2, COLS);
}

static void
ui_too_small_warning()
{
	clear();
	attron(A_BOLD);
	mvprintw(((LINES-2)/2),0,"Your window is too small\n\n"
			"Minimum size is %dx%d\n\n"
			"Please resize and press any key", MIN_LINES, MIN_COLS);
	attroff(A_BOLD);
	getch();
}

#ifdef SIGWINCH
static void
ui_resize()
{
	struct winsize winsz;
	ioctl (0, TIOCGWINSZ, &winsz);

	resizeterm(winsz.ws_row, winsz.ws_col);
	if((winsz.ws_col < MIN_COLS) || (winsz.ws_row < MIN_LINES)) {
		
		/*
		 * if window is too small notify user 
		 * until he changes it
		 */
		do {
			ui_too_small_warning();
			ioctl (0, TIOCGWINSZ, &winsz);
			resizeterm(winsz.ws_row, winsz.ws_col);
		} while ((winsz.ws_col < MIN_COLS) || (winsz.ws_row < MIN_LINES));

	} else {
		should_resize = FALSE;
		/*	resize_windows();
		resize_list();*/
		ui_free_windows();
		ui_init_windows();
		ui_refresh_windows();
	}
}
#endif

static void
ui_win_changed(int i)
{
	if( can_resize ){
		ui_resize();
		ui_refresh_windows(); /* dunno why i need this but it wont work without it */
	} else {
		should_resize = TRUE;
	}
}


int
ui_init_windows()
{
	top = newwin(2, COLS,0,0);
	bottom = newwin(3, COLS, LINES - 3, 0);

	uilist_init();
}

int 
ui_free_windows()
{
	uilist_free();

	erase();
	delwin(top);
	delwin(bottom);
}

int
ui_init()
{
	initscr();
	cbreak();
	noecho();
	nonl();

	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	if((LINES < MIN_LINES) || (COLS < MIN_COLS)){
		clear(); refresh(); endwin();
		fprintf(stderr, "Your terminal is to small\n");
		fprintf(stderr, "Min size is %dx%d\n", MIN_COLS, MIN_LINES);

		return 1;
	}

#ifdef SIGWINCH
	signal(SIGWINCH, ui_win_changed);
#endif

	ui_init_windows();
	ui_refresh_windows();
}

int
ui_run()
{
	Pw *current_item;
	int ch;
	int i = 0;
	int load_worked = 0;
	int secret_key_message = 0;
	struct stat st;
	char secring[STRING_LONG];
	strcpy(secring, getenv("HOME"));
	strcat(secring, "/.gnupg/secring.gpg");
#ifdef DEBUG
	int debug_i = 0;
#endif
	char msg[80];

	time_base = time(NULL);

	while(1){
		can_resize = TRUE;
		if( should_resize ){
			ui_resize();
		}
		ch = getch();
		ui_statusline_clear();
		can_resize = FALSE;

		if((time_base < (time(NULL) - (options->passphrase_timeout*60)))
				&& options->passphrase_timeout != 0 && tolower(ch) != 'q'){
			pwlist_write_file();
			pwlist_free_all();

			ui_statusline_msg("Passphrase has timed out and you must enter it again.");
			getch();
			// Try to avoid segfault if secring isn't available
			// Need to check for 0 byte file size since something in pwlist_write_file() "touches" it
			stat(secring, &st);
			while ( st.st_size == 0 ) {
				if (secret_key_message == 0) {
					ui_statusline_msg("Secret key not available... Waiting until it is");
					secret_key_message = 1;
				}
				stat(secring, &st);
				sleep(1);
			}
			secret_key_message = 0;
			
			load_worked = pwlist_read_file();
			if(load_worked != 0) {
				ui_statusline_msg("Error - unable to re-load the password file!");
				break;
				// It segfaults here.
			}

			time_base = time(NULL);
		}
		
		switch(ch){
			case 'Q':
			case 'q':
				if(search_results != NULL) {
					search_remove();
				} else {
					if(action_list_at_top_level()){
						return 0;
					}
				}
				break;
			case '?':
				ui_display_help();
				break;
			case KEY_PPAGE:
				uilist_page_up();
				break;
			case KEY_NPAGE:
				uilist_page_down();
				break;
			case KEY_UP:
			case 'k':
				uilist_up();
				break;
			case KEY_DOWN:
			case 'j':
				uilist_down();
				break;
			case 'A':
				if (!options->readonly){
					action_list_add_sublist();
				} else {
					statusline_readonly();
				}
				break;
			case 'U':
				action_list_up_one_level();
				break;
			case 'r':
				if (!options->readonly){
					action_list_rename();
					pwlist_write_file();
				} else {
					statusline_readonly();
				}
				break;
			case 'a':
				if (!options->readonly){
					action_list_add_pw();
					pwlist_write_file();
				} else {
					statusline_readonly();
				}
				break;
			case 'e':
			case ' ':
			case 13: /* return/enter key */
				action_list_select_item();
				/*current_item = get_current_item();
				if(current_item){
					edit_pw(current_item);
				}*/
				break;
			case 'd':
			case 0x14A: /* DEL key */
				if (!options->readonly){
					action_list_delete_item();
				} else {
					statusline_readonly();
				}
				break;
			case 'm':
				if (!options->readonly){
					action_list_move_item();
				} else {
					statusline_readonly();
				}
				break;
			case 'M':
				if (!options->readonly){
					action_list_move_item_up_level();
				} else {
					statusline_readonly();
				}
				break;
			case 'h':
				hide_cursor();
				break;
				case 's':
				show_cursor();
				break;
			case 'o':
				action_edit_options();
				break;
			case 0x17: /* control-w */
				if (!options->readonly){
					pwlist_write_file();
				} else {
					statusline_readonly();
				}
				break;
			case 0x12: /* control-r */
				action_list_read_file();
				break;
			case 0x07: /* control-g */
				pwgen_indep();
				break;
			case 0x06: /* control-f */
				gnupg_forget_passphrase();
				break;
			case 0x0C: /* control-l */
				ui_refresh_windows();
				break;
			case '/':
			case 'F':
				search_get();
				break;
			case 'f':
				filter_get();
				break;
			case 'E':
				action_list_export();
				break;
			case 'I':
				if (!options->readonly){
					pwlist_import_passwd();
					uilist_refresh();
				} else {
					statusline_readonly();
				}
				break;
			case 'L':
				action_list_locate();
				break;
			case 'l':
				action_list_launch();
				break;
			case 0x0B: /* control-k (up) */
			case '[':
				action_list_move_item_up();
				break;
			case 0x0A: /* control-j (down) */
			case ']':
				action_list_move_item_down();
				break;
#ifdef DEBUG
			case '$':
				debug_i++;
				snprintf(msg, 80, "Name %d", debug_i);
					
				pwlist_add(current_pw_sublist, msg, "myhost", "myuser", "mypasswd", "mylaucnh");
				uilist_refresh();
				break;
#endif
			default:
				break;
		}
	}
}

int
ui_end()
{
	ui_free_windows();
	clear();
	refresh();
	endwin();
	echo();
}

int 
ui_statusline_msg(char * msg)
{
	ui_statusline_clear();
	mvwaddstr(bottom, 1, 0, msg);
	refresh();
	wrefresh(bottom);
}

int 
ui_statusline_clear()
{
	wmove(bottom, 1, 0);
	wclrtoeol(bottom);
	wrefresh(bottom);
	refresh();
}

void
ui_statusline_ask_num(char *msg, int *i)
{
	int x = strlen(msg) + 5;
	char input[STRING_SHORT];

	ui_statusline_clear();
	ui_statusline_msg(msg);

	echo();
	show_cursor();

	mvwgetnstr(bottom, 1, x, input, STRING_SHORT);
	*i = atoi(input);
	
	noecho();
	hide_cursor();

	ui_statusline_clear();
}

void
ui_statusline_ask_char(char *msg, char *c, char* valid)
{
	int x = strlen(msg) + 5;
	char input[STRING_SHORT];

	*c = 0;
	do {
		ui_statusline_clear();
		if(*c != 0){
			ui_statusline_msg("Bad choice, press any key to try again");
			getch();
			ui_statusline_clear();
		}
		ui_statusline_msg(msg);

		echo();
		show_cursor();

		*c = mvwgetch(bottom, 1, x);

		noecho();
		hide_cursor();
		
	} while ( !strchr(valid, *c) );
	
	ui_statusline_clear();
}

char *
ui_statusline_ask_str(char *msg, char *input, int len)
{
	char *tmp;
	char *tmp2;
	char *tmp3;
	char *oldinput;
	int x = strlen(msg) + 5;

	oldinput = malloc(strlen(input)+1);
	//Back-up the old value	
	strcpy(oldinput, input);

	if(input == NULL){
		input = malloc(len);
	}
	ui_statusline_clear();
	ui_statusline_msg(msg);

	echo();
	show_cursor();
	mvwgetnstr(bottom, 1, x, input, len);
	noecho();
	hide_cursor();

	ui_statusline_clear();

	//If just return entered, don't overwrite old value
	if(strlen(input) == 0){
		strcpy(input, oldinput);
	}

	// Tabs don't play nicely with ncurses or xml
	// So, swap any for (a single) space
	tmp = input;
	while(*tmp != 0) {
		if(*tmp == 9) *tmp = ' ';
		tmp++;
	}

	// In some cases (eg when inside screen), the backspace
	// comes through to us. Handle it here if needed
	tmp = input;
	while(*tmp != 0) {
		if(*tmp == 8) {
         // tmp2 is where to copy to, tmp3 is where to copy from
         tmp3 = tmp + 1;
         if(tmp == input) {
            tmp2 = tmp;
         } else {
            tmp2 = tmp - 1;
         }

         // When we're done, start from the character
         //  we copied in to
         tmp = tmp2;

         // Process forward
         while(*tmp3 != 0) {
            *tmp2 = *tmp3;
            tmp2++;
            tmp3++;
         }
         *tmp2 = 0;
      } else {
   		tmp++;
      }
	}
	
	// All done
	free(oldinput);
	return input;
}

char *
ui_statusline_ask_str_with_autogen(char *msg, char *input, int len, char *(*autogen)(char *), int ch)
{
	int i = 0;
	int c;
	char *text[2], *s;
	char *oldinput;
	int x;

	oldinput = malloc(strlen(input)+1);
	//Back-up the old value	
	strcpy(oldinput, input);

	if(input == NULL){
		input = malloc(len);
	}
	text[0] = malloc(STRING_MEDIUM);
	text[1] = malloc(STRING_SHORT);
	
	strncpy(text[1], msg, STRING_SHORT);
	if(s = strrchr(text[1], ':')){
		*s = 0;
	}
	snprintf(text[0], STRING_MEDIUM, "%s(%c for autogen):\t", text[1],ch);
	x = strlen(text[0]) + 5;

	ui_statusline_clear();
	ui_statusline_msg(text[0]);

	show_cursor();
	noecho();

	wmove(bottom, 1, x);

	while(i < len){
		c = wgetch(bottom);
		if(c == 0x7f){
			if(i){
				i--;
				mvwaddch(bottom, 1, x+i, ' ');
				wmove(bottom, 1, x+i);
			}
		} else if(c == 0xd){
			input[i] = 0;
			break;
		} else if(c == ch){
			input = autogen(input);
			break;
		} else {
			input[i] = c;
			mvwaddch(bottom, 1, x + i, c);
			i++;
		}
	}
	
	hide_cursor();
	
	ui_statusline_clear();

	//If just return entered, don't overwrite old value
	if(strlen(input) == 0){
		strcpy(input, oldinput);
	}

	free(text[0]);
	free(text[1]);
	free(oldinput);

	return input;
}

char *
ui_statusline_ask_passwd(char *msg, char *input, int len, int cancel)
{
	int i = 0;
	int c;
	int x = strlen(msg) + 5;

	if(!input){
		input = malloc(len);
	}
	ui_statusline_clear();
	ui_statusline_msg(msg);

	show_cursor();
	noecho();

	wmove(bottom, 1, x);

	while(i < len){
		c = wgetch(bottom);
		if(c == 0x7f){ /* 0x7f = delete */
			if(i){
				i--;
				mvwaddch(bottom, 1, x+i, ' ');
				wmove(bottom, 1, x+i);
			}
		} else if(c == 0xd){ /* 0xd == enter/return */
			input[i] = 0;
			break;
		} else if(c == cancel){
			free(input);
			input = NULL;

			return input;
		} else {
			input[i] = c;
			mvwaddch(bottom, 1, x + i, '*');
			i++;
		}
	}
	
	hide_cursor();
	
	ui_statusline_clear();
	
	return input;
}

int 
ui_statusline_yes_no(char *msg, int def)
{
	int ret = -1, len;
	char *msg2;
	int ch;
	
	len = strlen(msg) + 10;
	msg2 = malloc(len);

	snprintf(msg2, len,  "%s%s", msg, def ? " (Y/n)?" : " (y/N)?", NULL);

	while(ret == -1){
		ui_statusline_msg(msg2);
		
		ch = getch();
		switch( ch ){
			case 'n':
			case 'N':
				ret = FALSE;
				break;
			case 'y':
			case 'Y':
				ret = TRUE;
				break;
			case 13:
				ret = def;
				break;
			default:
				ui_statusline_msg("Bad option, try again.");
				getch();
				break;
		}
	}

	free(msg2);
	ui_statusline_clear();

	return ret;
}

void
statusline_readonly()
{
	ui_statusline_msg("Password file is opened readonly");
}

int 
ui_display_help()
{
	int i;
	WINDOW *helpwin;

	helpwin = newwin(LINES - 5, COLS - 6, 3, 3);
	uilist_clear();

	for(i = 0; help[i] != NULL; i++){
		waddstr(helpwin, help[i]);
		if( !((i+1) % (LINES - 9)) || (help[i+1] == NULL) ){
	/*		refresh();*/
			wrefresh(helpwin);
			ui_statusline_msg("Press any key to continue...");
			getch();
			wclear(helpwin);
		}
	}
	uilist_refresh();
	ui_statusline_clear();
	delwin(helpwin);
}
