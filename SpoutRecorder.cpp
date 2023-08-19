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
//        R      - RGBA/RGB pixel data
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
//				   A-sytem audio / R-rgba/rgb / C-codec / V-show video folder / P-prompt for filename
//				   Add initialization file to save ad restore options
//				   Add ALT+F2 hotkey to stop FFmpeg
//				   Icon from imageres.dll
//				   Change caption to "Recording" if recording to indicate recording status
//				   Add "Sender" caption command to change to active sender
//				   Version 1.004
//
// =========================================================================
// 
// Copyright(C) 2023 Lynn Jarvis.
// 
// http://spout.zeal.co/
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
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include "SpoutDX/SpoutDX.h"

// Global Variables:
spoutDX receiver;                     // Receiver object
HANDLE g_hConsole = nullptr;          // Console window handle
unsigned char* pixelBuffer = nullptr; // Receiving pixel buffer
char g_SenderName[256]={};            // Received sender name
unsigned int g_SenderWidth = 0;       // Received sender width
unsigned int g_SenderHeight = 0;      // Received sender height
HWND g_hWnd = NULL;                   // Console window handle
char g_Initfile[MAX_PATH]={ 0 };      // Initialization file
FLASHWINFO fwi={0};                   // For flashing title bar and taskbar icon
// https://learn.microsoft.com/en-us/windows/console/reading-input-buffer-events
DWORD fdwSaveOldMode = 0;
HANDLE hStdIn = NULL;

// For FFmpeg recording
FILE* m_FFmpeg = nullptr; // FFmpeg pipe
std::string g_FFmpegArgs=" -vcodec mpeg4 -q:v 5"; // FFmpeg arguments from batch file
std::string g_OutputFile; // Output video file
bool bActive = false;     // Sender is active
bool bEncoding = false;   // Encode in progress
bool bTopMost = false;    // Topmost (ALT-T)
bool bExit = false;       // User quit flag

// Command line arguments
// -start  - Immediate start encoding (default false)
// -hide   - Hide the console when recording (show on taskbar)
// -prompt - Prompt user with file name entry dialog (default false)
// -rgb    - RGB input pixel format instead of default RGBA
// -audio  - Record speaker audio using directshow virtual-audio-device 
// -ext    - File type (mp4, mkv, avi, mov etc - default mp4)
bool bCommandLine  = false;
bool bStart  = false;
bool bHide   = false;
bool bPrompt = true;
bool bRgb    = false;
bool bAudio  = false;
int codec    = 0; // 0 - mp4, 1 - h264
std::string g_FileExt = "mp4";

// FFmpeg arguments input by batch file (see aa-record.bat)
int g_FPS = 30; // Output frame rate is extracted from the FFmpeg arguments

