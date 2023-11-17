rem
rem Starts "play-zmq.bat" without showing the FFplay console window
rem There is a delay for ZMQ discovery and start so wait for it
rem
Set oShell = CreateObject ("Wscript.Shell") 
Dim strArgs
strArgs = "cmd /c play-zmq.bat"
oShell.Run strArgs, 0, false


