#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_exit_handler : public vm_handler_entry
{
public:
    vm_exit_handler();

private:
    virtual instructions_vec construct_single(reg_size size) override;
};