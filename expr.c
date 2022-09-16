

struct exprstate {
	int			is_post_value;
	int			is_incomplete;
	int			is_missing_token;
	uint		last_cast_index;
	int			result;
};

struct opentry {
	const char	*token;
	int			precedence;
	int			is_unary;
	int			is_postfix;
	int			is_right_to_left;
};

struct opentry	g_opentries[] = {
	{ "(", 0, 1, 0, 0 },
	{ "", 0, 1, 0, 0 },
	{ "", 0, 1, 0, 0 },

	{ "(", 1, 1, 1, 0 },
	{ "[", 1, 1, 1, 0 },
	{ ".", 1, 0, 1, 0 },
	{ "->", 1, 0, 1, 0 },

	{ "+", 2, 1, 0, 1 },
	{ "-", 2, 1, 0, 1 },
	{ "!", 2, 1, 0, 1 },
	{ "~", 2, 1, 0, 1 },
	{ "*", 2, 1, 0, 1 },
	{ "[", 2, 1, 0, 1 },
	{ "&", 2, 1, 0, 1 },
	{ "sizeof", 2, 1, 0, 1 },
	{ "alignof", 2, 1, 0, 1 },

	{ "*", 3, 0, 0, 0 },
	{ "/", 3, 0, 0, 0 },
	{ "%", 3, 0, 0, 0 },

	{ "+", 4, 0, 0, 0 },
	{ "-", 4, 0, 0, 0 },

	{ "<<", 5, 0, 0, 0 },
	{ ">>", 5, 0, 0, 0 },

	{ "<", 6, 0, 0, 0 },
	{ ">", 6, 0, 0, 0 },
	{ "<=", 6, 0, 0, 0 },
	{ ">=", 6, 0, 0, 0 },

	{ "==", 7, 0, 0, 0 },
	{ "!=", 7, 0, 0, 0 },

	{ "&", 8, 0, 0, 0 },

	{ "^", 9, 0, 0, 0 },

	{ "|", 10, 0, 0, 0 },

	{ "&&", 11, 0, 0, 0 },

	{ "||", 12, 0, 0, 0 },

	/* ternary condition */

	{ "=", 14, 0, 0, 1 },
	{ "+=", 14, 0, 0, 1 },
	{ "-=", 14, 0, 0, 1 },
	{ "*=", 14, 0, 0, 1 },
	{ "/=", 14, 0, 0, 1 },
	{ "%=", 14, 0, 0, 1 },
	{ "<<=", 14, 0, 0, 1 },
	{ ">>=", 14, 0, 0, 1 },
	{ "&=", 14, 0, 0, 1 },
	{ "^=", 14, 0, 0, 1 },
	{ "|=", 14, 0, 0, 1 },

	/* comma */
};

const char	*get_op_name (enum optype type) {
	static char		buffer[16];

	if (type == OpType (group)) {
		snprintf (buffer, sizeof buffer, "(_)");
	} else if (type == OpType (typesizeof)) {
		snprintf (buffer, sizeof buffer, "sizeof [_]");
	} else if (type == OpType (typealignof)) {
		snprintf (buffer, sizeof buffer, "alignof [_]");
	} else if (type == OpType (function_call)) {
		snprintf (buffer, sizeof buffer, "_ (_)");
	} else if (type == OpType (array_subscript)) {
		snprintf (buffer, sizeof buffer, "_[_]");
	} else if (type == OpType (cast)) {
		snprintf (buffer, sizeof buffer, "[_] _");
	} else if (type == OpType (sizeof)) {
		snprintf (buffer, sizeof buffer, "sizeof _");
	} else if (type == OpType (alignof)) {
		snprintf (buffer, sizeof buffer, "alignof _");
	} else if (g_opentries[type].is_unary) {
		if (g_opentries[type].is_postfix) {
			snprintf (buffer, sizeof buffer, "_%s", g_opentries[type].token);
		} else {
			snprintf (buffer, sizeof buffer, "%s_", g_opentries[type].token);
		}
	} else if (g_opentries[type].is_postfix) {
		snprintf (buffer, sizeof buffer, "_%s_", g_opentries[type].token);
	} else {
		snprintf (buffer, sizeof buffer, "_ %s _", g_opentries[type].token);
	}
	return (buffer);
}

