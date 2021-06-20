#include <windows.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <thread>

#define RANDOM
// RANDOM will start with a randomly genereted first generation
// GLIDER will start will a screen full of gliders
#define FHD
// FHD will force the screen to 1080p, windows scaling fucks up the automatic detection
// NATIVE will allow adapting to the resolution of the display
#define NODELAY
// PATIENT will wait before rendering each genereation
// NODELAY will disable the sleep between renders
#define EFFICENT
// EFFICENT will run until the activity drops low enogh to be boring
// BRUTAL will disable activity detection, running to the bitter end (and probably never getting there)
#define MULTIRENDER
// LINEAR will render the scren as a big chunk
// MULTIRENDER will split rendering into a few columns
// TEXTPRINT will print the grid in text instead of pixels (recommended to reduce render rez)
#define DOOMED
// DOOMED will end the simulation after one iteration ends
// UNCERTAIN will give the user a choise
// RESURGENT will endlessly reset the simulation without confirmation

using namespace std;
#ifdef RANDOM
constexpr auto INITIAL_LIFE_RATIO = 100; // this is an INVERSE (aka 1 is EVERY pixel)
#endif // RANDOM
constexpr auto ALIVE = true;
constexpr auto DEAD = false;

//Set colors
const COLORREF COLOR_ALIVE = RGB(255, 255, 255);
const COLORREF COLOR_DEAD = RGB(0, 0, 0);
const COLORREF COLOR_RED = RGB(255, 0, 0);


// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

void ShowConsoleCursor(bool showFlag)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag; // set the cursor visibility
    SetConsoleCursorInfo(out, &cursorInfo);
}

#ifdef MULTIRENDER
void renderThread(bool** current_state, int vertical, int begin, int end, bool** old_state, bool initial) {
    //Get a console handle
    HWND myconsole = GetConsoleWindow();
    //Get a handle to device context
    HDC mydc = GetDC(myconsole);

    for (int cellX = begin; cellX < end; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (initial || old_state[cellX][cellY] != current_state[cellX][cellY]) { // will not update uncahnged cells
                if (current_state[cellX][cellY] == ALIVE)
                {
                    SetPixel(mydc, cellX, cellY, COLOR_ALIVE);
                }
                else {
                    SetPixel(mydc, cellX, cellY, COLOR_DEAD);
                }
            }
        }
    }

    ReleaseDC(myconsole, mydc);
}
#endif // MULTIRENDER


void updateScreen(bool** current_state, int horizontal, int vertical, bool** old_state = NULL, bool initial = true) {
#ifdef MULTIRENDER
    unsigned int numOfThreads = thread::hardware_concurrency();
    thread* threads = new thread[numOfThreads];
    const unsigned int renderZoneSize = horizontal / numOfThreads;
    unsigned int firstOffset = horizontal - renderZoneSize * numOfThreads;
    int begin = 0;
    int end = firstOffset + renderZoneSize; // the first zone takes the rounding error

    threads[0] = thread(renderThread, current_state, vertical, begin, end, old_state, initial);

    for (int created = 1; created < numOfThreads; created++) // !IMPORTANT! set to 1 to skip the first thread creation
    {
        begin = end;
        end += renderZoneSize;
        threads[created] = thread(renderThread, current_state, vertical, begin, end, old_state, initial);
    }

    for (int joined = 0; joined < numOfThreads; joined++) // !IMPORTANT! set to 1 to skip the first thread creation
    {
        threads[joined].join();
    }
#endif // MULTIRENDER

#ifdef LINEAR
    //Get a console handle
    HWND myconsole = GetConsoleWindow();
    //Get a handle to device context
    HDC mydc = GetDC(myconsole);

    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (initial || old_state[cellX][cellY] != current_state[cellX][cellY]) { // will not update uncahnged cells
                if (current_state[cellX][cellY] == ALIVE)
                {
                    SetPixel(mydc, cellX, cellY, COLOR_ALIVE);
                }
                else {
                    SetPixel(mydc, cellX, cellY, COLOR_DEAD);
                }
            }
        }
    }
#endif // LINEAR

