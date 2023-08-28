//
// SpoutRecorder.cpp
//
// Encodes Spout input to a video file using FFmpeg by way of a pipe.
// Resolution and speed are improved over using SpoutCam as a source
//
//    Recording
//	      F1     - start recording
//        F2/ESC - stop recording
//        V      - show video folder
//        Q      - stop and quit
//
//    Settings
//        A      - system audio
//        C      - codec mpeg4/x264
//        P      - prompt for file name
//        T      - tompost
//
//    HotKeys (always active)
//        ALT+F1 - start
//        ALT+F2 - stop
//        ALT+Q  - stop and quit
//
// Records system audio together with the video using the DirectShow filter by Roger Pack
// https://github.com/rdp/virtual-audio-capture-grabber-device
//
// Uses the SpoutDX support class.
// After build, SpoutRecorder.exe is copied to the "Binaries" folder.
// Any FFmpeg option can be added by way of a command line and batch file.
// Command line options can be found in "DATA\Scripts\aa-record.bat".
// Edit that file for details.
//
// Reference :
// https://batchloaf.wordpress.com/2017/02/12/a-simple-way-to-read-and-write-audio-and-video-files-in-c-using-ffmpeg-part-2-video/
//
// =========================================================================
//
// Revisions
//		26-03-23 - Initial version 1.000
//		30.03.23 - Add audio record option using virtual audio device
//				   Developed by Roger Pack
//				   https://github.com/rdp/virtual-audio-capture-grabber-device
//				 - Add batch file to record audio only
//		31.03.23 - Add VirtualAudioRegister utility to register the filter
//		01.04.23 - Add ALT+Q hotkey to stop and quit
//				   Allows quit when minimised or when another application is full screen
//				   Version 1.001
//		01.04.23 - Add ALT-T hotkey to toggle topmost
//		02.04.23 - Remove redundant rgba invert and FFmpeg vflip
//				   Add "Q" to quit when window is visible
//				   Show that ALT keys are Hot Keys
//				   Restore from minimized for ALT-T topmost
//		03.04.23 - Release receiver on FFmpeg stop
//				   Add console handler to detect console close
//				   Get active sender when F1 is pressed
//				   Change keys information text to be more clear
//				   Version 1.002
//		10.08.23 - Update to 2.007.011 SpoutDX files
//				   FlashWindow notification when recording
//				   Auto-detect sender if started without one
//				   Handle sender change
//		11.08.23   Use events for keys and mouse
//				   Remove "ALT+T" topmost hotkey, replace with "T" key
//				   Display menu after sender change
//				   Version 1.003
//		14.08.23   Toggle key options and more options
//				   A-system audio / R-rgba/rgb / C-codec / V-show video folder / P-prompt for filename
//				   Add initialization file to save ad restore options
//				   Add ALT+F2 hotkey to stop FFmpeg
//				   Icon from imageres.dll
//				   Change caption to "Recording" if recording to indicate recording status
//				   Add "Sender" caption command to change to active sender
//				   Version 1.004
//		20.08.23   Changes since Release
//				   Move construct ini file path before read
//				   Erase the top line if the sender closes
//				   Stop recording and receiving on pixel format change
//				   -mpeg4 and -x264 command line arguments
//		22.08.23   -hwaccel auto - hardware encode if available
//		24.08.23   SpoutRecord class to manage FFmpeg recording
//				   Remove RGB option - timing tests show no benefit
//				   Version 1.005
//
// =========================================================================
// 
// Copyright(C) 2023 Lynn Jarvis.
// 
// https://spout.zeal.co/
// 
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program. If not, see < http://www.gnu.org/licenses/>.
// 
// ========================================================================
//
#include "SpoutRecord.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include "SpoutDX/SpoutDX.h"


