@echo off
rem
rem Command line options for SpoutRecorder
rem
rem These overiride the options selected in the SpoutRecorder console window
rem They must be individual arguments, enclosed in double quotes, and precede FFmpeg options.
rem
rem "-start" - start encoding as soon as a sender is detected
rem
rem "-hide" - hide console window on start
rem A flashing program icon is visible on the taskbar and can be restored and minimized again as required
rem
rem "-rgb" - RGB pixel data instead of rgba (default false)
rem Using rgba pixel data is approximately twice as fast due to optimized 
rem SSE data copy from the Spout sender texture.
rem
rem "-audio" - record desktop audio using directshow "virtual-audio-device"
rem https://github.com/rdp/virtual-audio-capture-grabber-device
rem "virtual-audio-device" must be registered'
rem You may see an FFmpeg warning "Guessed Channel Layout". Ignore it.
rem Default no audio is produced.
rem
rem "-ext" - video file name extension required by the selected codec (default "mp4)
rem Example for matroska codec "-ext mkv"
rem
rem "-name" - output file name. Full path or user defined.
rem
rem "-prompt" - prompt user for file name dialog
rem Default a video file with the sender name is saved in DATA\Videos.
rem
rem -args - additional arguments for FFmpeg
rem
rem CODECS
rem
rem The default codec is FFmpeg "mp4". It is fast and could suffice for most purposes.
rem h264 encoding can be used for more control over the output.
rem Tune "zerolatency" is used to improve encoding speed.
rem
rem "-mpeg4" - use mp4 codec
rem
rem "-h264" - use h264 codec
rem H264 options
rem h264 options for quality and preset are combined on one line
rem Example : "-h264 -quality medium -preset veryfast"
rem
rem h264 Quality
rem "-quality low"
rem "-quality medium"
rem "-quality high"
rem
rem h264 preset
rem "-preset ultrafast"
rem "-preset superfast"
rem "-preset veryfast"
rem "-preset faster"
rem 
rem OTHER CODECS
rem rem https://ffmpeg.org/ffmpeg-codecs.html
rem "-codec" - user specified FFmpeg codec string
rem "-codec none" use FFmpeg default
rem
rem Not all codecs are suitable for high speed encoding
rem Examples :
rem "-codec -vcodec libx264 -preset fast -tune zerolatency -crf 23 -r 30"
rem "-codec " -vcodec prores" "-ext mov" 
rem "-codec -movflags faststart -vcodec libx265 -preset veryfast -tune zerolatency -crf 28 -r 30" "-ext mkv"
rem "-codec -vcodec wmv2 -b:v 1024k" "-ext wmv"
rem
rem USER ARGUMENTS
rem Additional FFmpeg arguments can be added
rem double quotes must be escaped
rem Example
rem "-args -vf \"eq=contrast=1.50:saturation=1.50\""
rem
rem
rem EXAMPLES
rem
rem Start normally with user key input F1 to start and ESC to stop encoding
"..\..\SpoutRecorder.exe"
rem
rem Prompt user for destination file name and folder before encoding the video
rem "..\..\SpoutRecorder.exe" "-prompt"
rem
rem Start encoding as soon as a sender is detected and stop when it is closed
rem "..\..\SpoutRecorder.exe" "-start"
rem
rem Start encoding as soon as a sender is detected
rem hide the recorder windows and top when the sender is closed
rem "..\..\SpoutRecorder.exe" "-start"
rem
rem Encode video together with desktop audio
rem "..\..\SpoutRecorder.exe" "-audio"
rem
rem ==============================================================================================
rem
rem ZeroMQ streaming (under development)
rem https://ffmpeg.org/ffmpeg-all.html#zmq
rem https://ffmpeg.org/ffmpeg-all.html#zmq_002c-azmq
rem https://github.com/FFmpeg/FFmpeg/blob/ef43a4d6b38de941dd2ede0711d4fd5d811127ed/doc/protocols.texi
rem
rem "..\..\SpoutRecorder.exe" "-start" "-name -f mpegts zmq:tcp://127.0.0.1:5555"
rem "..\..\SpoutRecorder.exe" "-start" "-h264" "-name -f mpegts zmq:tcp://127.0.0.1:5555"
rem "..\..\SpoutRecorder.exe" "-start" "-codec none" "-name -f mpegts zmq:tcp://127.0.0.1:5555"
rem "..\..\SpoutRecorder.exe" "-start" "-hide" "-name -f mpegts zmq:tcp://127.0.0.1:5555"
rem
rem Can be played using another batch file
rem ffplay.exe zmq:tcp://127.0.0.1:5555
rem See play-zmq.bat
rem
rem Local network testing
rem "..\..\SpoutRecorder.exe" "-start" "-codec none" "-name -f mpegts zmq:tcp://192.168.1.104:5555"
rem "..\..\SpoutRecorder.exe" "-start" "-h264" "-name -f mpegts zmq:tcp://192.168.1.104:5555"

