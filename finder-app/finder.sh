#!/bin/sh

# the first argument is a path to a directory on the filesystem
FILESDIR=$1
# the second argument is a text string which will be searched within these files
SEARCHSTR=$2

# Exits with return value 1 error and print statements if any of the parameters above were not specified
noOfArguments=$#
if [ ${noOfArguments} -lt 2 ]
then
	echo "ERROR:the number of argument is invalid"
	echo "Hint: There must be 2 arguments"
	echo "The Argument order should be:"
	echo "	1 : Path to a directory on the filesystem"
	echo "	2 : Text string which will be searched within these files"
	exit 1

fi

# Exits with return value 1 error and print statements if $FILESDIR does not represent a directory on the filesystem
if [ -d "$FILESDIR" ];
then
	X=$(find "$FILESDIR" -type f | wc -l)
	Y=$(grep -s -r "$SEARCHSTR" "$FILESDIR" | wc -l)
	# X is the number of files in the directory and all subdirectories and Y is the number of matching lines found in respective files, 
	# where a matching line refers to a line which contains searchstr (and may also contain additional content).
	echo "The number of files are ${X} and the number of matching lines are ${Y}"
	
else
	echo "ERROR: $FILESDIR not a directory"
	exit 1
fi


