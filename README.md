# EagleProtect

Playground for generating virtual machine protected x64 assembly.

## EagleVM
First instruction virtualized 2/13/2024!

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

## Potential Contributions
The project is difficult to maintain and develop solo, if you are looking to contribute I encourage you to take a look at any of the following bugs or possible features!
### Bugs
- The virtualizer does not handle cases where there are jumps into virtualized code resulting in undefined behavior
- The idea for base instruction virtualization is not concrete. There will be edge cases for future handlers where the virtualization of operands might have to be different and will not function generically.
### Features
- Implement encryption routine for entering VM (can be replaced by MBA)
- Utilize the MBA generation by taking in target registers and an expression to turn into a set of instructions.
- Control flow flattening and other kinds of mutation
- For chunks of VM code, a VM enter could decrypt them using some kind of algorithm at runtime.
- Simple VM packer.
- Implement a proper stack check instead of allocating a static amount of stack space for virtualized code.
- Import table builder. Allow for addition and removal of imports.
### Refacatoring
- Potential CMKR implementation instead of normal vanilla CMake
- Implementation of C++ exceptions to handle deeply nested exceptions that occur while virtualizing instead of using INT3s
- Dealing with RECOMPILE macro and making the code cleaner when creating a function container. I do not see how this is possible without causing container builders to become longer.
- Use of smart pointers over C-style pointers since code_labels will never be deallocated properly until the program closes
- Unit tests starting with MBA generation

## Thank You To:
- r0da - [VMP3 Virtulization](https://whereisr0da.github.io/blog/posts/2021-02-16-vmp-3/) and clarification.
- _xeroxz - [VMP2 Virtualization](https://back.engineering/17/05/2021/) and clarification.
- mrexodia - Project contribution and advice. 
- Iizerd - General virtualization clarification.

## Resources:
- [Quick look around VMP 3.x - Part 3 : Virtualization](https://whereisr0da.github.io/blog/posts/2021-02-16-vmp-3/)
- [VMProtect 2 - Detailed Analysis of the Virtual Machine Architecture](https://back.engineering/17/05/2021/)
- [Obfuscation with Mixed Boolean-Arithmetic Expressions : reconstruction, analysis and simplification tools](https://theses.hal.science/tel-01623849/document)
- [Justus Polzin](https://plzin.github.io/)
- [PE-Bear](https://github.com/hasherezade/pe-bear-releases)