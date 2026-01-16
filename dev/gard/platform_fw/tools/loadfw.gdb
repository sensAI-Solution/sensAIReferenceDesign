# Loads firmware using command in VS Code Debug Console: -exec source ../tools/loadfw.gdb
monitor arm semihosting enable
monitor reset init
monitor reset halt
load