// Global Variables:
spoutRecord recorder;                 // FFmpeg recorder
spoutDX receiver;                     // Receiver object
HANDLE g_hConsole = nullptr;          // Console window handle
unsigned char* pixelBuffer = nullptr; // Receiving pixel buffer
char g_SenderName[256]={};            // Received sender name
unsigned int g_SenderWidth = 0;       // Received sender width
unsigned int g_SenderHeight = 0;      // Received sender height
HWND g_hWnd = NULL;                   // Console window handle
char g_Initfile[MAX_PATH]={ 0 };      // Initialization file
char g_ExePath[MAX_PATH]={ 0 };       // Executable path
std::string g_FFmpegPath;             // FFmpeg path
FLASHWINFO fwi={0};                   // For flashing title bar and taskbar icon
// https://learn.microsoft.com/en-us/windows/console/reading-input-buffer-events
DWORD fdwSaveOldMode = 0;
HANDLE hStdIn = NULL;

// For FFmpeg recording
std::string g_FFmpegArgs=" -vcodec mpeg4 -q:v 5"; // FFmpeg arguments from batch file
std::string g_OutputFile; // Output video file
bool bActive = false;     // Sender is active
bool bTopmost = false;    // Topmost (ALT-T)
bool bExit = false;       // User quit flag

// Command line arguments
// -start  - Immediate start encoding (default false)
// -hide   - Hide the console when recording (show on taskbar)
// -prompt - Prompt user with file name entry dialog (default false)
// -audio  - Record speaker audio using directshow virtual-audio-device 
// -ext    - File type (mp4, mkv, avi, mov etc - default mp4)
bool bCommandLine  = false;
bool bStart  = false;
bool bHide   = false;
bool bPrompt = true;
bool bAudio  = false;
int codec    = 0; // 0 - mp4, 1 - h264
std::string g_FileExt = "mp4";

// FFmpeg arguments input by batch file (see aa-record.bat)
int g_FPS = 30; // Output frame rate is extracted from the FFmpeg arguments

int main(int argc, char* argv[]);
void ParseCommandLine(int argc, char* argv[]);
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
void Receive();
bool StartFFmpeg();
void StopFFmpeg();
void SetHotKeys();
void ClearHotKeys();
void WriteInitFile(const char* initfile);
void ReadInitFile(const char* initfile);
void ShowKeyCommands();
void Close();

