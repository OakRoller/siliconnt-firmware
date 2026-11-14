/**
 * @file BootRamdiskHelperDxe.c
 * @author amarioguy (Arminder Singh)
 *
 * Registers the boot disk image the loader left in memory as a RAM disk.
 *
 * @version 1.0
 * @date 2022-12-25
 *
 * @copyright Copyright (c) amarioguy (Arminder Singh), 2022.
 *
 * SiliconNT: the image is no longer a file embedded in the firmware volume and
 * copied to the heap. The loader puts it into memory and names the range in the
 * "RAMDisk" property of the device tree node /chosen/memory-map; the entry code
 * (PrePi) has reserved that range, and it is registered here where it is.
 *
 */

#include <PiDxe.h>

#include "BootRamdiskHelperDxe.h"

/**
 * @brief Registers the memory the device tree names as the boot disk with RamDiskDxe.
 *
 * Nothing is copied and nothing is allocated for the image: reads and writes of
 * the RAM disk go to the memory the loader filled.
 *
 * @param ImageHandle
 * @param SystemTable
 * @return
 *
 * EFI_SUCCESS - the RAM disk is registered, or the device tree names no boot disk.
 * EFI_INVALID_PARAMETER - the device tree entry cannot be used (the entry code stops on that, so this is not expected here).
 * Others - the RAM disk protocol was not found, or it refused the range.
 */
EFI_STATUS
EFIAPI
BootRamdiskHelperDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;
    UINT64 RamDiskBase;
    UINT64 RamDiskSize;
    EFI_RAM_DISK_PROTOCOL *RamdiskProtocol;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;

    Status = AppleGetBootRamdisk(AppleGetBootInfo(), &RamDiskBase, &RamDiskSize);
    if (Status == EFI_NOT_FOUND) {
        DEBUG((DEBUG_INFO, "BootRamdiskHelperDxe: no RAMDisk entry in /chosen/memory-map, no boot disk\n"));
        return EFI_SUCCESS;
    }
    if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "BootRamdiskHelperDxe: bad RAMDisk entry in /chosen/memory-map: base 0x%llx, size 0x%llx\n", RamDiskBase, RamDiskSize));
        return Status;
    }

    Status = gBS->LocateProtocol(&gEfiRamDiskProtocolGuid, NULL, (VOID **)&RamdiskProtocol);
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "BootRamdiskHelperDxe: Couldn't find the RAMDisk protocol - %r\n", Status));
        return Status;
    }
    // DevicePath belongs to RamDiskDxe from here on; it frees it when the disk is unregistered
    Status = RamdiskProtocol->Register(RamDiskBase, RamDiskSize, &gEfiVirtualDiskGuid, NULL, &DevicePath);
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "BootRamdiskHelperDxe: Cannot register RAM Disk at 0x%llx (size 0x%llx) - %r\n", RamDiskBase, RamDiskSize, Status));
        return Status;
    }

    DEBUG((DEBUG_INFO, "BootRamdiskHelperDxe: boot disk registered in place, 0x%llx (size 0x%llx)\n", RamDiskBase, RamDiskSize));
    return EFI_SUCCESS;
}
