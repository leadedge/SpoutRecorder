# SpoutRecorder

A Windows console application to encode Spout input to a video file using FFmpeg by way of a pipe.
Resolution and speed are improved over using SpoutCam as a source within FFmpeg.

Designed as a simple recorder and for those who are familiar with FFmpeg command line arguments. It is also an example of receiving pixels from a Spout source using a basic console program without dependency on OpenGL or DirectX or even a window.

A solution without dependency on FFmpeg has been developed by [LightJams](https://www.lightjams.com/spout-recorder.html).\
[OBS studio](https://obsproject.com/) can also produce video by way of the [Off World Live plugin](https://github.com/Off-World-Live/obs-spout2-plugin).

The program can be started directly or with command line arguments.\
A batch file "aa-record.bat" located in the "\DATA\Scripts" folder can be
run directly or from a command console or activated from a program
using ShellExecute of similar. Edit the file for documentation
and to change the options.

After build, SpoutRecorder.exe is copied to the "Binaries" folder.\
Videos are saved to "\DATA\Videos". If you want to add audio, copy the audio file you want
to "\DATA\Audio" and use ""aa-audio.bat" in the "\DATA\Scripts" folder.
It's best to run this from a command prompt console so that you can see any see any errors.
The combined "out" file can be found in the "\DATA\Scripts" folder

FFmpeg.exe is required in "\DATA\FFMPEG".

* Go to https://github.com/GyanD/codexffmpeg/releases
* Choose the "Essentials" build. e.g. "ffmpeg-6.0-essentials_build.zip" and download the archive.
* Unzip the archive and copy bin\FFmpeg.exe to : &nbsp;\DATA\FFMPEG

FFmpeg encoding speed is shown in the console window. You should see a speed
of 1.0 if the encoding is keeping pace with the input frame rate. The default encoding framerate
is 30fps and can be changed with the batch file options. Any of the FFmpeg options can be changed
or added within the batch file.

The project is for Visual Studio 2022 and uses SpoutDX support class. Files from the beta development branch
are included in the project and can be [updated as required](https://github.com/leadedge/Spout2).

For development, copy the complete DATA folder to the folder containing the executable file.\
This may be for example, "..\x64\Release".
