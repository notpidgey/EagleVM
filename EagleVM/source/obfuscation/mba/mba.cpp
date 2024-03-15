#include "obfuscation/mba/mba.h"

#include "util/random.h"

#include <algorithm>
#include <random>

#define u_var_op(...) std::make_unique<mba_var_exp>(__VA_ARGS__)
#define u_var_xy(...) std::make_unique<mba_var_xy>(__VA_ARGS__)
#define u_var_const(x, y) std::make_unique<mba_var_const<x>>(y)

template <typename T>
mba_gen<T>::mba_gen(uint8_t size)
{
	// this has so much potential to be expanded
	// you can add your own truths, build your own truths, randomize truths per operator etc

    gen_size = size;

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
		minus_truth.vars[1] = u_var_op(u_var_const(T, 2), u_var_op(u_var_xy(var_x), u_var_op(u_var_const(int8_t, -1), u_var_xy(var_y), op_mul), op_and), op_mul);
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
		or_truth.vars[0] = u_var_op(u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_plus), u_var_const(T, 1), op_plus);
		or_truth.vars[1] = u_var_op(u_var_xy(var_x, mod_not), u_var_xy(var_y, mod_not), op_or);
		or_truth.operation = op_plus;
	}

	// https://secret.club/2022/08/08/eqsat-oracle-synthesis.html
	{
		// ~(x * y) -> ((~x * y) + (y - 1))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_mul;

			mba_var_exp result{};
			result.vars[0] = u_var_op(u_var_xy(var_x, mod_not), u_var_xy(var_y), op_plus);
			result.vars[1] = u_var_op(u_var_xy(var_y), u_var_const(uint32_t, 1), op_minus);
			result.operation = op_plus;

			mba_simple_truth.push_back({ target, result });
		}

		// ~(x + y) -> (~x + (~y + 1))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_plus;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_not);
			result.vars[1] = u_var_op(u_var_xy(var_y, mod_not), u_var_const(uint32_t, 1), op_plus);
			result.operation = op_plus;

			mba_simple_truth.push_back({ target, result });
		}

		// ~(x - y) -> (~x - (~y + 1))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_minus;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_not);
			result.vars[1] = u_var_op(u_var_xy(var_y, mod_not), u_var_const(uint32_t, 1), op_plus);
			result.operation = op_minus;

			mba_simple_truth.push_back({ target, result });
		}

		// ~(x & y) -> (~x | ~y)
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_and;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_not);
			result.vars[1] = u_var_xy(var_y, mod_not);
			result.operation = op_or;

			mba_simple_truth.push_back({ target, result });
		}

		// ~(x ^ y) -> ((x & y) | ~(x | y))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_xor;

			mba_var_exp result{};
			result.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_and);
			result.vars[1] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_or, mod_not);
			result.operation = op_or;

			mba_simple_truth.push_back({ target, result });
		}

		// ~(x | y) -> (~x & ~y)
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_not;
			target.operation = op_or;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_not);
			result.vars[1] = u_var_xy(var_y, mod_not);
			result.operation = op_and;

			mba_simple_truth.push_back({ target, result });
		}

		// -(x * y) -> (-x * y)
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_neg;
			target.operation = op_mul;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_neg);
			result.vars[1] = u_var_xy(var_y);
			result.operation = op_mul;

			mba_simple_truth.push_back({ target, result });
		}

		// -(x * y) -> (x * -y)
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_neg;
			target.operation = op_mul;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x);
			result.vars[1] = u_var_xy(var_y, mod_neg);
			result.operation = op_mul;

			mba_simple_truth.push_back({ target, result });
		}

		// (x - y) -> (x + (-y))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.operation = op_minus;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x);
			result.vars[1] = u_var_xy(var_y, mod_neg);
			result.operation = op_plus;

			mba_simple_truth.push_back({ target, result });
		}

		// -x -> (~x + 1)
		{

		}

		// ((x * y) + y) -> ((x + 1) * y)
		{

		}

		// -(x + y) -> ((-x) + (-y))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_neg;
			target.operation = op_plus;

			mba_var_exp result{};
			result.vars[0] = u_var_xy(var_x, mod_neg);
			result.vars[1] = u_var_xy(var_y, mod_neg);
			result.operation = op_plus;

			mba_simple_truth.push_back({ target, result });
		}

		// (X + Y) -> ((X + Y) - (X & 0))
		{
			mba_var_exp target{};
			target.vars[0] = u_var_xy(var_x);
			target.vars[1] = u_var_xy(var_y);
			target.modifier = mod_none;
			target.operation = op_plus;

			mba_var_exp result{};
			result.vars[0] = u_var_op(u_var_xy(var_x), u_var_xy(var_y), op_plus);
			result.vars[1] = u_var_op(u_var_xy(var_x), u_var_const(uint32_t, 0), op_and);
			result.operation = op_minus;

			mba_simple_truth.push_back({ target, result });
		}
	}

	// self equivalent truths
	{
		// (X) -> ~(~X + 0)
		{
			mba_var_exp eql{ u_var_xy(var_x), u_var_const(uint32_t, 0), op_plus, mod_not };
			mba_variable_truth.push_back(eql);
		}
		
		// (X) -> (X + 0)
		{
			mba_var_exp eql{ u_var_xy(var_x), u_var_const(uint32_t, 0), op_plus };
			mba_variable_truth.push_back(eql);
		}
		
		// (X) -> (X - 0)
		{
			mba_var_exp eql{ u_var_xy(var_x), u_var_const(uint32_t, 0), op_minus };
			mba_variable_truth.push_back(eql);
		}
		
		// (X) -> (X | 0)
		{
			mba_var_exp eql{ u_var_xy(var_x), u_var_const(uint32_t, 0), op_or };
			mba_variable_truth.push_back(eql);
		}
		
		// (X) -> (X ^ 0)
		{
			mba_var_exp eql{ u_var_xy(var_x), u_var_const(uint32_t, 0), op_xor };
			mba_variable_truth.push_back(eql);
		}
	}

	// zero truths
	{
		// (X ^ Y) ^ (X ^ Y)

		// (X + Y) & 0

		// (X | Y) ^ (X | Y)

		// (X & Y) ^ (X & Y)

		// (X & ~X)

		// (X ^ X)
	}
}

