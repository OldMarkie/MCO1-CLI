## Group Members

- **GAYAMO, Jasmine Baltazar**  
- **PORCIUNCULA, Lexrey Dancel**  
- **SEGURA, Gabriel Pangilinan**  
- **TASARRA, Marc Lowell Anthony Oliva**  

---
## FCFS Scheduler in OS Emulator

## What It Is
The program is a multithreaded command-line operating system emulator written in C++. It simulates a basic operating system shell and supports process creation and management. This implementation utilizes a First-Come, First-Serve (FCFS) scheduling algorithm, ensuring processes are executed in the order they arrive.

## How It Works
1. Upon startup, the emulator automatically creates 10 processes, with each process submitting 100 print commands. These processes are scheduled using the First-Come, First-Served (FCFS) algorithm, ensuring that the earliest arriving process is always executed first.
2. The emulator simulates a 4-core CPU, with each core managed by a dedicated worker thread. A central scheduler thread is responsible for dispatching processes to available cores one at a time, following their arrival order.
3. Each process logs its output to its own text file, which contains the time of execution and the core on which it ran. This setup emulates realistic scheduling and parallel execution behavior.

## How to Run the Program

1. Open the solution file: `MCO1-CLI.sln` using **Microsoft Visual Studio**.
2. Ensure the build configuration is set to **Debug** and the platform is set to either **x64** or **Win32**, depending on your setup.
3. In Visual Studio, navigate to the top menu and click:  
   **`Debug` > `Start Without Debugging`**  
   (or press `Ctrl + F5`) to build and run the application.

## Program Features

### Entry Class File:
- `main.cpp` - The program starts here and launches the console interface
- `Console.cpp` - Manages user input, executes shell commands, and controls screen logic
- `menuFunc.cpp` - Provides menu and command functionality

## Header Files:
- `Console.h` - Declaration of console user interface and interaction handlers
- `ScreenSession.h` - Defines the process object and print behavior
- `ScreenSessionManager.h` - Manages active and finished sessions
- `cmdArt.h` - Contains ASCII art or command-line display assets
