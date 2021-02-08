#include <windows.h>
void cls()
{

    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD coordScreen = { 0, 0 };    // home for the cursor
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    // Get the number of character cells in the current buffer.

    if( !GetConsoleScreenBufferInfo( hConsole, &csbi ))
        return;

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire screen with blanks.

    if( !FillConsoleOutputCharacter( hConsole,        // Handle to console screen buffer
                                    (TCHAR) ' ',     // Character to write to the buffer
                                    dwConSize,       // Number of cells to write
                                    coordScreen,     // Coordinates of first cell
                                    &cCharsWritten ))// Receive number of characters written
        return;

    // Get the current text attribute.

    if( !GetConsoleScreenBufferInfo( hConsole, &csbi ))
        return;

    // Set the buffer's attributes accordingly.

    if( !FillConsoleOutputAttribute( hConsole,         // Handle to console screen buffer
                                    csbi.wAttributes, // Character attributes to use
                                    dwConSize,        // Number of cells to set attribute
                                    coordScreen,      // Coordinates of first cell
                                    &cCharsWritten )) // Receive number of characters written
        return;

   // Put the cursor at its home coordinates.

    SetConsoleCursorPosition( hConsole, coordScreen );

    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hConsole, &info);
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &info);
}

void setChar(short x, short y, const char* c, WORD color=0x0F) {
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD coordScreen = { x, y };    // home for the cursor
    DWORD written;
    SetConsoleCursorPosition( hConsole, coordScreen );
    SetConsoleTextAttribute(hConsole, color);
    WriteConsole(hConsole, c, strlen(c), &written, NULL);
    SetConsoleTextAttribute(hConsole, 0);
}

