#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>
#include <conio.h>

// Get the console handle
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Editor config
const int TAB_SIZE = 2;

// Cursor position
int cursorX = 0;
int cursorY = 0;

// Scroll position
int scrollY = 0;

// Lines
std::vector<std::string> lines;

// Mode: true for insert mode, false for command mode
bool insertMode = false;

void clearScreen() {
#ifdef _WIN32
    DWORD count;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X * csbi.dwSize.Y, csbi.dwCursorPosition, &count);
    SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
#else
    std::cout << "\x1B[2J\x1B[H"; // Clear screen and move cursor to top-left
#endif
}

void redrawScreen() {
    clearScreen();

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top;

    for (int i = scrollY; i < scrollY + windowHeight && i < lines.size(); i++) {
        std::cout << lines[i] << "\n";
    }

    SetConsoleCursorPosition(hConsole, { (SHORT)cursorX, (SHORT)(cursorY - scrollY) });
}

void moveCursor(int dx, int dy) {
    cursorX += dx;
    cursorY += dy;

    if (cursorX < 0) cursorX = 0;
    if (cursorY < 0) cursorY = 0;
    if (cursorY >= lines.size()) cursorY = lines.size() - 1;
    if (cursorX > lines[cursorY].size()) cursorX = lines[cursorY].size();

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top;

    if (cursorY < scrollY) {
        scrollY = cursorY;
        redrawScreen();
    }
    else if (cursorY >= scrollY + windowHeight) {
        scrollY = cursorY - windowHeight + 1;
        redrawScreen();
    }
    else {
        SetConsoleCursorPosition(hConsole, { (SHORT)cursorX, (SHORT)(cursorY - scrollY) });
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: textEditor <filename>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    redrawScreen();

    while (true) {
        int ch = _getch();

        if (ch == 224) {
            ch = _getch();
            switch (ch) {
            case 72: moveCursor(0, -1); break; // Up arrow
            case 80: moveCursor(0, 1); break;  // Down arrow
            case 75: moveCursor(-1, 0); break; // Left arrow
            case 77: moveCursor(1, 0); break;  // Right arrow
            }
        }
        else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    return 0;
}