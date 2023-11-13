//
// Class: spoutRecord
//
// Functions for recording with FFmpeg using a stdout pipe
//
// =========================================================================
//
// Revisions
//		26-08-23 - Initial class
//		01.09.23 - Add quality and preset
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
#include "spoutRecord.h"


spoutRecord::spoutRecord() {
	m_FFmpeg = nullptr;
	m_pixelBuffer = nullptr;
	m_nBytes=0;
}

spoutRecord::~spoutRecord() {
	if (m_FFmpeg) Stop();
	if (m_pixelBuffer)
		delete[] m_pixelBuffer;
	if (m_hConsole)
		FreeConsole();

}


// -----------------------------------------------
// Start FFmpeg
//
//    ffmpegPath - full FFmpeg path
//    OutputFile - full output file path
//    width      - image width
//    height     - image height
//    bRgba      - rgba data (default bgra)
//
bool spoutRecord::Start(std::string ffmpegPath, std::string OutputFile,
	unsigned int width, unsigned int height, bool bRgba)
{
	// No FFmpeg
	if (ffmpegPath.empty() || OutputFile.empty()) {
		return false;
	}

	// Stop if already recording
	if (m_FFmpeg) Stop();

	// OpenGL context required
	// DirectX device is created by 
#ifdef SPOUTGL
	if (!wglGetCurrentContext()) {
		SpoutMessageBox("OpenGL context required", MB_OK | MB_ICONWARNING);
		return false;
	}
#endif

	std::string str = ffmpegPath; // FFmpeg.exe full path

	//
	// FFmpeg argument string
	//
	//  -hwaccel auto -threads 0 -thread_queue_size 4096 -y
	//    Audio
	//  -f dshow -i audio=\"virtual-audio-capturer\" -thread_queue_size 512
	//    or default
	//  -an
	//  -f rawvideo -vcodec rawvideo -pix_fmt bgra -s widthxheight -r 30
	//  -i - 
	//    User args for codec, for example x264
	//  -vcodec libx264 -preset superfast -tune zerolatency -crf 23";
	//    or default
	//  -vcodec mpeg4 -q:v 5
	//  -shortest (for audio)

	// Hardware encode if available
	std::string args = " -hwaccel auto";

	// Maximum threads
	args += " -threads 0";

	// -thread_queue_size
	// This option sets the maximum number of queued packets when reading from the file or device. 
	// With low latency / high rate live streams, packets may be discarded if they are not read in a
	// timely manner; setting this value can force ffmpeg to use a separate input thread and read
	// packets as soon as they arrive. By default ffmpeg only do this if multiple inputs are specified.
	// TODO : optimize ?
	// There appears to be no documentation on what value should be set
	args += " -thread_queue_size 4096";

	// Overwrite output files without asking
	args += " -y";

	// Record system audio using the directshow audio capture device
	// (Option set by "aa-record.bat" batch file or by F2 to start)
	// thread_queue_size must be set on this input as well
	if (m_bAudio)
		args += " -f dshow -i audio=\"virtual-audio-capturer\" -thread_queue_size 512";
	else
		args += " -an";

	// Input raw video
	args += " -f rawvideo -vcodec rawvideo";

	// Input pixel format bgra (RGBA pixels)
	// rgba/bgra is faster than rgb24/bgr24 due to SSE optimized pixel copy from the sender texture
	if(bRgba)
		args += " -pix_fmt rgba";
	else
		args += " -pix_fmt bgra";

	// Size of frame
	args += " -s ";
	args += std::to_string(width);
	args += "x";
	args += std::to_string(height);

	// FFmpeg input frame rate must be the same as
	// the video frame output rate (see HoldFps)
	args += " -r ";
	args += std::to_string(m_FPS);

	// The final “-” tells FFmpeg to write to stdout
	args += " -i - ";

	// Codec options
	if (m_FFmpegCodec.empty()) {
		if (m_codec == 1) { // h264
			// Example
			// " -vcodec libx264 -preset ultrafast -tune zerolatency -crf 23"; // 7,098
			args += " -vcodec libx264";
			// 0 - ultrafast, 1 - superfast, 2 - veryfast, 3 - faster
			if (m_Preset == 0)
				args += " -preset ultrafast";
			else if (m_Preset == 1)
				args += " -preset superfast";
			else if (m_Preset == 2)
				args += " -preset veryfast";
			else
				args += " -preset faster";

			// Tune
			args += " -tune zerolatency";

			// Quality
			//    0 - low (crf 28)
			//    1 - medium (crf 23)
			//    2 - high (crf 18)
			if (m_Quality == 0)
				args += " -crf 28";
			else if (m_Quality == 1)
				args += " -crf 23";
			else
				args += " -crf 18";
			m_FileExt = "mkv";
		}
		else {
			// Default FFmpeg “mpeg4” encoder (MPEG-4 Part 2 format)
			// https://trac.ffmpeg.org/wiki/Encode/MPEG-4
			args += " -vcodec mpeg4 -qscale:v 3"; // high quality
			m_FileExt = "mp4";
		}
	}
	else {
		args += m_FFmpegCodec;
	}

	// Necessary for adding an audio stream
	if (m_bAudio)
		args += " -shortest";

	str += args; // add FFmpeg arguments

	// Find the last '.' char
	size_t pos = OutputFile.rfind(".");
	// Is it 3 from the end ?
	int extsize = (int)(OutputFile.size()-pos-1);
	printf("Extension size (%d)\n", extsize);
	if (extsize == 3) {
		// Strip existing extension
		if (pos != std::string::npos)
			OutputFile = OutputFile.substr(0, pos+1);
		// Add codec extension
		OutputFile += m_FileExt;
		str += " \""; // Insert a space and double quote before the output file
		str += OutputFile; // Output file full path
		str += "\""; // Final double quote
	}
	else {
		str += " ";
		str += OutputFile; // User defined output
	}

	// _popen for FFmpeg will open a console window.
	// To hide the output, open a console first and then hide it.
	// An application can have only one console window.
	// If one exists, leave management to the application.
	if (!GetConsoleWindow()) {
		if (AllocConsole()) {
			FILE* pCout = nullptr;
			freopen_s(&pCout, "CONOUT$", "w", stdout);
		}
		m_hConsole = GetConsoleWindow();
		if (m_hConsole) {
			ShowWindow(m_hConsole, SW_HIDE);
			ShowWindow(m_hConsole, SW_MINIMIZE);
			ShowWindow(m_hConsole, SW_HIDE);
		}
	}

	// Open pipe to ffmpeg's stdin in binary write mode.
	// The STDIO lib will use block buffering when talking to
	// a block file descriptor such as a pipe.
	m_FFmpeg = _popen(str.c_str(), "wb");
	if(!m_FFmpeg) printf("_popen failed\n");

	// Allow FFmpeg to start
	Sleep(10);

	return (m_FFmpeg != nullptr);
}


