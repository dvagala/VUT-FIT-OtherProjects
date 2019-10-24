#!/bin/bash

export POSIXLY_CORRECT=yes


# Check if $WEDI_RC is defined, if not exit
# Create path and file to wedirc file, if not already
function wedi_rc_handle {

	# Check if WEDI_RC is defined, if not, exit"
	if [ -z "$WEDI_RC" ] 
	then	
		echo "wedi: WEDI_RC not defined!"
		exit 3
	fi

	# Check if WEDI_RC path to file and file itself exists
	# If not create path and touch file
	if ! [ -d "$(dirname "$WEDI_RC")" ]
	then	
		mkdir -p "$(dirname "$WEDI_RC")"
	fi

	if ! [ -f "$WEDI_RC" ]
	then	
		touch "$WEDI_RC"
	fi
}

# Check if default editor variables are set, if not, set vi to EDITOR,
# so with EDITOR you could run vi
function editor_assign {

	if ! [ -z "$EDITOR" ] 
	then
		:
	elif ! [ -z "$VISUAL" ] 
	then
		EDITOR="$VISUAL"

	else
		EDITOR=$( which vi )

	fi	
}


wedi_rc_handle

editor_assign


# Check if all files in wedirc exists
while read -r line
do
	line="${line} "
	check_path="$( echo "$line" | awk -F" /// " '{print $5}' )"
	if ! [ -f "$check_path" ];
		then
		check_path="${check_path//\//\\/} \/\/\/"
		sed -i "/$check_path/d" "$WEDI_RC"
	fi
        
done < "$WEDI_RC"


# Add script arguments to global variables
arg1="$1"
arg2="$2"
arg3="$3"

# Replace empty argument with '.'
if [ $# -eq 0 ];
then	
	arg1="."
elif [ $# -eq 1 ];
then
	arg2="."
elif [ $# -eq 2 ];
then
	arg3="."
fi	


# Is file
if [ -f "$arg1" ];
then
	PATH_FILE="$( realpath "$arg1" )"

# Is directory
elif [ -d "$arg1" ];
then
	DIR_PATH="$(realpath "$arg1")"
	PATH_FILE="$(grep "${DIR_PATH}/[^/]\+///" "$WEDI_RC" | tail -n 1 | awk -F" /// " '{print $5}')"

	if ! [ -f "$PATH_FILE" ]
		then
		echo "wedi: No edited files in this folder!" >&2
		exit 2
	fi

# Is -m	
elif [[ "$arg1" =~ -m ]] && [ -d "$arg2" ];
then
	DIR_PATH="$(realpath "$arg2")"
	PATH_FILE="$(grep "${DIR_PATH}/[^/]\+///" "$WEDI_RC" | sort -n -r -k2 | head -n 1 | awk -F" /// " '{print $5}')"

# Is -l	
elif [[ "$arg1" =~ -l ]] && [ -d "$arg2" ];
then
	DIR_PATH="$(realpath "$arg2")"
	grep "${DIR_PATH}/[^/]\+///" "$WEDI_RC" | awk -F" /// " '{print $5}' | awk -F "/" '{print $NF}' 
	exit	

# Is -a or -b	
elif [[ "$arg1" =~ ((-a)|(-b)) ]] && [[ "$arg2" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] && [ -d "$arg3" ];
then

	DIR_PATH="$(realpath "$arg3")"

	if [ "$arg1" == "-a" ];
	then
		grep "${DIR_PATH}/[^/]\+///" "$WEDI_RC" | awk -v mydate="$arg2" -F " /// " ' {if ($3 >= mydate) print $5 } ' | awk -F "/" '{print $NF}' 	
		
	else
		grep "${DIR_PATH}/[^/]\+///" "$WEDI_RC" | awk -v mydate="$arg2" -F " /// " ' {if ($2 < mydate) print $5 } ' | awk -F "/" '{print $NF}' 	
	fi

	exit

else
	err_code=$(echo $?)
	echo "wedi: Wrong aruments!" >&2
	exit $err_code

fi

# If new file will be edited, date created will be this date, if not, below it will overwrite it
DATE_CREATED=$(date +'%Y-%m-%d')

EDIT_NUMBER=1

# Add escape characters after / and append -//- what is behin path, 
# so it will seach exacly what we want, not what we want ($1) + anything else
PATH_FILE_EDIT="${PATH_FILE//\//\\/} \/\/\/"

# Find if file we want to edit is already in wedirc, return his EDIT_NUMBER (after //i)
FOUND=$(grep "$PATH_FILE_EDIT" "$WEDI_RC" | cut -f 2 -d' ' )	

# If there was that file increment EDIT_NUMBER value, so later when we add the file
# there will be this incremented value
# And remove the old file

if [[ "$FOUND" =~ [0-9]+ ]];
then		
	DATE_CREATED=$(grep "$PATH_FILE_EDIT" "$WEDI_RC" | awk -F" /// " '{print $2}' )
	EDIT_NUMBER=$(($FOUND + 1))		
	sed -i "/$PATH_FILE_EDIT/d" "$WEDI_RC"
fi


echo "EDIT: $EDIT_NUMBER /// $DATE_CREATED /// $(date +'%Y-%m-%d /// %H:%M:%S') /// $PATH_FILE /// " >> $WEDI_RC

$EDITOR "$PATH_FILE"

exit



