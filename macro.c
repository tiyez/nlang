

int		link_macro_scope (struct unit *unit, int eval_scope_index, int scope_index, struct typestack *typestack);

int		link_macro_scope_flow (struct unit *unit, int eval_scope_index, int scope_index, int flow_index, struct typestack *typestack) {
	int		result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (if): {
			struct typestack	condstack;

			init_typestack (&condstack);
			push_expr_path (unit, flow->fif.expr);
			if (link_expr (unit, eval_scope_index, flow->fif.expr, 1, &condstack)) {
				/* todo: check if condstack is ok for if statement */
				result = 1;
			} else {
				result = 0;
			}
			pop_path (unit);
			if (result) {
				init_typestack (&condstack);
				if (link_macro_scope_flow (unit, eval_scope_index, scope_index, flow->fif.flow_body, &condstack)) {
					if (link_macro_scope_flow (unit, eval_scope_index, scope_index, flow->fif.else_body, typestack)) {
						if (is_implicit_castable (unit, &condstack, typestack, 1)) {
							result = 1;
						} else {
							Code_Error (unit, "branches' types are incompatible");
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			}
		} break ;
		case FlowType (block): {
			result = link_macro_scope (unit, eval_scope_index, flow->block.scope, typestack);
		} break ;
		case FlowType (expr): {
			push_expr_path (unit, flow->expr.index);
			result = link_expr (unit, eval_scope_index, flow->expr.index, 1, typestack);
			pop_path (unit);
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_macro_scope (struct unit *unit, int eval_scope_index, int scope_index, struct typestack *typestack) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			push_flow_path (unit, scope_index, get_flow_index (unit, flow));
			if (flow->next >= 0) {
				struct typestack	innerstack;

				init_typestack (&innerstack);
				result = link_macro_scope_flow (unit, eval_scope_index, scope_index, get_flow_index (unit, flow), &innerstack);
				flow = get_flow (unit, flow->next);
			} else {
				result = link_macro_scope_flow (unit, eval_scope_index, scope_index, get_flow_index (unit, flow), typestack);
				flow = 0;
			}
			pop_path (unit);
		} while (result && flow);
	} else {
		Code_Error (unit, "empty macro body");
		result = 0;
	}
	return (result);
}

int		link_macro_eval (struct unit *unit, int scope_index, int decl_index, int expr_index, struct typestack *typestack) {
	int				result;
	struct decl		*macro_decl;
	struct scope	*param_scope;

	macro_decl = get_decl (unit, decl_index);
	push_macro_path (unit, decl_index);
	Assert (macro_decl->kind == DeclKind (macro));
	param_scope = get_scope (unit, macro_decl->macro.param_scope);
	if (param_scope->decl_begin >= 0) {
		if (expr_index >= 0) {
			struct decl	*decl;
			struct expr	*expr;

			decl = get_decl (unit, param_scope->decl_begin);
			expr = get_expr (unit, expr_index);
			do {
				push_decl_path (unit, get_decl_index (unit, decl));
				push_expr_path (unit, get_expr_index (unit, expr));
				Assert (decl->kind == DeclKind (param));
				Assert (expr->type == ExprType (funcparam));
				decl->param.expr = expr->funcparam.expr;
				if (decl->next >= 0) {
					if (expr->funcparam.next >= 0) {
						decl = get_decl (unit, decl->next);
						expr = get_expr (unit, expr->funcparam.next);
						result = 1;
					} else {
						Code_Error (unit, "too few arguments for macro call");
						result = 0;
					}
				} else if (expr->funcparam.next >= 0) {
					Code_Error (unit, "too many arguments for macro call");
					result = 0;
				} else {
					decl = 0;
					result = 1;
				}
				pop_path (unit);
				pop_path (unit);
			} while (result && decl);
		} else {
			Code_Error (unit, "too few arguments for macro call");
			result = 0;
		}
	} else if (expr_index < 0) {
		result = 1;
	} else {
		Code_Error (unit, "too many arguments for macro call");
		result = 0;
	}
	if (result) {
		result = link_macro_scope (unit, scope_index, macro_decl->macro.scope, typestack);
	}
	pop_path (unit);
	return (result);
}


int		parse_macro_decl_flow (struct unit *unit, int scope_index, char **ptokens, int *out) {
	int		result;

	Assert (is_token (*ptokens, Token (identifier), "macro"));
	*ptokens = next_token (*ptokens, 0);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;

		name = *ptokens;
		*ptokens = next_token (*ptokens, 0);
		if (is_token (*ptokens, Token (punctuator), "(")) {
			int				param_scope_index;
			int				is_continue = 1;

			param_scope_index = make_scope (unit, ScopeKind (param), -1);
			result = 1;
			do {
				*ptokens = next_token (*ptokens, 0);
				if ((*ptokens)[-1] == Token (identifier)) {
					const char	*name;

					name = *ptokens;
					*ptokens = next_token (*ptokens, 0);
					make_param_decl (unit, param_scope_index, name, -1);
					if (is_token (*ptokens, Token (punctuator), ")")) {
						is_continue = 0;
					} else if (!is_token (*ptokens, Token (punctuator), ",")) {
						Error ("unexpected token");
						result = 0;
					} else {
						result = 1;
					}
				} else if (is_token (*ptokens, Token (punctuator), ")")) {
					is_continue = 0;
				} else {
					Error ("unexpected token %s", *ptokens);
					result = 0;
				}
			} while (result && is_continue);
			if (result) {
				Assert (is_token (*ptokens, Token (punctuator), ")"));
				*ptokens = next_token (*ptokens, 0);
				if (is_token (*ptokens, Token (punctuator), "{")) {
					int		code_scope;
					int		macro_decl;

					*ptokens = next_token (*ptokens, 0);
					code_scope = make_scope (unit, ScopeKind (macro), scope_index);
					macro_decl = make_macro_decl (unit, scope_index, name, code_scope, param_scope_index);
					get_scope (unit, code_scope)->param_scope = param_scope_index;
					if (parse_scope (unit, code_scope, ptokens)) {
						if (is_token (*ptokens, Token (punctuator), "}")) {
							*ptokens = next_token (*ptokens, 0);
							*out = make_decl_flow (unit, macro_decl);
							result = 1;
						} else {
							Error ("unexpected token %d[%s]", (*ptokens)[-1], *ptokens);
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					Error ("unexpected token");
					result = 0;
				}
			}
		} else {
			Error ("unexpected token");
			result = 0;
		}
	} else {
		Error ("unexpected token");
		result = 0;
	}
	return (result);
}

int		insert_macro_param_exprs (struct unit *unit, int scope_index, int expr_index) {
	int		result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	switch (expr->type) {
		case ExprType (op): {
			if (is_expr_unary (expr)) {
				if (insert_macro_param_exprs (unit, scope_index, expr->op.forward)) {
					if (expr->op.type == OpType (function_call)) {
						if (expr->op.backward >= 0) {
							result = insert_macro_param_exprs (unit, scope_index, expr->op.backward);
						} else {
							result = 1;
						}
					} else if (expr->op.type == OpType (array_subscript)) {
						if (expr->op.backward >= 0) {
							result = insert_macro_param_exprs (unit, scope_index, expr->op.backward);
						} else {
							result = 1;
						}
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
		} break ;
		case ExprType (funcparam): {
			if (insert_macro_param_exprs (unit, scope_index, expr->funcparam.expr)) {
				if (expr->funcparam.next >= 0) {
					result = insert_macro_param_exprs (unit, scope_index, expr->funcparam.next);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} break ;
		case ExprType (identifier): {
			struct scope	*scope;

			scope = get_scope (unit, get_scope (unit, scope_index)->param_scope);
			if (scope->decl_begin >= 0) {
				struct decl	*decl;

				decl = get_decl (unit, scope->decl_begin);
				do {
					if (0 == strcmp (expr->iden.name, decl->name)) {
						expr->type = ExprType (macroparam);
						expr->macroparam.name = decl->name;
						expr->macroparam.decl = get_decl_index (unit, decl);
					}
					if (decl->next >= 0) {
						decl = get_decl (unit, decl->next);
					} else {
						decl = 0;
					}
				} while (expr->type == ExprType (identifier) && decl);
			}
			result = 1;
		} break ;
		default: {
			result = 1;
		} break ;
	}
	return (result);
}

int		parse_macro_scope_flow (struct unit *unit, int scope_index, char **ptokens, int *out) {
	int		result;

	if (is_token (*ptokens, Token (identifier), "if")) {
		int		expr;

		*ptokens = next_token (*ptokens, 0);
		if (parse_expr (unit, ptokens, &expr)) {
			if (expr >= 0) {
				int		body;

				if (insert_macro_param_exprs (unit, scope_index, expr)) {
					if (parse_macro_scope_flow (unit, scope_index, ptokens, &body)) {
						if (is_token (*ptokens, Token (identifier), "else")) {
							int		else_body;

							*ptokens = next_token (*ptokens, 0);
							if (parse_macro_scope_flow (unit, scope_index, ptokens, &else_body)) {
								*out = make_if_flow (unit, expr, body, else_body);
								result = 1;
							} else {
								result = 0;
							}
						} else {
							Error ("'else' branch for macro condition is mandatory");
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Error ("empty condition");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (is_token (*ptokens, Token (punctuator), "{")) {
		int		inner_scope;

		*ptokens = next_token (*ptokens, 0);
		inner_scope = make_scope (unit, ScopeKind (macro), scope_index);
		get_scope (unit, inner_scope)->param_scope = get_scope (unit, scope_index)->param_scope;
		*out = make_block_flow (unit, inner_scope);
		if (parse_scope (unit, inner_scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				if (get_scope (unit, inner_scope)->flow_begin >= 0) {
					*ptokens = next_token (*ptokens, 0);
					result = 1;
				} else {
					Error ("empty macro block");
					result = 0;
				}
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else {
		int		expr;

		if (parse_expr (unit, ptokens, &expr)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				if (expr >= 0) {
					if (insert_macro_param_exprs (unit, scope_index, expr)) {
						*out = make_expr_flow (unit, expr);
						*ptokens = next_token (*ptokens, 0);
						result = 1;
					} else {
						result = 0;
					}
				} else {
					Error ("empty expression in macro");
					result = 0;
				}
			} else if (expr < 0) {
				result = 2;
			} else {
				Error ("unexpected token [%s]", *ptokens);
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	return (result);
}





