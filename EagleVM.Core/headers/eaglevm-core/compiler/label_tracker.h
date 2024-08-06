#pragma once
#include <set>
#include <unordered_set>

#include "eaglevm-core/compiler/code_label.h"

namespace eagle::asmb
{
    class label_tracker_t
    {
    public:
        code_label_ptr operator[](const code_label_ptr& ptr);

        void clear();
        std::unordered_set<code_label_ptr> dump();

    private:
        std::unordered_set<code_label_ptr> tracked_labels;
    };

    using label_tracker_ptr = std::shared_ptr<label_tracker_t>;
}
