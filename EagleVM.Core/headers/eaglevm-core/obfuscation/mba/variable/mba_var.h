#pragma once
#include <string>
#include <memory>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>

namespace eagle::mba
{
	enum truth_operator
	{
		op_none,
		op_xor,
		op_plus,
		op_minus,
		op_and,
		op_or,
		op_mul
	};

	enum variable_modifier
	{
		mod_none,
		mod_not,
		mod_neg,
	};

	enum variable_type
	{
		type_none,
		variable,
		constant,
		expression
	};

	class mba_var_exp;
	class mba_variable
	{
	public:
		variable_type type;
		variable_modifier modifier;

		mba_variable();
		mba_variable(variable_type type, variable_modifier modifier);
		virtual ~mba_variable() = default;

		virtual std::string print() const;
		virtual void expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y) = 0;
		virtual void walk(const std::function<void(mba_var_exp*)>& walk_callback) = 0;
		virtual void walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback) = 0;
		virtual std::unique_ptr<mba_variable> clone() = 0;

		std::string modifier_string() const;
	};
}