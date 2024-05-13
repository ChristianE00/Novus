// novus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <Windows.h>
#include <conio.h>
#include <cstdlib>

#define NOMINMAX
// Global variables

// Get the console handle
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Editor config
const int TAB_SIZE = 2;

// Save previous x and y position
int prevX = 0;
int prevY = 0;

int upCount = 0;

// Cursor position
int cursorLine = 0;

int cursorPos = 0;

int windowSize;

// Scroll position
int scrollPos = 0;

// Lines
std::vector<std::string> lines;
std::string line;


// Mode: true for insert mode, false for command mode
bool insertMode = false;

// A global variable to store a user command
std::string boxCommand;

// variable to track the commandLineMode
bool commandLineMode = false;

// A new global variable to track the command input mode
bool commandInputMode = false;




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

// Get the window size
COORD getWindowSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.dwSize;
}

COORD cursorPoo;
// Function to set the cursor position
void setCursorPos(int line, int pos) {
    // Set the cursor position
    cursorPoo.X = pos;
    cursorPoo.Y = line;
    SetConsoleCursorPosition(hConsole, cursorPoo);
}

// Function to display the node
void displayMode() {
    COORD windowSize = getWindowSize();
    setCursorPos(windowSize.Y - 1, windowSize.X - 20);

    if (insertMode) {
        std::cout << "INSERT MODE";
    }
    else if (commandLineMode) {
        std::cout << "COMMAND MODE";
    }
    else {
        std::cout << "NORMAL MODE";
    }
    setCursorPos(cursorLine - scrollPos, cursorPos);
}




void redrawScreen() {
    // Clear the screen
    clearScreen();

    // Get the window size
    COORD windowSize = getWindowSize();

    // Redisplay the text
    for (int i = scrollPos; i < scrollPos + windowSize.Y - 1 && i < lines.size(); i++) {
        std::cout << lines[i] << std::endl;
    }

    // Set the cursor position
    setCursorPos(std::abs(cursorLine - scrollPos), cursorPos);
}

void redrawScreenUp() {
    // Check if there are more lines above to scroll
    for (int i = cursorPos; i < cursorPos + windowSize - 1 && i < lines.size() && i >= 0; i++) {
		std::cout << lines[i] << std::endl;
    }
    setCursorPos(0, cursorPos);

}




