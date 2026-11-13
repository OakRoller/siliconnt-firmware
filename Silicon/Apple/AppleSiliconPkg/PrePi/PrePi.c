/**
 * PrePi.c
 * 
 * Main implementation of SEC phase
 * 
 * PEI is not necessary in this implementation, so build up HOBs and jump right to DXE.
 * 
 * TODO: Adapt code to dynamically change PcdSystemMemorySize to use the values from FDT instead of
 * hardcoding values.
 * 
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 * 
 */


#include "PrePi.h"
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <PiDxe.h>
#include <PiPei.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/PrePiLib.h>
#include <Library/SerialPortLib.h>
#include <Library/AppleDTLib.h>

UINT32 InitializeUART(VOID);

VOID EFIAPI ProcessLibraryConstructorList(VOID);

//
// SiliconNT: keep a range the loader filled out of the hands of the DXE core.
// Boot services data: the operating system may reuse it once it has taken over.
//
STATIC VOID ReserveLoaderRange(IN UINT64 Base, IN UINT64 Size)
{
    UINT64 Start = Base & ~(UINT64)EFI_PAGE_MASK;
    UINT64 End   = ALIGN_VALUE(Base + Size, (UINT64)EFI_PAGE_SIZE);

    BuildMemoryAllocationHob(Start, End - Start, EfiBootServicesData);
}

//
// SiliconNT: the boot arguments and the Apple device tree stay where the loader
// put them. Reserve both and tell the DXE drivers where they are
// (AppleGetBootInfo() in AppleDTLib reads this HOB).
//
STATIC VOID PublishBootInfo(IN struct boot_args *BootArgs)
{
    APPLE_BOOT_INFO Info;

    Info.BootArgs       = BootArgs;
    Info.DeviceTree     = (dt_node_t *)((UINTN)BootArgs->devtree - BootArgs->virt_base + BootArgs->phys_base);
    Info.DeviceTreeSize = BootArgs->devtree_size;

    DEBUG(
        (EFI_D_INFO | EFI_D_LOAD,
        "Boot arguments: 0x%p, device tree: 0x%p (size 0x%x)\n",
        Info.BootArgs, Info.DeviceTree, Info.DeviceTreeSize));

    BuildGuidDataHob(&gAppleSiliconPkgBootInfoHobGuid, &Info, sizeof(Info));

    ReserveLoaderRange((UINTN)Info.BootArgs, sizeof(struct boot_args));
    ReserveLoaderRange((UINTN)Info.DeviceTree, Info.DeviceTreeSize);
}

