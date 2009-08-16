#!/bin/sh

RESULT=53
STOP_REASON="initial bootup"

ulimit -c unlimited

while [ $RESULT != 0 ]; do
	DATESTR=`date +%C%y.%m.%d-%H.%M.%S`

  if [ $RESULT != 52 ]; then
    if [ -f src/dms_new ]; then
			if [ -f dms ]; then
		  	mv dms dms.$DATESTR
			fi
      mv src/dms_new dms
    fi
  fi

  if [ -d logs/log ]; then
    #LOGNAME=`date +%b%d-%H%M`
    mkdir logs/old-logs/$DATESTR
    mv logs/log/* logs/old-logs/$DATESTR
  fi

  echo "Generating list of function names.."
  nm --demangle dms | grep " T " | sed -e 's/[(].*[)]//g' > lib/misc/event_names

	if [ -f /usr/local/bin/sendEmail ]; then
		/usr/local/bin/sendEmail -t torgal@durismud.com \
			-f mud@durismud.com -u "Duris Booting..." \
			-m "Mud booting at ${DATESTR}, previous shutdown reason: ${STOP_REASON} [${RESULT}]."
    sleep 10 # slow down in order to prevent 83284828234 emails from being sent per second
	fi

  echo "Starting duris..."
  ./dms 7777 > dms.out

	# capture the exit code
  RESULT=${PIPESTATUS[0]}

	# determine the reason for shutting down
	case $RESULT in
		0) STOP_REASON="shutdown";;
		139) STOP_REASON="crash";;
		52) STOP_REASON="reboot";;
		53) STOP_REASON="copyover reboot";;
		*) STOP_REASON="unknown";;
	esac	

	echo "Mud stopped, reason: ${STOP_REASON} [${RESULT}]"
done
