#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cstdint>

//shared memory:
std::mutex mtx;
uint8_t displayBuffer[8] = {0,0,0,0,0,0,0,0}