VOID Main(IN VOID *StackBase, IN UINTN StackSize, IN VOID *BootArgsPtr, IN UINT64 UefiMemoryBase)
{
    EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
    EFI_STATUS Status;

    UINTN UefiMemoryLength = FixedPcdGet32(PcdSystemMemoryUefiRegionSize);

    DEBUG(
      (EFI_D_INFO | EFI_D_LOAD,
       "UEFI Memory Base = 0x%llx, Size = 0x%llx, Stack Base = 0x%p, Stack "
       "Size = 0x%llx\n",
       UefiMemoryBase, UefiMemoryLength, StackBase, StackSize));

    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Setting up DXE Hand-Off Blocks.\n"));

    HobList = HobConstructor(
        (VOID *)UefiMemoryBase,
        UefiMemoryLength,
        (VOID *)UefiMemoryBase,
        StackBase
    );

    PrePeiSetHobList(HobList);

    //set up memory HOBs
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Invalidating data cache.\n"));

    InvalidateDataCacheRange((VOID *)(UINTN)PcdGet64(PcdFdBaseAddress), PcdGet32(PcdFdSize));    

    DEBUG(
        (EFI_D_INFO | EFI_D_LOAD, 
        "Beginning memory hand off block setup.\n"));
    Status = MemoryPeim(UefiMemoryBase, UefiMemoryLength);
    if(EFI_ERROR(Status))
    {
        DEBUG(
            (DEBUG_ERROR | DEBUG_INFO | DEBUG_LOAD,
            "PrePi: Memory setup failed! Status: 0x%llx \n",
            Status)
            );
        DEBUG(
            (DEBUG_ERROR | DEBUG_INFO | DEBUG_LOAD,
            "Looping forever, check serial log for failure point\n")
            );
        DEBUG(
            (DEBUG_ERROR | DEBUG_INFO | DEBUG_LOAD,
            "Issue reboot command through hypervisor shell, JTAG interface, or send VDM command to reboot\n")
            );
        CpuDeadLoop();
    }

    PublishBootInfo((struct boot_args *)BootArgsPtr);

    //set up stack and CPU HOBs
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Building up Stack/CPU HOBs\n"));
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Stack Base: 0x%llx, Stack Size: 0x%llx\n", (UINT64)StackBase, StackSize));
    BuildStackHob((UINT64)StackBase, StackSize);
    BuildCpuHob(ArmGetPhysicalAddressBits(), PcdGet8(PcdPrePiCpuIoSize));

    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Setting boot mode to default settings...\n"));
    SetBootMode(BOOT_WITH_DEFAULT_SETTINGS);

    DEBUG((EFI_D_INFO | EFI_D_LOAD, "Starting PlatformPeim\n"));
    Status = PlatformPeim();
    ASSERT_EFI_ERROR(Status);

    // SEC phase needs to run library constructors by hand.
    ProcessLibraryConstructorList();

    // Assume the FV that contains the PI (our code) also contains a compressed
    // FV.
    DEBUG((DEBUG_INFO, "Decompressing FV...\n"));
    Status = DecompressFirstFv();
    ASSERT_EFI_ERROR(Status);

    // Load the DXE Core and transfer control to it
    DEBUG(
        (EFI_D_INFO | EFI_D_LOAD,
        "Loading DXE Core now\n")
        );
    Status = LoadDxeCoreFromFv(NULL, 0);
    ASSERT_EFI_ERROR(Status);
    
    //if we reach here, something has *seriously* gone wrong
    CpuDeadLoop();
}

VOID CEntryPoint(IN VOID *StackBase, IN UINTN StackSize, IN VOID *BootArgsPtr, IN UINT64 UefiMemoryBase)
{
    Main(StackBase, StackSize, BootArgsPtr, UefiMemoryBase);
    //DXE Core should not return, if it does, something is *very* wrong
    ASSERT(FALSE);
}


UINT32 InitializeUART(VOID)
{
    SerialPortInitialize();
    DEBUG(
        (EFI_D_INFO | EFI_D_LOAD, 
        "Apple Silicon Project Mu Firmware (arm64/arm64e)\n")
        );
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "If you can see this message, UART works\n"));
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "FD Base Address - 0x%llx\n", PcdGet64(PcdFdBaseAddress)));
    DEBUG((EFI_D_INFO | EFI_D_LOAD, "FV Base Address - 0x%llx\n", PcdGet64(PcdFvBaseAddress)));
    return EFI_SUCCESS;
}

//borrowed from ArmVirtPkg/PrePi/PrePi.c
VOID
RelocatePeCoffImage (
  IN  EFI_PEI_FV_HANDLE         FwVolHeader,
  IN  PE_COFF_LOADER_READ_FILE  ImageRead
  )
{
  EFI_PEI_FILE_HANDLE           FileHandle;
  VOID                          *SectionData;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  EFI_STATUS                    Status;

  FileHandle = NULL;
  Status     = FfsFindNextFile (
                 EFI_FV_FILETYPE_SECURITY_CORE,
                 FwVolHeader,
                 &FileHandle
                 );
  ASSERT_EFI_ERROR (Status);

  Status = FfsFindSectionData (EFI_SECTION_PE32, FileHandle, &SectionData);
  if (EFI_ERROR (Status)) {
    Status = FfsFindSectionData (EFI_SECTION_TE, FileHandle, &SectionData);
  }

  ASSERT_EFI_ERROR (Status);

  ZeroMem (&ImageContext, sizeof ImageContext);

  ImageContext.Handle    = (EFI_HANDLE)SectionData;
  ImageContext.ImageRead = ImageRead;
  PeCoffLoaderGetImageInfo (&ImageContext);

  if (ImageContext.ImageAddress != (UINTN)SectionData) {
    ImageContext.ImageAddress = (UINTN)SectionData;
    PeCoffLoaderRelocateImage (&ImageContext);
  }
}