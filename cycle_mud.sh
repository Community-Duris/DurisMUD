#!/bin/bash

RESULT=53
STOP_REASON="initial bootup"

ulimit -c unlimited

while [ $RESULT != 0 ]; do
	DATESTR=`date +%C%y.%m.%d-%H.%M.%S`

  if [ $RESULT == 53 ]; then
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
    if [ -f core ]; then
      mv core core.$DATESTR
    fi
  fi

  echo "Generating list of function names.."
  nm --demangle dms | grep " T " | sed -e 's/[(].*//g' > lib/misc/event_names

	if [ -f /usr/bin/sendemail ]; then
		/usr/bin/sendEmail -t alert@durismud.com \
			-f mud@durismud.com -u "Duris Booting..." \
			-m "Mud booting at ${DATESTR}, previous shutdown reason: ${STOP_REASON} [${RESULT}]."
	fi

  echo "Starting duris..."
  ./dms 7777 # > dms.out

	# capture the exit code
  RESULT=${PIPESTATUS[0]}

	# determine the reason for shutting down
	case $RESULT in
		0) STOP_REASON="shutdown";;
		139) STOP_REASON="crash";;
		52) STOP_REASON="reboot";;
		53) STOP_REASON="copyover reboot";;
		54) STOP_REASON="auto reboot";;
		*) STOP_REASON="unknown";;
	esac	

	echo "Mud stopped, reason: ${STOP_REASON} [${RESULT}]"

  echo "Sleeping 10 seconds to prevent coreflood..."
  sleep 10
done