const char	*get_expr_name (struct expr *expr) {
	static char		buffer[32];
	const char		*result;

	if (expr->type == ExprType (op)) {
		result = get_op_name (expr->op.type);
	} else if (expr->type == ExprType (constant)) {
		snprintf (buffer, sizeof buffer, "c_%d", (int) expr->constant.value);
		result = buffer;
	} else if (expr->type == ExprType (identifier)) {
		snprintf (buffer, sizeof buffer, "id_%s", expr->iden.name);
		result = buffer;
	} else {
		Error ("unknown expr type");
		result = 0;
	}
	return (result);
}

uint	get_expr_index (struct unit *unit, struct expr *expr) {
	return (Get_Bucket_Element_Index (unit->exprs, expr));
}

uint	make_expr_identifier (struct unit *unit, const char *name) {
	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Bucket (unit->exprs);
		expr->type = ExprType (identifier);
		expr->iden.name = name;
		expr->iden.decl = 0;
		index = Get_Bucket_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = 0;
	}
	return (index);
}

char	*strnotchrs(char *token, const char *chars) {
	while (*token && 0 != strchr (chars, *token)) {
		token += 1;
	}
	return (*token ? token : 0);
}

int		get_basictype_from_integer_suffix (const char *suffix, enum basictype *out) {
	int		result;

	result = 1;
	if (0 == strcmp (suffix, "u") || 0 == strcmp (suffix, "U")) {
		*out = BasicType (uint);
	} else if (0 == strcmp (suffix, "l") || 0 == strcmp (suffix, "L")) {
		*out = BasicType (long);
	} else if (0 == strcmp (suffix, "ll") || 0 == strcmp (suffix, "LL")) {
		*out = BasicType (longlong);
	} else if (0 == strcmp (suffix, "ull") || 0 == strcmp (suffix, "ULL") || 0 == strcmp (suffix, "uLL") || 0 == strcmp (suffix, "Ull")) {
		*out = BasicType (ulonglong);
	} else {
		result = 0;
	}
	return (result);
}

int		get_occupied_octal_bits (int num_of_digits, char significant_digit) {
	return (num_of_digits * 3 - 2 + (significant_digit >= '2') + (significant_digit >= '4'));
}

int		hex_digit_to_num (char hex) {
	int		result;

	if (hex >= '0' && hex <= '9') {
		result = hex - '0';
	} else if (hex >= 'a' && hex <= 'f') {
		result = 10 + (hex - 'a');
	} else if (hex >= 'A' && hex <= 'F') {
		result = 10 + (hex - 'A');
	} else {
		Unreachable ();
	}
	return (result);
}

int		count_occupied_bits_for_dec (const char *digits) {
	unsigned long long	number;
	unsigned long long	powered;
	int		power;

	number = 0;
	power = 0;
	powered = 1;
	while (*digits >= '0' && *digits <= '9') {
		number *= 10;
		number += *digits - '0';
		while (number > powered) {
			powered *= 2;
			power += 1;
		}
		digits += 1;
	}
	return (power);
}