// -----------------------------------------------
// Stop FFmpeg encoding and free resources
//
void spoutRecord::Stop()
{
	if (!m_FFmpeg)
		return;

	// Flush FFmpeg
	fflush(m_FFmpeg);
	// FFmpeg pipe is still open so close it
	_pclose(m_FFmpeg);
	m_FFmpeg = nullptr;

	// Allow FFmpeg to finish
	Sleep(10);

	if (m_pixelBuffer) delete[] m_pixelBuffer;
	m_pixelBuffer = nullptr;

	if (m_hConsole)
		FreeConsole();
	m_hConsole = nullptr;
	
}

#ifdef SPOUTGL
// -----------------------------------------------
// Write pixels of an OpenGL texture image frame
//
bool spoutRecord::Write(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height)
{
	unsigned int nBytes = width*height*4;
	if (m_pixelBuffer) {
		// Stop if the size changed
		if (m_nBytes != nBytes) {
			Stop();
			delete[] m_pixelBuffer;
			m_pixelBuffer = nullptr;
			m_nBytes = 0;
			return false;
		}
	}
	else {
		m_pixelBuffer = new unsigned char[nBytes];
		m_nBytes = nBytes;
	}

	// Extract pixels from the OpenGL texture
	if (!spout.UnloadTexturePixels(TextureID, TextureTarget, width, height,
		width*4, m_pixelBuffer, GL_RGBA, false)) {
		printf("UnloadTexturePixels failed\n");
		return false;
	}

	// FFmpeg write
	return Write(m_pixelBuffer, nBytes);

}

