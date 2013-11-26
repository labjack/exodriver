#! /bin/bash

RULES=10-labjack.rules
RULES_DEST_PRIMARY=/lib/udev/rules.d
RULES_DEST_ALTERNATE=/etc/udev/rules.d
GROUP=adm
SUPPORT_EMAIL=support@labjack.com
TRUE=1
FALSE=0
IS_SUCCESS=$TRUE
# Assume these are unneeded until otherwise
NEED_RECONNECT=$FALSE
NEED_RESTART=$FALSE
NEED_RELOG=$FALSE
NO_RULES=$FALSE
NO_RULES_ERR=2

go ()
{
	$@
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "Error, please make sure you are running this script as:"
		echo "  $ sudo $0"
		echo "If you are still having problems, please contact LabJack support: <$SUPPORT_EMAIL>"
		exit $ret
	fi
}

success ()
{
	e=0
	if [ $IS_SUCCESS -eq $TRUE ]; then
		echo "Install finished. Thanks for choosing LabJack."
	fi
	if [ $NEED_RECONNECT -eq $TRUE ]; then
		echo "If you have any LabJack devices connected, please disconnect and reconnect them now for device rule changes to take effect."
	fi
	if [ $NO_RULES -eq $TRUE ]; then
		echo "No udev rules directory found. Searched for $RULES_DEST_PRIMARY, $RULES_DEST_ALTERNATE."
		echo "Please copy $RULES to your device rules directory and reload the rules or contact LabJack support for assistence: <$SUPPORT_EMAIL>"
		let e=e+$NO_RULES_ERR
	fi
	if [ $NEED_RESTART -eq $TRUE ]; then
		echo "Please manually restart the device rules or restart your computer now."
	elif [ $NEED_RELOG -eq $TRUE ]; then
		echo "Please log off and log back in for the group changes to take effect. To confirm the group changes have taken effect, enter the command:"
		echo "  $ groups"
		echo "and make sure $GROUP is in the list. (You probably have to log out of your entire account, not just your shell.)"
	fi
	exit $e
}

##############################
# Exodriver make and install #
##############################
go cd liblabjackusb/

echo "Making.."
go make clean
go make

echo "Installing.."
go make install

# Mac OS doesn't need rules config
if [ `uname -s` == "Darwin" ]; then
	success
fi

go cd ../

#################
# LabJack Rules #
#################
if [ -d $RULES_DEST_PRIMARY ]; then
	RULES_DEST=$RULES_DEST_PRIMARY

	OLD_FILE_TO_REMOVE=$RULES_DEST_ALTERNATE/$RULES
	if [ -f $OLD_FILE_TO_REMOVE ]; then
		rm $OLD_FILE_TO_REMOVE
	fi
elif [ -d $RULES_DEST_ALTERNATE ]; then
	RULES_DEST=$RULES_DEST_ALTERNATE
else
	NO_RULES=$TRUE
fi

if [ $NO_RULES -ne $TRUE ]; then
	echo "Adding $RULES to $RULES_DEST.."
	go cp -f $RULES $RULES_DEST
	NEED_RECONNECT=$TRUE
fi

#####################
# Restart the Rules #
#####################
echo -n "Restarting the rules.."
udevadm control --reload-rules 2> /dev/null
ret=$?
if [ $ret -ne 0 ]; then
	udevadm control --reload_rules 2> /dev/null
	ret=$?
fi
if [ $ret -ne 0 ]; then
	/etc/init.d/udev-post reload 2> /dev/null
	ret=$?
fi
if [ $ret -ne 0 ]; then
	udevstart 2> /dev/null
	ret=$?
fi
if [ $ret -ne 0 ]; then
	NEED_RESTART=$TRUE
	echo " cound not restart the rules."
else
	echo # Finishes previous echo -n
fi

#####################
# Add user to group #
#####################
user=`logname`

in_group=$FALSE
for g in `id -nG $user`; do
	if [ "$g" == "$GROUP" ]; then
		in_group=$TRUE
		break
	fi
done

if [ $in_group -eq $TRUE ]; then
	# Make sure the user is logged into the adm group
	current_groups=$FALSE
	for g in `groups $user`; do
		if [ "$g" == "$GROUP" ]; then
			current_groups=$TRUE
			break
		fi
	done
	if [ $current_groups -ne $TRUE ]; then
		NEED_RELOG=$TRUE
	fi

	success
fi

echo "Adding $user to the $GROUP group.."
NEED_RELOG=$TRUE

declare -a tasks=("add/create a group named $GROUP ($ groupadd -f $GROUP)" "add your user account to the group named $GROUP ($ usermod -a -G $GROUP $user)")

print_tasks_if_needed()
{
	ret=$?
	if [ $ret -ne 0 ]; then
		echo
		echo "It looks like this installer failed with only a few task(s) left to complete."
		echo "LabJack developed this installer on Ubuntu/Debian Linux, so if you are running"
		echo "Ubuntu/Debian, please contact LabJack. However, if you are running a"
		echo "different distribution of Linux, it is likely that your distribution varies"
		echo "enough to make these last task(s) fail."
		echo
		echo "You can search the internet for how to complete the following task(s) on your"
		echo "distribution of Linux:"
		for n in "${tasks[@]}"; do
			echo "    - $n"
		done
		echo
		echo "Where the Ubuntu/Debian commands for the tasks are in parenthesis."
		echo
		echo "Once these tasks are complete, your installation of Exodriver will be complete."
		echo
		echo "If you are on a common distribution of Linux or if you are not sure how to"
		echo "complete the above task(s), please contact LabJack support. If your"
		echo "distribution of Linux is old, consider upgrading to see if that solves the"
		echo "problem."
		echo
		echo "LabJack support: support@labjack.com"
		echo
		echo "Please also follow any following instructions."
		echo
		IS_SUCCESS=$FALSE
		success
	fi
}

groupadd -f $GROUP
print_tasks_if_needed
unset tasks[0]

usermod -a -G $GROUP $user
print_tasks_if_needed
unset tasks[1]

success
