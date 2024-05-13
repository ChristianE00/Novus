# Novus - A Terminal-Based Text Editor for Windows 

Novus is a simple, terminal-based text editor for Windows. It's designed to be lightweight and easy to use, making it perfect for quick edits to text files and simple programming tasks. Novus is written in C++ and uses the Windows API for console manipulation.

## How to Use

To open a file with Novus, navigate to the directory containing the **novus.exe** executable in your terminal, then run the following command:

```powershell
./novus ./filename
```
Replace filename with the name of the file you want to edit.

Once you've opened a file, you can switch between command mode and insert mode.

In command mode, you can use the following controls:
•	i: Enter insert mode
•	x: Delete the character under the cursor
•	l: Move the cursor right
•	h: Move the cursor left
•	j: Move the cursor down
•	k: Move the cursor up
•	q: Quit the editor


## How to Build
To build Novus, you'll need a C++ compiler that supports C++11 or later, and the Windows SDK for the Windows API functions.

If you're using Visual Studio, you can open the solution file and build the project from there.

If you're using a different IDE or a command-line compiler, the exact build steps might vary. In 
general, you'll need to compile the novus.cpp file and link against the necessary libraries.

Here's an example of how you might compile Novus with the g++ compiler:
```powershell
g++ -std=c++11 -o novus novus.cpp -lwindows
```

This command compiles the novus.cpp file with C++11 support, links against the Windows library, and outputs an executable named novus.

***Please note that this is a basic text editor and does not support advanced features like syntax highlighting or auto-completion. It's intended for simple text editing tasks in a terminal environment.***



