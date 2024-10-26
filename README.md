# EagleVM

Playground for generating virtual machine protected x64 assembly.

## Project Breakdown

### EagleVM
Main protection application that virtualizes code.

### EagleVM.Core
Static library used by EagleVM that provides all virtualization and analysis functionality.

### EagleVM.Sandbox

Project for future testing on full binaries.

### EagleVM.Stub

This is a DLL which is used in a project that needs to be protected. The EagleVM protector application searches for the usages of the stub imports to hollow the marked code sections and create virtualized code.

## Contributing
You can reach out to me on Discord `@writecr3` for question related to contributing. I try to keep the issues well organized and marked for potential contributions. If you want to contribute and don't know where to get started or do not fully understand the project structure, reach out.

## Thank You To
- r0da
- xeroxz
- mrexodia  
- Iizerd
- And everyone else along the way that helped :)

## Resources
- [r0da - Quick look around VMP 3.x - Part 3 : Virtualization](https://whereisr0da.github.io/blog/posts/2021-02-16-vmp-3/)
- [xeroxz - VMProtect 2 - Detailed Analysis of the Virtual Machine Architecture](https://back.engineering/17/05/2021/)
- [Ninon Eyrolles - Obfuscation with Mixed Boolean-Arithmetic Expressions](https://theses.hal.science/tel-01623849/document)
- [Justus Polzin - MBA obfuscation blog](https://plzin.github.io/)
- [hasherezade - PE-Bear](https://github.com/hasherezade/pe-bear-releases)
