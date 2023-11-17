## Command line

* "aa-record.bat" - record Spout using SpoutRecorder\
Options include command line arguments for FFmpeg. Edit the file to change options. 
The batch file can be run directly or from a command prompt console window
or activated from a program using ShellExecute of similar.

* "aa-audio.bat" - add audio from a file to a video

* "aa-record-audio.bat" - record system audio\
Requires the virtual-audio-device by Roger Pack
to be registered using "VirtualAudioRegister"

* "aa-dos.bat" - opens a command prompt window

* "zmq-play.bat" - play ZMQ output\
Uses FFplay (see "aa-record.bat" for ZMQ output option)

* "zmq-start.bat" - start zmq-play.bat without showing the FFplay console window\
There is a delay for ZMQ discovery and start so wait for it.



