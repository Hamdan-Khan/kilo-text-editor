/*** includes  ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

// 0x1f - hexadecimal representation of 00011111
// this function bitwise-ANDs a character with the hexadec number defined to convert it into the ASCII equivalent of: Ctrl + that character
#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

struct termios original_termios;

/*** terminal ***/

// function for error demonstration, to be used in possible failure conditions
void die(const char*s){
	// from stdio, prints a descriptive error message for the global errno variable along with the string given to it
	perror(s);
	// from stdlib, used to terminate the program immediately.
	// 0 means program ended succesfully, 1 (or any non-zero value) means it ended with an error
	exit(1);
}

//
void disableRawMode(){
	//
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1){
		die("tcsetattr");
	}
}

//
void enableRawMode(){
	//
	if(tcgetattr(STDIN_FILENO, &original_termios) == -1){
		die("tcsetattr");
	}
	// register the disableRawMode function to be executed when the main function exits
	atexit(disableRawMode);

	// copying the original_termios
	struct termios raw = original_termios;
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

/*** init ***/

int main(){
	enableRawMode();

	while (1){
		char c = '\0';
		if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){
			die("read");
		}
		if(iscntrl(c)){
			printf("%d\r\n", c);
		} else{
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == CTRL_KEY('q')){
			 break;
		}
	}

	return 0;
}
