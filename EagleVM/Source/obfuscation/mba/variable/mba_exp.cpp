#include "obfuscation/mba/variable/mba_exp.h"
#include "obfuscation/mba/variable/mba_xy.h"

mba_var_exp::mba_var_exp()
	: mba_variable(type_none, mod_none)
{
	operation = op_none;
}

mba_var_exp::mba_var_exp(const mba_var_exp& other)
	: mba_variable(expression, other.modifier)
{
	vars[0] = other.vars[0]->clone();
	vars[1] = other.vars[1]->clone();
	operation = other.operation;
}

mba_var_exp::mba_var_exp(std::unique_ptr<mba_variable> v1, std::unique_ptr<mba_variable> v2, truth_operator v_operation, variable_modifier modifier)
	: mba_variable(expression, modifier)
{
	vars[0] = std::move(v1);
	vars[1] = std::move(v2);
	operation = v_operation;
}

std::string mba_var_exp::print() const
{
	std::string op;
	switch (operation)
	{
	case op_none:
		op = "?";
		break;
	case op_xor:
		op = "^";
		break;
	case op_plus:
		op = "+";
		break;
	case op_minus:
		op = "-";
		break;
	case op_and:
		op = "&";
		break;
	case op_or:
		op = "|";
		break;
	case op_mul:
		op = "*";
		break;
	default:
		break;
	}

	return "(" + modifier_string() + "(" + vars[0]->print() + op + vars[1]->print() + "))";
}

void mba_var_exp::expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y)
{
	for (int i = 0; i < vars.size(); i++)
	{
		std::unique_ptr<mba_variable>& var = vars[i];
		switch (var->type)
		{
		case expression:
		{
			var->expand(x, y);
			break;
		}
		case variable:
		{
			mba_var_xy& xy = dynamic_cast<mba_var_xy&>(*var);
            variable_modifier template_mod = xy.modifier;

            switch (xy.variable_type)
			{
			case var_x:
			{
				std::unique_ptr<mba_variable> copy = x->clone();
				vars[i] = std::move(copy);
                break;
			}
			case var_y:
			{
				std::unique_ptr<mba_variable> copy = y->clone();
				vars[i] = std::move(copy);
                break;
			}
			default:
				break;
			}

            if (template_mod == mod_none)
            {
                // current template does not have a modifier
                // we can override it with the variable modifier
                template_mod = vars[i]->modifier;
            }
            else
            {
                // the template has a modifier
                if (vars[i]->modifier != mod_none)
                {
                    // the variable also has a modifier
                    if (vars[i]->modifier == template_mod)
                    {
                        // the modifier agrees, we can cancel it (will not always be true but just for now)
                        template_mod = mod_none;
                    }
                    else
                    {
                        __debugbreak();
                    }
                }
            }

            vars[i]->modifier = template_mod;
			break;
		}
		case constant:
		{
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

void mba_var_exp::walk(const std::function<void(mba_var_exp*)>& walk_callback)
{
	walk_callback(this);

	for (int i = 0; i < vars.size(); i++)
		vars[i]->walk(walk_callback);
}

void mba_var_exp::walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback)
{
	if (vars[0]->type != expression && vars[1]->type != expression)
	{
		// we reached the bottom, allow expand
		walk_callback(this);
	}
	else
	{
		// more to go
		for (int i = 0; i < vars.size(); i++)
			vars[i]->walk_bottom(walk_callback);
	}
}

std::unique_ptr<mba_variable> mba_var_exp::clone()
{
	mba_var_exp clone;
	clone.modifier = modifier;
	clone.operation = operation;
	clone.type = type;
	clone.vars[0] = vars[0]->clone();
	clone.vars[1] = vars[1]->clone();

	return std::make_unique<mba_var_exp>(clone);
}

std::unique_ptr<mba_var_exp> mba_var_exp::clone_exp()
{
	mba_var_exp clone;
	clone.modifier = modifier;
	clone.operation = operation;
	clone.type = type;
	clone.vars[0] = vars[0]->clone();
	clone.vars[1] = vars[1]->clone();

	return std::make_unique<mba_var_exp>(clone);
}