int main(int argc, char* argv[])
{

	// For console window functions
	g_hWnd = GetConsoleWindow();

	// Prevent Edit popup for right mouse click
	HMENU hMenu = GetSystemMenu(g_hWnd, FALSE);
	HMENU hSubMenu = GetSubMenu(hMenu, 7);
	DestroyMenu(hSubMenu);
	RemoveMenu(hMenu, 7, MF_BYPOSITION); 

	// Add an icon from shell32.dll or imageres.dll to the console window.
	// If activated from a batch file, the cmd.exe icon may be shown in the task bar
	HICON hIconBig = nullptr;
	HICON hIconSmall = nullptr;
	// https://help4windows.starlink.us/windows_7_shell32_dll.shtml
	// 176 - start arrow
	// https://diymediahome.org/windows-icons-reference-list-with-details-locations-images/
	// https://renenyffenegger.ch/development/Windows/PowerShell/examples/WinAPI/ExtractIconEx/imageres.html
	// 236 - black arrow
	//  18 - film frame
	// 262 - command prompt
	ExtractIconExA("%SystemRoot%\\system32\\imageres.dll", 236, &hIconBig, &hIconSmall, 1);
	SendMessage(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
	SendMessage(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

	// Register a console handler to detect [X] console close
	// https://learn.microsoft.com/en-us/windows/console/registering-a-control-handler-function
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// Register HotKeys
	SetHotKeys();

	// Executable file path
	GetModuleFileNameA(NULL, g_ExePath, MAX_PATH);
	PathRemoveFileSpecA(g_ExePath);

	// FFmpeg path
	g_FFmpegPath = g_ExePath;
	g_FFmpegPath +="\\DATA\\FFMPEG\\ffmpeg.exe";

	// Does FFmpeg.exe exist there?
	if (_access(g_FFmpegPath.c_str(), 0) == -1) {
		g_FFmpegPath.clear(); // disable functions using FFmpeg
	}

	// SpoutRecorder.ini file path
	strcpy_s(g_Initfile, MAX_PATH, g_ExePath);
	strcat_s(g_Initfile, MAX_PATH, "\\SpoutRecorder.ini");

	// Read recording settings
	ReadInitFile(g_Initfile);

	// Parse command line arguments
	// "-start"  - immediate start when sender detected and end when sender closes (default false)
	// "-prompt" - prompt user for output file (default false)
	// "-audio"  - Record system audio with video (default false)
	// "-ext"    - file type required by codec (default "mp4")
	// FFmpeg arguments are last (see "DATA\Scripts\aa-record.bat")ff
	if (argc > 1) {
		bCommandLine = true;
		ParseCommandLine(argc, argv);
		// Hide console command line option
		if (bHide) {
			ShowWindow(g_hWnd, SW_HIDE);
			ShowWindow(g_hWnd, SW_MINIMIZE);
			ShowWindow(g_hWnd, SW_SHOWMINIMIZED);
		}
	}

	// Show console title and key commands
	ShowKeyCommands();

	// Set up the flashwindow recording status
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-flashwinfo
	fwi.cbSize = sizeof(fwi);
	fwi.hwnd = g_hWnd;
	fwi.uCount = 0;
	fwi.dwTimeout = 0;

	// Monitor console input
	// https://learn.microsoft.com/en-us/windows/console/reading-input-buffer-events
	// Get the standard input handle.
	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	// Save the current input mode, to be restored on exit.
	GetConsoleMode(hStdIn, &fdwSaveOldMode);
	// Enable the window and mouse input events.
	DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS;
	
	char title[256];
	DWORD cNumRead = 0;
	INPUT_RECORD irInBuf={ 0 };
	DWORD NumberOfEvents = 0;
	MSG msg ={0};
	do {

		// Commands from other programs are the in the caption
		if (GetWindowTextA(g_hWnd, title, 256)) {
			if (strcmp(title, "SpoutRecorder") != 0) {
				if (strstr(title, "select") != 0) { // "select sender"
					if (receiver.GetActiveSender(g_SenderName)) {
						if (recorder.IsEncoding()) {
							StopFFmpeg();
							bStart = false;
							bExit = false;
						}
						ShowKeyCommands();
					}
				}
				else if (strstr(title, "close") != 0 || strstr(title, "quit") != 0) {
					Close();
					PostMessage(g_hWnd, WM_CLOSE, 0, 0);
				}
				else if (strstr(title, "stop") != 0) {
					if(recorder.IsEncoding()) {
						bPrompt = false;
						StopFFmpeg();
						bStart = false;
						ShowKeyCommands();
						ShowWindow(g_hWnd, SW_SHOWNORMAL);
					}
				}
				else if (strstr(title, "start") != 0) {
					bStart = true;
					if (strstr(title, "hide") != 0) {
						bHide = true;
						ShowWindow(g_hWnd, SW_HIDE);
						ShowWindow(g_hWnd, SW_MINIMIZE);
						ShowWindow(g_hWnd, SW_SHOWMINIMIZED);
					}
					// If a sender is active, start encodng
					if (bActive) {
						bStart = true;
						StartFFmpeg();
					}
					else {
						SpoutMessageBox(NULL, "Start a sender to record", "SpoutRecorder", MB_ICONWARNING | MB_TOPMOST, 3000);
						ShowKeyCommands();
					}
				}
				if (!recorder.IsEncoding())
					SetWindowTextA(g_hWnd, "SpoutRecorder"); // Set default title
			}
		}
	
		SetConsoleMode(hStdIn, fdwMode);
		GetNumberOfConsoleInputEvents(hStdIn, &NumberOfEvents);

		if(NumberOfEvents > 0) {
			if (ReadConsoleInput(hStdIn, &irInBuf, 1, &cNumRead)) {
				switch (irInBuf.EventType) 	{

				case KEY_EVENT: // keyboard input 
					if (irInBuf.Event.KeyEvent.bKeyDown) {
						if (irInBuf.Event.KeyEvent.wRepeatCount == 1) {
							WORD vCode = irInBuf.Event.KeyEvent.wVirtualKeyCode;
							// printf("vCode = %d\n", vCode);
							if (vCode > 111) {
								// F1 - start recording
								if (vCode == 112) {
									// If a sender is active, start encodng
									if (bActive) {
										bStart = true;
										StartFFmpeg();
									}
									else {
										SpoutMessageBox(NULL, "Start a sender to record", "SpoutRecorder", MB_ICONWARNING | MB_TOPMOST, 3000);
										ShowKeyCommands();
									}
								}

								// F2 - stop recording
								if (vCode == 113) {
									if (recorder.IsEncoding()) {
										StopFFmpeg();
										bStart = false;
										system("cls"); // Clear the console
										ShowKeyCommands();
									}
								}
							} // endif vCode
							else {
								CHAR key = irInBuf.Event.KeyEvent.uChar.AsciiChar;
								// printf("0x%X\n", key);

								// ESC - stop recording
								if (key == 0x1B) {
									if (recorder.IsEncoding()) {
										StopFFmpeg();
										bStart = false;
										system("cls"); // Clear the console
										ShowKeyCommands();
									}
								}

								// A - toggle audio
								if (key == 0x61) {
									bAudio = !bAudio;
									recorder.EnableAudio(bAudio);
									ShowKeyCommands();
								}

								// C - codec
								if (key == 0x63) {
									codec += 1;
									if (codec > 1) codec = 0;
									if (codec == 0) { // mpeg4
										g_FFmpegArgs = " -vcodec mpeg4 -q:v 5";
										g_FileExt = "mp4";
										g_FPS = 30;
									}
									if (codec == 1) { // x264
										g_FFmpegArgs = " -vcodec libx264 -preset superfast -tune zerolatency -crf 23";
										g_FileExt = "mkv";
										g_FPS = 30;
									}
									recorder.SetFps(g_FPS);
									recorder.SetCodec(codec);
									ShowKeyCommands();
								}

								// T - toggle topmost
								if (key == 0x74) {
									if (bTopmost) {
										SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
										bTopmost = false;
									}
									else {
										SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
										SetForegroundWindow(g_hWnd);
										bTopmost = true;
									}
									ShowKeyCommands();
								}

								// V - show video folder
								if (key == 0x76) {
									char datapath[256]={};
									GetModuleFileNameA(NULL, &datapath[0], 256);
									PathRemoveFileSpecA(datapath);
									strcat_s(datapath, 256, "\\data\\videos");
									if (_access(datapath, 0) != -1)
										ShellExecuteA(g_hWnd, "open", datapath, NULL, NULL, SW_SHOWNORMAL);
									else
										SpoutMessageBox(NULL, "video folder not found", "SpoutRecorder", MB_OK | MB_ICONWARNING, 3000);
								}

								// P - prompt for file name
								if (key == 0x70) {
									bPrompt = !bPrompt;
									ShowKeyCommands();
								}

								// H - help dialog
								if (key == 0x68) {

									std::string str = "A - Audio\n";
									str += "Record system audio with the video, ";
									str += "A DirectShow <a href=\"https://github.com/rdp/virtual-audio-capture-grabber-device/\">virtual audio device</a>, ";
									str += "developed by Roger Pack, allows FFmpeg to record system audio together with the video. ";
									str += "Register it using \"VirtualAudioRegister.exe\" in the <a href=\"";
									str += g_ExePath;
									str += "\\data\\audio\\VirtualAudio\">\"VirtualAudio\"</a> folder.\n\n";
									str += "C - Codec\n";
									str += "x264 codec can be used instead of default Mpeg4. ";
									str += "Playback compatibility and quality may be improved. File size approximately 20% less. ";
									str += "Encoding speed may be reduced slightly depending on the computer specifications. ";
									str += "To check, click on the SpoutRecorder taskbar icon while recording. ";
									str += "FFmpeg encoding speed is shown in the console window. ";
									str += "You should see a speed of 1.0 if the encoding is keeping pace with the input frame rate.\n\n";
									str += "P - Prompt\nPrompt for file name and show file details after save. By default, a file with the sender name is saved in \"DATA\\Videos\" and over-written if it exists.\n\n";
									str += "Topmost\n";
									str += "Keep the SpoutRecorder window topmost.\n\n\n";
									SpoutMessageBox(NULL, str.c_str(), "Options", MB_OK | MB_TOPMOST);
								}

								// Q - quit and close console
								if (key == 0x71) {
									StopFFmpeg();
									bStart = false;
									bExit = true;
								}

							}
						}
					}
					break;

				case MOUSE_EVENT:
					// Right click - select sender if not encoding
					if (irInBuf.Event.MouseEvent.dwButtonState == 2 && !recorder.IsEncoding()) {
						receiver.SelectSender();
					}
					break;

				case FOCUS_EVENT:
				case MENU_EVENT:
				case WINDOW_BUFFER_SIZE_EVENT:
				default:
					break;
				}
			}
		}

		// Monitor windows messages to look for HotKeys
		ZeroMemory(&msg, sizeof(msg));
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			// Test for HotKeys
			if (msg.message == WM_HOTKEY) {
				// ALT+Q - stop and quit
				if (msg.wParam == 1) {
					StopFFmpeg();
					bExit = true;
				}
				// ALT+F1 - 0x70 - start recording
				if (msg.wParam == 2) {
					// If a sender is active, start encodng
					if (bActive) {
						bStart = true;
						StartFFmpeg();
					}
					else {
						SpoutMessageBox(NULL, "Start a sender to record", "SpoutRecorder", MB_ICONWARNING | MB_TOPMOST, 3000);
						ShowKeyCommands();
					}
				}

				// ALT+F2 - 0x71 - stop recording
				if (msg.wParam == 3) {
					StopFFmpeg();
					bStart = false;
					system("cls");
					ShowKeyCommands();
				}
			}
		}

		// Receive from a sender
		Receive();

	} while (!bExit);

	// Stop encoding, close receiver and free resources
	Close();

	// Save recording settings
	WriteInitFile(g_Initfile);

	// Close the console window
	PostMessage(g_hWnd, WM_CLOSE, 0, 0);

	return 0;
}

void ShowKeyCommands()
{
	// Show our text bright yellow - a different colour to FFmpeg
	// https://learn.microsoft.com/en-us/windows/console/setconsoletextattribute
	g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	// Remove the blinking cursor
	CONSOLE_CURSOR_INFO cursorInfo={};
	GetConsoleCursorInfo(g_hConsole, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(g_hConsole, &cursorInfo);

	// Start printing from the top of the screen
	COORD pos ={ 0, 0 };
	SetConsoleCursorPosition(g_hConsole, pos);

	// Set console font size
	CONSOLE_FONT_INFOEX cfi={ 0 };
	cfi.cbSize = sizeof(cfi);
	GetCurrentConsoleFontEx(g_hConsole, false, &cfi);
	cfi.dwFontSize.Y = 16; // Height (width follows height)
	SetCurrentConsoleFontEx(g_hConsole, FALSE, &cfi);

	// Is a sender running ?
	if (receiver.GetActiveSender(g_SenderName)) {
		printf("[%s]\nRight click - select sender\n\n", g_SenderName);
	}
	else {
		// Erase the top line
		printf("\33[2K\r");
		printf(" \nStart a sender to record   \n\n");
	}
	
	SetWindowTextA(g_hWnd, "SpoutRecorder");

	// Show key commands
	char codecstr[256]={0};

	std::string startstr   = "  F1     - start recording\n";
	startstr              += "  F2/ESC - stop recording \n";
	startstr              += "  Q      - stop and quit\n";
	startstr              += "  V      - show video folder\n";
	startstr              += "  H      - help\n";
	startstr              += "\nSettings\n";

	std::string audiostr   = "  A      - no audio    \n";
	if(bAudio) audiostr    = "  A      - system audio\n";
	if(codec == 0) 
	sprintf_s(codecstr, 256, "  C      - codec mpeg4\n");
	else
	sprintf_s(codecstr, 256, "  C      - codec x264 \n");
	std::string promptstr  = "  P      - auto file name      \n";
	if(bPrompt) promptstr  = "  P      - prompt for file name\n";
	std::string topstr     = "  T      - not topmost\n";
	if (bTopmost) topstr   = "  T      - topmost    \n";

	std::string keystr = startstr;
	keystr += audiostr;
	keystr += codecstr;
	keystr += promptstr;
	keystr += topstr;
	printf("%s\n", keystr.c_str());

	printf("Hot Keys\n");
	printf("  ALT+F1 - start\n");
	printf("  ALT+F2 - stop\n");
	printf("  ALT+Q  - stop and quit\n");

}


// Parse command line argsuments
void ParseCommandLine(int argc, char* argv[])
{
	// argv[0] is the executable name
	// FFmpeg arguments are last
	if (argc > 0) {

		std::string argstr;
		for (int i = 1; i < argc; i++)
		{
			argstr = argv[i];
			if (argv[i]) {
				// Program arguments
				if (strstr(argv[i], "-start") != 0) {
					// Command line immediate start
					bStart = true;
					bHide = false;
					bAudio = false;
					bPrompt = false;
					g_FFmpegArgs = " -vcodec mpeg4 -q:v 5";
					g_FileExt = "mp4";
					codec = 0;
				}
				else if (strstr(argv[i], "-hide") != 0) {
					// Hide window on record
					bHide = true;
				}
				else if (strstr(argv[i], "-prompt") != 0) {
					// Prompt for file name entry
					bPrompt = true;
				}
				else if (strstr(argv[i], "-audio") != 0) {
					// Record system audio with video
					bAudio = true;
				}
				else if(strstr(argv[i], "-mpeg4") != 0) { // mpeg4
					g_FFmpegArgs = " -vcodec mpeg4 -q:v 5";
					g_FileExt = "mp4";
					codec = 0;
				}
				else if (strstr(argv[i], "-x264") != 0) { // x264
					g_FFmpegArgs = " -vcodec libx264 -preset ultrafast -tune zerolatency -crf 23";
					g_FileExt = "mkv";
					codec = 1;
				}
				else if (strstr(argv[i], "-ext") != 0) {
					// Alternate file type required by codec (mp4/mkv/avi/mov/wmv etc)
					size_t pos = argstr.find("-ext");
					if (pos != std::string::npos) {
						argstr = argstr.substr(pos+5, 3);
						g_FileExt = argstr;
					}
				}
				else {
					// FFmpeg arguments are last
					g_FFmpegArgs = argstr;
					if (!g_FFmpegArgs.empty()) {
						// Extract frame rate for FFmpeg and video receive
						size_t pos = argstr.find("-r ");
						if (pos != std::string::npos) {
							argstr = argstr.substr(pos+3, 2); // 2 digits for fps
							g_FPS = atoi(argstr.c_str());
							recorder.SetFps(g_FPS);
						}
					}
				}
			}
		}
	}
}


void Receive()
{
	// No senders
	if (bActive && receiver.GetSenderCount() == 0) {
		if (recorder.IsEncoding()) {
			SpoutMessageBox(NULL, "No senders", "SpoutRecorder", MB_OK | MB_ICONWARNING | MB_TOPMOST, 2000);
			StopFFmpeg();
		}
		bStart = false;
		bExit = false;
		bActive = false;

		system("cls");
		ShowKeyCommands();

		return;
	}

	// Get pixels from the sender shared texture.
	// ReceiveImage handles sender detection, creation and update.
	if (receiver.ReceiveImage(pixelBuffer, g_SenderWidth, g_SenderHeight)) {
		bActive = true;
		// IsUpdated() returns true if the sender has changed
		if (receiver.IsUpdated()) {
			if (g_SenderName[0]) {
				// Different sender selected
				if (receiver.GetSenderCount() > 1
					&& strcmp(g_SenderName, receiver.GetSenderName()) != 0) {
					strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());
					if (recorder.IsEncoding()) {
						StopFFmpeg();
						bStart = false;
						bExit = false;
					}
					ShowKeyCommands();
					return;
				}
			}
			else {
				// New sender selected
				strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());
				if (receiver.GetSenderCount() == 1) {
					ShowKeyCommands();
				}
			}

			// Update the sender name - it could be different
			strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());

			// Update globals
			g_SenderWidth  = receiver.GetSenderWidth();
			g_SenderHeight = receiver.GetSenderHeight();

			// Update the receiving buffer
			if (pixelBuffer) delete[] pixelBuffer;
			pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 4];

			// Stop FFmpeg if already encoding and the stream size has changed.
			// Do not exit. Return the user to the start.
			if(recorder.IsEncoding()) {
				StopFFmpeg();
				bStart = false;
				ShowKeyCommands();
			}

			bActive = true;

			// Start FFmpeg for F1 or the command line "-start" argument
			if (bStart) {
				if (!StartFFmpeg()) {
					// Quit completely for command line problem
					if (bCommandLine) {
						bStart = false;
						bExit = true;
						SpoutMessageBox(NULL, "FFmpeg failed to start with command line", "SpoutRecorder", MB_OK | MB_ICONWARNING | MB_TOPMOST);
						return;
					}
				}
			}
		}
		else if (pixelBuffer && recorder.IsEncoding()) {
			recorder.Write(pixelBuffer, g_SenderWidth*g_SenderHeight*4);
			bActive = true;
		}
	}
	else {
		// If FFmpeg is encoding, stop and exit
		if(recorder.IsEncoding()) {
			SpoutMessageBox(NULL, "Sender closed", "SpoutRecorder", MB_OK | MB_ICONWARNING | MB_TOPMOST, 2000);
			StopFFmpeg();
			g_SenderName[0] = 0;
			bActive = false;
			bStart = false;
			system("cls");
			ShowKeyCommands();
		}

		// Sender has closed
		if (g_SenderName[0] && !receiver.sendernames.FindSenderName(g_SenderName)) {
			g_SenderName[0] = 0;
			bActive = false;
			ShowKeyCommands();
		}
		// No sender
	}

	// Limit input frame rate for FFmpeg
	// to the video frame rate
	receiver.HoldFps(g_FPS);

}