#include <iostream>

template <typename T>
std::string mba_gen<T>::create_tree(truth_operator op, uint32_t max_expansions, uint8_t equal_expansions)
{
	mba_var_exp& root_truth = mba_base_truth[op];
	std::unique_ptr<mba_var_exp> root_truth_exp = root_truth.clone_exp();

	// TODO: add function to walk from bottom of tree, top of tree, and random
	for (uint32_t i = 0; i < max_expansions; i++)
	{
		std::cout << "[" << i << "] " << root_truth_exp->print() << std::endl;

		// walk the table to expand with simple truths
		bottom_expand_base(root_truth_exp);

		// walk the table to expand with self equivalent truths
		//bottom_expand_simple(root_truth_exp);

		// walk every constant and just create junk
		bottom_expand_variable(root_truth_exp);

        bottom_insert_identity(root_truth_exp);
	}

	return root_truth_exp->print();
}

template <typename T>
void mba_gen<T>::bottom_expand_base(std::unique_ptr<mba_var_exp>& exp)
{
	// expand bottoms
	exp->walk_bottom([&](mba_var_exp* inst)
		{
			if (!mba_base_truth.contains(inst->operation))
				return;

			mba_var_exp& root_expansion = mba_base_truth[inst->operation];
			std::unique_ptr<mba_var_exp> root_expansion_exp = root_expansion.clone_exp();

			root_expansion_exp->expand(inst->vars[0], inst->vars[1]);
			inst->vars[0] = std::move(root_expansion_exp->vars[0]);
			inst->vars[1] = std::move(root_expansion_exp->vars[1]);
			inst->operation = root_expansion_exp->operation;
			inst->modifier = root_expansion_exp->modifier;
		}
	);
}

template <typename T>
void mba_gen<T>::bottom_expand_simple(std::unique_ptr<mba_var_exp>& exp)
{
	exp->walk_bottom([this](mba_var_exp* inst)
		{
			std::ranges::for_each(mba_simple_truth,
			[inst](std::pair<mba_var_exp, mba_var_exp>& pair)
				{
					auto& [target_exp, result_exp] = pair;
					if (inst->operation != target_exp.operation)
						return;

					if (target_exp.modifier != mod_none && inst->modifier != target_exp.modifier)
						return;

					std::array<std::unique_ptr<mba_variable>, 2>& vars = inst->vars;
					std::unique_ptr<mba_variable>& x_var = vars[0];
					std::unique_ptr<mba_variable>& y_var = vars[1];

					std::unique_ptr<mba_variable>& target_x_var = target_exp.vars[0];
					std::unique_ptr<mba_variable>& target_y_var = target_exp.vars[1];

					if (target_x_var->modifier != x_var->modifier)
						return;

					if (target_y_var->modifier != y_var->modifier)
						return;

					// successful match, expand, and copy result
					std::unique_ptr<mba_var_exp> match_exp = result_exp.clone_exp();
					match_exp->expand(x_var, y_var);

					vars[0] = std::move(match_exp->vars[0]);
					vars[1] = std::move(match_exp->vars[1]);
					inst->operation = match_exp->operation;
					inst->modifier = match_exp->modifier;
				});
		});
}

