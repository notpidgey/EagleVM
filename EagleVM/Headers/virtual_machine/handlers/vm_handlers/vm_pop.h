#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_pop_handler : public vm_handler_entry
{
public:
    vm_pop_handler();

private:
    virtual instructions_vec construct_single(reg_size size) override;
};