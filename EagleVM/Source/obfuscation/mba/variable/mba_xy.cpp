#include "obfuscation/mba/variable/mba_xy.h"

mba_var_xy::mba_var_xy(variable_xy var, variable_modifier modifier)
	: mba_variable(variable, modifier)
{
	variable_type = var;
}

std::string mba_var_xy::print() const
{
	switch (variable_type)
	{
	case var_x:
		return modifier_string() + "X";
	case var_y:
		return modifier_string() + "Y";
	default:
		return "<unknown>";
	}
}

void mba_var_xy::expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y)
{
	// should be exapnded by operation expand function
	// should not happen
	return;
}

void mba_var_xy::walk(const std::function<void(mba_var_exp*)>& walk_callback)
{
	return;
}

void mba_var_xy::walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback)
{
	return;
}

std::unique_ptr<mba_variable> mba_var_xy::clone()
{
	return std::make_unique<mba_var_xy>(*this);
}