#include "virtual_machine/vm_mba.h"

#define u_var_op(x, y, z) std::make_unique<mba_var_exp>(x, y, z)
#define u_var_xy(x) std::make_unique<mba_var_xy>(x)
#define u_var_const(x, y) std::make_unique<mba_var_const<int>>(y)

vm_mba::vm_mba()
{
	// this has so much potential to be expanded
	// you can add your own truths, build your own truths, randomize truths per operator etc

	// (X ^ Y) = (X | Y) - (X & Y)
	{
		mba_var_exp& xor_truth = mba_base_truth[op_xor];
		xor_truth.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_or);
		xor_truth.vars[1] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_and);
		xor_truth.operation = op_minus;
	}

	// (X + Y) = (X & Y) + (X | Y)
	{
		mba_var_exp& plus_truth = mba_base_truth[op_plus];
		plus_truth.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_and);
		plus_truth.vars[1] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_or);
		plus_truth.operation = op_plus;
	}

	// (X - Y) = (X ^ (-1 * Y)) + (2 * (X & (-1 * Y)))
	{
		mba_var_exp& minus_truth = mba_base_truth[op_minus];
		minus_truth.vars[0] = u_var_op(u_var_xy(var_x), u_var_op(u_var_const(int8_t, -1), u_var_xy(var_y), op_mul), op_xor);
		minus_truth.vars[1] = u_var_op(u_var_const(uint8_t, 2), u_var_op(u_var_xy(var_x), u_var_op(u_var_const(int8_t, -1), u_var_xy(var_y), op_mul), op_and), op_mul);
		minus_truth.operation = op_plus;
	}

	// (X & Y) = (X + Y) - (X | Y)
	{
		mba_var_exp& and_truth = mba_base_truth[op_and];
		and_truth.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_plus);
		and_truth.vars[1] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_or);
		and_truth.operation = op_minus;
	}

	// (X | Y) = ((X + Y) + 1) + (~X | ~Y)
	{
		mba_var_exp& or_truth = mba_base_truth[op_or];
		or_truth.vars[0] = u_var_op(u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_plus), u_var_const(uint8_t, 1), op_plus);
		or_truth.vars[1] = u_var_op(u_var_xy(var_y, mod_not), u_var_xy(var_y, mod_not), op_or);
		or_truth.operation = op_plus;
	}

	// https://secret.club/2022/08/08/eqsat-oracle-synthesis.html
	{
		// ~(x * y)              ->   ((~x * y) + (y - 1))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_mul;

			mba_var_exp eql;
			eql.vars[0] = u_var_op(u_var_xy(var_x, mod_not), u_var_xy(var_y), op_plus);
			eql.vars[1] = u_var_op(u_var_xy(var_y), u_var_const(uint32_t, 1), op_minus);
			exp.operation = op_plus;

			mba_simple_truth.push_back({ exp, eql });
		}

		// ~(x + y)              ->   (~x + (~y + 1))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_plus;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_not);
			eql.vars[1] = u_var_op(u_var_xy(var_y, mod_not), u_var_const(uint32_t, 1), op_plus);
			exp.operation = op_plus;

			mba_simple_truth.push_back({ exp, eql });
		}

		// ~(x - y)              ->   (~x - (~y + 1))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_minus;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_not);
			eql.vars[1] = u_var_op(u_var_xy(var_y, mod_not), u_var_const(uint32_t, 1), op_plus);
			exp.operation = op_minus;

			mba_simple_truth.push_back({ exp, eql });
		}

		// ~(x & y)              ->   (~x | ~y)
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_and;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_not);
			eql.vars[1] = u_var_xy(var_y, mod_not);
			exp.operation = op_or;

			mba_simple_truth.push_back({ exp, eql });
		}

		// ~(x ^ y)              ->   ((x & y) | ~(x | y))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_xor;

			mba_var_exp eql;
			eql.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_and);
			eql.vars[1] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_or, mod_not);
			exp.operation = op_or;

			mba_simple_truth.push_back({ exp, eql });
		}

		// ~(x | y)              ->   (~x & ~y)
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_not;
			exp.operation = op_or;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_not);
			eql.vars[1] = u_var_xy(var_y, mod_not);
			exp.operation = op_and;

			mba_simple_truth.push_back({ exp, eql });
		}

		// -(x * y)              ->   (-x * y)
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_neg;
			exp.operation = op_mul;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_neg);
			eql.vars[1] = u_var_xy(var_y);
			exp.operation = op_mul;

			mba_simple_truth.push_back({ exp, eql });
		}

		// -(x * y)              ->   (x * -y)
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_neg;
			exp.operation = op_mul;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x);
			eql.vars[1] = u_var_xy(var_y, mod_neg);
			exp.operation = op_mul;

			mba_simple_truth.push_back({ exp, eql });
		}

		// (x - y)               ->   (x + (-y))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.operation = op_minus;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x);
			eql.vars[0] = u_var_xy(var_y, mod_neg);
			eql.operation = op_plus;

			mba_simple_truth.push_back({ exp, eql });
		}

		// -x                    ->   (~x + 1)
		{

		}

		// ((x * y) + y)         ->   ((x + 1) * y)
		{

		}

		// -(x + y)              ->   ((-x) + (-y))
		{
			mba_var_exp exp;
			exp.vars[0] = u_var_xy(var_x);
			exp.vars[1] = u_var_xy(var_y);
			exp.modifier = mod_neg;
			exp.operation = op_plus;

			mba_var_exp eql;
			eql.vars[0] = u_var_xy(var_x, mod_neg);
			eql.vars[1] = u_var_xy(var_y, mod_neg);
			exp.operation = op_plus;

			mba_simple_truth.push_back({ exp, eql });
		}
	}
}

std::string vm_mba::create_tree(truth_operator op, uint32_t max_expansions)
{
	mba_var_exp& root_truth = mba_base_truth[op];
	std::unique_ptr<mba_var_exp> root_truth_exp = root_truth.clone_exp();

	// allow this to expand a few times until it starts to repeat
	truth_operator oper = op_none;
	for (uint32_t i = 0; i < max_expansions; i++)
	{
		if (oper == root_truth_exp->operation)
			break;

		mba_var_exp& root_expansion = mba_base_truth[root_truth_exp->operation];
		std::unique_ptr<mba_var_exp> root_expansion_exp = root_expansion.clone_exp();

		oper = root_truth_exp->operation;

		root_expansion_exp->expand(root_truth_exp->vars[0], root_truth_exp->vars[1]);
		root_truth_exp = std::move(root_expansion_exp);
	}

	return root_truth_exp->print();
}

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
			variable_modifier mod = xy.modifier;

			switch (xy.variable_type)
			{
			case var_x:
			{
				std::unique_ptr<mba_variable> copy = x->clone();
				copy->modifier = mod;

				vars[i] = std::move(copy);
			}
			break;
			case var_y:
			{
				std::unique_ptr<mba_variable> copy = y->clone();
				copy->modifier = mod;

				vars[i] = std::move(copy);
			}
			break;
			default:
				break;
			}
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
		return "X";
		break;
	case var_y:
		return "Y";
		break;
	default:
		break;
	}
}

void mba_var_xy::expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y)
{
	// should be exapnded by operation expand function
	// should not happen
	return;
}

std::unique_ptr<mba_variable> mba_var_xy::clone()
{
	return std::make_unique<mba_var_xy>(*this);
}