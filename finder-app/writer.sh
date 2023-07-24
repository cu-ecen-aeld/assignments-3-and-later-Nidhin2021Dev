#!/bin/bash

# the first argument is a full path to a file (including filename) on the filesystem
WRITERFILE=$1
# the second argument is a text string which will be written within this file
WRITESTR=$2

# Exits with return value 1 error and print statements if any of the parameters above were not specified
noOfArguments=$#
if [ ${noOfArguments} -lt 2 ]
then
	echo "ERROR:the number of argument is invalid, ${noOfArguments}"
	echo "Hint: There must be 2 arguments"
	echo "The Argument order should be:"
	echo "	1 : Path to a file (including filename) on the filesystem"
	echo "	2 : Text string which will be written within this file"
	exit 1

fi
# create a directory
CHECKDIR=$(dirname "$WRITERFILE")
mkdir -p "$CHECKDIR"

# exits with value 1 and error print statement if the file could not be created
if [ ! -f "$WRITERFILE" ];
then
	# Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist.
	echo "$WRITESTR" > $WRITERFILE
fi


