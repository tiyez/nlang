

int		insert_macro_param_exprs (struct unit *unit, uint scope_index, uint expr_index);

int		insert_macro_param_types (struct unit *unit, uint scope_index, uint type_index) {
	int			result;
	struct type	*type;

	type = get_type (unit, type_index);
	if (type->kind == TypeKind (mod)) {
		result = insert_macro_param_types (unit, scope_index, type->mod.forward);
	} else if (type->kind == TypeKind (typeof)) {
		Assert (type->typeof.expr);
		result = insert_macro_param_exprs (unit, scope_index, type->typeof.expr);
	} else if (type->kind == TypeKind (deftype) || type->kind == TypeKind (internal) || type->kind == TypeKind (basic)) {
		result = 1;
	} else {
		Debug ("typekind %s", g_typekind[type->kind]);
		Unreachable ();
	}
	return (result);
}

uint	find_decl_in_scope (struct unit *unit, uint decl_begin, const char *name) {
	struct decl		*decl;
	uint			result;

	result = 0;
	while (!result && decl_begin) {
		decl = get_decl (unit, decl_begin);
		if (0 == strcmp (name, decl->name)) {
			result = decl_begin;
		}
		decl_begin = decl->next;
	}
	return (result);
}

int		insert_macro_param_exprs (struct unit *unit, uint scope_index, uint expr_index) {
	int			result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	if (expr->type == ExprType (op)) {
		if (is_expr_unary (expr)) {
			if (expr->op.type == OpType (typesizeof) || expr->op.type == OpType (typealignof)) {
				result = insert_macro_param_types (unit, scope_index, expr->op.forward);
			} else if (insert_macro_param_exprs (unit, scope_index, expr->op.forward)) {
				if (expr->op.type == OpType (function_call)) {
					if (expr->op.backward) {
						result = insert_macro_param_exprs (unit, scope_index, expr->op.backward);
					} else {
						result = 1;
					}
				} else if (expr->op.type == OpType (array_subscript)) {
					if (expr->op.backward) {
						result = insert_macro_param_exprs (unit, scope_index, expr->op.backward);
					} else {
						result = 1;
					}
				} else if (expr->op.type == OpType (cast)) {
					result = insert_macro_param_types (unit, scope_index, expr->op.backward);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else {
			if (insert_macro_param_exprs (unit, scope_index, expr->op.backward)) {
				result = insert_macro_param_exprs (unit, scope_index, expr->op.forward);
			} else {
				result = 0;
			}
		}
	} else if (expr->type == ExprType (funcparam)) {
		if (expr->funcparam.expr) {
			if (insert_macro_param_exprs (unit, scope_index, expr->funcparam.expr)) {
				if (expr->funcparam.next) {
					result = insert_macro_param_exprs (unit, scope_index, expr->funcparam.next);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else {
			uint	decl_index;

			decl_index = find_decl_in_scope (unit, get_scope (unit, get_scope (unit, scope_index)->param_scope)->decl_begin, "...");
			if (decl_index) {
				Assert (!expr->funcparam.next);
				expr->type = ExprType (macroparam);
				expr->macroparam.name = get_decl (unit, decl_index)->name;
				expr->macroparam.decl = decl_index;
				expr->macroparam.expr = 0;
				result = 1;
			} else {
				Error ("found '...' in macro body without variadic parameter");
				result = 0;
			}
		}
	} else if (expr->type == ExprType (identifier)) {
		uint	decl_index;

		decl_index = find_decl_in_scope (unit, get_scope (unit, get_scope (unit, scope_index)->param_scope)->decl_begin, expr->iden.name);
		if (decl_index) {
			expr->type = ExprType (macroparam);
			expr->macroparam.name = get_decl (unit, decl_index)->name;
			expr->macroparam.decl = decl_index;
			expr->macroparam.expr = 0;
		}
		result = 1;
	} else if (expr->type == ExprType (typeinfo)) {
		result = insert_macro_param_types (unit, scope_index, expr->typeinfo.type);
	} else if (expr->type == ExprType (constant) || expr->type == ExprType (string)) {
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		insert_macro_param_scope (struct unit *unit, uint scope_index);

int		insert_macro_param_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (if)) {
		if (insert_macro_param_exprs (unit, scope_index, flow->fif.expr)) {
			if (insert_macro_param_scope_flow (unit, scope_index, flow->fif.flow_body)) {
				result = insert_macro_param_scope_flow (unit, scope_index, flow->fif.else_body);
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (static_if)) {
		if (insert_macro_param_exprs (unit, scope_index, flow->static_if.expr)) {
			if (insert_macro_param_scope_flow (unit, scope_index, flow->static_if.flow_body)) {
				result = insert_macro_param_scope_flow (unit, scope_index, flow->static_if.else_body);
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (block)) {
		result = insert_macro_param_scope (unit, flow->block.scope);
	} else if (flow->type == FlowType (expr)) {
		result = insert_macro_param_exprs (unit, scope_index, flow->expr.index);
	} else if (flow->type == FlowType (assert) || flow->type == FlowType (static_assert)) {
		result = insert_macro_param_exprs (unit, scope_index, flow->assert.expr);
	} else {
		Unreachable ();
	}
	return (result);
}

int		insert_macro_param_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	flow_index = get_scope (unit, scope_index)->flow_begin;
	Assert (flow_index);
	do {
		result = insert_macro_param_scope_flow (unit, scope_index, flow_index);
		flow_index = get_flow (unit, flow_index)->next;
	} while (result && flow_index);
	return (result);
}

int		insert_macro_params (struct unit *unit, uint macro_decl) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, macro_decl);
	Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (macro));
	result = insert_macro_param_scope (unit, decl->define.macro.scope);
	return (result);
}

int		link_macro_scope (struct unit *unit, uint eval_scope_index, uint scope_index, struct typestack *typestack);

int		link_macro_scope_flow (struct unit *unit, uint eval_scope_index, uint scope_index, uint flow_index, struct typestack *typestack) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	flow->line = unit->pos.line;
	if (flow->type == FlowType (if)) {
		struct typestack	condstack;

		init_typestack (&condstack, 0);
		push_expr_path (unit, flow->fif.expr);
		if (link_expr (unit, eval_scope_index, flow->fif.expr, 1, &condstack)) {
			struct type	*type;

			type = get_typestack_head (&condstack);
			if (is_pointer_type (type, 0) || is_type_integral (type)) {
				result = 1;
			} else {
				Link_Error (unit, "if statement condition must have an integral or pointer type");
				print_type_typestack (unit, &condstack);
				result = 0;
			}
		} else {
			result = 0;
		}
		pop_path (unit);
		if (result) {
			init_typestack (&condstack, 0);
			if (link_macro_scope_flow (unit, eval_scope_index, scope_index, flow->fif.flow_body, typestack)) {
				if (link_macro_scope_flow (unit, eval_scope_index, scope_index, flow->fif.else_body, &condstack)) {
					if (is_typestacks_compatible (typestack, &condstack)) {
						result = 1;
					} else if (is_typestacks_compatible (&condstack, typestack)) {
						*typestack = condstack;
						result = 1;
					} else {
						Link_Error (unit, "branches types are incompatible");
						print_left_right_typestacks (unit, &condstack, typestack);
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		}
	} else if (flow->type == FlowType (static_if)) {
		struct typestack	condstack = {0};

		init_typestack (&condstack, 0);
		if (link_expr (unit, scope_index, flow->static_if.expr, 0, &condstack)) {
			struct evalvalue	value = {0};

			/* check if type is ok for if statement */
			if (eval_const_expr (unit, flow->static_if.expr, &value)) {
				int		is_true;

				if (value.type == EvalType (basic)) {
					if (is_basictype_integral (value.basic)) {
						if (is_basictype_signed (value.basic)) {
							is_true = value.value != 0;
						} else {
							is_true = value.uvalue != 0;
						}
						result = 1;
					} else if (value.basic == BasicType (void)) {
						Link_Error (unit, "value for static_if condition has void type");
						result = 0;
					} else {
						Link_Error (unit, "cannot convert floating point number to boolean in static_if condition");
						result = 0;
					}
				} else if (value.type == EvalType (string)) {
					is_true = value.string != 0;
					result = 1;
				} else if (value.type == EvalType (typeinfo_pointer)) {
					is_true = value.typeinfo_index != 0;
					result = 1;
				} else if (value.type == EvalType (typemember_pointer)) {
					is_true = value.typemember_index != 0;
					result = 1;
				} else {
					Link_Error (unit, "invalid value for static_if condition");
					result = 0;
				}
				if (result) {
					uint	next;

					next = flow->next;
					if (is_true) {
						*flow = *get_flow (unit, flow->static_if.flow_body);
						flow->next = next;
						result = link_macro_scope_flow (unit, eval_scope_index, scope_index, flow_index, typestack);
					} else {
						*flow = *get_flow (unit, flow->static_if.else_body);
						flow->next = next;
						result = link_macro_scope_flow (unit, eval_scope_index, scope_index, flow_index, typestack);
					}
				}
			} else {
				Link_Error (unit, "cannot evaluate constant expression");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (assert) || flow->type == FlowType (static_assert)) {
		struct typestack	condstack = {0};

		init_typestack (&condstack, 0);
		if (link_expr (unit, scope_index, flow->assert.expr, 0, &condstack)) {
			if (is_pointer_type (get_typestack_head (&condstack), 0) || is_type_integral (get_typestack_head (&condstack))) {
				if (flow->type == FlowType (static_assert)) {
					struct evalvalue	value = {0};

					if (eval_const_expr (unit, flow->assert.expr, &value)) {
						int		is_true;

						if (value.type == EvalType (basic)) {
							if (is_basictype_integral (value.basic)) {
								if (is_basictype_signed (value.basic)) {
									is_true = value.value != 0;
								} else {
									is_true = value.uvalue != 0;
								}
								result = 1;
							} else if (value.basic == BasicType (void)) {
								Link_Error (unit, "value for static_assert condition has void type");
								result = 0;
							} else {
								Link_Error (unit, "cannot convert floating point number to boolean in static_assert condition");
								result = 0;
							}
						} else if (value.type == EvalType (string)) {
							is_true = value.string != 0;
							result = 1;
						} else if (value.type == EvalType (typeinfo_pointer)) {
							is_true = value.typeinfo_index != 0;
							result = 1;
						} else if (value.type == EvalType (typemember_pointer)) {
							is_true = value.typemember_index != 0;
							result = 1;
						} else {
							Link_Error (unit, "invalid value for static_assert condition");
							result = 0;
						}
						if (result) {
							if (is_true) {
								result = 1;
							} else {
								Link_Error (unit, "Assertion failed");
								fprintf (stderr, "Asserted expression: ");
								print_expr (unit, flow->assert.expr, stderr);
								fprintf (stderr, "\n");
								result = 0;
							}
						}
					} else {
						Link_Error (unit, "cannot evaluate constant expression of static_assert statement");
						result = 0;
					}
				} else {
					result = 1;
				}
				init_typestack (typestack, typestack->is_sizeof_context);
				push_basictype_to_typestack (typestack, BasicType (int), 0);
			} else {
				Link_Error (unit, "assert condition must have an integral or pointer type");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (block)) {
		result = link_macro_scope (unit, eval_scope_index, flow->block.scope, typestack);
	} else if (flow->type == FlowType (expr)) {
		push_expr_path (unit, flow->expr.index);
		result = link_expr (unit, eval_scope_index, flow->expr.index, 1, typestack);
		pop_path (unit);
	} else {
		Unreachable ();
	}
	return (result);
}

int		link_macro_scope (struct unit *unit, uint eval_scope_index, uint scope_index, struct typestack *typestack) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	flow_index = get_scope (unit, scope_index)->flow_begin;
	Assert (flow_index);
	do {
		push_flow_path (unit, scope_index, flow_index);
		init_typestack (typestack, typestack->is_sizeof_context);
		result = link_macro_scope_flow (unit, eval_scope_index, scope_index, flow_index, typestack);
		flow_index = get_flow (unit, flow_index)->next;
		pop_path (unit);
	} while (result && flow_index);
	return (result);
}

int		link_macro_eval (struct unit *unit, uint scope_index, uint decl_index, uint expr_index, struct typestack *typestack) {
	int				result;
	struct decl		*macro_decl;
	struct scope	*param_scope;

	macro_decl = get_decl (unit, decl_index);
	macro_decl->define.macro.expr_params = expr_index;
	push_macro_path (unit, decl_index);
	Assert (macro_decl->kind == DeclKind (define) && macro_decl->define.kind == DefineKind (macro));
	param_scope = get_scope (unit, macro_decl->define.macro.param_scope);
	if (param_scope->decl_begin) {
		if (expr_index) {
			struct decl	*decl;
			struct expr	*expr;

			decl = get_decl (unit, param_scope->decl_begin);
			expr = get_expr (unit, expr_index);
			do {
				push_decl_path (unit, get_decl_index (unit, decl));
				push_expr_path (unit, get_expr_index (unit, expr));
				Assert (decl->kind == DeclKind (param));
				if (expr->type == ExprType (macroparam)) {
					uint	copy_expr;
					struct decl	*decl;

					Assert (!expr->macroparam.expr);
					decl = get_decl (unit, expr->macroparam.decl);
					Assert (decl->kind == DeclKind (param));
					Assert (decl->param.expr);
					copy_expr = make_expr_copy (unit, unit, decl->param.expr, scope_index);
					Assert (copy_expr);
					*expr = *get_expr (unit, copy_expr);
				}
				Assert (expr->type == ExprType (funcparam));
				if (0 == strcmp (decl->name, "...")) {
					decl->param.expr = get_expr_index (unit, expr);
				} else {
					decl->param.expr = expr->funcparam.expr;
				}
				if (decl->next) {
					if (expr->funcparam.next) {
						decl = get_decl (unit, decl->next);
						expr = get_expr (unit, expr->funcparam.next);
						result = 1;
					} else {
						Link_Error (unit, "too few arguments for macro call");
						result = 0;
					}
				} else if (expr->funcparam.next && 0 != strcmp (decl->name, "...")) {
					Link_Error (unit, "too many arguments for macro call");
					result = 0;
				} else {
					decl = 0;
					result = 1;
				}
				pop_path (unit);
				pop_path (unit);
			} while (result && decl);
		} else {
			Link_Error (unit, "too few arguments for macro call");
			result = 0;
		}
	} else if (!expr_index) {
		result = 1;
	} else {
		Link_Error (unit, "too many arguments for macro call");
		result = 0;
	}
	if (result) {
		if (insert_macro_params (unit, decl_index)) {
			if (link_macro_scope (unit, scope_index, macro_decl->define.macro.scope, typestack)) {
				macro_decl->define.macro.param_scope = 0;
				get_scope (unit, macro_decl->define.macro.scope)->param_scope = 0;
				macro_decl->type = make_basic_type (unit, BasicType (void), 0);
				insert_typestack_to_type (unit, macro_decl->type, typestack);
				result = 1;
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	pop_path (unit);
	return (result);
}


int		parse_macro_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;
	int		line;

	Assert (is_token (*ptokens, Token (identifier), "macro"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;

		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if (is_token (*ptokens, Token (punctuator), "(")) {
			uint			param_scope_index;
			int				is_continue;

			is_continue = 1;
			param_scope_index = make_scope (unit, ScopeKind (param), 0);
			do {
				*ptokens = next_token (*ptokens, &unit->pos);
				if ((*ptokens)[-1] == Token (identifier)) {
					const char	*name;

					name = *ptokens;
					*ptokens = next_token (*ptokens, &unit->pos);
					make_param_decl (unit, param_scope_index, name, 0, line);
					if (is_token (*ptokens, Token (punctuator), ")")) {
						is_continue = 0;
						result = 1;
					} else if (!is_token (*ptokens, Token (punctuator), ",")) {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					} else {
						result = 1;
					}
				} else if (is_token (*ptokens, Token (punctuator), "...")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					make_param_decl (unit, param_scope_index, "...", 0, line);
					if (is_token (*ptokens, Token (punctuator), ")")) {
						is_continue = 0;
						result = 1;
					} else {
						Parse_Error (*ptokens, unit->pos, "'...' token must be last one in macro parameters");
						result = 0;
					}
				} else if (is_token (*ptokens, Token (punctuator), ")")) {
					is_continue = 0;
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token %s", *ptokens);
					result = 0;
				}
			} while (result && is_continue);
			if (result) {
				Assert (is_token (*ptokens, Token (punctuator), ")"));
				*ptokens = next_token (*ptokens, &unit->pos);
				if (is_token (*ptokens, Token (punctuator), "{")) {
					uint	code_scope;
					uint	macro_decl;

					*ptokens = next_token (*ptokens, &unit->pos);
					code_scope = make_scope (unit, ScopeKind (macro), scope_index);
					macro_decl = make_define_macro_decl (unit, scope_index, name, code_scope, param_scope_index, line);
					get_scope (unit, code_scope)->param_scope = param_scope_index;
					if (parse_scope (unit, code_scope, ptokens)) {
						if (is_token (*ptokens, Token (punctuator), "}")) {
							*ptokens = next_token (*ptokens, &unit->pos);
							*out = make_decl_flow (unit, macro_decl, line);
							result = 1;
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token %d[%s]", (*ptokens)[-1], *ptokens);
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				}
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "unexpected token");
			result = 0;
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_macro_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;
	int		line;

	line = unit->pos.line;
	if (is_token (*ptokens, Token (identifier), "if")) {
		uint	expr;

		*ptokens = next_token (*ptokens, &unit->pos);
		if (parse_expr (unit, ptokens, &expr)) {
			if (expr) {
				uint	body;

				if (parse_macro_scope_flow (unit, scope_index, ptokens, &body)) {
					if (is_token (*ptokens, Token (identifier), "else")) {
						uint	else_body;

						*ptokens = next_token (*ptokens, &unit->pos);
						if (parse_macro_scope_flow (unit, scope_index, ptokens, &else_body)) {
							*out = make_if_flow (unit, expr, body, else_body, line);
							result = 1;
						} else {
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "'else' branch for macro condition is mandatory");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "empty condition");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (is_token (*ptokens, Token (identifier), "static_if")) {
		uint	expr;

		*ptokens = next_token (*ptokens, &unit->pos);
		if (parse_expr (unit, ptokens, &expr)) {
			if (expr) {
				uint	body;

				if (parse_macro_scope_flow (unit, scope_index, ptokens, &body)) {
					if (is_token (*ptokens, Token (identifier), "else")) {
						uint	else_body;

						*ptokens = next_token (*ptokens, &unit->pos);
						if (parse_macro_scope_flow (unit, scope_index, ptokens, &else_body)) {
							*out = make_static_if_flow (unit, expr, body, else_body, line);
							result = 1;
						} else {
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "'else' branch for macro condition is mandatory");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "empty condition");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (is_token (*ptokens, Token (identifier), "assert") || is_token (*ptokens, Token (identifier), "static_assert")) {
		uint		expr_index;
		int			is_static;
		char		*start, *end;

		is_static = 0 == strcmp (*ptokens, "static_assert");
		*ptokens = next_token (*ptokens, &unit->pos);
		start = *ptokens;
		if (parse_expr (unit, ptokens, &expr_index)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				if (expr_index) {
					char	*string;

					end = *ptokens;
					string = g_tokenizer.current;
					if (push_string_token (&g_tokenizer, 0, start, get_token_length (start), 0)) {
						string = get_next_from_tokenizer (&g_tokenizer, string);
						result = 1;
						while (result && start != end) {
							start = next_token (start, 0);
							result = push_string_token (&g_tokenizer, get_token_offset (start), start, get_token_length (start), 1);
						}
						if (result) {
							*ptokens = next_token (*ptokens, &unit->pos);
							if (is_static) {
								*out = make_static_assert_flow (unit, expr_index, string, line);
							} else {
								*out = make_assert_flow (unit, expr_index, string, line);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "cannot create string of asserted expression");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "cannot create string of asserted expression");
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "empty assert condition");
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (is_token (*ptokens, Token (punctuator), "{")) {
		uint	inner_scope;

		*ptokens = next_token (*ptokens, &unit->pos);
		inner_scope = make_scope (unit, ScopeKind (macro), scope_index);
		get_scope (unit, inner_scope)->param_scope = get_scope (unit, scope_index)->param_scope;
		*out = make_block_flow (unit, inner_scope, line);
		if (parse_scope (unit, inner_scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				if (get_scope (unit, inner_scope)->flow_begin) {
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "empty macro block");
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else {
		uint	expr;

		if (parse_expr (unit, ptokens, &expr)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				if (expr) {
					*out = make_expr_flow (unit, expr, line);
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "empty expression in macro");
					result = 0;
				}
			} else if (!expr) {
				result = 2;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token [%s]", *ptokens);
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	return (result);
}





