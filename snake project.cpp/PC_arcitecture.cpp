#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <windows.h>

HANDLE hSerial;

struct point {
	int x;
	int y; 
	//describing two identical points:
	bool operator ==(const point& other) const {
		return x == other.x && y == other.y;
	}
};

//for shared memory:
std::mutex mtx;

uint8_t displayBuffer[8] = { 0,0,0,0,0,0,0,0 };
int jsx = 512, jsy = 512, pot = 512;
bool isGameRunning = true;

;//thread 1: serial comunication
void io_thread(){
	while (isGameRunning) {
		uint8_t local_buffer[8];
		{
			//critical section:
			std::lock_guard<std::mutex> lock(mtx);
			for (int i = 0; i < 8; i++) {
				local_buffer[i] = displayBuffer[i];
			}
		}

		//PC to Arduino:
		DWORD bytesWritten;

		//send sync bite first:
		uint8_t syncByte = 0xFF;
		WriteFile(hSerial, &syncByte, 1, &bytesWritten, NULL);

		//send 8 byte graphics frame:
		WriteFile(hSerial, local_buffer, 8, &bytesWritten, NULL);
		
		{
		//wait 20 ms:
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}




//thread 2: game logics
void game_thread() {
	//initialize snake and apple coordinates:
	std::vector<point>snake = {{3,3},{4,3},{5,3}};
	point apple = {1, 1};
	int dx = -1, dy = 0; //move left

	while (isGameRunning) {
		//read current sensor values:
		int currx, curry, currSpeed;
		{
			std::lock_guard<std::mutex> lock(mtx);
			currx = jsx;
			curry = jsy;
			currSpeed = pot;
		}
		
		//uptade vector:
		if (currx < 400 && dx != 1) { dx = -1; dy = 0;}//left
		if (currx > 600 && dx != -1) { dx = 1; dy = 0;}//right
		if (curry < 400 && dy != 1) { dx = 0; dy = -1;}//up
		if (currx > 600 && dy != -1) { dx = 0; dy = 1;}//down

		//calculate new head:
		point newHead = { snake[0].x + dx, snake[0].y + dy };

		//wall colision:
		if (newHead.x<0 || newHead.y<0 || newHead.x>7 || newHead.y>7) {
			std::cout << "Game Over! you hit a wall!\n";
			isGameRunning = false;
			break;
		}

		//self colision:
		for (const auto& segment : snake) {
			if (newHead == segment) {
				std::cout << "Game Over! you hit yourself!\n";
				isGameRunning = false;
				break;
			}
		}

		//move and eat:
		//insert new head:
		snake.insert(snake.begin(), newHead);
		//if the apple was eaten: do'nt pop the tail.
		if (newHead == apple) {
			//generate new random apple:
			apple.x = rand() % 8;
			apple.y = rand() % 8;
		}
		else {
			//pop tail to move:
			snake.pop_back();
		}


		{
			std::lock_guard<std::mutex> lock(mtx);
			//clean the buffer:
			for (int i = 0; i < 8;i++) {
				displayBuffer[i] = 0;
			}
			//create apple:
			displayBuffer[apple.y] |= (1 << apple.x);

			//draw snake:
			for (const auto& p : snake) {
				displayBuffer[p.y] |= (1 << p.x);
			}
		}
		//delay:
		int gameSpeed = 50 + (1023 - currSpeed)*450/1023;
		std::this_thread::sleep_for(std::chrono::milliseconds(gameSpeed));
	}
}






int main() {
	std::cout << "starting HIL snake engine...\n";
	//open serial port:
	hSerial = CreateFileA("\\\\.\\COM3",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hSerial == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Unable to open COM port. Check if Arduino is plugged in\n";
		return 1;
	}

	//Configure Device Control Block (DCB)
	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (GetCommState(hSerial, &dcbSerialParams)) {
		dcbSerialParams.BaudRate = CBR_115200; // Match Arduino baud rate
		dcbSerialParams.ByteSize = 8;
		dcbSerialParams.StopBits = ONESTOPBIT;
		dcbSerialParams.Parity = NOPARITY;

		SetCommState(hSerial, &dcbSerialParams);
	}

	//Prevent thread from locking up during reads/writes
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutConstant = 50;
	SetCommTimeouts(hSerial, &timeouts);


	//spawn threads:
	std::thread io(io_thread);
	std::thread game(game_thread);

	//wait for threads to finish execution:
	io.join();
	game.join();

	CloseHandle(hSerial);
	return 0;
}