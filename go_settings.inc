#!/bin/bash
# Copyright (c) 2019, Stein Gunnar Bakkeby, all rights reserved.
#
# Optional configuration settings for the go script. There is no
# reason to source this unless you actually intend on changing
# anything.
#
# This is more meant as a reference documenting the default values
# used by the script.

typeset -Ag _GOSETTINGS

# Print settings
_GOSETTINGS[help_format]="  %-31s%s\n"; # controls go -h layout
_GOSETTINGS[list_format]="%-17s%s%-10s\n"; # controls go -l layout
_GOSETTINGS[ext_scripts]=0 # external scripts requires -e or --ext flag, set this to 1 to enable them by default
_GOSETTINGS[symbol_tmp]="ᴛ"
_GOSETTINGS[symbol_pin]="🖈"
_GOSETTINGS[symbol_rem]="ʀ"
_GOSETTINGS[symbol_dir]="‒"

# Evaluation settings
_GOSETTINGS[filestore]=${HOME:-~}/.go.db
_GOSETTINGS[cmd]="ls -a --color=auto"; # command to run after changing directory