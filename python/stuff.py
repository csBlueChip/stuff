# UNTESTED - requires X

import fcntl
import sys
import termios

with open('/proc/116919/fd/0', 'w') as fd:
    for char in "y\n":
        fcntl.ioctl(fd, termios.TIOCSTI, char)