bool StartFFmpeg()
{
	// Already recording, no sender or no FFmpeg
	if (recorder.IsEncoding() || !bActive || g_FFmpegPath.empty()) {
		return false;
	}

	// Default output file
	g_OutputFile = g_ExePath; // exe folder
	g_OutputFile += "\\DATA\\Videos\\";
	g_OutputFile += (char*)g_SenderName;
	g_OutputFile += ".";
	g_OutputFile += g_FileExt;

	//
	// User file name entry
	//
	if (bPrompt) {
		char filePath[MAX_PATH]={};
		sprintf_s(filePath, MAX_PATH, g_OutputFile.c_str());
		OPENFILENAMEA ofn={};
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		HWND hwnd = NULL;
		ofn.hwndOwner = hwnd;
		ofn.hInstance = GetModuleHandle(0);
		ofn.nMaxFileTitle = 31;
		// if ofn.lpstrFile contains a path, that path is the initial directory.
		ofn.lpstrFile = (LPSTR)filePath;
		ofn.nMaxFile = MAX_PATH;
		if(codec ==0)
			ofn.lpstrFilter = "Mpeg-4 (*.mp4)\0*.mp4\0Matroska (*.mkv)\0*.mkv\0Audio Video Interleave (*.avi)\0*.avi\0Quicktime (*.mov)\0*.mov\0All files (*.*)\0*.*\0";
		else
			ofn.lpstrFilter = "Matroska (*.mkv)\0*.mkv\0Mpeg-4 (*.mp4)\0*.mp4\0Audio Video Interleave (*.avi)\0*.avi\0Quicktime (*.mov)\0*.mov\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt = "";
		// OFN_OVERWRITEPROMPT prompts for over-write
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		ofn.lpstrTitle = "Output File";
		if (!GetSaveFileNameA(&ofn)) {
			// FFmpeg has not been started yet, return to try again
			return false;
		}
		// User file name entry
		g_OutputFile = filePath;
	}

	// Options for audio, codec and fps
	recorder.EnableAudio(bAudio); // For recording system audio
	recorder.SetCodec(codec); // x264 codec
	recorder.SetFps(g_FPS); // Fps for FFmpeg (see HoldFps)

	// Start FFmpeg pipe
	// recorder.Start(g_FFmpegPath, g_FFmpegArgs, g_OutputFile, g_SenderWidth, g_SenderHeight, false);
	recorder.Start(g_FFmpegPath, g_OutputFile, g_SenderWidth, g_SenderHeight, false);

	// Reset console text colour
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// Set caption
	std::string capstr = g_SenderName;
	capstr += ".";
	capstr += g_FileExt;
	
	SetWindowTextA(g_hWnd, "Recording");

	// Start flashing to show recording status
	fwi.dwFlags = FLASHW_ALL | FLASHW_TIMER;
	FlashWindowEx(&fwi);

	// Code in Receive() is activated
	return true;

} // end StartFFmpeg


