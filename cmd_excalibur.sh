#!/bin/bash

usage="Usage: $0 [options] \"COMMAND\"

Run the COMMAND on all 7 excalibur servers.
Options:
    -u USER     Specify a certain username to use. The default user is i13-1detector.
    -n          Print the server hostname before running COMMAND
    -h          Print this help
"
as_user=i13-1detector
hostname_cmd=""
while getopts "hnu:" opt
do
  case $opt in
    h) echo "$usage"; exit;;
    n) hostname_cmd="hostname; ";;
    u) as_user="${OPTARG}";;
    \?) exit 1;;
  esac
done
shift $(( $OPTIND -1 ))

if [ $# -lt 1 ]
  then
    echo "### ERROR ### Missing command to run on excalbur servers!"
    echo ""
    echo "$usage"
    exit
fi

command=$1
for host_number in {1..7}; do 
    ssh $as_user@i13-1-excalibur0$host_number "$hostname_cmd $command"
done
