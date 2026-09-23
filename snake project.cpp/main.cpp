#define _CRT_SECURE_NO_WARNINGS 
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <windows.h>
#include <string>

//handeling USB data for Windows:
HANDLE hSerial;
//mutex for synchronizing between the two threads:
std::mutex mtx;
//initializing input data:
int jsx = 512, jsy = 512, pot = 512, btn = 0;
//switch for controling all threads:
bool isGameRunning = true;
//commands for hardware: 0,1,2,3,4:
uint8_t currentCommand = 0;

//defining coordinates:
struct point {
    int x;
    int y;
    bool operator ==(const point& other) const {
        return x == other.x && y == other.y;
    }
};

//handeling the console screen:
void setCursorPosition(int x, int y) {
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout.flush();
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hOut, coord);
}



//THREAD 1: SERIAL COMMUNICATION
void io_thread() {
    std::string incomingData = "";
    while (isGameRunning) {

        //DOWNLINK - send commands to Arduino:
        //for commands: 0,1,2,3,4:
        uint8_t cmdToSend = 0;
        //scope for safely updating the command for sending, while locking the mutex:   
        {
            std::lock_guard<std::mutex> lock(mtx);
            cmdToSend = currentCommand;
            currentCommand = 0;
        }

        if (cmdToSend > 0) {
            //push one byte to serial:
            DWORD bytesWritten;
            WriteFile(hSerial, &cmdToSend, 1, &bytesWritten, NULL);
        }

        //UPLINK - read input data from arduino:
        char tempBuf[32];
        DWORD bytesRead;
        //read in to buffer:
        if (ReadFile(hSerial, tempBuf, sizeof(tempBuf) - 1, &bytesRead, NULL) && bytesRead > 0) {
            tempBuf[bytesRead] = '\0';
            incomingData += tempBuf;

            size_t pos;
            //extract whole "sentence" in to proccesable data as 4 vriables:
            while ((pos = incomingData.find('\n')) != std::string::npos) {
                std::string line = incomingData.substr(0, pos);
                incomingData.erase(0, pos + 1);

                int newX, newY, newPot, newBtn;
                if (sscanf(line.c_str(), "%d,%d,%d,%d", &newX, &newY, &newPot, &newBtn) == 4) {
                    std::lock_guard<std::mutex> lock(mtx);
                    jsx = newX;
                    jsy = newY;
                    pot = newPot;
                    btn = newBtn; 
                }
            }
        }
        //wait for 10 ms:
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//THREAD 2 - GAME LOGIC
void game_thread() {
    //initialize console:
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    //power switch:
    while (isGameRunning) {

        //Setup a fresh game and clear the screan
        std::vector<point> snake = { {3,3},{4,3},{5,3} };
        point apple = { 1, 1 };
        int dx = -1, dy = 0;
        bool isGameOver = false;
        system("cls");

        //Active Gameplay
        while (!isGameOver && isGameRunning) {
            //head's next position:
            point newHead = { snake[0].x + dx, snake[0].y + dy };

            //Wall collision:
            if (newHead.x < 0 || newHead.y < 0 || newHead.x > 10 || newHead.y > 10) {
                setCursorPosition(0, 10);
                std::cout << "\n\n GAME OVER! You hit a wall!       \n";
                std::cout << "\n PRESS BUTTON TO RESTART!         \n\n";
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    currentCommand = 3;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                isGameOver = true;
                break;
            }

            //Self collision:
            for (const auto& segment : snake) {
                if (newHead == segment) {
                    setCursorPosition(0, 10);
                    std::cout << "\n\n\n\n\n\n GAME OVER! You hit yourself!     \n";
                    std::cout << " PRESS BUTTON TO RESTART!         \n\n";
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        currentCommand = 4;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    isGameOver = true;
                    break;
                }
            }

            //Move and eat:
            snake.insert(snake.begin(), newHead);
            if (newHead == apple) {
                apple.x = rand() % 8;
                apple.y = rand() % 8;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    currentCommand = 2;
                }
            }
            else {
                snake.pop_back();
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    currentCommand = 1;
                }
            }

            //draw console grid:
            setCursorPosition(0, 0);
            std::cout << "=== HIL SNAKE ===\n";

            for (int x = 0; x < 13; x++) std::cout << ". ";
            std::cout << "\n";

            for (int y = 0; y < 11; y++) {
                std::cout << ". ";
                for (int x = 0; x < 11; x++) {
                    point p = { x, y };
                    if (p == apple) {
                        std::cout << "@ ";
                    }
                    else {
                        bool isSnake = false;
                        for (const auto& s : snake) {
                            if (p == s) {
                                isSnake = true;
                                break;
                            }
                        }
                        if (isSnake) std::cout << "0 ";
                        else std::cout << "  ";
                    }
                }
                std::cout << ". \n";
            }

            for (int x = 0; x < 13; x++) std::cout << ". ";
            std::cout << "\n\n";

            //scope for setting the game speed:
            int currentPot;
            {
                std::lock_guard<std::mutex> lock(mtx);
                currentPot = pot;
            }

            int totalNaps = 5 + (currentPot * 35 / 1023);
            std::cout << "Game speed: " << currentPot << "   \n";

            for (int i = 0; i < totalNaps; i++) {
                int currx, curry;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    currx = jsx;
                    curry = jsy;
                }

                if (currx < 400 && dx != 1) { dx = -1; dy = 0; }
                if (currx > 600 && dx != -1) { dx = 1; dy = 0; }
                if (curry < 400 && dy != 1) { dx = 0; dy = -1; }
                if (curry > 600 && dy != -1) { dx = 0; dy = 1; }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        //wait for restart button:
        while (isGameOver && isGameRunning) {
            int currentBtn;
            {
                std::lock_guard<std::mutex> lock(mtx);
                currentBtn = btn;
            }

            if (currentBtn == 1) {
                //when restart is pressed, leave the loop and enter the game:
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}




int main() {
    std::cout << "Connecting to Arduino Controller...\n";
    hSerial = CreateFileA("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open COM port. Is Arduino plugged in & IDE Serial Monitor closed?\n";
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams)) {
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        SetCommState(hSerial, &dcbSerialParams);
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutConstant = 50;
    SetCommTimeouts(hSerial, &timeouts);

    std::thread io(io_thread);
    std::thread game(game_thread);

    io.join();
    game.join();
    CloseHandle(hSerial);
    return 0;
}