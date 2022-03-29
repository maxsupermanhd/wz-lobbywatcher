# wz-lobbywatcher

Small script that shows up Warzone2100 lobby to stdout. (and more)

## Get sources

Clone github repository and initialize submodules.\
`git clone https://github.com/maxsupermanhd/wz-lobbywatcher.git && cd wz-lobbywatcher`

**Downloading zip from github will not include submodule, without it you will be unable to build lobbywatcher!**

## Compile

Create build directory, configure cmake project, build it.\
`mkdir build && cd build && cmake .. && cmake --build .`

## Run

Execure built binary in build directory called `wz-lobbywatcher`

## Arguments

`-help` and `--help` show help page

`-t <delay>` set *delay*

`-w <timeout>` set timeout to lobby connection

`-notify` for sending notifications about new rooms

`-n-nopass` for not sending notifications about new rooms that are password protected

`-c` for compact mode