int		parse_constant_value (struct unit *unit, char *token, struct exprvalue *value) {
	char	*suffix;
	char	*digits;
	char	*end;
	usize	digits_len;
	int		occupied_bits;
	int		base;
	int		is_float;
	int		result;

	is_float = 0;
	if (*token == '0' && (token[1] == 'x' || token[1] == 'X') && token[2] && strchr ("0123456789ABCDEFabcdef", token[2])) {
		int		hex_num;

		digits = token + 2;
		while (*digits && digits[0] == '0' && digits[1] == '0') {
			digits += 1;
		}
		suffix = strnotchrs (digits, "0123456789ABCDEFabcdef");
		digits_len = suffix ? suffix - digits : strlen (digits);
		hex_num = hex_digit_to_num (digits[0]);
		occupied_bits = digits_len * 4 - 3 + (hex_num >= 2) + (hex_num >= 4) + (hex_num >= 8);
		base = 16;
		result = 1;
	} else if (*token == '0' && (token[1] == 'b' || token[1] == 'B') && token[2] && strchr ("01", token[2])) {
		digits = token + 2;
		while (*digits && digits[0] == '0' && digits[1] == '0') {
			digits += 1;
		}
		suffix = strnotchrs (digits, "01");
		digits_len = suffix ? suffix - digits : strlen (digits);
		occupied_bits = digits_len;
		base = 2;
		result = 1;
	} else if (*token == '0' && token[1] && strchr ("01234567", token[1])) {
		digits = token + 1;
		while (*digits && digits[0] == '0' && digits[1] == '0') {
			digits += 1;
		}
		suffix = strnotchrs (digits, "01234567");
		digits_len = suffix ? suffix - digits : strlen (digits);
		occupied_bits = digits_len * 3 - 2 + ((digits[0] - '0') >= 2) + ((digits[0] - '0') >= 4);
		base = 8;
		result = 1;
	} else if ((*token >= '0' && *token <= '9') || *token == '.') {
		digits = token;
		while (*digits && digits[0] == '0' && digits[1] == '0') {
			digits += 1;
		}
		suffix = strnotchrs (digits, "0123456789");
		if (suffix && *suffix == '.') {
			is_float = 1;
			suffix += 1;
			if (*suffix >= '0' && *suffix <= '9') {
				suffix = strnotchrs (suffix, "0123456789");
				result = 1;
			} else if (*digits >= '0' && *digits <= '9') {
				if (!*suffix) {
					suffix = 0;
				}
				result = 1;
			} else {
				Parse_Error (token, unit->pos, "invalid floating point literal");
				result = 0;
			}
		} else {
			digits_len = suffix ? suffix - digits : strlen (digits);
			occupied_bits = count_occupied_bits_for_dec (digits);
			base = 10;
			result = 1;
		}
	} else {
		Parse_Error (token, unit->pos, "unrecognized integer literal");
		result = 0;
	}
	if (result) {
		if (is_float) {
			if (suffix) {
				if (0 == strcmp (suffix, "f") || 0 == strcmp (suffix, "F")) {
					value->type = BasicType (float);
					value->fvalue = strtof (digits, &end);
					result = 1;
				} else if (0 == strcmp (suffix, "l") || 0 == strcmp (suffix, "L")) {
					value->type = BasicType (longdouble);
					value->fvalue = strtold (digits, &end);
					result = 1;
				} else {
					Parse_Error (token, unit->pos, "unrecognized integer suffix '%s'", suffix);
					result = 0;
				}
			} else {
				value->type = BasicType (double);
				value->fvalue = strtod (digits, &end);
				result = 1;
			}
		} else {
			if (suffix) {
				if (get_basictype_from_integer_suffix (suffix, &value->type)) {
					if (occupied_bits <= 8 * get_basictype_size (value->type)) {
						if (is_basictype_signed (value->type)) {
							value->value = strtoll (digits, &end, base);
						} else {
							value->uvalue = strtoull (digits, &end, base);
						}
						result = 1;
					} else {
						Parse_Error (token, unit->pos, "number is too big");
						result = 0;
					}
				} else {
					Parse_Error (token, unit->pos, "unrecognized integer suffix '%s'", suffix);
					result = 0;
				}
			} else if (occupied_bits <= 32) {
				if (occupied_bits == 32) {
					value->type = BasicType (uint);
					value->uvalue = strtoul (digits, &end, base);
				} else {
					value->type = BasicType (int);
					value->value = strtol (digits, &end, base);
				}
				result = 1;
			} else if (occupied_bits <= 64) {
				if (occupied_bits == 64) {
					value->type = BasicType (ulonglong);
					value->uvalue = strtoull (digits, &end, base);
				} else {
					value->type = BasicType (longlong);
					value->value = strtoll (digits, &end, base);
				}
				result = 1;
			} else {
				Parse_Error (token, unit->pos, "number is too big");
				result = 0;
			}
		}
	}
	return (result);
}

