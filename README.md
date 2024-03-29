# SpoutRecorder

SpoutRecorder is a Windows console application to encode Spout input to a video file using [FFmpeg](https://www.ffmpeg.org/) by way of a pipe. Resolution and speed are improved over using SpoutCam as a source within FFmpeg. The function of FFmpeg can be be adapted with additional command line arguments.

The program is also an example of receiving pixels from a Spout source using a basic console program without dependency on OpenGL or DirectX or even a window.

### FFmpeg

FFmpeg.exe is required in "DATA\FFMPEG".

* Go to https://github.com/GyanD/codexffmpeg/releases
* Choose the "Essentials" build. e.g. "ffmpeg-6.0-essentials_build.zip" and download the archive.
* Unzip the archive and copy bin\ffmpeg.exe to : &nbsp;DATA\FFMPEG

### Virtual audio filter

To record system audio together with the video, a virtual audio device developed by [Roger Pack](https://github.com/rdp/virtual-audio-capture-grabber-device) is used. The device is a DirectShow filter and can be used with FFmpeg to record the audio. Register it using "VirtualAudioRegister.exe" in the "AUDIO" folder.

### Keyboard operation

Recording
* F1 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - start recording
* F2/ESC &nbsp;&nbsp; - stop recording
* V &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - show video folder
* H &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - show help

Settings
* T &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - tompost
* F &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - prompt for file name
* A &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - system audio
* C &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - codec mpeg4/h264
* Q &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - h264 quality
* P &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - h264 preset
* R &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; - Reset

HotKeys (always active)
* ALT+F1 &nbsp; - start
* ALT+F2 &nbsp; - stop
* ALT+Q &nbsp;&nbsp; - stop and quit

Settings are saved in an initialization file on exit and recalled when the program opens again.

FFmpeg encoding speed is shown in the console window. You should see a speed of 1.0 if the encoding is keeping pace with the input frame rate. The default encoding framerate is 30fps and can be changed with the batch file options.

Videos are saved to "DATA\Videos" if a file name is not entered. 

### Command line operation

SpoutRecorder can be activated by way of a command line from a batch file or program using ShellExecute of similar.

* -start\
Start encoding as soon as a sender is detected.\
Default is F1 to start ESC to stop.
* -prompt\
Prompt user for file name dialog\
By default a file with the sender name is saved in DATA\Videos.
* -audio\
Record desktop audio using directshow\
You may see an FFmpeg warning "Guessed Channel Layout". Ignore it.\
By default, no audio is recorded.

A batch file "aa-record.bat", located in the "DATA\Scripts" folder. Edit the file for documentation and to change the options. Custom FFmpeg arguments can also be added from the batch file.

### Program communication

As well as execution by way of command line, programs can communicate by finding the "SpoutRecorder" window handle and setting the window caption. Recording status can be obtained by finding the "Recorder" window.

For example

&nbsp;&nbsp;&nbsp;&nbsp;HWND hRecord = FindWindowA(NULL, "SpoutRecorder"); // SpoutRecorder is open\
or\
&nbsp;&nbsp;&nbsp;&nbsp;HWND hRecord = FindWindowA(NULL, "Recording"); // SpoutRecorder is recording

&nbsp;&nbsp;&nbsp;&nbsp;if (hRecord)\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;SetWindowTextA(hRecord, "quit");

Caption commands are -

* "-select"\
Activate sender selection dialog
* "-close" or "-quit"\
Stop recording and close the program
* "-stop"\
Stop recording without closing the program
* "-start"\
Start recording while the program is open
* "-start -hide"\
Start recording and hide the program to the taskbar

### Building

The project is for Visual Studio 2022 and uses SpoutDX support class. Files from the beta development branch are included in the project and can be [updated as required](https://github.com/leadedge/Spout2).

After build, SpoutRecorder.exe is copied to the "Binaries" folder.

For development, copy the complete DATA and AUDIO folders to the folder containing the executable file.
This may be for example, "..\x64\Release".

### Options

* Add audio from a file to the video.

Copy the audio file required to "\DATA\Audio" and use "aa-audio.bat" in the "\DATA\Scripts" folder. It's best to run this from a command prompt console so that you can see any see any errors. The duration of the output file is the shortest of the video or audio. The combined "_out" file can be found in the "\DATA\Videos" folder

* Record audio only.

System audio can be recorded to file using "aa-record-audio.bat".

### Other recorders

Other than commercial software, a recorder has been developed by [LightJams](https://www.lightjams.com/spout-recorder.html).\
[OBS studio](https://obsproject.com/) can also produce video by way of the [Off World Live plugin](https://github.com/Off-World-Live/obs-spout2-plugin). 
