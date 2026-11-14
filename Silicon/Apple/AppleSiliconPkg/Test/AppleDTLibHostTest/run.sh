#!/bin/sh
# Host test of AppleGetBootRamdisk(): compiles BootRamdiskTest.c, which includes
# Library/AppleDTLib/AppleDTLib.c, for the host against the Mu headers of this tree.
# Needs: cc. Usage: ./run.sh
set -e
cd "$(dirname "$0")"
root=../../../../..
case "$(uname -m)" in
  x86_64) arch=X64 ;;
  aarch64) arch=AArch64 ;;
  *) echo "run.sh: no MdePkg processor headers for host $(uname -m)" >&2; exit 1 ;;
esac
out="$(mktemp)"
trap 'rm -f "$out"' EXIT
# MDEPKG_NDEBUG: no DebugLib. ARM_LIB_H_: skip ArmLib.h, which AppleDTLib.c includes
# without using it and which refuses to compile for anything but ARM.
${CC:-cc} -Wall -Werror -fshort-wchar -DMDEPKG_NDEBUG -DARM_LIB_H_ \
  -I "$root/MU_BASECORE/MdePkg/Include" -I "$root/MU_BASECORE/MdePkg/Include/$arch" \
  -I ../../Include \
  BootRamdiskTest.c -o "$out"
"$out"
