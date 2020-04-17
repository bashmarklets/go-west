# Go

Go is a bash (and zsh) solution that allows you to bookmark directories so that you can quickly "jump" to them later.

## Why would I want to use something like this?

I believe that *most* people that do work on the console eventually make the realisation that they naviate to the same directories over and over again and they end up optimising it by doing one of two things. Either they create a symlink to the directory in their home directory, or they set up an alias to `cd` into the given directory.

Both of these solutions work great and save the user a lot of time when navigating around. The main problem with these solutions though is that they end up being cumbersome to maintain as the number of directories grow. Symlinks can also clutter your home directory while it can be hard to remember all the aliases.

Go is a relatively simple bash (or zsh) function that allows you to easily bookmark directories, list and jump to them at will. It also has some convenience addons like doing an `ls` automatically after moving to a directory.

## Installation

Download or clone this repository and add the following to your `~/.bashrc` file:
```bash
source path/to/go.inc
```

## Example usage
```
$ go --help

Usage: go [OPTION?] [DIRECTORY?] [BOOKMARK?]

  -a, --add                      adds directory with the given bookmark
  -l, --list                     lists current bookmarks and directories
  -r, --remove                   removes a given bookmark
      --clear                    removes all bookmarks
      --purge                    removes temporary bookmarks and bookmarks for
                                 non-existing directories, pinned bookmarks
                                 are not affected by purge
      --pin                      pin a bookmark
      --unpin                    removes the pin from a bookmark
      --temp                     mark a bookmark as temporary
      --untemp                   unmark a bookmark as temporary
      --remote                   mark a bookmark location as being remote
      --unremote                 unmark a bookmark location as being remote
  -h, --help                     display this help section
  -k, --keys                     lists current keys
      --locate                   list location of data file
      --setup_aliases            set up aliases for all bookmarks


Predefined bookmarks:
  home                           moves to home directory
  back                           moves back to previous directory
  ...                            moves up two directories
  ....                           moves up three directories, etc.

Examples:
  go -a . music                  bookmark current directory as "music"
  go -l                          lists bookmarked directories
  go music                       changes the current directory to "music"
  go -r music                    removes the bookmark with the key "music"

```

Example output:
```bash
[bash@marklet:~]
$ go west
.  ..  .git  .gitignore  go.inc  LICENSE  README.md
[bash@marklet:/home/bash/path/to/bashmarklets/go-west]
```

Note that if you don't like the function name of `go` then you can change this to something more to your liking.

How about giving it a go and see for yourself?

## Sorry, but I still prefer aliases

Old habits are hard to kick, one can understand that. Fret not because you can still use aliases while taking advantage of the features of the go script.

The `go --setup_aliases` command sets up aliases for all your bookmarks, e.g. for the bookmark "music" an alias of `alias music="go music"` will be added. Add this call to your .bashrc and you are good to go.

## Post cd actions

Most people have the tendency of doing a quick `ls` after changing directories. For convenience the go script does this automatically by default.

Should you not like this then you can change it to another command of choice by setting the \_GOSETTINGS\[post_cd_cmd\] variable, e.g.
```bash
_GOSETTINGS[post_cd_cmd]="ls -l"
```
or, if you do not want this at all then try setting it to a no-op like this:
```bash
_GOSETTINGS[post_cd_cmd]=":"
```

NB: Have a look at [go_settings.inc](https://github.com/bashmarklets/go-west/blob/master/go_settings.inc) for other obscure settings. It is possible to source this file, but it is primarily meant as a point of reference listing the various configuration options.

## Remote directories

There is some basic support for jumping to directories on remote machines / servers.

The criteria for this is that the username, hostname and full path to the directory is included in the bookmark.

Example adding such a bookmark:
```
[bash@marklet:~]
$ go -a bash@remote:/full/path/to/logs remote_logs
bash@remote:/full/path/to/logs added as remote_logs
```

## Going beyond defined bookmarks

OK so you love the go script and can't live without it, but you'd wish that you could jump to any directory in your documents folder without having to bookmark each and every one individually?

It is possible to extend the realm of go by adding custom secondary lookups for unknown keys.

Here is an example that looks for additional directories in `~/Documents` using a case insensitive search.

```bash
typeset -gA _GOEXT
_GOEXT[${#_GOEXT[*]}]="_go_documents"

_go_documents () {
	find ~/Documents -maxdepth 1 -type d -iname "${*}*" -exec echo "{}" \; -quit 2>/dev/null
}
```

You can add any logic you want this way, the only criteria being that it either returns (echo) the path to a directory or nothing at all if none was found.

Note that tab completion is not supported for these kind of extensions.

## Also see these related projects

   - https://github.com/huyng/bashmarks
   - http://www.skamphausen.de/cgi-bin/ska/CDargs
   - https://github.com/bulletmark/cdhist
   - https://github.com/karlin/working-directory
   - http://micans.org/apparix/
   - https://github.com/wting/autojump/wiki
   - https://github.com/frazenshtein/fastcd
   - https://github.com/jeroenvisser101/project-switcher
   - http://xd-home.sourceforge.net/xdman.html
   - https://github.com/rupa/z/
   - https://github.com/wting/autojump
   - https://github.com/mfaerevaag/wd
   - https://github.com/rupa/z
   - https://github.com/junegunn/fzf
   - https://github.com/b4b4r07/enhancd