uint	make_expr_constant (struct unit *unit, char *token) {
	int					result;
 	uint				index;
 	struct exprvalue	value;

	if (parse_constant_value (unit, token, &value)) {
		if (Prepare_Bucket (unit->exprs, 1)) {
			struct expr	*expr;
			char	*ptr;

			expr = Push_Bucket (unit->exprs);
			expr->type = ExprType (constant);
			expr->constant = value;
			index = Get_Bucket_Element_Index (unit->exprs, expr);
		} else {
			Error ("cannot prepare array for expr");
			index = 0;
		}
	} else {
		index = 0;
	}
	return (index);
}

uint	make_expr_usize_constant (struct unit *unit, usize value) {
 	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Bucket (unit->exprs);
		expr->type = ExprType (constant);
		expr->constant.type = BasicType (usize);
		expr->constant.value = value;
		index = Get_Bucket_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = 0;
	}
	return (index);
}

uint	make_expr_string (struct unit *unit, const char *token) {
 	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Bucket (unit->exprs);
		expr->type = ExprType (string);
		expr->string.token = token;
		index = Get_Bucket_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = 0;
	}
	return (index);
}

uint	make_expr_op (struct unit *unit, enum optype type) {
	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Bucket (unit->exprs);
		expr->type = ExprType (op);
		expr->op.type = type;
		index = Get_Bucket_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = 0;
	}
	return (index);
}

uint	make_expr_funcparam (struct unit *unit, uint expr_index, uint next) {
	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Bucket (unit->exprs);
		expr->type = ExprType (funcparam);
		expr->funcparam.expr = expr_index;
		expr->funcparam.next = next;
		index = Get_Bucket_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = 0;
	}
	return (index);
}

struct expr	*get_expr (struct unit *unit, uint index) {
	Assert (Is_Bucket_Index_Valid (unit->exprs, index));
	return (Get_Bucket_Element (unit->exprs, index));
}

uint	make_expr_op_from_token (struct unit *unit, char *token, struct exprstate *state) {
	uint		index;

	index = 0;
	while (index < Array_Count (g_opentries) && !(
		0 == strcmp (token, g_opentries[index].token) &&
		(
			(g_opentries[index].is_unary && g_opentries[index].is_postfix == state->is_post_value) ||
			(!g_opentries[index].is_unary && !state->is_incomplete)
		)
		)) {
		index += 1;
	}
	if (index < Array_Count (g_opentries)) {
		state->last_cast_index = 0;
		index = make_expr_op (unit, index);
	} else if (state->is_post_value) {
		state->is_post_value = 0;
		index = make_expr_op_from_token (unit, token, state);
	} else if (state->last_cast_index) {
		struct expr		*cast;
		uint			type_index;

		cast = get_expr (unit, state->last_cast_index);
		type_index = cast->op.backward;
		cast->type = ExprType (typeinfo);
		cast->typeinfo.type = type_index;
		cast->typeinfo.index = 0;
		cast->typeinfo.lib_index = 0;
		state->is_post_value = 1;
		state->is_incomplete = 0;
		state->last_cast_index = 0;
		index = make_expr_op_from_token (unit, token, state);
	} else {
		index = 0;
	}
	return (index);
}

int		get_expr_precedence (struct expr *expr) {
	int		prec;

	if (expr->type == ExprType (op)) {
		prec = g_opentries[expr->op.type].precedence;
	} else {
		prec = 0;
	}
	return (prec);
}

int		is_expr_unary (struct expr *expr) {
	int		result;

	if (expr->type == ExprType (op)) {
		result = g_opentries[expr->op.type].is_unary;
	} else {
		result = 0;
	}
	return (result);
}

int		is_expr_postfix (struct expr *expr) {
	int		result;

	if (expr->type == ExprType (op)) {
		result = g_opentries[expr->op.type].is_postfix;
	} else {
		result = 0;
	}
	return (result);
}

int		is_expr_right_to_left (struct expr *expr) {
	int		result;

	if (expr->type == ExprType (op)) {
		result = g_opentries[expr->op.type].is_right_to_left;
	} else {
		result = 0;
	}
	return (result);
}

uint	link_expr_op_forward (struct unit *unit, struct expr *expr, uint head_index) {
	if (is_expr_unary (expr)) {
		expr->op.forward = head_index;
	} else {
		expr->op.backward = head_index;
	}
	return (get_expr_index (unit, expr));
}

