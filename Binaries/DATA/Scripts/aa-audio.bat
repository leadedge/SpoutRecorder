@echo off
rem

rem
rem To add audio to a video file produced by SpoutRecorder
rem

echo.
echo.
echo ADD AUDIO TO VIDEO
echo.
echo Video file in the "Videos" folder
echo Audio file in the "Audio" folder
echo.

set /P videoname=Video file name (without extension) : 
if "%videoname%"=="" goto Error

set /P ext=Video type (e.g. mp4, avi, mov) : 
if "%ext%"=="" goto Error

set /P audiofile=Audio file (with extension) : 
if "%audiofile%"=="" goto Error

..\\FFMPEG\\ffmpeg -hwaccel auto -i "..\\Videos\\%videoname%.%ext%" -i "..\\Audio\\%audiofile%" -c:v copy -map 0:v -map 1:a -shortest -y "..\\Videos\\%videoname%_out.%ext%"

goto End

:Error
echo Incorrect entry..
:End
