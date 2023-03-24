@echo off
rem
rem Command line options for SpoutRecorder
rem
rem -start
rem Start encoding as soon as a sender is detected
rem
rem -prompt
rem Prompt user for file name dialog
rem Default is no prompt. A video file with the sender name is saved in DATA\Videos.
rem
rem -x264
rem x264 encoding can be used if required for more control over the output.
rem Default encoding is the FFmpeg “mpeg4” encoder and is the fastest method.
rem
rem -preset
rem Preset for x264 encoding - fast, veryfast, ultrafast (default "veryfast")
rem Priority is for fast encoding rather than compression and file size.
rem medium, slow, slower, veryslow are not the best for live stream encoding.
rem Example -preset:veryfast
rem
rem -tune
rem Preset tune for x264 encoding (default "zerolatency", disable "none")
rem none, film, animation, grain, stillimage, psnr, ssim, fastdecode, zerolatency
rem Priority is fast encoding rather than effects.
rem "none" disables tuning but reduces speed
rem "animation" could be useful for graphical content but reduces speed
rem Example -tune:zerolatency
rem
rem -crf
rem CRF value for x264 encoding (default 23).
rem Example -crf:23
rem
rem -fps
rem Frame rate e.g. 60, 30 or 25 (default 30)
rem Example -fps:30
rem
rem Command line examples
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
rem Start with x264 options instead of default mp4 encoder
rem "..\..\SpoutRecorder.exe" "-start" "-x264" "-preset:veryfast" "-tune:zerolatency" "-crf:23" "-fps:30"
rem
