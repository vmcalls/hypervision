# Hypervision

> **Disclaimer:**  
> It **does not** include or execute privileged instructions nor operations because last time i uploaded the complete version of this i got banned.  
---

## Overview

The project is designed to show:
- how VMX capabilities are detected (CPUID + MSRs),
- how VMXON/VMCS regions are allocated and initialized,
- how a sandbox manager could organize per-CPU virtual contexts,
- how EPT data structures can be simulated and queried in a basic way,
- how user-mode and kernel-mode communicate through IOCTLs.

---

## Building

### Kernel (Driver)
1. Install the **Windows Driver Kit (WDK)** for your Visual Studio version.  
2. Open the solution in Visual Studio.  
3. Set the configuration to **x64 / Debug** (recommended for testing).  
4. Build the `hypervisor` project.  
5. The output `.sys` driver file will appear under:

### Usermode (EXE)
1. Build the `usermode` project (same solution).  
2. The output executable will appear as:

3. ## Running

### Environment requirements
- **Nested virtualization enabled**
- **Windows 10/11 x64** inside the guest VM.
- **Test signing enabled** in the guest:
```bash
bcdedit /set testsigning on
```
