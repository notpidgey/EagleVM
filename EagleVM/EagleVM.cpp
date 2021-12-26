#include "pe/pe_parser.h"
#include "virtual_machine/handle_generator.h"

int main(int argc, char* argv[])
{
    pe_parser parser("D:\\VM\\vgk.sys");
    parser.get_sections();
    parser.get_dll_imports();
    
    vm_handle_generator::create_vm_enter();
    vm_handle_generator::create_vm_exit();

    return 0;
}
