#include "eaglevm-core/obfuscation/mba/variable/mba_var.h"

namespace eagle::mba
{
	mba_variable::mba_variable()
	{
		this->type = type_none;
		this->modifier = mod_none;
	}

	mba_variable::mba_variable(variable_type type, variable_modifier modifier)
	{
		this->type = type;
		this->modifier = modifier;
	}

	std::string mba_variable::print() const
	{
		return "ERR";
	}

	std::string mba_variable::modifier_string() const
	{
		std::string mod;
		switch (modifier)
		{
			case mod_none:
				mod = "";
			break;
			case mod_not:
				mod = "~";
			break;
			case mod_neg:
				mod = "-";
			break;
			default:
				break;
		}

		return mod;
	}
}