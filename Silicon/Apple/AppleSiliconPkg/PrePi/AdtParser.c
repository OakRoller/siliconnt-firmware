/*
 * Copyright (c) 2015, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <Library/AppleDTLib.h>

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))

//
// SiliconNT: the boot arguments and the device tree are used where the loader
// put them; nothing is copied to a fixed address any more.
//
BOOLEAN
EarlySetup (
  IN VOID     *BootArgsAddr,
  OUT UINT64  *SystemMemoryBase,
  OUT UINT64  *SystemMemorySize
  )
{

  struct boot_args *BootArgs = (struct boot_args *)BootArgsAddr;
  VOID *DeviceTree = (VOID *)((UINTN)BootArgs->devtree - BootArgs->virt_base + BootArgs->phys_base);

  if(dt_check(DeviceTree, BootArgs->devtree_size, NULL) != 0) {
    //
    // Going on would take the memory base and size from the build-time
    // defaults, which describe no real machine.
    //
    DEBUG((EFI_D_INFO | EFI_D_LOAD | EFI_D_ERROR, "no ADT supplied, stopping here\n"));
    CpuDeadLoop();
  }

  *SystemMemoryBase = BootArgs->phys_base;
  *SystemMemorySize = BootArgs->mem_size;


  PatchPcdSet64(PcdFrameBufferAddress, BootArgs->video.base);
  PatchPcdSet64(PcdFrameBufferSize, ALIGN_UP(BootArgs->video.stride * BootArgs->video.height, 0x4000)); // for notched/internal displays, the FB size has to be aligned up!

  DEBUG((EFI_D_INFO | EFI_D_LOAD | EFI_D_ERROR, "Framebuffer address loaded to PCDs: 0x%llx (size 0x%llx)\n", PcdGet64(PcdFrameBufferAddress), PcdGet64(PcdFrameBufferSize)));

  return TRUE;
}
