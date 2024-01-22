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

- Improve the system for which encryption/decryption routines are created for entering the virtual machine. Currently it is the same rol, add, ror routine. In the future this should be random and custom for each VMENTER. Ideally, when a protected function needs to enter the virtual machine, it will push a virtual address which contains a unique encryption routine in the VM section. So that during VMENTER routine, the other constant which was pushed to the stack can be decrypted uniquely.
- Everything in the VM section of the PE is in a linear order. This makes reverse engineering the application fairly easy. This will be difficult but some kind of algorithm needs to be developed to create a PE section for the virtual machine first and then at the end fill the addresses which depend on one another.
- Full project refactor? Project might become extremely messy after the item above gets added. Maybe some thinking needs to be done
- Obfuscation (duh) since a VM is not going to carry all the weight of making the generated VMs difficult to reverse engineer.
- For chunks of virtualized instructions, a VM enter could decrypt them using some kind of algorithm at runtime. Would hinder reverse engineering statically
- Potential CMKR implementation instead of normal vanilla CMake
- Use of smart pointers over C-style pointers since code_labels will never be deallocated properly until the program closes
- Unit tests starting with MBA generation

## Thank You To:

- r0da - Inspiring this project with [VMP3 Virtulization](https://whereisr0da.github.io/blog/posts/2021-02-16-vmp-3/) analysis.
- _xeroxz - Great analysis of [VMP2](https://back.engineering/17/05/2021/) and clarification of VMP routines.
- Iizerd - Help with general understanding of code virtualization when starting the project.
- hasherezade - Creator of tool [PE-Bear](https://github.com/hasherezade/pe-bear-releases) used for this project.

## Resources:
Todo