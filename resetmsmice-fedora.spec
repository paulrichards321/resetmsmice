Name:           resetmsmice
Version:        1.1.3 
Release:        1%{?dist}
Summary:        Program to reset certain models of Microsoft Mice when the vertical scroll wheel scrolls too fast. Only needed if you dual boot between Windows and Linux.

License:        GPLv2
URL:            http://sourceforge.net/projects/resetmsmice/
Source0:        http://sourceforge.net/projects/resetmsmice/files/resetmsmice-1.1.3.tar.gz

BuildRequires:  libusb1-devel, pkgconfig, gtk2-devel 
Requires:       libusb1, gtk2, pkgconfig

%description 
This fixes scroll wheel issues with certain Wireless Microsoft mice in X.org (includes KDE & Gnome applications), where the vertical wheel scrolls abnormally fast. The Microsoft mouse driver will set a scrolling mode that will not get reset when you do a warm reboot, so this is only needed if you dual boot between Microsoft Windows and some linux distro. 
%prep
%setup -q


%build
%configure --with-udevdir=/etc/udev
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%post
groupadd -f ms-usb
chgrp ms-usb %{_bindir}/resetmsmice
chmod 2755 %{_bindir}/resetmsmice
udevadm trigger --attr-match=idVendor=045e
update-desktop-database
%{_sbindir}/resetmsmice-enable-boot --enable 

%preun
%{_sbindir}/resetmsmice-enable-boot --disable

%files
%{_bindir}/resetmsmice
%{_bindir}/resetmsmice-gui
%{_sbindir}/resetmsmice-enable-boot
%{_datadir}/resetmsmice/resetmsmice.ui
%{_datadir}/applications/resetmsmice.desktop
%{_sysconfdir}/udev/rules.d/60-resetmsmice.rules

%doc AUTHORS ChangeLog COPYING NEWS README TODO


%changelog
* Fri Apr 20 2018 paulrichards321@gmail.com 1.1.3
- 1.1.3 Fixed gui loader that was failing and giving weird UTF errors when activating the disable/enable scripts. Added Microsoft Sculpt mouse to list of fixable devices. Fixed Systemd return value error. Fixed configure script problems.
* Thu Mar 20 2014 paulrichards321@gmail.com 1.1.2
- Fixed debian package installation and removal scripts. dpkg-statoverride is called differently so it doesn't cause the scripts to exit prematurely if there is no override. 
* Fri Mar 14 2014 paulrichards321@gmail.com 1.1.1
- Fixed resetmsmice-gui desktop icon on linux distributions that do not have /usr/local/sbin or /usr/sbin in the default PATH. resetmsmice-gui and resetmsmice have been moved to the bin folder. Included detection of /usr/local or /usr for executables and ui file so conflicting installed versions will not cause problems. The udev rules file location is autodetected except for rpm installs where it is installed into /etc/udev/rules.d.
* Wed Mar 5 2014 paulrichards321@gmail.com 1.1.0
- Made resetmsmice able to run without admin rights. It now runs with set group id with the group only expanding to ms-usb, with the ms-usb group able to talk to usb Microsoft mice devices via udev. Simplified the init script installer, it now autodetects what type of init setup you have, ie system v, systemd, or upstart. Made a gui for the utility to setup and remove startup scripts and reset the mouse.
* Mon Sep 9 2013 paulrichards321@gmail.com 1.0.6
- Further fixes to System V startup scripts. Tested on Centos 6.4 and Debian 7.1.
* Sat Aug 31 2013 paulrichards321@gmail.com 1.0.5
- Thanks to Sverker Wiberg and Marc Atlisent for sending patches in the fix a segfault that occurred in certain models. A variable was not initialized and comparison was not accurate in parse.c.
* Tue Sep 11 2012 paulrichards321@gmail.com 1.0.4
- Was able to the Microsoft Comfort Mouse 4500 and probably other mice working by using the current interface number as the wIndex parameter to libusb_control_transfer. 
* Sun Aug 26 2012 paulrichards321@gmail.com 1.0.3
- Enabled all warnings during compilation and found a couple of functions not returning a value. Systemd config file is now stored in /lib/systemd/system, where they should go. Added more notes to README.
* Thu Aug 2 2012 paulrichards321@gmail.com 1.0.2 
- We now check HID devices that could be a mouse, found a user who has a Microsoft Wireless Optical Desktop 3000 and the subclass and interface protocol values are 0.
* Thu Aug 2 2012 paulrichards321@gmail.com 1.0.1 
- Large code creations and rewrites. Rewrote a lot of Alan Ott's code and merged it with main.c. Now scans the usb descriptors and checks for a usb mouse interface, and then reads the basic HID report and check to see if the smooth scroll feature is there before sending out any data to the mouse.
* Fri Jul 20 2012 paulrichards321@gmail.com 1.0.0 
- Using Alan Ott's hidapi for set feature and get feature usb code. This code differs in that it performs a interrupt on the mouse before getting the feature report. This seems to be needed to perform the correct connection with the mouse. Hopefully this will now work with different models. In the enable boot script, for systemd now call systemctl enable resetmsmice.service to
enable boot script.
* Mon May 28 2012 Paul Richards <paulrichards321@gmail.com> 0.9.4
- Fixed problem with sys v init scripts not using prefix of 'S'. resetmsmice should now correctly startup on boot with sys v method. Mon Apr 02 2012 Paul Richards <paulrichards321@gmail.com> 0.9
* Thu May 24 2012 Paul Richards <paulrichards321@gmail.com> 0.9.3
- The code for the 6000 series mice does not work as expected. Will do further debugging of this once I get my hands on this model. Included message in program and in README that more debugging is needed for this model. Fixed an issue where LOG_DEBUG was too low of a priority on Centos and Fedora to get written to the syslog. Logging is now set to LOG_NOTICE.
* Thu Apr 19 2012 Paul Richards <paulrichards321@gmail.com> 0.9.2
- Fixed resetmsmice-enable-boot.sh to look in /usr/local/sbin for resetmsmice. Also fixed a automake/autoconfig problem where resetmsmice-enable-boot was not being called in the right directory.
* Thu Apr 19 2012 Paul Richards <paulrichards321@gmail.com> 0.9.1 
- Thanks to Tony Houghton, fixed a segfault issue from a missing va_end and va_start. Included test functionality for the 6000 series mice.
* Mon Apr 02 2012 Paul Richards <paulrichards321@gmail.com> 0.9 
- First public release

