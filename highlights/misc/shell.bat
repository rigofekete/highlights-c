@echo off

REM this line calls the vs batch in order for us to use the cl compiler in our shell 
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM sets the misc folder of our project to the system environment paths 
set path=c:\dev\highlightsc\highlights\misc;%path%

REM opens the shell in the following directory 
cd \dev\highlightsc\highlights\code