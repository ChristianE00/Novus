// novus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>
#include <conio.h>
#include <cstdlib>

// Global variables

// Get the console handle
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Cursor position
int cursorLine = 0;
int cursorPos = 0;

// Lines
std::vector<std::string> lines;
std::string line;

// Function to set the cursor position
void setCursorPos(int line, int pos) {
    // Set the cursor position
    COORD cursorPo;
    cursorPo.X = pos;
    cursorPo.Y = line;
    SetConsoleCursorPosition(hConsole, cursorPo);
}

// Clear the screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Redraw the screen
void redrawScreen() {
    // Clear the screen
    clearScreen();

    // Redisplay the text
    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }

    // Set the cursor position
    setCursorPos(cursorLine, cursorPos);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: textEditor <filename>" << std::endl;
        return 1;
    }

    // Mode: true for insert mode, false for command mode
    bool insertMode = false;

    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    // Display the text
    redrawScreen();

    // Simple editing loop
    while (true) {
        if (insertMode) {
            // In insert mode, add typed character to the current line
            char c = _getch();
            if (c == 27) { // ESC key
                insertMode = false;
            }
            else if (c == 8) { // Backspace
                if (cursorPos > 0) {
                    lines[cursorLine].erase(cursorPos - 1, 1);
                    cursorPos--;
                    redrawScreen();
                }
            }
            else if (c == 13) { // Enter key
                std::string newLine = lines[cursorLine].substr(cursorPos);
                lines[cursorLine].erase(cursorPos);
                cursorLine++;
                lines.insert(lines.begin() + cursorLine, newLine);
                cursorPos = 0;
                redrawScreen();
            }
            else {
                lines[cursorLine].insert(cursorPos, 1, c);
                cursorPos++;
                redrawScreen();
            }
        }
        else {
            // In command mode, process the command
            char command = _getch();
            switch (command) {
            case 'i':
                insertMode = true;
                break;
            case 'x':
                if (cursorPos < lines[cursorLine].size()) {
                    lines[cursorLine].erase(cursorPos, 1);
                    redrawScreen();
                }
                break;
            case 'l':
                if (cursorPos < lines[cursorLine].size()) {
                    cursorPos++;
                    setCursorPos(cursorLine, cursorPos);
                }
                break;
            case 'h':
                if (cursorPos > 0) {
                    cursorPos--;
                    setCursorPos(cursorLine, cursorPos);
                }
                break;
            case 'j':
                if (cursorLine < lines.size() - 1) {
                    cursorLine++;
                    cursorPos = cursorPos < static_cast<int>(lines[cursorLine].size()) ? cursorPos : static_cast<int>(lines[cursorLine].size());
                    setCursorPos(cursorLine, cursorPos);
                }
                break;
            case 'k':
                if (cursorLine > 0) {
                    cursorLine--;
                    cursorPos = cursorPos < static_cast<int>(lines[cursorLine].size()) ? cursorPos : static_cast<int>(lines[cursorLine].size());
                    setCursorPos(cursorLine, cursorPos);
                }
                break;
            case 'q':
                goto end_of_loop;
            }
        }
    }
end_of_loop:

    // Save the file
    /*
    std::ofstream outFile(filename);
    for (const auto& line : lines) {
        outFile << line << std::endl;
    }
    */
	clearScreen();
    return 0;
}