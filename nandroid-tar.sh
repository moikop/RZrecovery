#!/sbin/sh

source /ui_commands.sh
export PATH="/sbin:$PATH"
PTOTAL=0
	

for arg in $@; do
	if [ "$arg" == "-opt" ]; then
		shift
		OPERATION=$1
		shift
	fi
	if [ "$arg" == "-partition" ]; then
		shift
		PARTITION=$1
		shift
	fi

	if [ "$arg" == "-tar_opts" ]; then
		shift
		TAR_OPTS=$1
		shift
	fi
	if [ "$arg" == "-dest" ]; then
		shift
		DESTINATION=$1
		shift
	fi
	if [ "$arg" == "-source" ]; then
		shift
		SOURCE=$1
		shift
	fi	
	if [ "$arg" == "-prefix" ]; then
		shift
		PREFIX="$1"
		shift
	fi
	if [ "$arg" == "-ext" ]; then
		shift
		EXTENSION=$1
		shift
	fi
	if [ "$arg" == "-exclude" ]; then
		shift
		EXCLUSION=$1
		shift
	fi	
	if [ "$arg" == "-ptotal" ]; then
		shift
		PTOTAL=$1
		shift
	fi
done

echo "OPERATION = $OPERATION"
echo "PARTITION = $PARTITION"
echo "TAR_OPTS = $TAR_OPTS"
echo "PREFIX = $PREFIX"
echo "DESTINATION = $DESTINATION"
echo "EXTENSION = $EXTENSION"
echo "PTOTAL = $PTOTAL"

TAR_END=""
if [ !-z "$EXCLUSIONS" ]; then
	TAR_END="--exclude $EXCLUSIONS"
fi

#pipeline handles progress bars
pipeline() {
    if [ ! -z "$PTOTAL" ]; then
	[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"
	awk "NR==1 {print \"* ptotal $1\"} {print \"* pcur \" NR}"
	[ ! -z "$PTOTAL" ] && echo "* reset_progress"
    else
	cat
    fi
}

if [ "$OPERATION" == "backup" ]; then
	if [ "$PARTITION" == "secure" ]; then
		PARTITION_NAME="/sdcard/.android_secure"
		PARTITION="secure"
	else
   	PARTITION_NAME=/$PARTITION
   fi
	cd $PARTITION_NAME
	tar $TAR_OPTS $PREFIX/$PARTITION.$EXTENSION . $TAR_END | pipeline $PTOTAL
	if [ $? -eq 0 ]; then
		exit 0
	else
		exit 1
	fi
fi

if [ "$OPERATION" == "restore" ]; then
	if [ "$PARTITION" == "secure" ]; then
		DESTINATION="/sdcard/.android_secure"
	else
   	DESTINATION="/${PARTITION}"
   fi
  tar $TAR_OPTS $PREFIX/$PARTITION.$EXTENSION -C $DESTINATION | pipeline $PTOTAL
  if [ $? -eq 0 ]; then
	exit 0
  else
	exit 1
  fi
fi

exit



