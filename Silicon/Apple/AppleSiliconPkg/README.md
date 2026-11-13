# AppleSiliconPkg

Everything in this directory was taken from
[NT-for-ASi/apple_silicon_platforms_mu](https://github.com/NT-for-ASi/apple_silicon_platforms_mu)
at commit `861dcc490c564bd362197fdd7c30fcf4f7cafd15`, directory
`Silicon/Apple/AppleSiliconPkg`. `LICENSE` is that repository's licence file. Each
file keeps the notice it had; some had none.

That repository is not an upstream of this one: updates are not followed.

| Here | What | Changed |
|---|---|---|
| `PrePi/` | Entry from m1n1, hand-off blocks, jump to the DXE core | The boot arguments address is taken from `x0` instead of `x4`. The non-relocating entry file was not taken. `LzmaDecompressLib` removed from the module's library list (the platform build links the LZMA library explicitly) |
| `Library/AppleDTLib/` | Apple device tree reader (from pongoOS) | — |
| `Library/AppleUartSerialPortLib/` | Samsung-style serial port | — |
| `Library/PlatformPeiLib/` | Publishes the firmware volume | — |
| `Drivers/SimpleFbDxe/` | Framebuffer left by iBoot and m1n1 | — |
| `Drivers/BootRamdiskHelperDxe/` | Registers a ramdisk embedded in the image | — |
| `Drivers/SmbiosInfoDxe/` | SMBIOS tables | The Mu version library and two unused package references removed, with the two calls that wrote the version and date into string literals |
| `AppleSiliconFdfRules.fdf.inc` | Image build rules | — |
| `AppleSiliconPkg.dec` | Package declaration | Cut down to what the files above use |

Line endings were converted to LF.

Also derived from the same commit, outside this directory:

| File | From |
|---|---|
| `Platform/MacBookProM4Pkg/Library/MemoryInitPeiLib/` | `Silicon/Apple/T602XFamilyPkg/Library/MemoryInitPeiLib/` |
| `Platform/MacBookProM4Pkg/AcpiTables/Madt.aslc`, `Gtdt.aslc` | `Silicon/Apple/T602XFamilyPkg/AcpiTables/` |
| `Platform/MacBookProM4Pkg/AcpiTables/Fadt.aslc`, `Dbg2.aslc` | `Platform/MacBookProEarly2023Pkg/AcpiTables/` |
| `Platform/MacBookProM4Pkg/Include/Acpi/AcpiHeader.h` | `Silicon/Apple/AppleSiliconPkg/Include/Acpi/AcpiHeader.h` |
| `Platform/MacBookProM4Pkg/MacBookProM4.dsc`, `.fdf`, `PlatformBuild.py` | `Silicon/Apple/AppleSiliconPkg/AppleSiliconPkg.dsc.inc`, `Platform/MacBookProEarly2023Pkg/` |
