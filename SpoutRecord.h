//
// spoutRecord.h
//

#pragma once

#ifndef __spoutRecord__
#define __spoutRecord__

#include <windows.h>
#include <stdio.h>
#include <iostream>

// #define SPOUTGL

// Change path as necessary
#ifdef SPOUTGL
#include "..\..\..\SpoutGL\Spout.h"
#else
#include "SpoutDX\SpoutDX.h"
#endif


class spoutRecord {

public:

	spoutRecord();
	~spoutRecord();

	bool Start(std::string ffmpegPath, std::string OutputFile,
		unsigned int width, unsigned int height, bool bRgba = false);
	void Stop();
#ifdef SPOUTGL
	bool Write(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height);
#else
	bool OpenDirectX(ID3D11Device* pDevice = nullptr);
	bool Write(ID3D11Texture2D* pTexture);
#endif
	bool Write(unsigned char* pixelBuffer, unsigned int nBytes);
	bool IsEncoding();

	void EnableAudio(bool bAudio = true);
	void SetCodec(int codec = 0);
	void SetCodec(std::string codecString);
	void SetExtension(std::string extension);
	void SetPreset(int preset = 0); // 0 - ultrafast, 1 - superfast, 2 - veryfast, 3 - faster
	void SetQuality(int quality); // 0 - low, 1 - medium, 2 - high
	void SetRate(int rate = 23);

	void SetFps(int fps);

private:

#ifdef SPOUTGL
	Spout spout;
#else
	spoutDX spout;
#endif
	FILE* m_FFmpeg = nullptr; // FFmpeg pipe for recording
	FILE* m_pCout = nullptr;
	HWND m_hConsole = NULL;
	unsigned char* m_pixelBuffer = nullptr; // Receiving pixel buffer
	unsigned int m_nBytes=0;
	bool m_bAudio = false; // system audio
	int m_codec = 0; // 0 - mp4, 1 - h264
	int m_Preset = 0; // x264 preset
	int m_Quality = 0; // x264 quality
	int m_Crf = 23; // x264 rate factor (crf)
	int m_FPS = 30; // input and output frame rates must match
	std::string m_FFmpegCodec; // User FFmpeg codec args
	std::string m_FileExt = "mp4";

};

#endif