#ifdef TEXTPRINT
    for (int cellx = 0; cellx < horizontal; cellx++)
    {
        for (int celly = 0; celly < vertical; celly++)
        {
            if (current_state[celly][cellx] == ALIVE)
            {
                if ((cellx == horizontal - 1 || cellx == 0) || (celly == vertical - 1 || celly == 0))
                {
                    cout << 'x';
                }
                else {
                    cout << 'x';
                }
            }
            else {
                cout << '-';
            }
        }
        cout << endl;
    }
    cout << endl;
#endif // TEXTPRINT

#ifdef PATIENT
    Sleep(1500);
#endif // PATIENT
}

bool cellStatus(bool** old_state, int cellX, int cellY, int horizontal, int vertical, bool looping = false) {
    int neighbors = 0;
    bool status;

    if (!looping)
    {
        // counting neighbors with bound limits
        if (cellX > 0 && old_state[cellX - 1][cellY] == 1)
            neighbors++;
        if (cellY > 0 && old_state[cellX][cellY - 1] == 1)
            neighbors++;
        if (cellX > 0 && cellY > 0
            && old_state[cellX - 1][cellY - 1] == 1)
            neighbors++;
        if (cellX < horizontal - 1 && old_state[cellX + 1][cellY] == 1)
            neighbors++;
        if (cellY < vertical - 1 && old_state[cellX][cellY + 1] == 1)
            neighbors++;
        if (cellX < horizontal - 1 && cellY < vertical - 1
            && old_state[cellX + 1][cellY + 1] == 1)
            neighbors++;
        if (cellX < horizontal - 1 && cellY > 0
            && old_state[cellX + 1][cellY - 1] == 1)
            neighbors++;
        if (cellX > 0 && cellY < vertical - 1
            && old_state[cellX - 1][cellY + 1] == 1)
            neighbors++;
    }
    else {
        // counting neighbors, looping edges
        if ((cellX > 0 && old_state[cellX - 1][cellY] == ALIVE) || // left
            (cellX == 0 && old_state[horizontal - 1][cellY] == ALIVE))
            neighbors++;
        if ((cellY > 0 && old_state[cellX][cellY - 1] == ALIVE) || // up
            (cellY == 0 && old_state[cellX][vertical - 1] == ALIVE))
            neighbors++;
        if ((cellX < horizontal - 1 && old_state[cellX + 1][cellY] == ALIVE) ||  // right
            (cellX == horizontal - 1 && old_state[0][cellY] == ALIVE))
            neighbors++;
        if ((cellY < vertical - 1 && old_state[cellX][cellY + 1] == ALIVE) || // down
            (cellY == vertical - 1 && old_state[cellX][0] == ALIVE))
            neighbors++;
        if ((cellX > 0 && cellY > 0
            && old_state[cellX - 1][cellY - 1] == ALIVE) || // top left
            (cellX == 0 && cellY > 0 // x out of bounds
                && old_state[horizontal - 1][cellY - 1] == ALIVE) ||
            (cellX > 0 && cellY == 0 // y out of bounds
                && old_state[cellX - 1][vertical - 1] == ALIVE) ||
            (cellX == 0 && cellY == 0 // both out of bounds
                && old_state[horizontal - 1][vertical - 1] == ALIVE))
            neighbors++;
        if ((cellX < horizontal - 1 && cellY < vertical - 1 // bottom right
            && old_state[cellX + 1][cellY + 1] == ALIVE) ||
            (cellX == horizontal - 1 && cellY < vertical - 1 // x out of bounds
                && old_state[0][cellY + 1] == ALIVE) ||
            (cellX < horizontal - 1 && cellY == vertical - 1 // y out of bounds
                && old_state[cellX + 1][0] == ALIVE) ||
            (cellX == horizontal - 1 && cellY == vertical - 1 // both out of bounds
                && old_state[0][0] == ALIVE))
            neighbors++;
        if ((cellX < horizontal - 1 && cellY > 0 // top right
            && old_state[cellX + 1][cellY - 1] == 1) ||
            (cellX == horizontal - 1 && cellY > 0 // x out of bounds
                && old_state[0][cellY - 1] == ALIVE) ||
            (cellX < horizontal - 1 && cellY == 0 // y out of bounds
                && old_state[cellX + 1][vertical - 1] == ALIVE) ||
            (cellX == horizontal - 1 && cellY == 0 // both out of bounds
                && old_state[0][vertical - 1] == ALIVE))
            neighbors++;
        if ((cellX > 0 && cellY < vertical - 1 // bottom left
            && old_state[cellX - 1][cellY + 1] == 1) ||
            (cellX == 0 && cellY < vertical - 1 // x out of bounds
                && old_state[horizontal - 1][cellY + 1] == ALIVE) ||
            (cellX > 0 && cellY == vertical - 1 // y out of bounds 
                && old_state[cellX - 1][0] == ALIVE) ||
            (cellX == 0 && cellY == vertical - 1 // both out of bounds 
                && old_state[horizontal - 1][0] == ALIVE))
            neighbors++;
    }



    if (old_state[cellX][cellY] == ALIVE && neighbors <= 1) { // loneliness
        status = DEAD;
    }
    else if (old_state[cellX][cellY] == ALIVE && (neighbors == 2 || neighbors == 3)) { // survival
        status = ALIVE;
    }
    else if (old_state[cellX][cellY] == ALIVE && neighbors >= 4) { // overpopulation
        status = DEAD;
    }
    else if (old_state[cellX][cellY] == DEAD && neighbors == 3) { // multiplication
        status = ALIVE;
    }
    else { // just in case, assume DEAD
        status = DEAD;
    }

    return status;
}