uint	link_expr_op (struct unit *unit, struct expr *head, uint head_index, struct expr *expr, uint expr_index) {
	if (get_expr_precedence (expr) > get_expr_precedence (head)) {
		head_index = link_expr_op_forward (unit, expr, head_index);
	} else if (get_expr_precedence (expr) == get_expr_precedence (head) && ((is_expr_right_to_left (expr) && head->op.forward) || !is_expr_right_to_left (expr))) {
		if (is_expr_right_to_left (expr)) {
			struct expr	*forward;

			forward = get_expr (unit, head->op.forward);
			if (get_expr_precedence (expr) == get_expr_precedence (forward)) {
				head->op.forward = link_expr_op (unit, forward, head->op.forward, expr, expr_index);
			} else {
				head->op.forward = link_expr_op_forward (unit, expr, head->op.forward);
			}
		} else {
			head_index = link_expr_op_forward (unit, expr, head_index);
		}
	} else if (get_expr_precedence (head) != 0) {
		if (!head->op.forward) {
			head->op.forward = expr_index;
		} else {
			struct expr	*forward = get_expr (unit, head->op.forward);

			if (get_expr_precedence (forward) != 0) {
				head->op.forward = link_expr_op (unit, forward, head->op.forward, expr, expr_index);
			} else {
				head->op.forward = link_expr_op_forward (unit, expr, head->op.forward);
			}
		}
	} else {
		Unreachable ();
	}
	return (head_index);
}

int		parse_expr (struct unit *unit, char **ptokens, uint *out);