template <typename T>
void mba_gen<T>::bottom_expand_variable(std::unique_ptr<mba_var_exp>& exp)
{
	return;
	exp->walk_bottom([this](mba_var_exp* inst)
		{
			auto size = mba_variable_truth.size();

			int index1 = ran_device::get().gen_16() % size;
			int index2 = ran_device::get().gen_16() % size;

			mba_var_exp& first_result = mba_variable_truth[index1];
			std::unique_ptr<mba_var_exp> first_exp = first_result.clone_exp();
			first_exp->expand(inst->vars[0], nullptr);

			inst->vars[0] = std::move(first_exp);

			mba_var_exp& second_result = mba_variable_truth[index2];
			std::unique_ptr<mba_var_exp> second_exp = second_result.clone_exp();
			second_exp->expand(inst->vars[1], nullptr);

			inst->vars[1] = std::move(second_exp);
		}
	);
}

#pragma once 

template <typename T>
void mba_gen<T>::bottom_insert_identity(std::unique_ptr<mba_var_exp>& exp)
{
    // [1] Defeating MBA-based Obfuscation
    // https://hal.science/hal-01388109v1/document

    // [2] Mixed Boolean-Arithmetic (Part 1): Introduction
    // https://plzin.github.io/posts/mba

    // [3] Extended Euclidean algorithm
    // https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm#Modular_integers

    // we are going to use the "Insertions of identities" component mentioned in [1]
    // expression e can be rewritten as p(p^{-1}(e)) where p is an invertible function on the set of integers Z/2^nZ

    // we define p(x) as p(x) = a1x+a0 where a1 is an odd term [2]
    // to compute the inverse of this function we must use the extended euclidean algorithm [3]
    // the general form of the inverse is given by q(x) = a1^{-1}x-a1^{-1}a0 [2]

	std::function<T(T)> mod_inverse = [](T a)
	{
		// An Improved Integer Multiplicative Inverse (modulo 2^w)
		// https://arxiv.org/ftp/arxiv/papers/2204/2204.04342.pdf

		// this algorith is actually genius, i cannot believe that it works

		T y = a - 1;
		T u0 = 2 - a;

		y *= y;
		T u1 = u0*(1 + y);
		y *= y;
		T u2 = u1*(1 + y);
		y *= y;
		T u3 = u2*(1 + y);
		y *= y;
		T u4 = u3*(1 + y);
		y *= y;
		T u5 = u4*(1 + y);

		return u5;
	};

    static std::uniform_int_distribution<uint64_t> distribution(0, std::numeric_limits<T>::max());
    exp->walk_bottom([&](mba_var_exp* inst)
        {
            // p(x) = a1x + a0
            // generate a a1 that is odd
            T a1 = ran_device::get().gen_dist(distribution);
            while (a1 % 2 == 0)
                a1 = ran_device::get().gen_dist(distribution);
            T a0 = ran_device::get().gen_dist(distribution);

            // q(x) = a1^{-1}x-a1^{-1}a0
            T c = mod_inverse(a1);
            T d;

            truth_operator inverse_op;
            if (ran_device::get().gen_8() % 2)
            {
                // do plus
                d = static_cast<T>((-static_cast<std::make_signed_t<T>>(c * a0)) & std::numeric_limits<T>::max());
                inverse_op = op_plus;
            }
            else
            {
                // do minus
                d = c * a0;
                inverse_op = op_minus;
            }

            mba_var_exp affine_expr;
            affine_expr.vars[0] = u_var_op(u_var_const(T, a1), inst->clone_exp(), op_mul);
            affine_expr.vars[1] = u_var_const(T, a0);
            affine_expr.operation = op_plus;

            // turn the root expression into the inverse of the affine
            inst->vars[0] = u_var_op(u_var_const(T, c), u_var_op(affine_expr), op_mul);
            inst->vars[1] = u_var_const(T, d);
            inst->operation = inverse_op;
        }
    );
}

template<typename T>
void mba_gen<T>::expand_constants(std::unique_ptr<mba_var_exp>& exp)
{
    // Automated Localisation of a Mixed Boolean
    // Arithmetic Obfuscation Window in a Program Binary
    // https://libstore.ugent.be/fulltxt/RUG01/003/014/584/RUG01-003014584_2021_0001_AC.pdf

    // P ∈ Pm (Z/2nZ) and Q its inverse: P (Q(X)) = X ∀X ∈ Z/2nZ
    // K ∈ Z/2nZ the constant to hide
    // E an MBA expression of variables (x1, . . . , xt) ∈ (Z/2nZ)t non-trivially equal to zero
    // Then K can be replaced by P (E + Q(K)) = P (Q(K)) = K, no matter the values of the variables (x1, . . ., xt).

    // Generate polynomial inverses?
    // https://plzin.github.io/posts/perm-poly
}

template class mba_gen<uint8_t>;
template class mba_gen<uint16_t>;
template class mba_gen<uint32_t>;
template class mba_gen<uint64_t>;