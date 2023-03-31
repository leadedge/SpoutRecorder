VIRTUAL AUDIO DEVICE

A virtual audio device that captures what you hear from the speakers, developed by Roger Pack.

https://github.com/rdp/virtual-audio-capture-grabber-device

The device is a DirectShow filter and can be used with FFmpeg to record the audio.

VirtualAudioRegister.exe can be used to register or un-register the filters.\
This has Administrator privileges and Windows UAC might intercept, but just allow it.

If "virtual-audio-capturer" has not been registered, click the 'Register' button.\ 
This will register both 32 bit and 64 bit versions.\
After confirmation of success, the button will show 'UnRegister'.\
Click 'Unregister' to remove it from the system.

To update to new dll files, click 'Unregister' and then 'Register' again.
				
You can also register manually.\
There are separate folders for 32 bit and 64 bit versions and you can install both on a 64 bit system.\
In each folder you will find "_register_run_as_admin.bat".\
RH click and "Run as Administrator" to register.\
Register the 32 bit version first.