void	parse_expr_rec (struct unit *unit, char **ptokens, uint *phead, struct exprstate *state) {
	if ((*ptokens)[-1] == Token (punctuator) || ((*ptokens)[-1] == Token (identifier) && (0 == strcmp (*ptokens, "sizeof") || 0 == strcmp (*ptokens, "alignof")))) {
		uint		expr_index;

		expr_index = make_expr_op_from_token (unit, *ptokens, state);
		if (expr_index) {
			struct expr	*expr;

			expr = get_expr (unit, expr_index);
			Assert (expr->type == ExprType (op));
			if (expr->op.type == OpType (group)) {
				uint	innerhead;

				*ptokens = next_token (*ptokens, &unit->pos);
				innerhead = 0;
				if (parse_expr (unit, ptokens, &innerhead)) {
					if (is_token (*ptokens, Token (punctuator), ")")) {
						*ptokens = next_token (*ptokens, &unit->pos);
						state->is_post_value = 1;
						state->is_incomplete = 0;
						get_expr (unit, expr_index)->op.forward = innerhead;
						if (!*phead) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token; expecting ')'");
						state->result = 0;
					}
				} else {
					state->result = 0;
				}
			} else if (expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
				*ptokens = next_token (*ptokens, &unit->pos);
				if (is_token (*ptokens, Token (punctuator), "[")) {
					uint	type_index;

					*ptokens = next_token (*ptokens, &unit->pos);
					if (parse_type (unit, ptokens, &type_index)) {
						if (is_token (*ptokens, Token (punctuator), "]")) {
							if (type_index) {
								if (expr->op.type == OpType (sizeof)) {
									expr->op.type = OpType (typesizeof);
								} else if (expr->op.type == OpType (alignof)) {
									expr->op.type = OpType (typealignof);
								} else {
									Unreachable ();
								}
								expr->op.forward = type_index;
								state->is_post_value = 1;
								state->is_incomplete = 0;
								*ptokens = next_token (*ptokens, &unit->pos);
								if (!*phead) {
									*phead = expr_index;
								} else {
									*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
								}
								state->result = 1;
							} else {
								Parse_Error (*ptokens, unit->pos, "empty type");
								state->result = 0;
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token; expecting ']'");
							state->result = 0;
						}
					} else {
						state->result = 0;
					}
				} else {
					state->is_incomplete = 1;
					if (!*phead) {
						*phead = expr_index;
					} else {
						*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
					}
					state->result = 1;
				}
			} else if (expr->op.type == OpType (function_call)) {
				uint	funcparam_index;

				funcparam_index = 0;
				if (is_token (next_token (*ptokens, 0), Token (punctuator), ")")) {
					state->result = 1;
					*ptokens = next_token (*ptokens, &unit->pos);
				} else while (state->result && (*ptokens)[-1] && !is_token (*ptokens, Token (punctuator), ")")) {
					if (is_token (*ptokens, Token (punctuator), ",") || is_token (*ptokens, Token (punctuator), "(")) {
						int			param_index;

						param_index = 0;
						*ptokens = next_token (*ptokens, &unit->pos);
						if (parse_expr (unit, ptokens, &param_index)) {
							if (param_index) {
								if (funcparam_index) {
									funcparam_index = get_expr (unit, funcparam_index)->funcparam.next = make_expr_funcparam (unit, param_index, 0);
								} else {
									funcparam_index = get_expr (unit, expr_index)->op.backward = make_expr_funcparam (unit, param_index, 0);
								}
							} else if (is_token (*ptokens, Token (punctuator), "...")) {
								*ptokens = next_token (*ptokens, &unit->pos);
								if (funcparam_index) {
									funcparam_index = get_expr (unit, funcparam_index)->funcparam.next = make_expr_funcparam (unit, 0, 0);
								} else {
									funcparam_index = get_expr (unit, expr_index)->op.backward = make_expr_funcparam (unit, 0, 0);
								}
							} else {
								Parse_Error (*ptokens, unit->pos, "empty parameter");
								state->result = 0;
							}
						} else {
							state->result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						state->result = 0;
					}
				}
				if (state->result) {
					if ((*ptokens)[-1]) {
						*ptokens = next_token (*ptokens, &unit->pos);
						*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected end");
						state->result = 0;
					}
				}
			} else if (expr->op.type == OpType (array_subscript)) {
				uint	subscript_index;

				subscript_index = 0;
				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_expr (unit, ptokens, &subscript_index)) {
					if (subscript_index) {
						if (is_token (*ptokens, Token (punctuator), "]")) {
							*ptokens = next_token (*ptokens, &unit->pos);
							get_expr (unit, expr_index)->op.backward = subscript_index;
							Assert (*phead);
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							state->result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "empty subscript");
						state->result = 0;
					}
				} else {
					state->result = 0;
				}
			} else if (expr->op.type == OpType (cast)) {
				uint	type_index;

				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_type (unit, ptokens, &type_index)) {
					if (is_token (*ptokens, Token (punctuator), "]")) {
						state->is_incomplete = 1;
						state->last_cast_index = expr_index;
						*ptokens = next_token (*ptokens, &unit->pos);
						get_expr (unit, expr_index)->op.backward = type_index;
						if (!*phead) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token [%s]", *ptokens);
						state->result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "cannot parse tyoe");
					state->result = 0;
				}
			} else {
				struct expr	*expr = get_expr (unit, expr_index);

				if (is_expr_unary (expr)) {
					if (!is_expr_postfix (expr)) {
						state->is_incomplete = 1;
						state->result = 1;
					} else if (state->is_incomplete) {
						Parse_Error (*ptokens, unit->pos, "postfix op on incomplete expr tree");
						state->result = 0;
					}
					if (state->result) {
						if (!*phead) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
						}
						*ptokens = next_token (*ptokens, &unit->pos);
					}
				} else if (!state->is_incomplete) {
					state->is_incomplete = 1;
					state->is_post_value = 0;
					Assert (*phead);
					*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
					*ptokens = next_token (*ptokens, &unit->pos);
				} else {
					Parse_Error (*ptokens, unit->pos, "binary op on incomplete expr tree");
					state->result = 0;
				}
			}
		} else {
			state->is_missing_token = 1;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (identifier)) {
		uint	expr_index;

		expr_index = make_expr_identifier (unit, *ptokens);
		if (expr_index) {
			if (*phead) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = expr_index;
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, &unit->pos);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = 0;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (preprocessing_number)) {
		uint	expr_index;

		expr_index = make_expr_constant (unit, *ptokens);
		if (expr_index) {
			if (*phead) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = expr_index;
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, &unit->pos);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = 0;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (string)) {
		uint	expr_index;

		expr_index = make_expr_string (unit, *ptokens);
		if (expr_index) {
			if (*phead) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = expr_index;
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, &unit->pos);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = 0;
		}
	} else if (state->is_post_value || !*phead || state->last_cast_index) {
		state->is_missing_token = 1;
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		state->result = 0;
	}
}

