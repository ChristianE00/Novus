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

// Visual cursor position
int visualCursorX = 0;
int visualCursorY = 0;

// Scroll position
int scrollY = 0;

// Modes
enum class Mode {
    Normal,
    Insert,
    Command
};

// Filename
std::string filename;

// Lines
std::vector<std::string> lines;

// Current mode
Mode currentMode = Mode::Normal;

void displayMode() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    COORD modePosition = { (SHORT)(windowWidth - 20), (SHORT)(csbi.srWindow.Bottom - csbi.srWindow.Top) };
    SetConsoleCursorPosition(hConsole, modePosition);

    // Clear the previous mode text
    std::cout << std::string(20, ' ');

    SetConsoleCursorPosition(hConsole, modePosition);

    switch (currentMode) {
    case Mode::Insert:
        std::cout << "INSERT MODE";
        break;
    case Mode::Command:
        std::cout << "COMMAND MODE";
        break;
    case Mode::Normal:
        std::cout << "NORMAL MODE";
        break;
    }

    SetConsoleCursorPosition(hConsole, { (SHORT)visualCursorX, (SHORT)(visualCursorY - scrollY) });
}

void clearScreen() {
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X * csbi.dwSize.Y, { 0, 0 }, &count);
    SetConsoleCursorPosition(hConsole, { 0, 0 });
}

void redrawScreen() {
    clearScreen();

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    for (int i = scrollY; i < scrollY + windowHeight - 1 && i < lines.size(); i++) {
        std::string line = lines[i];
        int printWidth = 0;
        while (printWidth < line.length()) {
            std::cout << line.substr(printWidth, windowWidth);
            printWidth += windowWidth;
            if (printWidth < line.length()) {
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }

    displayMode();
}

void moveCursor(int dx, int dy) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    // Update visual cursor position
    visualCursorX += dx;
    visualCursorY += dy;

    // Handle line wrapping
    while (visualCursorX < 0) {
        if (visualCursorY > 0) {
            visualCursorY--;
            visualCursorX = lines[visualCursorY].length() % windowWidth;
        }
        else {
            visualCursorX = 0;
            break;
        }
    }
    while (visualCursorY < lines.size() && visualCursorX > lines[visualCursorY].length() % windowWidth) {
        visualCursorX -= windowWidth;
        visualCursorY++;
        if (visualCursorY >= lines.size()) {
            visualCursorX = lines[visualCursorY - 1].length() % windowWidth;
            visualCursorY = lines.size() - 1;
            break;
        }
    }

    // Update logical cursor position based on visual position
    cursorX = visualCursorX + (visualCursorY - cursorY) * windowWidth;
    cursorY = visualCursorY;

    // Adjust scroll position if necessary
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    if (cursorY < scrollY) {
        scrollY = cursorY;
        redrawScreen();
    }
    else if (cursorY >= scrollY + windowHeight - 1) {
        scrollY = cursorY - windowHeight + 2;
        redrawScreen();
    }
    else {
        SetConsoleCursorPosition(hConsole, { (SHORT)visualCursorX, (SHORT)(visualCursorY - scrollY) });
    }
}



void insertCharacter(char ch) {
    if (cursorY >= 0 && cursorY < lines.size() && cursorX >= 0 && cursorX <= lines[cursorY].length()) {
        lines[cursorY].insert(cursorX, 1, ch);
        moveCursor(1, 0);
        redrawScreen();
    }
}

void deleteCharacter() {
    if (cursorY >= 0 && cursorY < lines.size() && cursorX > 0 && cursorX <= lines[cursorY].length()) {
        lines[cursorY].erase(cursorX - 1, 1);
        moveCursor(-1, 0);
        redrawScreen();
    }
    else if (cursorY > 0 && cursorX == 0) {
        cursorX = lines[cursorY - 1].size();
        lines[cursorY - 1] += lines[cursorY];
        lines.erase(lines.begin() + cursorY);
        moveCursor(0, -1);
        redrawScreen();
    }
}

void saveFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }

    for (const auto& line : lines) {
        file << line << "\n";
    }

    file.close();
}

