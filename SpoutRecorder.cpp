//
// SpoutRecorder.cpp
//
// Encodes Spout input to a video file using FFmpeg by way of a pipe.
// Resolution and speed are improved over using SpoutCam as a source
//
//	  F1    - start recording
//    F2    - record with audio
//    ESC   - stop encoding
//    Q     - stop and quit
//    HotKeys
//    ALT+T - toggle tompost
//    ALT+Q - stop and quit
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

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include "SpoutDX\SpoutDX.h"

// Global Variables:
spoutDX receiver;                     // Receiver object
HANDLE g_hConsole = nullptr;          // Console window handle
unsigned char* pixelBuffer = nullptr; // Receiving pixel buffer
char g_SenderName[256]={};            // Received sender name
unsigned int g_SenderWidth = 0;       // Received sender width
unsigned int g_SenderHeight = 0;      // Received sender height
HWND g_hWnd = NULL;                   // Console window handle

// For FFmpeg recording
FILE* m_FFmpeg = nullptr; // FFmpeg pipe
std::string g_FFmpegArgs; // User FFmpeg arguments from batch file
std::string g_OutputFile; // Output video file
bool bActive = false;     // Sender is active
bool bEncoding = false;   // Encode in progress
bool bTopMost = false;    // Topmost (ALT-T)
bool bExit = false;       // User quit flag

// Command line arguments
// -start  - Immediate start encoding (default false)
// -prompt - Pronpt user with file name entry dialog (default false)
// -rgb    - RGB input pixel format instead of default RGBA
// -audio  - Record speaker audio using directshow virtual-audio-device 
// -ext    - File type (mp4, mkv, avi, mov etc - default mp4)
bool bStart  = false;
bool bPrompt = false;
bool bRgb    = false;
bool bAudio  = false;
std::string g_FileExt = "mp4";

// FFmpeg arguments input by batch file (see aa-record.bat)
int g_FPS = 30; // Output frame rate is extracted from the FFmpeg arguments

int main(int argc, char* argv[]);
void ParseCommandLine(int argc, char* argv[]);
void Receive();
void StartFFmpeg();
void StopFFmpeg();

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);


int main(int argc, char* argv[])
{
	// Add a title to the console window to replace the full path
	g_hWnd = GetConsoleWindow();
	SetWindowTextA(g_hWnd, "SpoutRecorder");

	// Add an icon from shell32.dll to the console window.
	// If activated from a batch file, the cmd.exe icon is shown in the task bar
	HICON hIconBig = nullptr;
	HICON hIconSmall = nullptr;
	ExtractIconExA("%SystemRoot%\\system32\\SHELL32.dll", 176, &hIconBig, &hIconSmall, 1);
	SendMessage(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
	SendMessage(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

	// Show our text bright yellow - a different colour to FFmpeg
	// https://learn.microsoft.com/en-us/windows/console/setconsoletextattribute
	g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	// Register a console handler to detect [X] console close
	// https://learn.microsoft.com/en-us/windows/console/registering-a-control-handler-function
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// Parse command line arguments
	// "-start"  - immediate start when sender detected and end when sender closes (default false)
	// "-prompt" - prompt user for output file (default false)
	// "-rgb"    - RGB or RGBA input pixel format (default false RGBA)
	// "-audio"  - Record system audio with video (default false)
	// "-ext"    - file type required by codec (default "mp4")
	// FFmpeg arguments are last (see "DATA\Scripts\aa-record.bat")ff
	if (argc > 1) {
		ParseCommandLine(argc, argv);
	}

	// Is a sender running ?
	if (receiver.GetActiveSender(g_SenderName)) {
		bActive = true;
	}

	// Show the sender
	if (bActive)
		printf("SpoutRecorder : [%s]\n\n", g_SenderName);
	else
		printf("SpoutRecorder : no sender active\n");

	// Show key commands
	printf("F1    - start recording\nF2    - record with audio\nESC   - stop recording\nQ     - stop and quit\n");
	printf("Hot Keys (always detected)\n");
	printf("ALT+T - toggle topmost\nALT+Q - stop and quit\n");

	// Register Quit HotKey (ALT+Q)
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey?redirectedfrom=MSDN
	RegisterHotKey(NULL, 1, MOD_NOREPEAT | MOD_ALT, 0x51); // 0x51 is 'q'

	// Register Topmost HotKey (ALT+T)
	RegisterHotKey(NULL, 2, MOD_NOREPEAT | MOD_ALT, 0x54); // 0x51 is 't'
	
	MSG msg ={0};
	do {
		
		// Monitor windows messages to look for HotKeys
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_HOTKEY) {
				// ALT+Q - stop and quit
				if (msg.wParam == 1) {
					StopFFmpeg();
					bExit = true;
				}
				// ALT+T - toggle topmost
				if (msg.wParam == 2) {
					if (bTopMost) {
						SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
						bTopMost = false;
					}
					else {
						SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
						SetForegroundWindow(g_hWnd);
						bTopMost = true;
					}
					ShowWindow(g_hWnd, SW_SHOWNORMAL);
				}
			}
		}

		// Monitor keyboard input - F1 / F2 / Q / ESC
		if (_kbhit()) {
			switch (_getch()) {
				// F1 to start recording
				// F2 for system audio with video
				case 0x3C: // F2
					bAudio = true;
				case 0x3B: // F1
					// Not topmost
					SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW);
					bTopMost = false;
					// If a sender is active, start encodng
					if (receiver.GetActiveSender(g_SenderName)) {
						printf("\nRecording [%s]\n", g_SenderName);
						bStart = true;
						StartFFmpeg();
					}
					else {
						printf("Start a sender to record\n");
					}
					break;

				// Q to quit
				case 0x71:
					bExit = true;
				// 'ESC' to stop recording
				case 0x1B:
					if (m_FFmpeg) {
						StopFFmpeg();
						bStart = false;
						SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
						printf("F1    - start recording\nF2    - record with audio\nESC   - stop recording\nQ     - stop and quit\n");
						printf("Hot Keys (always detected)\n");
						printf("ALT+T - toggle topmost\nALT+Q - stop and quit\n");
					}
					break;
				// Function keys can produce an extra 0 with getch()
				case 0:
				default:
					break;
			}
		}

		// Receive from a sender
		Receive();

	} while (!bExit);

	// Stop encoding if still active
	if (m_FFmpeg) StopFFmpeg();
	// Close receiver and free resources
	receiver.ReleaseReceiver();
	// Unregister ALT-Q hotkey
	UnregisterHotKey(NULL, 1);
	// Unregister ALT-T hotkey
	UnregisterHotKey(NULL, 2);
	// Close the console window
	PostMessage(g_hWnd, WM_CLOSE, 0, 0);

	return 0;
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
					bPrompt = false;
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

		/*
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
		*/
	}

}


