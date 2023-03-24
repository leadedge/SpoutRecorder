# SpoutRecorder

A Windows console application to encode Spout input with FFmpeg.

Encodes to a video file using FFmpeg by way of a pipe.
Resolution and speed are improved over using SpoutCam as a source.
It's also possible to modify for rgba video if that's necessary.

  F1 to start recording, ESC to quit

The program can also be started from the command line.
A batch file "aa-record.bat" can be found in the "\DATA\Scripts" folder
and can be run directly or from a command prompt or activated from
a program using ShellExecute of similar. Edit the file for documentation
and to change the options.

After build, SpoutRecorder.exe is copied to the "Binaries" folder.\
Videos are saved to "\DATA\Videos".

The DATA folder must be within the folder containing the executable file.\
For development, this may be "..\x64\Release".

FFmpeg.exe is required in "\DATA\FFMPEG".

* Go to https://github.com/GyanD/codexffmpeg/releases
* Choose the "Essentials" build. e.g. "ffmpeg-6.0-essentials_build.zip" and download the archive.
* Unzip the archive and copy bin\FFmpeg.exe to : &nbsp;\DATA\FFMPEG

Using the FFmpeg executable for realtime encoding in this way is dependent on
CPU performance. For example, encoding of a high resolution Spout source, such as 4K,
may not keep up with the input data rate.

FFmpeg encoding speed is shown in the console window. You should see a speed
of 1.0 if the encoding is keeping pace with the input frame rate. The default encoding framerate
is 30fps and can be changed with the batch file options. Any of the FFmpeg options can be changed
or added within the source code for rebuild.

The project uses SpoutDX support class. Files from the beta development branch
are included in the project and can be [updated as required](https://github.com/leadedge/Spout2).