void executeCommand(const std::string& command) {
    if (command == "q" || command == "quit") {
        clearScreen();
        exit(0);
    }
    else if (command == "w") {
        saveFile(filename);
    }
    else if (command == "wq") {
        saveFile(filename);
        clearScreen();
        exit(0);
    }
}

void enterCommandMode() {
    std::string command;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    COORD commandPosition = { 0, (SHORT)(windowHeight - 1) };
    SetConsoleCursorPosition(hConsole, commandPosition);

    std::cout << ":";
    while (true) {
        int ch = _getch();
        if (ch == '\r') { // Enter key
            executeCommand(command);
            break;
        }
        else if (ch == 8 || ch == 127) { // Backspace or Delete key
            if (!command.empty()) {
                command.pop_back();
                std::cout << "\b \b";
            }
        }
        else if (ch >= 32 && ch <= 126) { // Printable characters
            command.push_back(ch);
            std::cout << (char)ch;
        }
    }

    // Clear the command line
    SetConsoleCursorPosition(hConsole, commandPosition);
    std::cout << std::string(windowWidth - 1, ' ');

    // Restore the cursor position
    SetConsoleCursorPosition(hConsole, { (SHORT)visualCursorX, (SHORT)(visualCursorY - scrollY) });
    currentMode = Mode::Normal;
    displayMode();
}

void moveToNextWord() {
    while (cursorX < lines[cursorY].size() && std::isspace(lines[cursorY][cursorX])) {
        moveCursor(1, 0);
    }
    while (cursorX < lines[cursorY].size() && !std::isspace(lines[cursorY][cursorX])) {
        moveCursor(1, 0);
    }
}

void moveToEndOfWord() {
    while (cursorX < lines[cursorY].size() && !std::isspace(lines[cursorY][cursorX])) {
        moveCursor(1, 0);
    }
}

void moveToPreviousWord() {
    while (cursorX > 0 && std::isspace(lines[cursorY][cursorX - 1])) {
        moveCursor(-1, 0);
    }
    while (cursorX > 0 && !std::isspace(lines[cursorY][cursorX - 1])) {
        moveCursor(-1, 0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: textEditor <filename>\n";
        return 1;
    }

    filename = argv[1];
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
        switch (currentMode) {
        case Mode::Normal:
            if (ch == 'i') { // Enter insert mode
                currentMode = Mode::Insert;
                displayMode();
            }
            else if (ch == 'a') { // Move one character to the right and enter insert mode
                moveCursor(1, 0);
                currentMode = Mode::Insert;
                displayMode();
            }
            else if (ch == 'I') { // Move to the beginning of the line
                cursorX = 0;
                visualCursorX = 0;
                currentMode = Mode::Insert;
                displayMode();
            }
            else if (ch == 'A') { // Move to the end of the line line
                cursorX = lines[cursorY].size();
				visualCursorX = cursorX;
                currentMode = Mode::Insert;
                displayMode();
            }
            else if (ch == ':') {
                currentMode = Mode::Command;
                displayMode();
                enterCommandMode();
            }
            else if (ch == 'k') { // Move up
                if (visualCursorY > 0) {
                    moveCursor(0, -1);
                }
            }
            else if (ch == 'j') { // Move down
                if (visualCursorY < lines.size() - 1) {
                    moveCursor(0, 1);
                }
            }
            else if (ch == 'h') { // Move left
                moveCursor(-1, 0);
            }
            else if (ch == 'l') { // Move right
                moveCursor(1, 0);
            }
            else if (ch == 'q') { // quit
                clearScreen();
                return 0;
            }
            break;
        case Mode::Insert:
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
                clearScreen();
                return 0;
            }
            else if (ch == 27) {
                currentMode = Mode::Normal;
                displayMode();
            }
            else if (ch == '\b' || ch == 127) { // Backspace or Delete key
                deleteCharacter();
            }
            else if (ch >= 32 && ch <= 126) { // Printable ASCII characters
                insertCharacter(static_cast<char>(ch));
            }
            break;
        case Mode::Command:
            // Implement command mode functionality here
            break;
        }
    }
    return 0;
}