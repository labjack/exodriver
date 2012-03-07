#! /bin/bash

RULES=50-labjack.rules
OLD_RULES=10-labjack.rules
RULES_DEST_PRIMARY=/lib/udev/rules.d
RULES_DEST_ALTERNATE=/etc/udev/rules.d
GROUP=labjack
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
		echo "Please restart your computer now for the device rule changes to take effect."
	elif [ $NEED_RELOG -eq $TRUE ]; then
		echo "Please log off and log back in for the group changes to take effect."
	fi
	exit $e
}

##############################
# Exodriver make and install #
##############################
go cd liblabjackusb/

echo "Making.."
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
	if [ -f $RULES_DEST_ALTERNATE/$OLD_RULES ]; then
		echo "Removing old rules: $RULES_DEST_ALTERNATE/$OLD_RULES.."
		go rm $RULES_DEST_ALTERNATE/$OLD_RULES
	fi

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
	success
fi

echo "Adding $user to the $GROUP group.."
go groupadd -f $GROUP
go usermod -a -G $GROUP $user
NEED_RELOG=$TRUE

success
