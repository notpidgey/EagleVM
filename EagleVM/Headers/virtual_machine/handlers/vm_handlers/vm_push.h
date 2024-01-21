#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_push_handler : public vm_handler_entry
{
public:
    vm_push_handler();

private:
    virtual dynamic_instructions_vec construct_single(reg_size size) override;
};