HIL-Snake: Hardware-in-the-Loop Arcade Engine
--------------------------------------------------------
This project is a custom implementation of the classic game "Snake," built using a Hardware-in-the-Loop (HIL) architecture. 
The physical hardware (Arduino) acts as the arcade controller and sensory feedback system, while the PC handles the multithreaded game engine and rendering.

**Features**

Live Speed Control: Turn the physical potentiometer to dynamically change the game's speed/framerate while playing.

Sensory Feedback: The RGB LED shifts colors based on game events (Blue = Default, Green = Eat, Red/Yellow = Crash).

Arcade Audio: Multi-tone buzzer melodies for movement, eating, and a "Game Over" sequence.

Seamless Restart: Physical arcade button interrupts the game over screen to instantly reset the board.

Flicker-Free Rendering: Custom C++ Windows API implementation overwrites console coordinates instead of wiping the screen, ensuring smooth 0-latency animations.




**Hardware Setup**

Microcontroller: Arduino Uno

Display: PC Command Line (C++ Engine)

components: 
Joystick, potentiometer, buzzer, RGB LED, button, wires and resistors accordingly.




**Software Architecture**

The system consists of two highly decoupled environments communicating over UART (115200 Baud).

1. Arduino (Hardware Controller)
Acts as the physical interface. It utilizes a non-blocking millis() timer to guarantee 100Hz (10ms) deterministic data sampling without freezing the audio outputs.

2. C++ Game Engine (PC)
Built from scratch utilizing <thread> and <mutex> for safe concurrent execution:

io_thread: Dedicated purely to reading/writing serial data, ensuring I/O blocking never stalls the game.

game_thread: Manages collision logic, grid rendering, and active-listening delays.

Communication Protocol
Uplink (Arduino ➔ PC): Comma-separated string terminated by a newline character.

Format: JoystickX,JoystickY,Potentiometer,ButtonState\n

Example: 512,512,120,0\n

Downlink (PC ➔ Arduino): A single 8-bit unsigned integer (uint8_t) representing a specific feedback event.

1: Move

2: Eat Apple

3: Wall Crash

4: Self Crash

Core Concepts Demonstrated
Hardware-in-the-Loop (HIL) / Software-in-the-Loop (SIL) design

Multithreading & Safe Synchronization (Mutex Locks)

Time Synchronization & Non-blocking Delays

Data Parsing & UART Serial Communication

How to Run
Wire the components to the Arduino according to the pinout above.

Upload the arduino_controller.ino code to your Arduino Uno via the Arduino IDE.

Important: Completely close the Arduino IDE Serial Monitor.

Open the C++ main.cpp file and update the CreateFileA function with your specific Arduino COM port (e.g., "\\\\.\\COM3").

Compile and run the C++ executable.
