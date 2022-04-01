#!/bin/bash

# Define constants.
#==============================================================================
# The default sql directory.
#------------------------------------------------------------------------------
SQL_DIR="sql"

create_directory()
{
    local DIRECTORY="$1"

    rm -rf "$DIRECTORY"
    mkdir "$DIRECTORY"
}

display_heading_message()
{
    echo
    echo "********************** $@ **********************"
    echo
}

display_message()
{
    echo "$@"
}

display_error()
{
    >&2 echo "$@"
}

pop_directory()
{
    popd >/dev/null
}

push_directory()
{
    local DIRECTORY="$1"

    pushd "$DIRECTORY" >/dev/null
}

display_help()
{
    display_message "Database installation management script."
    display_message ""
    display_message "Usage: ./runme.sh [OPTIONS]..."
    display_message ""
    display_message "Script options:"
    display_message "--help       This message."
    display_message "--make       [*] Creates users, database, schemas, tables, views, routines and initialization."
    display_message "--install    [*] Creates database, schemas, tables, views, routines and initialization."
    display_message "--create     [*] Creates database, schemas, tables, views, routines."
    display_message "--update     Updates routines, views."
    display_message "--patch      Updates tables, routines, views."
    display_message "--database   [*] Creates an empty database."
    display_message "--api        Drop and create api schema."
    display_message "--kladr      Loading data from KLADR."
    display_message ""
    display_message "WARNING: [*] All data in database will be lost."
}

# Initialize environment.
#==============================================================================
# Exit this script on the first error.
#------------------------------------------------------------------------------
set -e

# Parse command line options that are handled by this script.
#------------------------------------------------------------------------------
for OPTION in "$@"; do
    case $OPTION in
        # Standard script options.
        (--help)	DISPLAY_HELP="yes";;

        # Script options.
        (--install)	SCRIPT="install";;
        (--create)	SCRIPT="create";;
        (--make)	SCRIPT="make";;
        (--database)	SCRIPT="database";;
        (--patch)	SCRIPT="patch";;
        (--update)	SCRIPT="update";;
        (--kladr)	SCRIPT="kladr";;
        (--api)		SCRIPT="api";;
    esac
done

# Set param.
#------------------------------------------------------------------------------
if [[ !($SCRIPT) ]]; then
    DISPLAY_HELP="yes"
fi

display_configuration()
{
    display_message "Installer configuration."
    display_message "--------------------------------------------------------------------"
    display_message "SCRIPT: $SCRIPT"
    display_message "--------------------------------------------------------------------"
}

# Standard sql build.
build_sql()
{
    push_directory "$SQL_DIR"

    sudo -u postgres -H psql -d template1 -f $SCRIPT.psql 2>"../log/$SCRIPT.log"

    pop_directory
}

# Build.
#==============================================================================

if [[ $DISPLAY_HELP ]]; then
    display_help
else
    display_configuration
    time build_sql "${SCRIPT[@]}"
fi