void Receive()
{
	// For testing
	// StartTiming();

	// Get pixels from the sender shared texture.
	// ReceiveImage handles sender detection, creation and update.
	// If bRgb is true, receive an RGB data buffer
	if (receiver.ReceiveImage(pixelBuffer, g_SenderWidth, g_SenderHeight, bRgb)) {

		// IsUpdated() returns true if the sender has changed
		if (receiver.IsUpdated()) {

			// Stop FFmpeg if already encoding and the stream size has changed.
			// Return the user to the start.
			if (bEncoding) {
				printf("Sender size changed - stopping FFmpeg\n");
				StopFFmpeg();
				bEncoding = false;
				bStart = false;
				bExit = true;
			}

			// Update the sender name - it could be different
			strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());

			// Update globals
			g_SenderWidth  = receiver.GetSenderWidth();
			g_SenderHeight = receiver.GetSenderHeight();

			// Update the receiving buffer
			if (pixelBuffer) delete[] pixelBuffer;
			if(bRgb)
				pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 3];
			else
				pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 4];

			bActive = true;

			// Start FFmpeg for F1 or the command line "-start" argument
			if (bStart)
				StartFFmpeg();
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
	}
	else {
		// No sender found or sender has closed
		// If FFmpeg is encoding, stop and exit
		if (bEncoding) {
			printf("Sender closed - stopping FFmpeg\n");
			StopFFmpeg();
			bStart = false;
			bExit = true;
		}
	}

	// For testing
	// double elapsed = EndTiming();
	// printf("%.3f\n", elapsed);

	// Limit input frame rate for FFmpeg
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
	// printf("%s\n", g_OutputFile.c_str());

	// FFmpeg location
	char ffmpegPath[MAX_PATH];
	GetModuleFileNameA(NULL, ffmpegPath, MAX_PATH);
	PathRemoveFileSpecA(ffmpegPath);
	strcat_s(ffmpegPath, "\\DATA\\FFMPEG");
	std::string outputString = ffmpegPath;

	// Does ffmpeg.exe exist there ?
	outputString += "\\ffmpeg.exe";
	// printf("FFmpeg location\n%s\n", outputString.c_str());
	if (_access(outputString.c_str(), 0) == -1) {
		MessageBoxA(NULL, "FFmpeg not found\nRefer to the documentation", "Warning", MB_OK | MB_TOPMOST);
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
		if (bPrompt) {
			// Show the user the saved file details for 4 seconds
			char tmp[MAX_PATH];
			sprintf_s(tmp, MAX_PATH, "Saved [%s]", g_OutputFile.c_str());
			SpoutMessageBox(NULL, tmp, "Spout recorder", MB_OK | MB_TOPMOST | MB_ICONINFORMATION, 4000);
		}
	}
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// CTRL-CLOSE: when the console is closed by the user
		case CTRL_CLOSE_EVENT:
			// Stop recording
			if (m_FFmpeg) StopFFmpeg();
			// Close receiver and free resources
			receiver.ReleaseReceiver();
			// Unregister ALT-Q hotkey
			UnregisterHotKey(NULL, 1);
			// Unregister ALT-T hotkey
			UnregisterHotKey(NULL, 2);
			return TRUE;
		default:
			return FALSE;
	}
}

// The end ..
