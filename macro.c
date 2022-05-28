




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
			int				func_index;

			param_scope_index = make_scope (unit, ScopeKind (param), -1);
			func_index = make_function_type (unit, -1, param_scope_index);
			result = 1;
			do {
				*ptokens = next_token (*ptokens, 0);
				if ((*ptokens)[-1] == Token (identifier)) {
					const char	*name;

					name = *ptokens;
					*ptokens = next_token (*ptokens, 0);
					make_decl (unit, param_scope_index, name, -1, DeclKind (param));
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
					macro_decl = make_macro_decl (unit, scope_index, name, func_index, code_scope, param_scope_index);
					get_scope (unit, code_scope)->param_scope = param_scope_index;
					get_scope (unit, code_scope)->type_index = func_index;
					if (parse_scope (unit, code_scope, ptokens)) {
						if (is_token (*ptokens, Token (punctuator), "}")) {
							*ptokens = next_token (*ptokens, 0);
							result = 1;
						} else {
							Error ("unexpected token");
							result = 0;
						}
					}
				} else {
					Error ("unexpected token");
					result = 0;
				}
			}
		}
	} else {
		Error ("unexpected token");
		result = 0;
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
				*ptokens = next_token (*ptokens, 0);
				if (expr >= 0) {
					*out = make_expr_flow (unit, expr);
					*ptokens = next_token (*ptokens, 0);
					result = 1;
				} else {
					Error ("empty expression in macro");
					result = 0;
				}
			} else if (expr < 0) {
				result = 2;
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	return (result);
}