void updateStateMatrix(bool** old_state, bool** current_state, int horizontal, int vertical, bool looping = false) {
    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            current_state[cellX][cellY] = cellStatus(old_state, cellX, cellY, horizontal, vertical, looping);
        }
    }
}

// check if life has exausted itself in the system
bool extinction(bool** state, int horizontal, int vertical) {
    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (state[cellX][cellY] == ALIVE)
            {
                return false;
            }
        }
    }

    return true;
}

// checkes if the system reached a stable, static state
bool stasis(bool** old_state, bool** current_state, int horizontal, int vertical) {
    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (old_state[cellX][cellY] != current_state[cellX][cellY])
            {
                return false;
            }
        }
    }

    return true;
}

// checkes if the activity level in the system has dropped too low
bool low_Activity(bool** old_state, bool** current_state, int horizontal, int vertical, float threshold = 1) {
    const long total_cells = vertical * horizontal;
    long cells_changed = 0;
    bool result = false;

    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (old_state[cellX][cellY] != current_state[cellX][cellY])
            {
                cells_changed++;
            }
        }
    }

    if ((float)cells_changed / total_cells * 100 < threshold)
    {
        result = true;
    }

    return result;
}

// creates a single block matrix allocation and splits it into arrays
template <typename T>
T** createMatrix(int horizontal, int  vertical) {
    T** matrix = new T * [horizontal];
    matrix[0] = new T[horizontal * vertical];
    for (int cellX = 1; cellX < horizontal; cellX++) {
        matrix[cellX] = matrix[0] + cellX * vertical;
    }

    return matrix;
}

// deletes single block allocated matrixes 
template <typename T>
void deleteMatrix(T** matrix) {
    delete[] matrix[0];
    delete[] matrix;
}

// checks all stop conditions for the current run
bool endOfCycle(bool** old_state, bool** current_state, int horizontal, int vertical) {
    bool result = false;

#ifdef EFFICENT
    if (low_Activity(old_state, current_state, horizontal, vertical)) {
        cout << "low activity" << endl;
        result = true;
    }
#endif // EFFICENT

#ifdef BRUTAL
    if (extinction(current_state, horizontal, vertical)) {
        cout << "life has beed wiped out" << endl;
        result = true;
    }
    else if (stasis(old_state, current_state, horizontal, vertical)) { // this is effectivly impossible
        cout << "a static state has beed achived" << endl;
        result = true;
    }
#endif // BRUTAL

    return result;
}

// fil the first generation according to selecte ruels
void initialize(bool** current_state, int horizontal, int vertical) {
#ifdef RANDOM
    // randomize the first generation
    srand(time(0));
    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            if (rand() % INITIAL_LIFE_RATIO == 0)
            {
                current_state[cellX][cellY] = true;
            }
            else
            {
                current_state[cellX][cellY] = false;
            }
        }
    }
#endif // RANDOM

