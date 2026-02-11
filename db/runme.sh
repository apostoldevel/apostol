#!/usr/bin/env bash

INSTALL="$(pwd)/install.sh"

PS3='
WARNING: [*] All data in the database will be lost.
Please enter your choice: '

options=(
    "help (?): Help"                               # 1
    "init (f): [*] First Installation"             # 2
    "install (i): [*] Installation"                # 3
    "create (c): [*] Create without data"          # 4
    "update (u): Updates routines, views"          # 5
    "database (db): [*] Creates an empty database" # 6
    "quit (q): Exit from this menu"                # 7
)

function confirm_action() {
    read -p "This action will lead to data loss. Are you sure? (y/n): " confirm
    case $confirm in
        [yY][eE][sS]|[yY])
            return 0
            ;;
        [nN][oO]|[nN])
            echo "Action cancelled. Returning to menu."
            return 1
            ;;
        *)
            echo "Invalid input. Returning to menu."
            return 1
            ;;
    esac
}

function perform_action() {
    case $1 in
        "init"|"install"|"create"|"database")
            confirm_action || return
            ;;
    esac

    # Call the install script with the appropriate argument
    $INSTALL --"$1"
    exit
}

function _switch() {
    _reply="$1"

    case $_reply in
        ""|"?"|"--help"|"help"|"1")
            $INSTALL --help
            ;;
        ""|"f"|"--init"|"init"|"2")
            perform_action "init"
            ;;
        ""|"i"|"--install"|"install"|"3")
            perform_action "install"
            ;;
        ""|"c"|"--create"|"create"|"4")
            perform_action "create"
            ;;
        ""|"u"|"--update"|"update"|"5")
            $INSTALL --update
            exit
            ;;
        ""|"db"|"--database"|"database"|"6")
            perform_action "database"
            ;;
        ""|"q"|"quit"|"7")
            exit
            ;;
        *) echo "Invalid option. Use help option for the commands list.";;
    esac
}

while true
do
    # Run option directly if specified in argument
    [ ! -z $1 ] && _switch $1
    [ ! -z $1 ] && exit 0

    echo "==== DASHBOARD ===="
    select opt in "${options[@]}"
    do
        _switch $REPLY
        break
    done
done