int main(int argc, char* argv[]);
void ParseCommandLine(int argc, char* argv[]);
void Receive();
void StartFFmpeg();
void StopFFmpeg();
void ShowKeyCommands();
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
void SetHotKeys();
void ClearHotKeys();
void WriteInitFile(const char* initfile);
void ReadInitFile(const char* initfile);
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

	// Parse command line arguments
	// "-start"  - immediate start when sender detected and end when sender closes (default false)
	// "-prompt" - prompt user for output file (default false)
	// "-rgb"    - RGB or RGBA input pixel format (default false RGBA)
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

	// SpoutRecorder.ini file path
	GetModuleFileNameA(NULL, g_Initfile, MAX_PATH);
	PathRemoveFileSpecA(g_Initfile);
	strcat_s(g_Initfile, MAX_PATH, "\\SpoutRecorder.ini");

	// Read recording settings
	ReadInitFile(g_Initfile);

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
				// if(!bEncoding) SetWindowTextA(g_hWnd, "SpoutRecorder"); // Set default title
				if (strstr(title, "select") != 0) { // "select sender"
					if (receiver.GetActiveSender(g_SenderName)) {
						if (bEncoding) {
							StopFFmpeg();
							bEncoding = false;
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
					if (m_FFmpeg) {
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
					// Not topmost
					SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
					bTopMost = false;
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
									// Not topmost
									SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
									bTopMost = false;
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
									if (m_FFmpeg) {
										StopFFmpeg();
										bStart = false;
										// LJ DEBUG
										// system("cls"); // Clear the console
										ShowKeyCommands();
									}
								}
							} // endif vCode
							else {
								CHAR key = irInBuf.Event.KeyEvent.uChar.AsciiChar;
								// printf("0x%X\n", key);

								// ESC - stop recording
								if (key == 0x1B) {
									if (m_FFmpeg) {
										StopFFmpeg();
										bStart = false;
										system("cls"); // Clear the console
										ShowKeyCommands();
									}
								}

								// A - toggle audio
								if (key == 0x61) {
									bAudio = !bAudio;
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
									ShowKeyCommands();
								}

								// R - toggle rgb
								if (key == 0x72) {
									bRgb = !bRgb;
									ShowKeyCommands();
								}

								// T - toggle topmost
								if (key == 0x74) {
									if (bTopMost) {
										SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
										bTopMost = false;
									}
									else {
										SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
										SetForegroundWindow(g_hWnd);
										bTopMost = true;
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
					if (irInBuf.Event.MouseEvent.dwButtonState == 2 && !bEncoding) {
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
					// Not topmost
					SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
					bTopMost = false;
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
	// Clear the console
	// system("cls");

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
		printf("Start a sender to record\n\n");
	}
	
	SetWindowTextA(g_hWnd, "SpoutRecorder");

	// Show key commands
	char codecstr[256]={0};

	std::string startstr   = "  F1     - start recording\n";
	startstr              += "  F2/ESC - stop recording \n";
	startstr              += "  V      - show video folder\n";
	startstr              += "  Q      - stop and quit\n";
	startstr              += "\nSettings\n";

	std::string audiostr   = "  A      - no audio    \n";
	if(bAudio) audiostr    = "  A      - system audio\n";
	std::string rgbstr     = "  R      - RGBA pixel data\n";
	if(bRgb) rgbstr        = "  R      - RGB pixel data \n";
	if(codec == 0) 
	sprintf_s(codecstr, 256, "  C      - codec mpeg4\n");
	else
	sprintf_s(codecstr, 256, "  C      - codec x264 \n");
	std::string promptstr  = "  P      - auto file name      \n";
	if(bPrompt) promptstr  = "  P      - prompt for file name\n";
	std::string topstr     = "  T      - not topmost\n";
	if (bTopMost) topstr   = "  T      - topmost    \n";

	std::string keystr = startstr;
	keystr += audiostr;
	keystr += rgbstr;
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
					bPrompt = false;
				}
				else if (strstr(argv[i], "-hide") != 0) {
					// Hide window on record
					bHide = true;
				}
				else if (strstr(argv[i], "-prompt") != 0) {
					// Prompt for file name entry
					bPrompt = true;
				}
				else if (strstr(argv[i], "-rgb") != 0) {
					// RGB or RGBA input pixel format (default RGBA)
					bRgb = true;
				}
				else if (strstr(argv[i], "-audio") != 0) {
					// Record system audio with video
					bAudio = true;
				}
				else if (strstr(argv[i], "-ext") != 0) {
					// File type required by codec (mp4/mkv/avi/mov etc)
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
		if (bEncoding)
			StopFFmpeg();
		bEncoding = false;
		bStart = false;
		bExit = false;
		bActive = false;
		ShowKeyCommands();
		return;
	}

	// Get pixels from the sender shared texture.
	// ReceiveImage handles sender detection, creation and update.
	// If bRgb is true, receive an RGB data buffer
	if (receiver.ReceiveImage(pixelBuffer, g_SenderWidth, g_SenderHeight, bRgb)) {
		bActive = true;
		// IsUpdated() returns true if the sender has changed
		if (receiver.IsUpdated()) {
			if (g_SenderName[0]) {
				// Different sender selected
				if (receiver.GetSenderCount() > 1
					&& strcmp(g_SenderName, receiver.GetSenderName()) != 0) {
					strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());
					if (bEncoding) {
						StopFFmpeg();
						bEncoding = false;
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
			if (bRgb)
				pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 3];
			else
				pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 4];

			// Stop FFmpeg if already encoding and the stream size has changed.
			// Return the user to the start.
			if (bEncoding) {
				StopFFmpeg();
				bEncoding = false;
				bStart = false;
				ShowKeyCommands();
			}

			bActive = true;

			// Start FFmpeg for F1 or the command line "-start" argument
			if (bStart) {
				StartFFmpeg();
			}

		}
		else if (pixelBuffer && m_FFmpeg) {
			if (bEncoding) {
				// Encode pixels to video with the FFmpeg output pipe
				if(bRgb)
					fwrite((const void*)pixelBuffer, 1, g_SenderWidth*g_SenderHeight*3, m_FFmpeg);
				else
					fwrite((const void*)pixelBuffer, 1, g_SenderWidth*g_SenderHeight*4, m_FFmpeg);
			}
			else {
				// FFmpeg is encoding, stop and exit
				StopFFmpeg();
				bStart = false;
				bExit = true;
			}
		}
		bActive = true;
	}
	else {

		// If FFmpeg is encoding, stop and exit
		if (bEncoding) {
			MessageBoxA(NULL, "Sender closed - stopping FFmpeg", "Warning", MB_OK | MB_ICONWARNING | MB_TOPMOST);
			StopFFmpeg();
			bStart = false;
			bActive = false;
			ShowKeyCommands();
		}

		// Sender has closed
		if (g_SenderName[0] && !receiver.sendernames.FindSenderName(g_SenderName)) {
			// MessageBoxA(NULL, "Sender has closed", "Warning", MB_OK | MB_ICONWARNING | MB_TOPMOST);
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


void StartFFmpeg()
{
	// Already recording or no sender
	if (m_FFmpeg || !bActive) {
		return;
	}

	// Executable folder
	char exefolder[MAX_PATH];
	GetModuleFileNameA(NULL, exefolder, MAX_PATH);
	PathRemoveFileSpecA(exefolder);

	// Default output file path
	g_OutputFile = exefolder;
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
		ofn.lpstrFilter = "Mpeg-4 (*.mp4)\0*.mp4\0Matroska (*.mkv)\0*.mkv\0Audio Video Interleave (*.avi)\0*.avi\0Quicktime (*.mov)\0*.mov\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt = "";
		// OFN_OVERWRITEPROMPT prompts for over-write
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		ofn.lpstrTitle = "Output File";
		if (!GetSaveFileNameA(&ofn)) {
			// FFmpeg has not been started yet
			// Return to try again
			return;
		}
		// User file name entry
		g_OutputFile = filePath;
	}

	// FFmpeg location
	char ffmpegPath[MAX_PATH];
	GetModuleFileNameA(NULL, ffmpegPath, MAX_PATH);
	PathRemoveFileSpecA(ffmpegPath);
	strcat_s(ffmpegPath, "\\DATA\\FFMPEG");
	std::string outputString = ffmpegPath;

	// Does ffmpeg.exe exist there ?
	outputString += "\\ffmpeg.exe";
	if (_access(outputString.c_str(), 0) == -1) {
		SpoutMessageBox(NULL, "FFmpeg not found", "SpoutRecorder", MB_ICONWARNING | MB_TOPMOST, 3000);
		return;
	}

	// Maximum threads
	outputString += " -threads 0";

	// -thread_queue_size
	// This option sets the maximum number of queued packets when reading from the file or device. 
	// With low latency / high rate live streams, packets may be discarded if they are not read in a
	// timely manner; setting this value can force ffmpeg to use a separate input thread and read
	// packets as soon as they arrive. By default ffmpeg only do this if multiple inputs are specified.
	// TODO : optimize ?
	// There appears to be no documentation on what value should be set
	outputString += " -thread_queue_size 4096";

	// Overwrite output files without asking
	outputString += " -y";

	// Record system audio using the directshow audio capture device
	// (Option set by "aa-record.bat" batch file or by F2 to start)
	// thread_queue_size must be set on this input as well
	if(bAudio)
		outputString += " -f dshow -i audio=\"virtual-audio-capturer\" -thread_queue_size 512";
	else
		outputString += " -an";

	// Input raw video
	outputString += " -f rawvideo -vcodec rawvideo";

	// Input pixel format bgr24 (RGB pixels) or bgra (RGBA pixels)
	// bgra is faster due to SSE optimized pixel copy from the sender texture
	// Overall encoding rate is approximately double.
	if(bRgb)
		outputString += " -pix_fmt bgr24";
	else
		outputString += " -pix_fmt bgra";

	// Size of frame
	outputString += " -s ";
	outputString += std::to_string(g_SenderWidth);
	outputString += "x";
	outputString += std::to_string(g_SenderHeight);

	// FFmpeg input frame rate must be the same as
	// the video frame output rate (see HoldFps)
	outputString += " -r ";
	outputString += std::to_string(g_FPS);

	// The final “-” tells FFmpeg to write to stdout
	outputString += " -i - ";

	// User FFmpeg options
	// (See "DATA\Scripts\aa-record.bat")
	if (!g_FFmpegArgs.empty()) {
		outputString += g_FFmpegArgs;
	}
	else { // Defaults
		// FFmpeg “mpeg4” encoder (MPEG-4 Part 2 format)
		// https://trac.ffmpeg.org/wiki/Encode/MPEG-4
		outputString += " -vcodec mpeg4";
		// Quality (1 = best, 31 = worse) 
		// 5 - best trade off between file size and quality
		outputString += " -q:v 5";
	}

	if (bAudio)
		outputString += " -shortest";

	outputString += " \""; // Insert a space before the output file
	outputString += g_OutputFile;
	outputString += "\"";

	// printf("%s\n\n", outputString.c_str());

	// Open pipe to ffmpeg's stdin in binary write mode.
	// The STDIO lib will use block buffering when talking to
	// a block file descriptor such as a pipe.
	m_FFmpeg = _popen(outputString.c_str(), "wb");
	
	bEncoding = true; // Encoding active

	// Reset console text colour
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// Set caption
	std::string capstr = g_SenderName;
	capstr += ".";
	capstr += g_FileExt;
	// SetWindowTextA(g_hWnd, capstr.c_str());
	SetWindowTextA(g_hWnd, "Recording");

	// Start flashing to show recording status
	fwi.dwFlags = FLASHW_ALL | FLASHW_TIMER;
	FlashWindowEx(&fwi);

	// Code in Receive() is activated

} // end StartFFmpeg


// Stop encoding with the Escape key or if the sender closes
void StopFFmpeg()
{
	if (m_FFmpeg) {
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		// Flush FFmpeg
		fflush(m_FFmpeg);
		// FFmpeg pipe is still open so close it
		_pclose(m_FFmpeg);
		m_FFmpeg = nullptr;
		// Allow FFmpeg to finish
		Sleep(10);
		bEncoding = false;
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
	if (m_FFmpeg) StopFFmpeg();
	// Close receiver and free resources
	receiver.ReleaseReceiver();
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

	if (bRgb)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"RGB", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"RGB", (LPCSTR)"0", (LPCSTR)initfile);

	sprintf_s(tmp, 256, "%.1d", codec);
	WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Codec", (LPCSTR)tmp, (LPCSTR)initfile);

	if (bPrompt)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Prompt", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Prompt", (LPCSTR)"0", (LPCSTR)initfile);

	if (bTopMost)
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

	bRgb = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"RGB", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bRgb = (atoi(tmp) == 1);

	codec = 0;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Codec", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) codec = atoi(tmp);

	if (codec == 0) { // mpeg4
		g_FFmpegArgs = " -vcodec mpeg4 -q:v 5";
		g_FileExt = "mp4";
	}
	if (codec == 1) { // h264
		g_FFmpegArgs = " -vcodec libx264 -preset ultrafast -tune zerolatency -crf 23";
		g_FileExt = "mkv";
	}

	bPrompt = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Prompt", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bPrompt = (atoi(tmp) == 1);

	bTopMost = false;
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Topmost", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bTopMost = (atoi(tmp) == 1);

	if(bTopMost)
		SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);

}

// The end ..
