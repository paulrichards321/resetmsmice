#!/bin/bash

function find-it {
	resetmsmice=`which resetmsmice`
	if [ $? -ne 0 ]
	then
		echo "Warning: resetmsmice does not appear to be installed, can't find it in the default path!"
		export resetmsmice="resetmsmice"	
	else
		export resetmsmice=`which resetmsmice | head -n 1`
	fi
}

function enable-upstart {
	find-it
	if [ ! -d /etc/init ]
	then
		echo "Error: Cannot find upstart directory '/etc/init'"
		echo "Are you sure your system uses upstart?"
		echo ""
		exit 1
	fi
	touch /etc/init/resetmsmice.conf
	if [ "$?" -ne 0 ]
	then
		echo "Error: Cannot write configuration file '/etc/init/resetmsmice.conf'"
		echo "Are you sure you have permissions to write to this file and directory?"
		echo "Hint: Try running as root."
		echo ""
		exit 2
	fi
	echo "Installing resetmsmice upstart scripts..."
	echo "# resetmsmice
# 
# repairs abnormally fast vertical scroll wheel issues with
# certain Microsoft wireless mice and X.org.
# run 'resetmsmice --help' for more info.

description \"Resetting any Wireless Microsoft mice that are known to cause scroll wheel issues with X.org\"

author \"paulrichards321@gmail.com\"

start on started udev

exec $resetmsmice -u

console output" > /etc/init/resetmsmice.conf
	if [ "$?" -eq 0 ]
	then
		echo "Complete!"
		exit 0
	else
		echo "Installation failed writing to '/etc/init/resetmsmice.conf'!"
		exit 3
	fi
}

function enable-systemd {
	find-it
	if [ ! -d /etc/systemd/system ]
	then
		echo "Error: Cannot find systemd directory '/etc/systemd/system'"
		echo "Are you sure your system uses systemd startup scripts?"
		exit 1
	fi
	touch /etc/systemd/system/resetmsmice.service
	if [ "$?" -ne 0 ]
	then
		echo "Error: Cannot write configuration file '/etc/systemd/system/resetmsmice.service'"
		echo "Are you sure you have permissions to write to this file?"
		echo "Hint: Try running as root."
		exit 2
	fi
	echo Installing resetmsmice systemd startup scripts...
	echo "[Unit]
Description=Fixes scroll wheel issues with certain Wireless Microsoft mice in X.org
After=udev.service

[Service]
ExecStart=$resetmsmice -u
Type=forking

[Install]
WantedBy=graphical.target" > /etc/systemd/system/resetmsmice.service
	if [ "$?" -eq 0 ]
	then
		echo "Complete!"
		exit 0
	else
		echo "Installation failed writing to '/etc/systemd/system/resetmsmice.service'!"
		exit 3
	fi
}

function enable-sysv {
	find-it
	if [ ! -d /etc/init.d ]
	then
		echo "Error: Cannot find System V directory '/etc/init.d' are you sure your system uses System V startup scripts?"
		exit 1
	fi
	touch /etc/init.d/resetmsmice.sh
	if [ "$?" -ne 0 ]
	then
		echo "Error: Cannot write configuration file '/etc/init.d/resetmsmice.sh'"
		echo "Are you sure you have permissions to write to this file?"
		echo "Hint: Try running as root."
		exit 2
	fi
	echo "Installing resetmsmice System V startup scripts..."
	echo "#!/bin/sh" > /etc/init.d/resetmsmice.sh
	echo "$resetmsmice -u" >> /etc/init.d/resetmsmice.sh
	if [ $? -eq 0 ]; then \
		ln -s /etc/init.d/resetmsmice.sh /etc/rc2.d/99resetmsmice.sh
		ln -s /etc/init.d/resetmsmice.sh /etc/rc3.d/99resetmsmice.sh
		ln -s /etc/init.d/resetmsmice.sh /etc/rc4.d/99resetmsmice.sh
		ln -s /etc/init.d/resetmsmice.sh /etc/rc5.d/99resetmsmice.sh
	fi
	if [ "$?" -eq 0 ]
	then
		echo "Complete!"
		exit 0
	else
		echo "Installation failed creating links to '/etc/init.d/resetmsmice.sh'!"
		exit 3
	fi
}

function disable-all {
	rm -f /etc/init/resetmsmice.conf
	rm -f /etc/systemd/system/resetmsmice.service
	rm -f /etc/init.d/resetmsmice.sh
	rm -f /etc/rc2.d/99resetmsmice.sh
	rm -f /etc/rc3.d/99resetmsmice.sh
	rm -f /etc/rc4.d/99resetmsmice.sh
	rm -f /etc/rc5.d/99resetmsmice.sh
	echo "Full uninstall complete!"
}


case "$1" in

--upstart) enable-upstart ;;

--systemd) enable-systemd ;;

--sysv) enable-sysv ;;

--disable) disable-all ;;

*)	echo ""
	echo "To enable resetmsmice to run each time linux boots up, please run 'resetmsmice-enable-boot.sh' with one of the following options as root:"
	echo ""
	echo "If you run a recent version of Ubuntu, Kubuntu, Mint Linux, or any other Linux distro that runs upstart:"
	echo "resetmsmice-enable-boot.sh --upstart"
	echo ""
	echo "If you run a recent version Fedora or OpenSUSE or any other Linux distro that uses systemd and want systemd to launch resetmsmice automatically on bootup run:"
	echo "resetmsmice-enable-boot.sh --systemd"
	echo ""
	echo "If you run Debian, Redhat, or any other distro that uses System V style startup scripts and want resetmsmice to run automatically on bootup run:"
	echo "resetmsmice-enable-boot.sh --sysv"
	echo ""
	echo "To stop resetting the mouse on bootup, do the following:"
	echo "resetmsmice-enable-boot.sh --disable"
	echo ""
	echo "Don't know what option to use!? System V startup scripts are fairly common and a lot of distros still keep it for backwards compatitibility so if you don't know what type of startup scripts your system uses, as root, try running:"
	echo "resetmsmice-enable-boot.sh --sysv"
	echo ""

	;;

esac

echo "If you want to fix your mouse right now, instead of waiting to reboot your PC, just run as root: resetmsmice"
echo ""
