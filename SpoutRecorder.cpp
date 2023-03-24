//
// SpoutRecorder.cpp
//
// Encodes Spout input to a video file using FFmpeg by way of a pipe
// Resolution and speed are improved over using SpoutCam as a source
// It's also possible to modify for rgba video if that's necessary.
//
//	  F1 to start recording, ESC to quit
//
// Uses the SpoutDX support class
// SpoutRecorder.exe is copied to the "Binaries" folder
// Command line options are available (DATA\Scripts folder)
// Command line option "-start" starts encoding as soon as a sender is found
//
// References :
// https://batchloaf.wordpress.com/2017/02/12/a-simple-way-to-read-and-write-audio-and-video-files-in-c-using-ffmpeg-part-2-video/
// https://developers.google.com/media/vp9/live-encoding
//
// =========================================================================
// 
// Copyright(C) 2023 Lynn Jarvis.
// 
// http://spout.zeal.co/
// 
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see < http://www.gnu.org/licenses/>.
// 
// ========================================================================

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include "SpoutDX\SpoutDX.h"

// Global Variables:
spoutDX receiver;                      // Receiver object
HANDLE g_hConsole = nullptr;           // Console window handle
unsigned char* pixelBuffer = nullptr;  // Receiving pixel buffer
char g_SenderName[256];                // Received sender name
unsigned int g_SenderWidth = 0;        // Received sender width
unsigned int g_SenderHeight = 0;       // Received sender height

// For FFmpeg recording
FILE* m_FFmpeg = nullptr; // FFmpeg pipe
std::string g_OutputFile; // Output video file
bool bActive = false;     // Sender is active
bool bEncoding = false;   // Encode in progress
bool bExit = false;       // User quit flag

// Command line arguments
// -prompt	User file name entry dialog
// -x264	x264 encoding (default mp4)
// -preset  fast, veryfast, ultrafast
// -tune    zerolatency (default), (or others)
// -crf:xx  CRF value for x624 (23 default)
// -fps:xx  Specify frame rate e.g. 60, 30 or 25
bool bStart  = false; // Command line start
bool bPrompt = false; // User file name entry dialog
bool bx264 = false;  // x624 encode
int g_CRF = 23;      // x264 CRF value
int g_FPS = 30;      // Output frame rate
std::string g_Preset = "veryfast";  // x624 preset
std::string g_Tune = "zerolatency"; // x264 tune

int main(int argc, char* argv[]);
void ParseCommandLine(int argc, char* argv[]);
void Receive();
void StartFFmpeg();
void StopFFmpeg();


