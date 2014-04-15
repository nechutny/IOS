#!/bin/sh
#/*
# *	Name:		Project 1 for IOS
# *	Author:		Stanislav Nechutny - xnechu01
# * Repository:	git@nechutny.net:vut.git
# *	Revision:	7
# * Created:	2014-02-26 01:39
# * Modified:	2014-03-11 21:18
# */



# arguments...
arg_graph=false;
arg_call=false;
arg_caller=false

while getopts "gr:d:" opt
do case "$opt" in
    g) arg_graph=true;;
    r) arg_call="$OPTARG";;
    d) arg_caller="$OPTARG";;
    *) echo "Spatny argument" >&2
		exit 1;;
    esac
done

FOO=""
DUMP=""
CACHE=""

# dump all .o files to variable and do magic sed on it
for i in $(seq $OPTIND $#);
do
	eval i=\${"$i"}
	
	if [ "$arg_graph" == true ]
	then
		name=$(basename "$i" )
	else
		name="$i"
	fi

	
	CACHE="$CACHE\n$(nm "$i" | sed "s^[0-9a-f]*\s[BCDGT]\s^$name	DEF	^" | sed "s^\s*U\s^$name	USE	^")"
done

# remove duplicit lines
CACHE=$(echo -e "$CACHE" | grep -v '^\s*$' | sort -u);
CACHE_DEF=$(echo -e "$CACHE" | grep "	DEF	")

# found definiton for each required function
FOO=$(echo "$CACHE" | grep "	USE	" | while read line
do
	fo=($line)
	name=${fo[2]}

	x=$(echo "$CACHE_DEF" | grep "DEF\s$name$");
	if [ "$x" != ""  ]
	then
		# found definition, so print it
		x=($x)
		echo "${fo[0]} -> ${x[0]} [label=\"$name\"];";
	fi	
	
done)

# filter for -r arg
if [ "$arg_call" != false ]
then
	FOO=$(echo -e "$FOO" | grep "$arg_call \[" );
fi

# filter for -d arg
if [ "$arg_caller" != false ]
then
	FOO=$(echo -e "$FOO" | grep "$arg_caller \-> " );
fi

# format output for graphviz
if [ "$arg_graph" == true ]
then
	FOO=$(echo "$FOO"  | sed 's/\./D/g' | sed 's/-/_/g' | sed 's/+/P/g' | sed 's/_>/->/g')

	for i in $(seq $OPTIND $#);
	do
		eval i=\${"$i"}
		name=$(basename "$i" | sed 's/\./D/g' | sed 's/-/_/g' | sed 's/+/P/g' | sed 's/_>/->/g')
		# check if is module required/used and then print it
		if [ $(echo "$FOO" | grep -c "$name") -gt 0 ]
		then
			FOO="$FOO\n$name [label=\"$(basename $i)\"];"
		fi
	done

	# format output for graphviz ( . -> D, - -> _, + -> P )
	FOO="digraph GSYM {\n$FOO\n}";
else
	FOO=$(echo -e "$FOO" | sed 's/\[label\=\"/(/' | sed 's/\"\]\;/)/' );
fi

echo -e "$FOO"