int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: textEditor <filename>" << std::endl;
        return 1;
    }
    windowSize = getWindowSize().Y;

    std::ofstream debugFile("debug.txt");

    // Now you can write to debugFile instead of std::cout for debug 

    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    // Reset scroll position and cursor position
    scrollPos = 0;
    cursorLine = 0;
    cursorPos = 0;

    // Display the text
    redrawScreen();
    displayMode(); // Display the mode box

    // Simple editing loop
    while (true) {
        if (insertMode) {
            // In insert mode, add typed character to the current line
            char c = _getch();
            if (c == 27) { // ESC key
                insertMode = false;
                displayMode(); // update the mode box
            }
            else if (c == 8) { // Backspace
                if (cursorPos > 0) {
                    lines[cursorLine + scrollPos].erase(cursorPos - 1, 1);
                    cursorPos--;
                    redrawScreen();
                }
            }
            else if (c == 13) { // Enter key
                std::string newLine = lines[cursorLine + scrollPos].substr(cursorPos);
                lines[cursorLine + scrollPos].erase(cursorPos);
                cursorLine++;
                cursorLine++;
                lines.insert(lines.begin() + cursorLine + scrollPos, newLine);
                cursorPos = 0;
                redrawScreen();
            }
            else if (c == 9) { // Tab key
                lines[cursorLine + scrollPos].insert(cursorPos, TAB_SIZE, ' ');
                cursorPos += TAB_SIZE;
                redrawScreen();
            }
            else {
                lines[cursorLine + scrollPos].insert(cursorPos, 1, c);
                cursorPos++;
                redrawScreen();
            }
        }
        // Command line mode
        else if (commandLineMode) {
            char c = _getch();

            if (c == '\r') { // Enter key
                // Execute the command
                if (boxCommand == "wq") {
                    // Save the file
                    std::ofstream file(filename);
                    for (const auto& line : lines) {
                        file << line << "\n";
                    }
                    file.close();
                    // Exit the Program
                    clearScreen();
                    return 0;
                }

                // Just save the file
                else if (boxCommand == "w") {
                    // Save the file
                    std::ofstream file(filename);
                    for (const auto& line : lines) {
                        file << line << "\n";
                    }
                    file.close();
                    // Switch back to command node
                    commandLineMode = false;

                    displayMode(); // update the mode box
                    redrawScreen();
                }

                // Exit without saving
                else if (boxCommand == "q") {
                    clearScreen();
                    debugFile.close();
                    return 0;
                }
                // Switch back to command node
                commandLineMode = false;
                insertMode = false;

                displayMode(); // update the mode box
                redrawScreen();
            }

            else if (c == 27) { // ESC key
                insertMode = false;
                // Close the command box and reset cursor to its previous position
                setCursorPos(cursorLine - scrollPos, cursorPos);
                commandLineMode = false;
                boxCommand = "";
                redrawScreen();

                displayMode();
            }
            else {
                // Backspace
                if (c == 8) {
                    // Remove the last character from the command
                    if (boxCommand.size() > 0) {
                        boxCommand.pop_back();
                        // Move the cursor back
                        setCursorPos(getWindowSize().Y - 1, 0);
                        // Clear the line
                        std::cout << "          ";
                        // Move the cursor back
                        setCursorPos(getWindowSize().Y - 1, 0);
                        // Display the command
                        std::cout << ":" << boxCommand;
                    }

                }
                else {
                    // Add the types character to the command
                    boxCommand += c;

                    // Display the types character
                    std::cout << c;
                }
            }
        }
        else {
            // In command mode, process the command
            char command = _getch();
            switch (command) {
            case ':': {
                // Enter command mode
                commandLineMode = true;
                insertMode = false;
                displayMode(); // update the mode box
                boxCommand = "";

                // Display the command prompt
                setCursorPos(getWindowSize().Y - 1, 0);
                std::cout << ":";
                break;
            }
            case 'i':
                insertMode = true;
                displayMode(); // update the mode box
                break;
            case 'x':
                if (cursorPos < lines[cursorLine + scrollPos].size()) {
                    lines[cursorLine + scrollPos].erase(cursorPos, 1);
                    redrawScreen();
                }
                break;
            case 'l':
                if (cursorPos < lines[cursorLine + scrollPos].size()) {
                    cursorPos++;
                    redrawScreen();
                    setCursorPos(cursorLine - scrollPos, cursorPos);
                }
                break;
            case 'h':
                if (cursorPos > 0) {
                    cursorPos--;
                    redrawScreen();
                    //setCursorPos(cursorLine - scrollPos, cursorPos);
                }
                break;
            // In the 'j' case:
            case 'j':
                if (cursorLine < lines.size() - 1) {
                    cursorLine++;
                    if (cursorLine >= windowSize - 1) {
                        //cursorLine--;
                        scrollPos++;
                        redrawScreen();
                    }
                    else {
                        setCursorPos(cursorLine - scrollPos, cursorPos);
                    }
                }
                break;
                // cursorLine -> total count
                // 
                // In the 'k' case:
            case 'k':
               // debugFile << "CursorLine: " << cursorLine << " scrollPos: " << scrollPos << std::endl;
                if (cursorLine > 0) {
                    upCount++;
					cursorLine--;
					if (upCount == windowSize - 1) {
						redrawScreenUp();
						upCount = 0;
					}
                }
                else if (scrollPos == 0) {
                    cursorLine--;
                    setCursorPos(cursorLine, cursorPos);
                }
                else {
                    cursorLine--;

                }
                break;
                /*
                if(cursorLine - scrollPos == windowSize - 1) {
                  
                }
                if (cursorLine > 0) {
                    cursorLine--;
                    setCursorPos(cursorLine - scrollPos, cursorPos);
                }
                else if (scrollPos > 0) {
                    redrawScreenUp();
                }
                break;
                */



            case 'q':
                goto end_of_loop;
                
            }
        }
    }
end_of_loop:

    // Save the file
    std::ofstream outFile(filename);
    for (const auto& line : lines) {
        outFile << line << std::endl;
    }
    clearScreen();
    return 0;
}