int		parse_expr (struct unit *unit, char **ptokens, uint *out) {
	struct exprstate	state = {0};

	*out = 0;
	state.result = 1;
	state.last_cast_index = 0;
	state.is_incomplete = 1;
	while (state.result && !state.is_missing_token) {
		parse_expr_rec (unit, ptokens, out, &state);
	}
	if (state.result && state.is_incomplete && *out) {
		if (state.last_cast_index) {
			struct expr		*cast;
			uint			type_index;

			cast = get_expr (unit, state.last_cast_index);
			type_index = cast->op.backward;
			cast->type = ExprType (typeinfo);
			cast->typeinfo.type = type_index;
			cast->typeinfo.index = 0;
			cast->typeinfo.lib_index = 0;
			state.result = 1;
		} else {
			Parse_Error (*ptokens, unit->pos, "expr parsing is ended with incomplete expr tree");
			state.result = 0;
		}
	}
	return (state.result);
}

void	print_expr (struct unit *unit, uint head_index, FILE *file) {
	struct expr	*head;

	Assert (head_index);
	head = get_expr (unit, head_index);
	if (head->type == ExprType (op)) {
		if (head->op.type == OpType (group)) {
			fprintf (file, "(");
			print_expr (unit, head->op.forward, file);
			fprintf (file, ")");
		} else if (head->op.type == OpType (function_call)) {
			print_expr (unit, head->op.forward, file);
			if (head->op.backward) {
				fprintf (file, " (");
				print_expr (unit, head->op.backward, file);
				fprintf (file, ")");
			} else {
				fprintf (file, " ()");
			}
		} else if (head->op.type == OpType (array_subscript)) {
			print_expr (unit, head->op.forward, file);
			if (head->op.backward) {
				fprintf (file, "[");
				print_expr (unit, head->op.backward, file);
				fprintf (file, "]");
			} else {
				fprintf (file, "[]");
			}
		} else if (head->op.type == OpType (cast)) {
			if (head->op.backward) {
				fprintf (file, "[");
				print_type (unit, head->op.backward, file);
				fprintf (file, "] ");
			} else {
				fprintf (file, "[] ");
			}
			print_expr (unit, head->op.forward, file);
		} else if (head->op.type == OpType (typesizeof) || head->op.type == OpType (typealignof)) {
			if (head->op.type == OpType (typesizeof)) {
				fprintf (file, "sizeof [");
			} else {
				fprintf (file, "alignof [");
			}
			print_type (unit, head->op.forward, file);
			fprintf (file, "]");
		} else if (head->op.type == OpType (sizeof) || head->op.type == OpType (alignof)) {
			if (head->op.type == OpType (typesizeof)) {
				fprintf (file, "sizeof ");
			} else {
				fprintf (file, "alignof ");
			}
			print_expr (unit, head->op.forward, file);
		} else if (is_expr_unary (head)) {
			if (is_expr_postfix (head)) {
				print_expr (unit, head->op.forward, file);
				fprintf (file, "%s", g_opentries[head->op.type].token);
			} else {
				fprintf (file, "%s", g_opentries[head->op.type].token);
				print_expr (unit, head->op.forward, file);
			}
		} else {
			print_expr (unit, head->op.backward, file);
			if (g_opentries[head->op.type].is_postfix) {
				fprintf (file, "%s", g_opentries[head->op.type].token);
			} else {
				fprintf (file, " %s ", g_opentries[head->op.type].token);
			}
			print_expr (unit, head->op.forward, file);
		}
	} else if (head->type == ExprType (identifier)) {
		fprintf (file, "%s", head->iden.name);
	} else if (head->type == ExprType (constant)) {
		if (is_basictype_integral (head->constant.type)) {
			if (is_basictype_signed (head->constant.type)) {
				fprintf (file, "%zd", head->constant.value);
			} else {
				fprintf (file, "%zu", head->constant.uvalue);
			}
		} else {
			fprintf (file, "%f", head->constant.fvalue);
		}
	} else if (head->type == ExprType (funcparam)) {
		if (head->funcparam.expr) {
			print_expr (unit, head->funcparam.expr, file);
		}
		if (head->funcparam.next) {
			fprintf (file, ", ");
			print_expr (unit, head->funcparam.next, file);
		}
	} else if (head->type == ExprType (typeinfo)) {
		fprintf (file, "[");
		print_type (unit, head->typeinfo.type, file);
		fprintf (file, "]");
	} else if (head->type == ExprType (macroparam)) {
		fprintf (file, "%s", head->macroparam.name);
	} else if (head->type == ExprType (string)) {
		const char	*token;
		usize		size;
		char		string[4 * 1024];

		token = head->string.token;
		unescape_string (token, string, sizeof string, &size);
		fprintf (file, "\"%s\"", string);
	} else {
		Error ("unknown %d %u", head->type, head_index);
	}
}

