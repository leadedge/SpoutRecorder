@echo off
rem
rem Record system audio to a file
rem
echo.
echo RECORD SYSTEM AUDIO WITH FFMPEG
echo.
echo Enter a file name to start recording
echo Press "q" to stop
echo The file is saved in the "Audio" folder
echo.
set /P audiofile=Audio file name (wihout extension): 
if "%audiofile%"=="" goto Error
rem
..\\FFMPEG\\ffmpeg -f dshow -i audio="virtual-audio-capturer" "..\\Audio\\%audiofile%.mp3"
rem
goto End

:Error
echo Incorrect entry..
:End

