# wz-lobbywatcher

Small script that shows up Warzone2100 lobby to stdout. (and more)

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
