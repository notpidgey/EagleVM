#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_store_handler : public vm_handler_entry
{
public:
    vm_store_handler();

private:
    virtual handle_instructions construct_single(reg_size size) override;
};