void	print_expr_table (struct unit *unit, uint head_index, FILE *file) {
	static int	level = -1;
	struct expr	*head;

	Assert (head_index);
	head = get_expr (unit, head_index);
	level += 1;
	if (head->type == ExprType (op)) {
		if (head->op.type == OpType (group)) {
			fprintf (file, "%*.s(_)\t\t\t%u\n", level * 2, "", head_index);
			print_expr_table (unit, head->op.forward, file);
		} else if (head->op.type == OpType (function_call)) {
			fprintf (file, "%*.s_ (_)\t\t\t%u\n", level * 2, "", head_index);
			print_expr_table (unit, head->op.forward, file);
			if (head->op.backward) {
				print_expr_table (unit, head->op.backward, file);
			}
		} else if (head->op.type == OpType (array_subscript)) {
			fprintf (file, "%*.s_[_]\t\t\t%u\n", level * 2, "", head_index);
			print_expr_table (unit, head->op.forward, file);
			print_expr_table (unit, head->op.backward, file);
		} else if (head->op.type == OpType (cast)) {
			fprintf (file, "%*.s[_] _\t\t\t%u\n", level * 2, "", head_index);
			fprintf (file, "%*.s", (level + 1) * 2, "");
			print_type (unit, head->op.backward, file);
			fprintf (file, "\n");
			print_expr_table (unit, head->op.forward, file);
		} else if (is_expr_unary (head)) {
			if (is_expr_postfix (head)) {
				fprintf (file, "%*.s%s\t\t\t%u\n", level * 2, "", get_op_name (head->op.type), head_index);
				print_expr_table (unit, head->op.forward, file);
			} else {
				fprintf (file, "%*.s%s\t\t\t%u\n", level * 2, "", get_op_name (head->op.type), head_index);
				print_expr_table (unit, head->op.forward, file);
			}
		} else {
			fprintf (file, "%*.s%s\t\t\t%u\n", level * 2, "", get_op_name (head->op.type), head_index);
			print_expr_table (unit, head->op.backward, file);
			print_expr_table (unit, head->op.forward, file);
		}
	} else if (head->type == ExprType (identifier)) {
		fprintf (file, "%*.s%s\t\t\t%u\n", level * 2, "", head->iden.name, head_index);
	} else if (head->type == ExprType (constant)) {
		fprintf (file, "%*.s%d\t\t\t%u\n", level * 2, "", (int) head->constant.value, head_index);
	} else if (head->type == ExprType (funcparam)) {
		level -= 1;
		print_expr_table (unit, head->funcparam.expr, file);
		if (head->funcparam.next) {
			print_expr_table (unit, head->funcparam.next, file);
		}
		level += 1;
	} else if (head->type == ExprType (typeinfo)) {
		fprintf (file, "%*.s", level * 2, "");
		print_type (unit, head->typeinfo.type, file);
		fprintf (file, "\t\t\t%u\n", head_index);
	} else {
		Error ("unknown %d %u", head->type, head_index);
	}
	level -= 1;
}

struct expr	*get_constant_expr_from_parent (struct unit *unit, uint index) {
	struct expr	*expr;

	expr = get_expr (unit, index);
	while (expr->type == ExprType (op) && expr->op.type == OpType (group)) {
		expr = get_expr (unit, expr->op.forward);
	}
	if (expr->type != ExprType (constant)) {
		expr = 0;
	}
	return (expr);
}