int main(int argc, char* argv[])
{
	// Add a title to the console window to replace the full path
	HWND hwnd = GetConsoleWindow();
	SetWindowTextA(hwnd, "SpoutRecorder");

	// Show our text bright yellow - a different colour to FFmpeg
	// https://learn.microsoft.com/en-us/windows/console/setconsoletextattribute
	g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(g_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	// Parse command line arguments
	// "-start" - immediate start and end when sender closes
	if (argc > 1) {
		ParseCommandLine(argc, argv);
	}

	// Is a sender running ?
	if (receiver.GetActiveSender(g_SenderName)) {
		bActive = true;
	}

	// Console immediate start option 
	if (bStart) {
		ShowWindow(hwnd, SW_HIDE); // Prevent animated effect
		ShowWindow(hwnd, SW_SHOWMINIMIZED); // Taskbar
	}
	else {
		if (bActive) {
			printf("SpoutRecorder : [%s]\n", g_SenderName);
		}
		else {
			printf("SpoutRecorder : no sender active\n");
		}
		printf("F1 to record, ESC to quit\n");
	}

	// Monitor keyboard input
	do {
		if (_kbhit()) {
			switch (_getch()) {
				// Esc to stop encoding and quit
				case VK_ESCAPE:
					StopFFmpeg();
					bExit = true;
					break;
				// F1 to start recording
				// F2 for user prompt of output file name and folder
				case 0x3C: // F2
					bPrompt = true;
				case 0x3B:
					// If a sender is active, start encodng
					if (bActive) {
						bStart = true;
						StartFFmpeg();
					}
					else {
						printf("Start a sender before recording\n");
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

	return 0;
}


// Parse command line args.
void ParseCommandLine(int argc, char* argv[])
{
	// argv[0] is the executable name
	if (argc > 0) {
		for (int i = 1; i < argc; i++)
		{
			if (argv[i]) {
				std::string argstr = argv[i];
				if (strstr(argv[i], "-start") != 0) {
					// Command line immediate start
					bStart = true;
					bPrompt = false;
				}
				else if (strstr(argv[i], "-prompt") != 0) {
					// Prompt for file name entry
					bPrompt = true;
				}
				else if (strstr(argv[i], "-x264") != 0) {
					// x264 encoding
					bx264 = true;
				}
				else if (strstr(argv[i], "-preset") != 0) {
					// x264 preset
					size_t pos = argstr.find(":");
					argstr = argstr.substr(pos+1, argstr.size()-pos);
					g_Preset = argstr.c_str();
				}
				else if (strstr(argv[i], "-tune") != 0) {
					// x264 tune
					size_t pos = argstr.find(":");
					argstr = argstr.substr(pos+1, argstr.size()-pos);
					g_Tune = argstr.c_str();

				}
				else if (strstr(argv[i], "-crf") != 0) {
					// x264 CRF
					size_t pos = argstr.find(":");
					argstr = argstr.substr(pos+1, argstr.size()-pos);
					g_CRF = atoi(argstr.c_str());
				}
				else if (strstr(argv[i], "-fps") != 0) {
					// FPS
					size_t pos = argstr.find(":");
					argstr = argstr.substr(pos+1, argstr.size()-pos);
					g_FPS = atoi(argstr.c_str());
				}
			}
		}
	}
}


void Receive()
{
	// Get pixels from the sender shared texture.
	// ReceiveImage handles sender detection, creation and update.
	// Because Windows bitmaps are bottom-up, the rgb pixel buffer is flipped by the ReceiveImage function
	if (receiver.ReceiveImage(pixelBuffer, g_SenderWidth, g_SenderHeight, true, true)) { // RGB = true, invert = true

		// IsUpdated() returns true if the sender has changed
		if (receiver.IsUpdated()) {

			// Update the sender name - it could be different
			strcpy_s((char*)g_SenderName, 256, receiver.GetSenderName());

			// Update globals
			g_SenderWidth  = receiver.GetSenderWidth();
			g_SenderHeight = receiver.GetSenderHeight();

			// Update the rgb receiving buffer
			if (pixelBuffer) delete[] pixelBuffer;
			pixelBuffer = new unsigned char[g_SenderWidth * g_SenderHeight * 3];

			bActive = true;

			// Start FFmpeg for F1 or command line "-start" argument
			if (bStart)
				StartFFmpeg();
		}
		else if (pixelBuffer && m_FFmpeg) {

			if (bEncoding) {
				// Encode to video with the FFmpeg output pipe
				fwrite((const void*)pixelBuffer, 1, g_SenderWidth*g_SenderHeight*3, m_FFmpeg);
			}
			else {
				// FFmpeg is encoding, stop and exit
				StopFFmpeg();
				bExit = true;
			}
		}

		// Limit input frame rate for FFmpeg
		receiver.HoldFps(g_FPS);

	}
	else {
		// No sender found or sender has closed
		// If FFmpeg is encoding, stop and exit
		if (bEncoding) {
			StopFFmpeg();
			bExit = true;
		}
	}

}

void StopFFmpeg()
{
	if (m_FFmpeg) {
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		// ffmpeg file is still open so close it
		fflush(m_FFmpeg);
		_pclose(m_FFmpeg);
		m_FFmpeg = nullptr;
		// Allow FFmpeg to finish
		Sleep(10);
		bEncoding = false;
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		// Stop encoding with the Escape key
		if (bPrompt) {
			// Show the user the saved file details
			char tmp[MAX_PATH];
			sprintf_s(tmp, MAX_PATH, "Saved [%s]", g_OutputFile.c_str());
			MessageBoxA(NULL, tmp, "Spout recorder", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		}
	}
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
	g_OutputFile += exefolder;
	g_OutputFile += "\\DATA\\Videos\\";
	g_OutputFile += (char*)g_SenderName;
	g_OutputFile += ".mp4";

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// User file name entry
	//
	if (bPrompt) {

		// if ofn.lpstrFile contains a path, that path is the initial directory.
		char filePath[MAX_PATH];
		sprintf_s(filePath, MAX_PATH, g_OutputFile.c_str());

		OPENFILENAMEA ofn={};
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		HWND hwnd = NULL;
		ofn.hwndOwner = hwnd;
		ofn.hInstance = GetModuleHandle(0);
		ofn.nMaxFileTitle = 31;
		ofn.lpstrFile = (LPSTR)filePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = "Video files (*.mp4)\0*.mp4\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt = "";
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		ofn.lpstrTitle = "Output File";

		// OFN_OVERWRITEPROMPT prompts for over-write
		if (!GetSaveFileNameA(&ofn)) {
			// Stop FFmpeg and exit
			StopFFmpeg();
			bExit = true;
			return;
		}

		// User file name entry
		g_OutputFile = filePath;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// printf("%s\n", g_OutputFile.c_str());

	// FFmpeg location
	char ffmpegPath[MAX_PATH];
	GetModuleFileNameA(NULL, ffmpegPath, MAX_PATH);
	PathRemoveFileSpecA(ffmpegPath);
	strcat_s(ffmpegPath, "\\DATA\\FFMPEG");
	std::string outputString = ffmpegPath;

	// Does ffmpeg.exe exist there ?
	outputString += "\\ffmpeg.exe";
	printf("FFmpeg location\n%s\n", outputString.c_str());
	if (_access(outputString.c_str(), 0) == -1) {
		MessageBoxA(NULL, "FFmpeg not found\nRefer to the documentation", "Warning", MB_OK | MB_TOPMOST);
		return;
	}

	// Maximum threads
	outputString += " -threads 0";

	// Input frames bgr raw video
	outputString += " -y -f rawvideo -vcodec rawvideo -pix_fmt bgr24 -s ";

	// Size of frame
	outputString += std::to_string(g_SenderWidth);
	outputString += "x";
	outputString += std::to_string(g_SenderHeight);
	
	// Set the FFmpeg frame rate
	outputString += " -r ";
	outputString += std::to_string(g_FPS);

	// Final “-” tells FFmpeg to write to stdout
	outputString += " -i - ";

	// Disable audio
	outputString += " -an";

	// x264 Encoding option
	// Default mp4 format is the fastest method
	if(!bx264) {

		// MP4 output file format
		outputString += " -f mp4";

		// quality of the encoded MP4 file (1 = best, 31 = worse) 
		// 5 - best trade off between file size and quality
		outputString += " -q:v 5";

		// FFmpeg “mpeg4” encoder
		outputString += " -vcodec mpeg4";
	}
	else {

		// For a fast CPU, h264 or h256 can be used
		outputString += " -vcodec libx264";

		// Presets for x264
		// Slow presets are not suitable for stream encoding
		// High speed encoding, larger file size
		// fast
		// very fast (default)
		// utrafast
		outputString += " -preset ";
		outputString += g_Preset;

		// Quality
		// 18-25 recomended (23 default)
		// 18 is practically lossless
		outputString += " -crf ";
		outputString += std::to_string(g_CRF);

		// Tuning options after preset

		// -tune animation
		// Improves encode quality for animated content
		// Possibly better for generated graphics
		// --bframes {+2} --deblock 1:1
		// --psy-rd 0.4:<unset> --aq-strength 0.6
		// --ref{Double if >1 else 1}

		// -tune zerolatency
		// Optimization for fast encoding and low-latency streaming
		// Improves speed for x264 / x265
		// No lookahead, no B frames, no cutree
		// --bframes 0 --force-cfr --no-mbtree
		// --sync-lookahead 0 --sliced-threads
		// --rc-lookahead 0

		if (!g_Tune.empty() && g_Tune != "none") {
			outputString += " -tune ";
			outputString += g_Tune;
		}

	}

	// Flip bottom up bitmap 
	outputString += " -vf vflip ";
		
	outputString += "\"";
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

