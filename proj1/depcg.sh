#!/bin/sh
#/*
# *	Name:		Project 1 for IOS
# *	Author:		Stanislav Nechutny - xnechu01
# * Repository:	git@nechutny.net:vut.git
# *	Revision:	4
# * Created:	2014-02-26 01:39
# * Modified:	2014-03-11 21:18
# */

arg_file=""
arg_caller=false
arg_call=false
arg_all=false
arg_graph=false

while getopts "gpr:d:" arg
do case "$arg" in
    g) arg_graph=true;;
    p) arg_all=true;;
    r) arg_call="$OPTARG";;
    d) arg_caller="$OPTARG";;
    *) echo "Spatny argument" >&2
		exit 1;;
    esac
done

eval arg_file=\$$(seq "$OPTIND" $#);

# get objdumop with only functions and calls
DUMP=$(objdump -d -j .text "$arg_file" | grep -E "callq|>:" | grep "<" | grep -v "+" | sed 's/[:\<\>]//g')

# hide calling shared functions, or formate them
if [ "$arg_all" == false ]
then
	DUMP=$(echo "$DUMP" | grep -v '@pl')	
fi

IN=""

#echo "$DUMP";

# parse each line
FOO=$(echo "$DUMP" | while read line
do
		if [[ "$line" == *"callq"* ]]
		then
            line=($line)
            echo "$IN -> ${line[8]};"
		else
			IN=${line:17}
		fi;
	
done)


# filter for caller -> -d argument
if [ "$arg_caller" != false ]
then
	FOO=$(echo "$FOO" | grep -e "^$arg_caller \\-")
fi

# filter for call -> -r argument
if [ "$arg_call" != false ]
then
	FOO=$(echo "$FOO" | grep " $arg_call;\$")
fi

# only unique lines
FOO=$(echo "$FOO" | sort -u)

# output as graph?
if [ "$arg_graph" == true ]
then
	FOO=$(echo "$FOO" | sed 's/@plt/_PLT/');
	FOO="digraph CG {\n$FOO\n}"
else
	FOO=$(echo "$FOO" | sed 's/;//')
fi

# delete empty lines and print result
echo -e "$FOO" | grep -v '^$'