#else

// -----------------------------------------------
// Set the D3D11 device for recorder functions
//
bool spoutRecord::OpenDirectX(ID3D11Device* pDevice)
{
	// Use of create a DirectX 11.0 device
	return spout.OpenDirectX11(pDevice);
}

// -----------------------------------------------
// Write pixels of an D3D11 texture image frame
//
bool spoutRecord::Write(ID3D11Texture2D* pTexture)
{
	// Get texture size
	D3D11_TEXTURE2D_DESC desc={};
	pTexture->GetDesc(&desc);
	unsigned int nBytes = desc.Width*desc.Height*4;

	if (m_pixelBuffer) {
		// Stop if the size changed
		if (m_nBytes != nBytes) {
			Stop();
			delete[] m_pixelBuffer;
			m_pixelBuffer = nullptr;
			m_nBytes = 0;
			return false;
		}
	}
	else {
		m_pixelBuffer = new unsigned char[nBytes];
		m_nBytes = nBytes;
	}

	// Extract the texture pixels via staging textures
	if (spout.ReadTexurePixels(pTexture, m_pixelBuffer)) {
		// FFmpeg write
		return Write(m_pixelBuffer, nBytes);
	}

	return false;

}
#endif

// -----------------------------------------------
// Write pixels
//
bool spoutRecord::Write(unsigned char* pixelBuffer, unsigned int nBytes)
{
	if (!m_FFmpeg)
		return false;

	unsigned char* buffer = pixelBuffer;

	// No buffer specified
	if (!buffer) {
		// Create the receiving buffer
		if (m_pixelBuffer) {
			// Stop if the size changed
			if (nBytes != m_nBytes) {
				Stop();
				delete[] m_pixelBuffer;
				m_pixelBuffer = nullptr;
				m_nBytes = 0;
				return false;
			}
		}
		else {
			m_pixelBuffer = new unsigned char[nBytes];
			m_nBytes = nBytes;
		}
		buffer = m_pixelBuffer;
	}

	// Limit input frame rate for FFmpeg to the video frame rate
	spout.HoldFps(m_FPS);

	if (fwrite((const void*)buffer, 1, nBytes, m_FFmpeg) > 0)
		return true;

	return false;
}

// -----------------------------------------------
// Return encoding status
//
bool spoutRecord::IsEncoding()
{
	return (m_FFmpeg != nullptr);
}

// -----------------------------------------------
// Enable / disable system audio recording
//
void spoutRecord::EnableAudio(bool bAudio)
{
	m_bAudio = bAudio;
}

// -----------------------------------------------
// Set codec
//    0 - mpeg4, 1 - x264
//
void spoutRecord::SetCodec(int codec)
{
	m_codec = codec;
}

// -----------------------------------------------
// Set user defined codec string for FFmpeg
//
void spoutRecord::SetCodec(std::string codecString)
{
	m_FFmpegCodec = codecString;
}

// -----------------------------------------------
// Set usre defined container extension for output
//
void spoutRecord::SetExtension(std::string extension)
{
	m_FileExt = extension;
}

// https://trac.ffmpeg.org/wiki/Encode/H.264

// -----------------------------------------------
// Set x264 preset
//    0 - ultrafast, 1 - superfast, 2 - veryfast, 3 - faster
//
void spoutRecord::SetPreset(int preset)
{
	m_Preset = preset;
}

// -----------------------------------------------
// Set x264 constant rate factor - CRF
//    0 - 51
//    Default - 23
//    Visually lossless - 18
//
void spoutRecord::SetRate(int rate)
{
	m_Crf = rate;
}

// -----------------------------------------------
// Set x264 quality (crf)
//    0 - low (crf 28)
//    1 - medium (crf 23)
//    2 - high (crf 18)
//
void spoutRecord::SetQuality(int quality)
{
	m_Quality = quality;
	if(quality == 0)
		m_Crf = 28; // Low
	else if (quality == 1)
		m_Crf = 23; // Medium
	else
		m_Crf = 18; // High
}


// -----------------------------------------------
// Set frame rate for FFmpeg output
//    Producing rate should be the same
//
void spoutRecord::SetFps(int fps)
{
	m_FPS = fps;
}


