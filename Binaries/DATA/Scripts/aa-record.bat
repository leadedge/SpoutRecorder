@echo off
rem
rem Command line options for SpoutRecorder
rem These must be individual arguments, enclosed in double quotes, and precede FFmpeg options.
rem
rem -start
rem Start encoding as soon as a sender is detected
rem
rem -prompt
rem Prompt user for file name dialog
rem Default is no prompt. A video file with the sender name is saved in DATA\Videos.
rem
rem -ext
rem Video file name extension required by the selected codec (default "mp4)
rem Example for matroska codec
rem -ext:mkv
rem
rem Command line examples
rem
rem Start normally with user key input F1 to start and ESC to stop encoding
rem "..\..\SpoutRecorder.exe"
rem
rem Prompt user for destination file name and folder before encoding the video
rem "..\..\SpoutRecorder.exe" "-prompt"
rem
rem Start encoding as soon as a sender is detected and stop when it is closed
rem "..\..\SpoutRecorder.exe" "-start"
rem
rem ==============================================================================================
rem
rem FFmpeg options
rem
rem The default codec is FFmpeg "mp4". It is fast and could suffice for most purposes.
rem However, for more control over codecs etc., other options can be specifed as required.
rem
rem For example, x264 encoding can be used for more control over the output.
rem Is specified alone, the encoding speed is insufficient to keep pace with the video data.
rem The most effective option to improve encoding speed is preset tune for "zerolatency".
rem Other x264 options can also be included. For example, the frame rate can be changed if required.
rem
rem "..\..\SpoutRecorder.exe"  " -vcodec libx264 -preset fast -tune zerolatency -crf 23 -r 30"
rem
rem -preset
rem Preset for x264 encoding - fast, veryfast, ultrafast (default "veryfast")
rem Priority is for fast encoding rather than compression and file size.
rem medium, slow, slower, veryslow are not the best for live stream encoding.
rem
rem -tune
rem tune for x264 encoding (default "zerolatency", disable "none")
rem none, film, animation, grain, stillimage, psnr, ssim, fastdecode, zerolatency
rem Priority is fast encoding rather than effects.
rem "none" disables tuning but reduces speed
rem "animation" could be useful for graphical content but reduces speed
rem
rem -crf
rem CRF value for x264 encoding, controls quality/size (default 23, 18 visually lossless)
rem
rem -r
rem Frame rate e.g. 60, 30 or 25 (default 30)
rem
rem OTHER CODECS
rem rem https://ffmpeg.org/ffmpeg-codecs.html
rem Not all codecs are suitable for high speed encoding but the following have proved successful
rem
rem prores (high quality, large file size)
rem "..\..\SpoutRecorder.exe" "-ext mov" " -vcodec prores"
rme
rem x265 (equivalent to x264)
rem crf value controls quality/file size - default 28 equivalent to x264 23
rem https://trac.ffmpeg.org/wiki/Encode/H.265
rem "..\..\SpoutRecorder.exe" "-ext mkv" " -movflags faststart -vcodec libx265 -preset veryfast -tune zerolatency -crf 28 -r 30"
rem
rem wmv (legacy)
rem "..\..\SpoutRecorder.exe" "-ext wmv" "-vcodec wmv2 -b:v 1024k"
rem


