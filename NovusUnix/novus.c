/*** includes ***/
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

/*** defines ***/

#define NOVUS_VERSION "0.0.1"

// It sets the upper 3 bits of the character to 0 (which is what the CTRL key does)
// 00011111 (bitwise and)
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN
};

/*** data ***/

struct editorConfig {
  int cx, cy;
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/

// Prints and error message & exit the program
void die(const char *s){
  // Clear the screen on exit
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // perror() : Looks at global errno var * prints a descriptive error message for it.
  perror(s);

  // Exit program with error status of 1, which indicates failure (as would any non-zero value).
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  // tcsetattr() : return -1 on failure, and set the errno 
  // tcgetattr() : return -1 on failure, and set the errno 
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  // I in IXON stands for "input flag" (unlike the other I flags we'e seen so far)
  // XON comes from the names of the two control characters CTRL-S & CTRL-Q
  // produce: XOFF to pause transmission and XON to resume transmission
  // Now Ctrl-S can be read as a 19 byte and CTRL-Q as 17 byte
  // BRKINT : Stop break condition that causes SIGINT to be sent
  // ICRNL  : Fix Ctrl-M
  // INPCK  : Enables parite chcking (doesn't apply to modern terminals)
  // ISTRIP : Causes the 8th bit of each input byte to be stripped (turn off)
  // IXON   : Bit Mask (NOT A FLAG), set char size to 8 bits per bye
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

  // OPOST : Disable all ouput processing (like "\n" to "\r\n")
  raw.c_oflag &= ~(OPOST);

  // Misc flags
  raw.c_cflag |= (CS8);

  // ISIG   : Disable Ctrl-C & Ctrl-Z signals
  // IEXTEN : Disable Ctrl-V signals
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // VMIN  : The min. number of input needed before read() can return
  // VTIME : The max time to wait before read() returns (set it to 1/10 sec or 100 milliseconds)
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  // tcsetattr() : return -1 on failure, and set the errno 
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if (c == '\x1b') {
    char seq[3];
    
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          case '5': return PAGE_UP;
          case '6': return PAGE_DOWN;
        }
      }
    } else {
      switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }
 }

    return '\x1b';
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while(i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;

  /*

  printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);

  editorReadKey();

  return -1;
  */
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer **/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/** output ***/

void editorDrawRows(struct abuf *ab){
  int y;
  // Draw columns of tildes (~)
  for (y = 0; y < E.screenrows; y++){
    if ( y == E.screenrows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);
      if (welcomelen > E.screencols) welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--) abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    } else{
      abAppend(ab, "~", 1);
    }

    // K command (Erase in Line erases par of the curren tline.
    abAppend(ab, "\x1b[K", 3);
    if ( y < E.screenrows - 1){
      abAppend(ab, "\r\n", 2);
    }
  }
}


void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  // Hide the cursor when repainting
  abAppend(&ab, "\x1b[?25l", 6);

  // Clear the screen
//  abAppend(&ab, "\x1b[2J", 4);

  // Reposition the cursor
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf));

  // Reposition cursor back to top left after drawing tildes
//  abAppend(&ab, "\x1b[H", 3);

  // Hide the cursor when repainting
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/** input ***/

void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (E.cx != E.screencols - 1) {
        E.cx++;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      if (E.cy != E.screenrows - 1) {
        E.cy++;
      }
      break;
  }
}

void editorProcessKeypress() {
  int c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      // Clear the screen on exit
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
  }
}

/*** init ***/

// Initialize all fileds in the E struct
void initEditor() {
  E.cx = 0;
  E.cy = 0;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while(1){
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;

}
