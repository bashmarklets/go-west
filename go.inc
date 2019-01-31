#!/bin/bash
# Copyright (c) 2019, Stein Gunnar Bakkeby, all rights reserved.
# Source this file in your .bashrc/.zshrc, then run the function to
# begin. Alternatively you can rename this function to something
# more to your own liking. If you do then remember to also update
# the references for the tab completion at the bottom of this file.

go () {
	if [ $ZSH_VERSION ]; then
		local func_name=$funcstack[1]
	else
		local func_name=${FUNCNAME[0]}
	fi
	local filestore=${HOME:-~}/.${func_name}.db
	
	local row
	typeset -A row
	local tmpifs=$IFS
	if [[ -e "$filestore" ]]; then
		while IFS="" read -r line || [ -n "$line" ]; do
			if [[ -z "$line" ]]; then
				continue
			fi
			row[${line%%=*}]=${line#*=}
		done < "$filestore"
	fi
	IFS=$'\n'

	if [ $ZSH_VERSION ]; then
		local keys=($(echo ${(k@)row} | tr ' ' '\n' | sort))
	else
		local keys=$(echo ${!row[@]} | tr ' ' '\n' | sort)
	fi

	local dirty=0
	if [[ $# == 0 ]]; then
		set -- -h
	fi
	
	while (( $# )); do
		case "$1" in
		-a|--add)
			key=""
			case "$#" in
			1)
				val=$(pwd)
				shift
				;;
			2)
				if [[ -e $2 ]]; then
					val=$(realpath "$2")
				else
					val=$(pwd)
					key=$2
				fi
				shift 2
				;;
			*)
				if [[ -e $3 ]] || [[ $3 =~ "/" ]]; then
					val=$3
					key=$2
				else
					val=$2
					key=$3
				fi
				shift 3
				;;
			esac
			
			if [[ -d "$val" ]]; then
				val=$(realpath "$val")
			fi
			
			if [[ "$key" = "" ]]; then
				key=${val##*/}
				if [ $ZSH_VERSION ]; then
					key=$key:l
				else
					key=${key,,}
				fi
			fi
			
			# No support for spaces in bookmarks
			key=${key//[ ]/}
			
			if [[ -z "$key" ]]; then
				echo "Please specify a name to store this bookmark as"
			else
				row[$key]="$val"
				echo "$val added as $key"
				dirty=1
			fi
			;;
		-r|--remove)
			case "$#" in
			1)
				key=${$(pwd)##*/}
				shift
				;;
			*)
				key=$2
				shift 2
				;;
			esac
			
			if [[ -z ${row[$key]} ]]; then
				echo "Bookmark with key $key does not exist"
			else
				val=${row[$key]}
				unset "row[$key]"
				echo "key '$key' referring to $val removed"
				dirty=1
			fi
			;;
		--clear)
			if [[ -e "$filestore" ]]; then
				rm -f -- "$filestore"
			fi
			shift
			;;
		--purge)
			for key in $keys; do
				if [[ ! -e "${row[$key]}" ]]; then
					echo "Removing $key ➟ ${row[$key]}"
					unset "row[$key]"
					dirty=1
				fi
			done
			shift
			;;
		-l|--list)
			if [[ ${#row[@]} = 0 ]]; then
				echo "No bookmarks stored yet, try adding one with \"${func_name} -a . bookmark\""
				IFS=$tmpifs
				return
			fi

			local list_format="%-18s%-2s%-10s\n"
			for key in $keys; do
				if [[ -d ${row[$key]} ]]; then
					local exists=""
				else
					local exists="-"
				fi
				printf "$list_format" "$key" "$exists" "${row[$key]}"
			done
			echo
			
			shift
			;;
		--locate)
			echo "$filestore"
			shift
			;;
		--lookup)
			echo "${row[$2]}"
			shift 2
			;;
		-k|--keys)
			echo "$keys"
			shift
			;;
		-o)
			# Internal option used for tab completion purposes
			echo "-a --add -l --list -r --remove --clear --purge -h --help -k --keys --locate"
			shift
			;;
		-h|--help)
			local args_format="  %-31s%s\n"
			
			printf "%s\n\n" "Usage: ${func_name} [OPTION?] [DIRECTORY?] [BOOKMARK?]"
			printf "$args_format" "-a, --add" "adds directory with the given bookmark"
			printf "$args_format" "-l, --list" "lists current bookmarks and directories"
			printf "$args_format" "-r, --remove" "removes a given bookmark"
			printf "$args_format" "    --clear" "removes all bookmarks"
			printf "$args_format" "    --purge" "removes bookmarks for non-existing directories"
			printf "$args_format" "-h, --help" "display this help section"
			printf "$args_format" "-k, --keys" "lists current keys"
			printf "$args_format" "    --locate" "list location of data file"
			printf "\n%s\n" "Predefined bookmarks:"
			printf "$args_format" "home" "moves to home directory"
			printf "$args_format" "back" "moves back to previous directory"
			printf "$args_format" "..." "moves up two directories"
			printf "$args_format" "...." "moves up three directories, etc."
			printf "\n%s\n" "Examples:"
			printf "$args_format" "${func_name} -a . music" "bookmark current directory as \"music\""
			printf "$args_format" "${func_name} -l" "lists bookmarked directories"
			printf "$args_format" "${func_name} music" "changes the current directory to \"music\""
			printf "$args_format" "${func_name} -r music" "removes the bookmark with the key \"music\""
			printf "\n%s\n" "Warning! Violent fish. Do not ${func_name} outside the boundaries of your imagination."
			echo
			shift
			;;
		--) # end argument parsing
			shift
			break
			;;
		--*=) # unsupported flags
			echo "Error: Unsupported flag $1" >&2
			shift
			;;
		*) # positional arguments, look up key and cd into it
			# If the key contains a backslash, e.g. prj/other, then
			# only use the prj part for the lookup
			local key=${1%%/*}
			local dest=""
			if [[ -d "$1" ]]; then
				dest="$1"
			elif [[ ${row[$key]} != "" ]]; then
				dest=${row[$key]}
			elif [[ "$key" =~ ^[.]+$ ]]; then
				dest="."
				dots=${#key}
				until [ $dots -lt 2 ]; do
					if [[ ! -d $dest/.. ]]; then
						break
					fi
					dest="$dest/.."
					let dots-=1
				done
			elif [[ "$key" = "home" ]]; then
				dest=${HOME:-~}
			elif [[ "$key" = "-" ]] || [[ "$key" = "back" ]]; then
				dest="-"
			elif [[ -d "$key" ]]; then
				dest="$key"
			fi

			# Optionally other custom indexing solutions can be set
			# up as secondary lookups for unknown "keys" by adding
			# commands to to the _GOEXT array.
			if [[ -z "$dest" ]]; then
				for ext_script in "${_GOEXT[@]}"; do
					if [[ ! -z "$ext_script" ]]; then
						dest=$(eval $ext_script $@)
						if [ ! -z "$dest" ]; then
							break
						fi
					fi
				done
			fi
			
			# Multiple destinations found, print these
			if [[ "$dest" == *$'\n'* ]]; then
				echo "$dest"
				dest=""
			fi

			if [[ ! -z "$dest" ]]; then
				# We could test if directory exists here, but explicitly
				# trying and failing should allow support for autofs
				builtin cd "$dest" > /dev/null 2>&1
				if [ $? -eq 0 ]; then
					# If the key contains a backslash, e.g. prj/other, then
					# see if the remainder (other) directories exists, and
					# if so try to cd into these as well.
					if [[ ! -z "$key" ]] && [[ "$1" == *"/"* ]] && [[ ! -z "${1#*/}" ]]; then
						if [[ -d "${1#*/}" ]]; then
							builtin cd "${1#*/}" > /dev/null 2>&1
						else
							echo "Directory ${1#*/} does not seem to exist"
						fi
					fi
					
					# Most people have the habit of typing ls after
					# changing directory, let's do this automatically
					# This functionality can be tailored by setting
					# the _GOCMD variable to a command of choice.
					eval ${_GOCMD:-ls -a --color=auto}
				else
					echo "Directory $dest does not seem to exist, perhaps you need to mount something?"
				fi
			fi
			break
			;;
		esac
	done
	
	if [[ $dirty == 1 ]]; then
		if [ $ZSH_VERSION ]; then
			local keys=($(echo ${(k@)row} | tr ' ' '\n' | sort))
		else
			local keys=$(echo ${!row[@]} | tr ' ' '\n' | sort)
		fi
		local output=""
		for key in $keys; do
			output="${output}${key}=${row[$key]}\n"
		done
		echo -e $output > "$filestore"
	fi
	
	IFS=$tmpifs
}