// Stop encoding with the Escape key or if the sender closes
void StopFFmpeg()
{
	if (recorder.IsEncoding()) {

		// Stop encoding
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		recorder.Stop();
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		
		// Stop flashing
		fwi.dwFlags = FLASHW_STOP;
		FlashWindowEx(&fwi);

		// Show the user the saved file details for 3 seconds
		if(!bCommandLine && bPrompt) {
			char tmp[MAX_PATH];
			sprintf_s(tmp, MAX_PATH, "Saved [%s]", g_OutputFile.c_str());
			SpoutMessageBox(NULL, tmp, "SpoutRecorder", MB_OK | MB_TOPMOST | MB_ICONINFORMATION, 3000);
		}
	}
}


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// CTRL-CLOSE: when the console is closed by the user
		case CTRL_CLOSE_EVENT:
			// Stop encoding, close receiver and free resources
			Close();
			// Save recording settings
			WriteInitFile(g_Initfile);
			return TRUE;
		default:
			return FALSE;
	}

}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey?redirectedfrom=MSDN
void SetHotKeys()
{
	// 1 - ALT+Q  - stop and Quit
	RegisterHotKey(NULL, 1, MOD_NOREPEAT | MOD_ALT, 0x51); // 0x51 is 'q'
	// 2 - ALT+F1 - start recording
	RegisterHotKey(NULL, 2, MOD_NOREPEAT | MOD_ALT, 0x70); // ALT+F1 - 0x70
	// 3 - ALT+F2 - record with audio
	RegisterHotKey(NULL, 3, MOD_NOREPEAT | MOD_ALT, 0x71); // ALT+F2 - 0x71
}

