#include "eaglevm-core/compiler/label_tracker.h"

namespace eagle::asmb
{
    code_label_ptr label_tracker_t::operator[](const code_label_ptr& ptr)
    {
        tracked_labels.emplace(ptr);
        return ptr;
    }

    void label_tracker_t::clear()
    {
        tracked_labels.clear();
    }

    std::unordered_set<code_label_ptr> label_tracker_t::dump()
    {
        return tracked_labels;
    }
}