# Tab completion
if [ $ZSH_VERSION ]; then
        function _gocomp {
		if [[ "${#words[*]}" == "2" ]]; then
			if [[ "${words[CURRENT]}" == '-'* ]]; then
				reply=($(go -o))
			elif [[ "${words[CURRENT]}" == *'/'* ]]; then
				# Special handling for <bookmark>/<tab>, pretend that
				# bookmark is an actual directory.
				local slashes="${words[CURRENT]//[^\/]}"
				local key=${words[CURRENT]%%/*}
				local dir=$(go --lookup "$key")
				local suggestions
				typeset -A suggestions
				suggestions=()
				while read line; do
					suggestions[${#suggestions[*]}]="$line"
				done <<< $(find $dir -maxdepth ${#slashes} -type d -path "$dir/${words[CURRENT]#*/}*" -exec echo {} \; | sed "s#$dir#$key#" | sed 's/ /\\ /g' | sed "s#'#\\\\'#g")

				if [[ ${#suggestions[*]} == 1 ]]; then
					suggestions[${#suggestions[*]}]="${suggestions[0]}/"
				fi
				reply=("${suggestions[@]}")
			else
				reply=($(go -k))
			fi
		else
			case ${words[2]} in
				-r|--remove)
					reply=($(go -k))
					;;
			esac
		fi
		return 0
	}
        
        compctl -K _gocomp + -x \
		'c[-1,-l],c[-1,--list],c[-1,--clear],c[-1,--purge],c[-1,-h],c[-1,--help],c[-1,-k],c[-1,--keys],c[-1,--locate]' -k "()" - \
		's[prj/]' -s 'path/to/file' - \
		's[]' -/ -- \
		go
else
        function _gocomp {
		if [[ "${#COMP_WORDS[@]}" == "2" ]]; then
			if [[ "${COMP_WORDS[COMP_CWORD]}" == '-'* ]]; then
				COMPREPLY=($(compgen -W '$(go -o)' -- ${COMP_WORDS[COMP_CWORD]}))
			elif [[ "${COMP_WORDS[COMP_CWORD]}" == *'/' ]]; then
				# Special handling for <bookmark>/<tab>, replace input with the
				# bookmarked directory.
				if [[ ! -d "${COMP_WORDS[COMP_CWORD]}" ]] && [[ "${COMP_WORDS[COMP_CWORD]}" != '/'* ]]; then
					local key=${COMP_WORDS[COMP_CWORD]%%/*}
					local dir=$(go --lookup "$key")
					dir=${dir//\'/\\\'}
					dir=${dir//\"/\\\"}
					dir=${dir// /\\ }
					COMPREPLY=("$dir" "$dir/")
				else
					COMPREPLY=()
				fi
			else
				suggestions=($(compgen -W '$(go -k)' -- ${COMP_WORDS[COMP_CWORD]}))
				
				if [[ ${#suggestions[*]} == 1 ]]; then
					COMPREPLY=("${suggestions[*]}" "${suggestions[*]}/")
				else
					COMPREPLY=(${suggestions[*]})
				fi
			fi
		else
			case ${COMP_WORDS[1]} in
				-l|--list|--clear|--purge|-h|--help|-k|--keys|--locate)
					COMPREPLY=($'\0'); # no further suggestions
					;;
				-r|--remove)
					COMPREPLY=($(compgen -W '$(go -k)' -- ${COMP_WORDS[COMP_CWORD]}))
					;;
				-a|--add)
					if [[ "${#COMP_WORDS[@]}" < "4" ]]; then
						COMPREPLY=(); # default to directories
					else
						COMPREPLY=($'\0'); # no further suggestions
					fi
					;;
				*)
					COMPREPLY=()
					;;
			esac
		fi
                return 0
        }
        shopt -s progcomp
        complete -o dirnames -F _gocomp go
fi
