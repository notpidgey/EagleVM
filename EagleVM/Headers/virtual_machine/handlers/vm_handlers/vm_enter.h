#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_enter_handler : public vm_handler_entry
{
public:
    vm_enter_handler();

private:
    virtual handle_instructions construct_single(reg_size size) override;
};