#! /bin/sh -

# Given an arg, P, where P is a suitable lib-dir, the scripts prints:
#
#   P/$distro:P
#
# $distro is SuSE, Ubuntu or Default (at present, only SuSE and Ubuntu
# require customized installations). The above path combo may be used in
# the definition of LD_LIBRARY_PATH.
#
# If no arg is supplied, the script prints $distro.

# Ensure that Xilinx's updates to LD_LIBRARY_PATH don't cause this script
# to malfunction.
unset LD_LIBRARY_PATH

id=$(lsb_release -i 2>/dev/null | sed 's/^.*:[ 	]*//' | tr '[:upper:]' '[:lower:]')
# if [ $? -ne 0 ]; then
#   echo 1>&2 "$0: ensure that lsb_release exists and works correctly."
# fi
distro=Default

case "$id" in
  *centos*) ;;
  *debian*) ;;
  *fedora*) ;;
  *redhat*) ;;
  *suse*)   distro=SuSE ;;
  *ubuntu*) distro=Ubuntu ;;
  *)        ;;
esac

if [ $# -eq 0 ]; then
  echo $distro
else
  echo "$1/$distro":"$1"
fi
