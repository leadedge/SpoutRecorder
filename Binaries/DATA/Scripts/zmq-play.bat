@echo off
rem
rem ZeroMQ streaming (under development)
rem https://ffmpeg.org/ffmpeg-all.html#zmq
rem https://ffmpeg.org/ffmpeg-all.html#zmq_002c-azmq
rem https://github.com/FFmpeg/FFmpeg/blob/ef43a4d6b38de941dd2ede0711d4fd5d811127ed/doc/protocols.texi
rem
rem zmq:tcp://127.0.0.1:5555
rem
..\\ffmpeg\ffplay.exe -hide_banner -x 640 -y 360 zmq:tcp://127.0.0.1:5555
rem
rem Local network testing
rem ..\\ffmpeg\ffplay.exe -x 640 -y 360 zmq:tcp://192.168.1.104:5555
