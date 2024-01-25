# EagleProtect

Playground for generating virtual machine protected x64 assembly.

## EagleVM

Main protection application that virtualizes code.

## EagleVMSandbox

Project for future testing on full binaries.

## EagleVMStub

This is a DLL which is used in a project that needs to be protected. The EagleVM protector application searches for the usages of the stub imports to hollow the marked code sections and create virtualized code.

## How it Works

### Imports

Todo

### Code Sections

Todo

### PE VM Section

Todo

### Wishlist 
If you're looking to contribute to the project here are some ideas of what I would like to add.
- Implement encryption for entering VM 
- Potential project refactor
- Implement code obfuscation options
- For chunks of virtualized instructions, a VM enter could decrypt them using some kind of algorithm at runtime.
- Potential CMKR implementation instead of normal vanilla CMake
- Use of smart pointers over C-style pointers since code_labels will never be deallocated properly until the program closes
- Unit tests starting with MBA generation
- For each virtualized code section: instead of assuming there is no stack trickery going on, jump to a function in VM section which allocates stack space and then pushes an address (easy)
- Create proper way of determining what VM handler an instruction should call based on its operands

## Thank You To:

- r0da - Inspiring this project with [VMP3 Virtulization](https://whereisr0da.github.io/blog/posts/2021-02-16-vmp-3/) analysis.
- _xeroxz - Great analysis of [VMP2 Virtualization](https://back.engineering/17/05/2021/) and clarification of VMP routines.
- Iizerd - Help with general understanding of code virtualization when starting the project.
- hasherezade - Creator of tool [PE-Bear](https://github.com/hasherezade/pe-bear-releases) used for this project.

## Resources:
Todo