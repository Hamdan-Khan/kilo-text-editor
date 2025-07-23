/*** includes  ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

// 0x1f - hexadecimal representation of 00011111
// this function bitwise-ANDs a character with the hexadec number defined to convert it into the ASCII equivalent of: Ctrl + that character
#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

struct editorConfig {
	int screen_rows;
	int screen_cols;
	struct termios original_termios;
};

struct editorConfig E;

/*** terminal ***/

// function for error demonstration, to be used in possible failure conditions
void die(const char*s){
	// original def in editorRefreshScreen()
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	// from stdio, prints a descriptive error message for the global errno variable along with the string given to it
	perror(s);
	// from stdlib, used to terminate the program immediately.
	// 0 means program ended succesfully, 1 (or any non-zero value) means it ended with an error
	exit(1);
}

//
void disableRawMode(){
	//
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1){
		die("tcsetattr");
	}
}

//
void enableRawMode(){
	//
	if(tcgetattr(STDIN_FILENO, &E.original_termios) == -1){
		die("tcsetattr");
	}
	// register the disableRawMode function to be executed when the main function exits
	atexit(disableRawMode);

	// copying the original_termios
	struct termios raw = E.original_termios;
	// input flags
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	// output flags
	raw.c_oflag &= ~(OPOST);
	// control flags
	raw.c_cflag |= (CS8);
	// local flags
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	// raw.c_cc = control characters
	// VMIN (unit - bytes) = minimum number of bytes read() can return
	raw.c_cc[VMIN] = 0;
	// VTIME (unit - 0.1*second) = the maximum amount of time read() takes before it returns
	raw.c_cc[VTIME] = 10;

	//
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
		die("tcsetattr");
	}
}

char editorReadKey(){
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c, 1)) != 1){
		if(nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}
	return c;
}

int getCursorPosition(int *rows, int *cols){
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4){
		return -1;
	}

	while (i < sizeof(buf) -1){
		if (read(STDIN_FILENO, &buf[i], 1) != 1){
			break;
		}
		if (buf[i] == 'R') {
			break;
		}
	}

	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '['){
		return -1;
	}
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2){
		return -1;
	}

	return 0;
}

int getWindowSize(int *rows, int *cols){
	// from sys/ioctl
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		// fallback for ioctl failure, C - moves cursor to right, B moves it down
		// goal is to move the cursor to the bottom-right of screen
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
			return -1;
		}
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** output ***/

void editorDrawRows(){
	int y;
	for(y = 0; y < E.screen_rows; y++){
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

void editorRefreshScreen(){
	// write() - from unistd, low-level, unlike printf it writes bytes directly into the terminal
	// write(file, buffer, bytes)

	// \x1b is the byte rep of ESC character: '<esc>[2J'
	// [2J command is used to clear the entire screen (VT100 escape sequences)
	write(STDOUT_FILENO, "\x1b[2J", 4);
	// [H command is used to position the cursor, default position is 1,1
	// [x;yH e.g: [10,40H
	write(STDOUT_FILENO, "\x1b[H", 3);

	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** input ***/

void editorProcessKeypress(){
	char c = editorReadKey();

	switch(c){
		case CTRL_KEY('q'):
			// original def in editorRefreshScreen()
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

/*** init ***/

void initEditor() {
	if (getWindowSize(&E.screen_rows, &E.screen_cols) == -1){
		die("getWindowsize");
	}
}

int main(){
	enableRawMode();
	initEditor();

	while (1){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
