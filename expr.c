

struct exprstate {
	int			is_post_value;
	int			is_incomplete;
	int			is_missing_token;
	int			last_cast_index;
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
	} else if (type == OpType (function_call)) {
		snprintf (buffer, sizeof buffer, "_ (_)");
	} else if (type == OpType (array_subscript)) {
		snprintf (buffer, sizeof buffer, "_[_]");
	} else if (type == OpType (cast)) {
		snprintf (buffer, sizeof buffer, "[_] _");
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
	}
	return (result);
}

int		get_expr_index (struct unit *unit, struct expr *expr) {
	return (Get_Element_Index (unit->exprs, expr));
}

int		make_expr_identifier (struct unit *unit, const char *name) {
	int		index;

	if (Prepare_Array (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Array (unit->exprs);
		expr->type = ExprType (identifier);
		expr->iden.name = name;
		expr->iden.decl = -1;
		index = Get_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = -1;
	}
	return (index);
}

int		make_expr_constant (struct unit *unit, const char *string) {
 	int		index;

	if (Prepare_Array (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Array (unit->exprs);
		expr->type = ExprType (constant);
		if (strchr (string, '.')) {
			if (string[strlen (string) - 1] == 'f') {
				expr->constant.type = BasicType (float);
				expr->constant.fvalue = atof (string);
			} else {
				expr->constant.type = BasicType (double);
				expr->constant.fvalue = atof (string);
			}
		} else {
			if (string[strlen (string) - 1] == 'u') {
				expr->constant.type = BasicType (uint);
				expr->constant.value = atoi (string);
			} else {
				expr->constant.type = BasicType (int);
				expr->constant.value = atoi (string);
			}
		}
		index = Get_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = -1;
	}
	return (index);
}

int		make_expr_string (struct unit *unit, const char *token) {
 	int		index;

	if (Prepare_Array (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Array (unit->exprs);
		expr->type = ExprType (string);
		expr->string.token = token;
		index = Get_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = -1;
	}
	return (index);
}

int		make_expr_op (struct unit *unit, enum optype type) {
	int		index;

	if (Prepare_Array (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Array (unit->exprs);
		expr->type = ExprType (op);
		expr->op.type = type;
		expr->op.forward = -1;
		expr->op.backward = -1;
		index = Get_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = -1;
	}
	return (index);
}

int		make_expr_funcparam (struct unit *unit, int expr_index, int next) {
	int		index;

	if (Prepare_Array (unit->exprs, 1)) {
		struct expr	*expr;

		expr = Push_Array (unit->exprs);
		expr->type = ExprType (funcparam);
		expr->funcparam.expr = expr_index;
		expr->funcparam.next = next;
		index = Get_Element_Index (unit->exprs, expr);
	} else {
		Error ("cannot prepare array for expr");
		index = -1;
	}
	return (index);
}

struct expr	*get_expr (struct unit *unit, int index) {
	Assert (index >= 0 && index < Get_Array_Count (unit->exprs));
	return (unit->exprs + index);
}

int		make_expr_op_from_token (struct unit *unit, char *token, struct exprstate *state) {
	int			index;

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
		state->last_cast_index = -1;
		index = make_expr_op (unit, index);
		Debug ("found op: %s", get_op_name (get_expr (unit, index)->op.type));
	} else if (state->is_post_value) {
		state->is_post_value = 0;
		index = make_expr_op_from_token (unit, token, state);
	} else if (state->last_cast_index >= 0) {
		struct expr		*cast;
		int				type_index;

		Debug ("making last cast op as typeinfo");
		cast = get_expr (unit, state->last_cast_index);
		type_index = cast->op.backward;
		cast->type = ExprType (typeinfo);
		cast->typeinfo.type = type_index;
		cast->typeinfo.decl = -1;
		state->is_post_value = 1;
		state->is_incomplete = 0;
		state->last_cast_index = -1;
		index = make_expr_op_from_token (unit, token, state);
	} else {
		index = -1;
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

int		link_expr_op_forward (struct unit *unit, struct expr *expr, int head_index) {
	if (is_expr_unary (expr)) {
		expr->op.forward = head_index;
	} else {
		expr->op.backward = head_index;
	}
	return (get_expr_index (unit, expr));
}

int		link_expr_op (struct unit *unit, struct expr *head, int head_index, struct expr *expr, int expr_index) {
	if (get_expr_precedence (expr) > get_expr_precedence (head)) {
		head_index = link_expr_op_forward (unit, expr, head_index);
	} else if (get_expr_precedence (expr) == get_expr_precedence (head) && ((is_expr_right_to_left (expr) && head->op.forward >= 0) || !is_expr_right_to_left (expr))) {
		if (is_expr_right_to_left (expr)) {
			struct expr	*forward = get_expr (unit, head->op.forward);

			if (get_expr_precedence (expr) == get_expr_precedence (forward)) {
				head->op.forward = link_expr_op (unit, forward, head->op.forward, expr, expr_index);
			} else {
				head->op.forward = link_expr_op_forward (unit, expr, head->op.forward);
			}
		} else {
			head_index = link_expr_op_forward (unit, expr, head_index);
		}
	} else if (get_expr_precedence (head) != 0) {
		if (head->op.forward < 0) {
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
		Error ("unexpected op");
	}
	return (head_index);
}

int		parse_expr (struct unit *unit, char **ptokens, int *out);

void	parse_expr_rec (struct unit *unit, char **ptokens, int *phead, struct exprstate *state) {
	if ((*ptokens)[-1] == Token (punctuator)) {
		int			expr_index;

		expr_index = make_expr_op_from_token (unit, *ptokens, state);
		if (expr_index >= 0) switch (get_expr (unit, expr_index)->op.type) {
			case OpType (group): {
				int		innerhead;

				*ptokens = next_token (*ptokens, 0);
				innerhead = -1;
				if (parse_expr (unit, ptokens, &innerhead)) {
					if (is_token (*ptokens, Token (punctuator), ")")) {
						*ptokens = next_token (*ptokens, 0);
						state->is_post_value = 1;
						state->is_incomplete = 0;
						get_expr (unit, expr_index)->op.forward = innerhead;
						if (*phead < 0) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						}
					} else {
						Error ("unexpected token");
						state->result = 0;
					}
				} else {
					Error ("cannot parse group expr");
					state->result = 0;
				}
			} break ;
			case OpType (function_call): {
				int		funcparam_index = -1;

				while (state->result && (*ptokens)[-1] && !is_token (*ptokens, Token (punctuator), ")")) {
					if (is_token (*ptokens, Token (punctuator), ",") || is_token (*ptokens, Token (punctuator), "(")) {
						int			param_index = -1;

						*ptokens = next_token (*ptokens, 0);
						if (parse_expr (unit, ptokens, &param_index)) {
							if (param_index >= 0) {
								if (funcparam_index >= 0) {
									funcparam_index = get_expr (unit, funcparam_index)->funcparam.next = make_expr_funcparam (unit, param_index, -1);
								} else {
									funcparam_index = get_expr (unit, expr_index)->op.backward = make_expr_funcparam (unit, param_index, -1);
								}
								Debug ("funcparam_index %d", funcparam_index);
							} else {
								Error ("empty parameter");
								state->result = 0;
							}
						} else {
							state->result = 0;
						}
					} else {
						Error ("unexpected token");
						state->result = 0;
					}
				}
				if (state->result) {
					if ((*ptokens)[-1]) {
						*ptokens = next_token (*ptokens, 0);
						*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
					} else {
						Error ("unexpected end");
						state->result = 0;
					}
				}
			} break ;
			case OpType (array_subscript): {
				int		subscript_index = -1;

				*ptokens = next_token (*ptokens, 0);
				if (parse_expr (unit, ptokens, &subscript_index)) {
					if (subscript_index >= 0) {
						if (is_token (*ptokens, Token (punctuator), "]")) {
							*ptokens = next_token (*ptokens, 0);
							get_expr (unit, expr_index)->op.backward = subscript_index;
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						} else {
							Error ("unexpected token");
							state->result = 0;
						}
					} else {
						Error ("empty subscript");
						state->result = 0;
					}
				} else {
					state->result = 0;
				}
			} break ;
			case OpType (cast): {
				int		type_index;

				*ptokens = next_token (*ptokens, 0);
				if (parse_type (unit, ptokens, &type_index)) {
					if (is_token (*ptokens, Token (punctuator), "]")) {
						state->is_incomplete = 1;
						state->last_cast_index = expr_index;
						*ptokens = next_token (*ptokens, 0);
						get_expr (unit, expr_index)->op.backward = type_index;
						if (*phead < 0) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
						}
					} else {
						Error ("unexpected token");
						state->result = 0;
					}
				} else {
					Error ("cannot parse tyoe");
					state->result = 0;
				}
			} break ;
			default: {
				struct expr	*expr = get_expr (unit, expr_index);

				if (is_expr_unary (expr)) {
					if (!is_expr_postfix (expr)) {
						state->is_incomplete = 1;
					} else if (state->is_incomplete) {
						Error ("postfix op on incomplete expr tree");
						state->result = 0;
					}
					if (state->result) {
						if (*phead < 0) {
							*phead = expr_index;
						} else {
							*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
						}
						*ptokens = next_token (*ptokens, 0);
					}
				} else if (!state->is_incomplete) {
					state->is_incomplete = 1;
					state->is_post_value = 0;
					Assert (*phead >= 0);
					*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, expr, expr_index);
					*ptokens = next_token (*ptokens, 0);
				} else {
					Error ("binary op on incomplete expr tree");
					state->result = 0;
				}
			} break ;
		} else {
			state->is_missing_token = 1;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (identifier)) {
		int		expr_index;

		expr_index = make_expr_identifier (unit, *ptokens);
		if (expr_index >= 0) {
			if (*phead >= 0) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = make_expr_identifier (unit, *ptokens);
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, 0);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = -1;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (preprocessing_number)) {
		int		expr_index;

		expr_index = make_expr_constant (unit, *ptokens);
		if (expr_index >= 0) {
			if (*phead >= 0) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = expr_index;
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, 0);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = -1;
		}
	} else if (state->is_incomplete && (*ptokens)[-1] == Token (string)) {
		int		expr_index;

		expr_index = make_expr_string (unit, *ptokens);
		if (expr_index >= 0) {
			if (*phead >= 0) {
				*phead = link_expr_op (unit, get_expr (unit, *phead), *phead, get_expr (unit, expr_index), expr_index);
			} else {
				*phead = expr_index;
			}
		} else {
			state->result = 0;
		}
		if (state->result) {
			*ptokens = next_token (*ptokens, 0);
			state->is_post_value = 1;
			state->is_incomplete = 0;
			state->last_cast_index = -1;
		}
	} else if (state->is_post_value || *phead < 0 || state->last_cast_index >= 0) {
		state->is_missing_token = 1;
	} else {
		Error ("unexpected token [%s]", *ptokens);
		state->result = 0;
	}
}

int		parse_expr (struct unit *unit, char **ptokens, int *out) {
	struct exprstate	state = {0};

	*out = -1;
	state.result = 1;
	state.last_cast_index = -1;
	state.is_incomplete = 1;
	while (state.result && !state.is_missing_token) {
		parse_expr_rec (unit, ptokens, out, &state);
	}
	if (state.result && state.is_incomplete && *out >= 0) {
		if (state.last_cast_index >= 0) {
			struct expr		*cast;
			int				type_index;

			cast = get_expr (unit, state.last_cast_index);
			type_index = cast->op.backward;
			cast->type = ExprType (typeinfo);
			cast->typeinfo.type = type_index;
			cast->typeinfo.decl = -1;
			state.result = 1;
		} else {
			Error ("expr parsing is ended with incomplete expr tree");
			state.result = 0;
		}
	}
	return (state.result);
}

void	print_expr (struct unit *unit, int head_index, FILE *file) {
	struct expr	*head;

	Assert (head_index >= 0);
	head = get_expr (unit, head_index);
	switch (head->type) {
		case ExprType (op): {
			if (head->op.type == OpType (group)) {
				fprintf (file, "(");
				print_expr (unit, head->op.forward, file);
				fprintf (file, ")");
			} else if (head->op.type == OpType (function_call)) {
				print_expr (unit, head->op.forward, file);
				if (head->op.backward >= 0) {
					fprintf (file, " (");
					print_expr (unit, head->op.backward, file);
					fprintf (file, ")");
				} else {
					fprintf (file, " ()");
				}
			} else if (head->op.type == OpType (array_subscript)) {
				print_expr (unit, head->op.forward, file);
				if (head->op.backward >= 0) {
					fprintf (file, "[");
					print_expr (unit, head->op.backward, file);
					fprintf (file, "]");
				} else {
					fprintf (file, "[]");
				}
			} else if (head->op.type == OpType (cast)) {
				if (head->op.backward >= 0) {
					fprintf (file, "[");
					print_type (unit, head->op.backward, file);
					fprintf (file, "] ");
				} else {
					fprintf (file, "[] ");
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
		} break ;
		case ExprType (identifier): {
			fprintf (file, "%s", head->iden.name);
		} break ;
		case ExprType (constant): {
			if (is_basictype_integral (head->constant.type)) {
				if (is_basictype_signed (head->constant.type)) {
					fprintf (file, "%zd", head->constant.value);
				} else {
					fprintf (file, "%zu", head->constant.uvalue);
				}
			} else {
				fprintf (file, "%f", head->constant.fvalue);
			}
		} break ;
		case ExprType (funcparam): {
			if (head->funcparam.expr >= 0) {
				print_expr (unit, head->funcparam.expr, file);
			}
			if (head->funcparam.next >= 0) {
				fprintf (file, ", ");
				print_expr (unit, head->funcparam.next, file);
			}
		} break ;
		case ExprType (typeinfo): {
			fprintf (file, "[");
			print_type (unit, head->typeinfo.type, file);
			fprintf (file, "]");
		} break ;
		case ExprType (macroparam): {
			fprintf (file, "%s", head->macroparam.name);
		} break ;
		case ExprType (string): {
			const char	*token;
			usize		size;
			char		string[4 * 1024];

			token = head->string.token;
			unescape_string (&token, string, sizeof string, &size);
			fprintf (file, "\"%s\"", string);
		} break ;
		default: {
			Error ("unknown %d %d", head->type, head_index);
		} break ;
	}
}

void	print_expr_table (struct unit *unit, int head_index, FILE *file) {
	static int	level = -1;
	struct expr	*head;

	Assert (head_index >= 0);
	head = get_expr (unit, head_index);
	level += 1;
	switch (head->type) {
		case ExprType (op): {
			if (head->op.type == OpType (group)) {
				fprintf (file, "%*.s(_)\t\t\t%d\n", level * 2, "", head_index);
				print_expr_table (unit, head->op.forward, file);
			} else if (head->op.type == OpType (function_call)) {
				fprintf (file, "%*.s_ (_)\t\t\t%d\n", level * 2, "", head_index);
				print_expr_table (unit, head->op.forward, file);
				if (head->op.backward >= 0) {
					print_expr_table (unit, head->op.backward, file);
				}
			} else if (head->op.type == OpType (array_subscript)) {
				fprintf (file, "%*.s_[_]\t\t\t%d\n", level * 2, "", head_index);
				print_expr_table (unit, head->op.forward, file);
				print_expr_table (unit, head->op.backward, file);
			} else if (head->op.type == OpType (cast)) {
				fprintf (file, "%*.s[_] _\t\t\t%d\n", level * 2, "", head_index);
				fprintf (file, "%*.s", (level + 1) * 2, "");
				print_type (unit, head->op.backward, file);
				fprintf (file, "\n");
				print_expr_table (unit, head->op.forward, file);
			} else if (is_expr_unary (head)) {
				if (is_expr_postfix (head)) {
					fprintf (file, "%*.s%s\t\t\t%d\n", level * 2, "", get_op_name (head->op.type), head_index);
					print_expr_table (unit, head->op.forward, file);
				} else {
					fprintf (file, "%*.s%s\t\t\t%d\n", level * 2, "", get_op_name (head->op.type), head_index);
					print_expr_table (unit, head->op.forward, file);
				}
			} else {
				fprintf (file, "%*.s%s\t\t\t%d\n", level * 2, "", get_op_name (head->op.type), head_index);
				print_expr_table (unit, head->op.backward, file);
				print_expr_table (unit, head->op.forward, file);
			}
		} break ;
		case ExprType (identifier): {
			fprintf (file, "%*.s%s\t\t\t%d\n", level * 2, "", head->iden.name, head_index);
		} break ;
		case ExprType (constant): {
			fprintf (file, "%*.s%d\t\t\t%d\n", level * 2, "", (int) head->constant.value, head_index);
		} break ;
		case ExprType (funcparam): {
			level -= 1;
			print_expr_table (unit, head->funcparam.expr, file);
			if (head->funcparam.next >= 0) {
				print_expr_table (unit, head->funcparam.next, file);
			}
			level += 1;
		} break ;
		case ExprType (typeinfo): {
			fprintf (file, "%*.s", level * 2, "");
			print_type (unit, head->typeinfo.type, file);
			fprintf (file, "\t\t\t%d\n", head_index);
		} break ;
		default: {
			Error ("unknown %d %d", head->type, head_index);
		} break ;
	}
	level -= 1;
}



