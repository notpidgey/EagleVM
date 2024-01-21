#include "virtual_machine/handlers/vm_handler_entry.h"

class ia32_div_handler : public vm_handler_entry
{
public:
    ia32_div_handler();

private:
    virtual instructions_vec construct_single(reg_size size) override;
};