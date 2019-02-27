#!/bin/bash
# Copyright (c) 2019, Stein Gunnar Bakkeby, all rights reserved.
# Source this file in your .bashrc/.zshrc, then run the function to
# begin. Alternatively you can rename this function to something
# more to your own liking. If you do then remember to also update
# the references for the tab completion at the bottom of this file.

function go () {
	if [ $ZSH_VERSION ]; then
		local func_name=$funcstack[1]
	else
		local func_name=${FUNCNAME[0]}
	fi
	local filestore=${HOME:-~}/.${func_name}.db

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

	if [[ $# == 0 ]]; then
		set -- -h
	fi

	local dirty=0
	local key=""
	local val=""
	local meta=""
	local param
	local i

	# Prioritise specific arguments so that they are processed before others
	for param in "--pin" "--tmp" "--temp" "--remote"; do
		for i in $(seq 2 $#); do
			if [ $ZSH_VERSION ]; then
				val=${@[i]}
			else
				val="${!i}"
			fi
			if [[ "${val}" = "${param}" ]]; then
				set -- "${val}" "${@:1:$i-1}" "${@:$i+1:$#}"
			fi
		done
	done

	while (( $# )); do
		case "$1" in
		-a|--add)
			key=""
			val=""
			shift

			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				if [[ -d "$1" ]] || [[ "$1" =~ (/|:|@) ]]; then
					val=$1
				else
					key=$1
				fi
				shift
			fi

			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				if [[ -d "$1" ]]; then
					key=$val
					val=$1
				elif [[ -d "$val" ]]; then
					key=$1
				elif [[ "$1" =~ (/|:|@) ]]; then
					key=$val
					val=$1
				else
					key=$1
				fi
				shift
			fi

			if [[ -z "$val" ]]; then
				val=$(pwd)
			fi

			if [[ -d "$val" ]]; then
				val=$(realpath "$val")
			fi

			if [[ -z "$key" ]]; then
				key=${val##*/}
				if [ $ZSH_VERSION ]; then
					key=$key:l
				else
					key=${key,,}
				fi
			fi
			
			# No support for spaces and ¦ in bookmarks
			key=${key//[ ¦]/}
			
			if [[ -z "$key" ]]; then
				echo "Please specify a name to store this bookmark as"
			else
				local extra_text=""
				if [[ "$meta" =~ (¦|^)pin(¦|$) ]]; then
					extra_text="${extra_text} (pinned)"
				fi

				if [[ "$meta" =~ (¦|^)tmp(¦|$) ]]; then
					extra_text="${extra_text} (temp)"
				fi

				row[$key]="${val}${meta}"
				dirty=1
				echo "${val} added as ${key}${extra_text}"
				meta=""
			fi
			;;
		-r|--remove)
			shift
			if [[ $# > 0 ]]; then
				key=$1
				shift
			fi

			if [[ -z "$key" ]]; then
				key=${$(pwd)##*/}
			fi
			
			if [[ ! -z "${row[$key]}" ]]; then
				val=${row[$key]%%¦*}
				unset "row[$key]"
				echo "key '$key' referring to $val removed"
				dirty=1
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--clear)
			shift
			if [[ -e "$filestore" ]]; then
				rm -f -- "$filestore"
			fi
			;;
		--purge)
			shift
			for key in $keys; do
				if [[ "${row[$key]#*¦}" =~ (¦|^)pin(¦|$) ]]; then
					# pinned key, do not purge
					continue
				elif [[ "${row[$key]#*¦}" =~ (¦|^)tmp(¦|$) ]]; then
					# temporary key, explicitly purge
					echo "Removing temporary $key ➟ ${row[$key]%%¦*}"
					unset "row[$key]"
					dirty=1
					continue
				elif [[ "${row[$key]#*¦}" =~ (¦|^)remote(¦|$) ]] ||
					 [[ "${row[$key]%%¦*}" =~ @[^:]+: ]]; then
					# remote key, do not purge
					continue
				fi

				if [[ ! -e "${row[$key]%%¦*}" ]]; then
					echo "Removing $key ➟ ${row[$key]%%¦*}"
					unset "row[$key]"
					dirty=1
				fi
			done
			;;
		--pin)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				meta="${meta}¦pin"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)pin(¦|$) ]]; then
					echo "Bookmark with key \"$key\" is already pinned"
				else
					row[$key]="${row[$key]}¦pin"
					echo "Pinned key \"$key\""
					dirty=1
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--unpin)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				echo "Please specify the bookmark you wish to unpin"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)pin(¦|$) ]]; then
					row[$key]="${row[$key]//¦pin/}"
					echo "Unpinned key \"$key\""
					dirty=1
				else
					echo "Bookmark with key \"$key\" is not pinned"
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--tmp|--temp)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				meta="${meta}¦tmp"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)¦tmp(¦|$) ]]; then
					echo "Bookmark with key \"$key\" is already temporary"
				else
					row[$key]="${row[$key]}¦tmp"
					echo "Marked key \"$key\" as temporary"
					dirty=1
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--untmp|--untemp)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				echo "Please specify the bookmark you wish to unmark as being temporary"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)tmp(¦|$) ]]; then
					row[$key]="${row[$key]//¦tmp/}"
					echo "Unmarked key \"$key\" as temporary"
					dirty=1
				else
					echo "Bookmark with key \"$key\" is not marked as being temporary"
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--remote)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				meta="${meta}¦remote"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)¦remote(¦|$) ]]; then
					echo "Bookmark with key \"$key\" is already marked as remote"
				else
					row[$key]="${row[$key]}¦remote"
					echo "Marked key \"$key\" as remote"
					dirty=1
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		--unremote)
			shift
			if [[ $# > 0 ]] && [[ ! "$1" =~ ^- ]]; then
				key=$1
				shift
			fi

			if [[ -z "${key}" ]]; then
				echo "Please specify the bookmark you wish to unmark as being remote"
			elif [[ ! -z "${row[$key]}" ]]; then
				if [[ "${row[$key]#*¦}" =~ (¦|^)remote(¦|$) ]]; then
					row[$key]="${row[$key]//¦remote/}"
					echo "Unmarked key \"$key\" as remote"
					dirty=1
				else
					echo "Bookmark with key \"$key\" is not marked as being remote"
				fi
			else
				echo "Bookmark with key \"$key\" does not exist"
			fi
			;;
		-l|--list)
			shift
			if [[ ${#row[@]} = 0 ]]; then
				echo "No bookmarks stored yet, try adding one with \"${func_name} -a . bookmark\""
				IFS=$tmpifs
				return
			fi

			local list_format="%-17s%s%-10s\n"
			local exists
			for key in $keys; do
				exists=""
				if [[ "${row[$key]#*¦}" =~ (¦|^)pin(¦|$) ]]; then
					exists="${exists}🖈"
				else
					if [[ "${row[$key]#*¦}" =~ (¦|^)tmp(¦|$) ]]; then
						# exists="🇹  "
						exists="${exists}ᴛ"
					fi
					if [[ "${row[$key]#*¦}" =~ (¦|^)remote(¦|$) ]] ||
					   [[ "${row[$key]%%¦*}" =~ @[^:]+: ]]; then
						exists="${exists}ʀ"
						# exists="🇷  "
						# exists="𐌺  "
					fi
				fi

				if [[ -z "$exists" ]] && [[ ! -d ${row[$key]%%¦*} ]]; then
					exists="${exists}‒"
				fi

				while [[ ${#exists} < 3 ]]; do
					exists="${exists} "
				done

				printf "$list_format" "$key" "$exists" "${row[$key]%%¦*}"
			done
			echo
			;;
		--locate)
			shift
			echo "$filestore"
			;;
		--lookup)
			shift
			if [[ $# > 0 ]]; then
				echo "${row[$1]%%¦*}"
				shift
			fi
			;;
		-k|--keys)
			shift
			echo "$keys"
			;;
		-o)
			shift
			# Internal option used for tab completion purposes
			echo "-a --add -l --list -r --remove --clear --pin --unpin --purge -h --help"\
				 "-k --keys --locate --temp --untemp --remote --unremote"
			;;
		-h|--help)
			shift
			local fmt="  %-31s%s\n"
			
			printf "%s\n\n" "Usage: ${func_name} [OPTION?] [DIRECTORY?] [BOOKMARK?]"
			printf "$fmt" "-a, --add" "adds directory with the given bookmark"
			printf "$fmt" "-l, --list" "lists current bookmarks and directories"
			printf "$fmt" "-r, --remove" "removes a given bookmark"
			printf "$fmt" "    --clear" "removes all bookmarks"
			printf "$fmt" "    --purge" "removes temporary bookmarks and bookmarks for"
			printf "$fmt" "" "non-existing directories, pinned bookmarks"
			printf "$fmt" "" "are not affected by purge"
			printf "$fmt" "    --pin" "pin a bookmark"
			printf "$fmt" "    --unpin" "removes the pin from a bookmark"
			printf "$fmt" "    --temp" "mark a bookmark as temporary"
			printf "$fmt" "    --untemp" "unmark a bookmark as temporary"
			printf "$fmt" "    --remote" "mark a bookmark location as being remote"
			printf "$fmt" "    --unremote" "unmark a bookmark location as being remote"
			printf "$fmt" "-h, --help" "display this help section"
			printf "$fmt" "-k, --keys" "lists current keys"
			printf "$fmt" "    --locate" "list location of data file"
			printf "\n%s\n" "Predefined bookmarks:"
			printf "$fmt" "home" "moves to home directory"
			printf "$fmt" "back" "moves back to previous directory"
			printf "$fmt" "..." "moves up two directories"
			printf "$fmt" "...." "moves up three directories, etc."
			printf "\n%s\n" "Examples:"
			printf "$fmt" "${func_name} -a . music" "bookmark current directory as \"music\""
			printf "$fmt" "${func_name} -l" "lists bookmarked directories"
			printf "$fmt" "${func_name} music" "changes the current directory to \"music\""
			printf "$fmt" "${func_name} -r music" "removes the bookmark with the key \"music\""
			printf "\n%s%s\n" "Warning! Violent fish. Do not ${func_name} outside the boundaries "\
							  "of your imagination."

			if [ ! $ZSH_VERSION ]; then
				echo
			fi
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
			elif [[ ! -z "${row[$key]}" ]]; then
				dest=${row[$key]%%¦*}
				meta=${row[$key]#*¦}
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
				if [[ "$meta" =~ (¦|^)remote(¦|$) ]] || [[ "$dest" =~ @[^:]+: ]]; then
					if [[ "$dest" =~ : ]]; then
						local desthost=${dest%%:*}
						local destdir=${dest#*:}
						local destdir=${destdir//\~\//}
						ssh -t "$desthost" "builtin cd \"$destdir\" && "\
							   "${_GOCMD:-ls -a --color=auto} && exec \$SHELL -l"
					else
						ssh "$dest"
					fi
				else
					# We could test if directory exists here, but explicitly
					# trying and failing should allow support for autofs
					builtin cd "$dest" > /dev/null 2>&1
					if [[ $? -eq 0 ]]; then
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
						echo "Directory $dest does not seem to exist, perhaps you need to mount"\
							 "something?"
					fi
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
				local slashes="${words[CURRENT]//[^\/]/}"
				local key=${words[CURRENT]%%/*}
				local dir=$(go --lookup "$key")
				local suggestions
				typeset -A suggestions
				suggestions=()
				while read line; do
					suggestions[${#suggestions[*]}]="$line"
				done <<< $(
					find $dir -maxdepth ${#slashes} -type d -path "$dir/${words[CURRENT]#*/}*"\
						-exec echo {} \; | sed "s#$dir#$key#" | sed 's/ /\\ /g' | sed "s#'#\\\\'#g"
						  )

				if [[ ${#suggestions[*]} == 1 ]]; then
					suggestions[${#suggestions[*]}]="${suggestions[0]}/"
				fi
				reply=("${suggestions[@]}")
			else
				reply=($(go -k))
			fi
		else
			case ${words[2]} in
				-r|--remove|--pin|--unpin|--tmp|--untmp|--temp|--untemp|--remote|--unremote)
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
				if [[ ! -d "${COMP_WORDS[COMP_CWORD]}" ]] &&
				   [[ "${COMP_WORDS[COMP_CWORD]}" != '/'* ]]; then
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
				-r|--remove|--pin|--unpin|--tmp|--untmp|--temp|--untemp|--remote|--unremote)
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