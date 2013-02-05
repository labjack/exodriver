#! /bin/bash

RULES=10-labjack.rules
RULES_DEST_PRIMARY=/lib/udev/rules.d
RULES_DEST_ALTERNATE=/etc/udev/rules.d
GROUP=adm
SUPPORT_EMAIL=support@labjack.com
TRUE=0
# Assume these are unneeded until otherwise
NEED_RECONNECT=1
NEED_RESTART=1
NEED_RELOG=1
NO_RULES=1
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
	echo "Install finished. Thanks for choosing LabJack."
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
if [ ! $ret ]; then
	udevadm control --reload_rules 2> /dev/null
	ret=$?
fi
if [ ! $ret ]; then
	/etc/init.d/udev-post reload 2> /dev/null
	ret=$?
fi
if [ ! $ret ]; then
	udevstart 2> /dev/null
	ret=$?
fi
if [ ! $ret ]; then
	NEED_RESTART=$TRUE
	echo " cound not restart the rules."
else
	echo # Finishes previous echo -n
fi

#####################
# Add user to group #
#####################
if [ $USER == "root" ]; then
	user=$SUDO_USER
else
	user=$USER
fi

in_group=1
for g in `id -nG $user`; do
	if [ "$g" == "$GROUP" ]; then
		in_group=$TRUE
		break
	fi
done

if [ $in_group -eq $TRUE ]; then
	# Make sure the user is logged into the adm group
	current_groups=1
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
go groupadd -f $GROUP
go usermod -a -G $GROUP $user
NEED_RELOG=$TRUE

success
