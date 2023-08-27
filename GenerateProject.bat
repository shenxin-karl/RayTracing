xmake project -k vsxmake -y Solution
xmake project -k compile_commands

@echo off
set source=.clang-format
set dest=Solution

for /D %%d in (%dest%\*) do (
    xcopy %source% "%%d\" /Y
)