#ifdef GLIDER
    //initialize an empty screen
    for (int cellX = 0; cellX < horizontal; cellX++)
    {
        for (int cellY = 0; cellY < vertical; cellY++)
        {
            current_state[cellX][cellY] = false;
        }
    }

    int offsetX = 0;
    int offsetY = 0;
    int sizeX = 40;
    int sizeY = 40;
    int state = 0;

    for (offsetX = 0; offsetX + sizeX <= horizontal; offsetX += sizeX)
    {
        for (offsetY = 0; offsetY + sizeY <= vertical; offsetY += sizeY)
        {

            switch (state)
            {
            case (0): {
                current_state[offsetX + 0][offsetY + 2] = true;
                current_state[offsetX + 1][offsetY + 0] = true;
                current_state[offsetX + 1][offsetY + 2] = true;
                current_state[offsetX + 2][offsetY + 2] = true;
                current_state[offsetX + 2][offsetY + 1] = true;
                break;
            }
            case (1): {
                current_state[offsetX + 0][offsetY + 0] = true;
                current_state[offsetX + 1][offsetY + 1] = true;
                current_state[offsetX + 1][offsetY + 2] = true;
                current_state[offsetX + 2][offsetY + 0] = true;
                current_state[offsetX + 2][offsetY + 1] = true;
                break;
            }
            case (2): {
                current_state[offsetX + 0][offsetY + 1] = true;
                current_state[offsetX + 1][offsetY + 2] = true;
                current_state[offsetX + 2][offsetY + 0] = true;
                current_state[offsetX + 2][offsetY + 1] = true;
                current_state[offsetX + 2][offsetY + 2] = true;
                break;
            }
            case (3): {
                current_state[offsetX + 0][offsetY + 0] = true;
                current_state[offsetX + 0][offsetY + 2] = true;
                current_state[offsetX + 1][offsetY + 1] = true;
                current_state[offsetX + 1][offsetY + 2] = true;
                current_state[offsetX + 2][offsetY + 1] = true;
                break;
            }
            default: {
                break;
            }
            }
            state = (state + 1) % 4;
        }
    }
#endif // GLIDER

}

// handle responce options
bool handleResponse(char response) {
    bool result = false;
    if (response == 'y' or response == 'Y')
    {
        result = true;
    }

    return result;
}




int main()
{
    // sets makes the window fullscreen
    SetConsoleDisplayMode(GetStdHandle(STD_OUTPUT_HANDLE), CONSOLE_FULLSCREEN_MODE, 0);
    ShowScrollBar(GetConsoleWindow(), SB_VERT, 0);
    ShowConsoleCursor(false);
    Sleep(1500);


    // get screen rezsolution
    int horizontal = 0;
    int vertical = 0;

#ifdef NATIVE
    GetDesktopResolution(horizontal, vertical);
#endif // NATIVE

#ifdef FHD
    horizontal = 1920;
    vertical = 1080;
#endif // FHD

#ifdef TEXTPRINT
    horizontal /= 8; // adapt to fit text on screen
    vertical /= 8;
#endif // TEXTPRINT


    bool** old_state = createMatrix<bool>(horizontal, vertical);
    bool** current_state = createMatrix<bool>(horizontal, vertical);
    char response = 'y';

#ifdef RESURGENT
    while (true)
#endif // RESURGENT
#ifdef UNCERTAIN
    while (handleResponse(response) != false)
#endif // UNCERTAIN

#ifndef DOOMED
    {
#endif // DOOMED

        // fill the sceeen according to settings
        initialize(current_state, horizontal, vertical);

        // initial render
        updateScreen(current_state, horizontal, vertical);

        while (!endOfCycle(old_state, current_state, horizontal, vertical))
        {
            swap(old_state, current_state); // 'save' the old state without copying
            updateStateMatrix(old_state, current_state, horizontal, vertical, true);

            updateScreen(current_state, horizontal, vertical, old_state, false);
        }

#ifdef UNCERTAIN
        cout << "restart? (y/n)" << endl;
        cin >> response;
#endif // UNCERTAIN

#ifndef DOOMED
    }
#endif // DOOMED


    deleteMatrix<bool>(old_state);
    deleteMatrix<bool>(current_state);
    return 0;
}