void ClearHotKeys()
{
	// 1 - ALT+Q  - stop and Quit
	UnregisterHotKey(NULL, 1);
	// 2 - ALT+F1 - start recording
	UnregisterHotKey(NULL, 2);
	// 3 - ALT+F2 - record with audio
	UnregisterHotKey(NULL, 3);
}

void Close()
{
	// Stop recording
	recorder.Stop();
	// Close receiver and free resources
	receiver.ReleaseReceiver();
	// Clear hotkeyy registration
	ClearHotKeys();
	// Reset console text colour
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	// Restore console input mode
	SetConsoleMode(hStdIn, fdwSaveOldMode);
}



//--------------------------------------------------------------
// Save a configuration file in the executable folder
// Ini file is created if it does not exist
void WriteInitFile(const char* initfile)
{
	char tmp[256]={ 0 };

	//
	// OPTIONS
	//

	if (bAudio)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Audio", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Audio", (LPCSTR)"0", (LPCSTR)initfile);

	sprintf_s(tmp, 256, "%.1d", codec);
	WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Codec", (LPCSTR)tmp, (LPCSTR)initfile);

	if (bPrompt)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Prompt", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Prompt", (LPCSTR)"0", (LPCSTR)initfile);

	if (bTopmost)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Topmost", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Topmost", (LPCSTR)"0", (LPCSTR)initfile);

}

//--------------------------------------------------------------
// Read back settings from configuration file
void ReadInitFile(const char* initfile)
{
	char tmp[MAX_PATH]={ 0 };

	//
	// OPTIONS
	//

	bAudio = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Audio", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bAudio = (atoi(tmp) == 1);
	if (bAudio)

	codec = 0;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Codec", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) codec = atoi(tmp);

	if (codec == 0) { // mpeg4
		g_FFmpegArgs = " -vcodec mpeg4 -q:v 5";
		g_FileExt = "mp4";
	}
	if (codec == 1) { // x264
		g_FFmpegArgs = " -vcodec libx264 -preset ultrafast -tune zerolatency -crf 23";
		g_FileExt = "mkv";
	}

	bPrompt = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Prompt", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bPrompt = (atoi(tmp) == 1);

	bTopmost = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Topmost", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bTopmost = (atoi(tmp) == 1);

	if(bTopmost)
		SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);

}


// The end ..
