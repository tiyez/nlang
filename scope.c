


void	init_scope (struct scope *scope) {
	scope->parent_scope = -1;
	scope->decl_begin = -1;
	scope->decl_last = -1;
	scope->flow_begin = -1;
	scope->flow_last = -1;
}

struct scope	*get_scope (struct unit *unit, int scope_index) {
	struct scope	*scope;

	Assert (scope_index >= 0 && scope_index < Get_Array_Count (unit->scopes));
	scope = unit->scopes + scope_index;
	return (scope);
}

int		get_scope_index (struct unit *unit, struct scope *scope) {
	return (Get_Element_Index (unit->scopes, scope));
}

int		make_scope (struct unit *unit, enum scopekind kind, int parent) {
	int		index;

	if (Prepare_Array (unit->scopes, 1)) {
		struct scope	*scope;

		scope = Push_Array (unit->scopes);
		init_scope (scope);
		scope->kind = kind;
		scope->parent_scope = parent;
		scope->param_scope = -1;
		index = Get_Element_Index (unit->scopes, scope);
	} else {
		index = -1;
	}
	return (index);
}

struct flow	*get_flow (struct unit *unit, int index) {
	struct flow	*flow;

	Assert (index >= 0 && index < Get_Array_Count (unit->flows));
	flow = unit->flows + index;
	return (flow);
}

int		get_flow_index (struct unit *unit, struct flow *flow) {
	return (Get_Element_Index (unit->flows, flow));
}

int		make_flow (struct unit *unit, enum flowtype type) {
	int		index;

	if (Prepare_Array (unit->flows, 1)) {
		struct flow		*flow;

		flow = Push_Array (unit->flows);
		flow->next = -1;
		flow->type = type;
		index = Get_Element_Index (unit->flows, flow);
	} else {
		Error ("cannot prepare flow array");
		index = -1;
	}
	return (index);
}

int		make_block_flow (struct unit *unit, int inner_scope) {
	int		index;

	index = make_flow (unit, FlowType (block));
	if (index >= 0) {
		get_flow (unit, index)->block.scope = inner_scope;
	}
	return (index);
}

int		make_decl_block_flow (struct unit *unit, const char *name, int inner_scope) {
	int		index;

	index = make_flow (unit, FlowType (declblock));
	if (index >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, index);
		flow->declblock.name = name;
		flow->declblock.scope = inner_scope;
	}
	return (index);
}

int		make_expr_flow (struct unit *unit, int expr) {
	int		index;

	index = make_flow (unit, FlowType (expr));
	if (index >= 0) {
		get_flow (unit, index)->expr.index = expr;
	}
	return (index);
}

int		make_if_flow (struct unit *unit, int expr, int body) {
	int		index;

	index = make_flow (unit, FlowType (if));
	if (index >= 0) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->fif.expr = expr;
		flow->fif.flow_body = body;
	}
	return (index);
}

int		make_while_flow (struct unit *unit, int expr, int body) {
	int		index;

	index = make_flow (unit, FlowType (while));
	if (index >= 0) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->fwhile.expr = expr;
		flow->fwhile.flow_body = body;
	}
	return (index);
}

int		make_dowhile_flow (struct unit *unit, int expr, int body) {
	int		index;

	index = make_flow (unit, FlowType (dowhile));
	if (index >= 0) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->dowhile.expr = expr;
		flow->dowhile.flow_body = body;
	}
	return (index);
}

int		make_decl_flow (struct unit *unit, int decl_index) {
	int		index;

	index = make_flow (unit, FlowType (decl));
	if (index >= 0) {
		get_flow (unit, index)->decl.index = decl_index;
	}
	return (index);
}

struct decl	*get_decl (struct unit *unit, int index) {
	struct decl	*decl;

	Assert (index >= 0 && index < Get_Array_Count (unit->decls));
	decl = unit->decls + index;
	return (decl);
}

int		make_decl (struct unit *unit, int scope_index, const char *name, int type, enum declkind kind) {
	int		index;

	if (Prepare_Array (unit->decls, 1)) {
		struct scope	*scope = get_scope (unit, scope_index);
		struct decl		*decl;

		decl = Push_Array (unit->decls);
		if (scope->decl_last >= 0) {
			scope->decl_last = get_decl (unit, scope->decl_last)->next = Get_Element_Index (unit->decls, decl);
		} else {
			scope->decl_last = scope->decl_begin = Get_Element_Index (unit->decls, decl);
		}
		decl->next = -1;
		decl->name = name;
		decl->type = type;
		decl->kind = kind;
		decl->body = -1;
		index = Get_Element_Index (unit->decls, decl);
	} else {
		Error ("cannot prepare decl array");
		index = -1;
	}
	return (index);
}

void	add_flow_to_scope (struct unit *unit, int scope_index, int flow_index) {
	struct scope	*scope;

	Assert (flow_index >= 0);
	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		scope->flow_last = get_flow (unit, scope->flow_last)->next = flow_index;
	} else {
		scope->flow_last = scope->flow_begin = flow_index;
	}
}

int		parse_code_scope_flow (struct unit *unit, int scope_index, char **ptokens, int *out) {
	int		result;

	*out = -1;
	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "if")) {
			int		expr, body;

			*out = make_if_flow (unit, -1, -1);
			*ptokens = next_token (*ptokens, 0);
			if (parse_expr (unit, ptokens, &expr)) {
				if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
					int		else_body = -1;

					if (is_token (*ptokens, Token (identifier), "else")) {
						*ptokens = next_token (*ptokens, 0);
						if (parse_code_scope_flow (unit, scope_index, ptokens, &else_body)) {
							result = 1;
						} else {
							Error ("cannot parse 'else' body");
							result = 0;
						}
					} else {
						result = 1;
					}
					if (result) {
						struct flow	*fif;

						fif = get_flow (unit, *out);
						fif->fif.expr = expr;
						fif->fif.flow_body = body;
						fif->fif.else_body = else_body;
						result = 1;
					}
				} else {
					Error ("cannot parse 'if' body");
					result = 0;
				}
			} else {
				Error ("cannot parse 'if' condition");
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "while")) {
			int		expr, body;

			*out = make_while_flow (unit, -1, -1);
			*ptokens = next_token (*ptokens, 0);
			if (parse_expr (unit, ptokens, &expr)) {
				if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
					struct flow	*fwhile;

					fwhile = get_flow (unit, *out);
					fwhile->fwhile.expr = expr;
					fwhile->fwhile.flow_body = body;
					result = 1;
				} else {
					Error ("cannot parse 'while' body");
					result = 0;
				}
			} else {
				Error ("cannot parse 'while' condition");
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "do")) {
			int		expr, body;

			*out = make_dowhile_flow (unit, -1, -1);
			*ptokens = next_token (*ptokens, 0);
			if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
				if (is_token (*ptokens, Token (identifier), "while")) {
					*ptokens = next_token (*ptokens, 0);
					if (parse_expr (unit, ptokens, &expr)) {
						if (is_token (*ptokens, Token (punctuator), ";")) {
							struct flow	*dowhile;

							*ptokens = next_token (*ptokens, 0);
							dowhile = get_flow (unit, *out);
							dowhile->dowhile.expr = expr;
							dowhile->dowhile.flow_body = body;
							result = 1;
						} else {
							Error ("unexpected token");
							result = 0;
						}
					} else {
						Error ("cannot parse 'do while' condition");
						result = 0;
					}
				} else {
					Error ("unexpected token");
					result = 0;
				}
			} else {
				Error ("cannot parse 'do while' body");
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "var")) {
			const char	*name;
			int			type_index;

			*ptokens = next_token (*ptokens, 0);
			if ((*ptokens)[-1] == Token (identifier)) {
				name = *ptokens;
				*ptokens = next_token (*ptokens, 0);
				if (parse_type (unit, ptokens, &type_index)) {
					if (type_index >= 0) {
						if (is_token (*ptokens, Token (punctuator), ";")) {
							struct type	*type;

							type = get_type (unit, type_index);
							if (!(type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function))) {
								int		decl;

								*ptokens = next_token (*ptokens, 0);
								decl = make_decl (unit, scope_index, name, type_index, DeclKind (var));
								if (decl >= 0) {
									*out = make_decl_flow (unit, decl);
									Assert (*out >= 0);
									result = 1;
								} else {
									Error ("cannot create decl");
									result = 0;
								}
							} else {
								Error ("function declaration is forbidden in the code scope");
								result = 0;
							}
						} else {
							Error ("unexpected token");
							result = 0;
						}
					} else {
						Error ("empty type");
						result = 0;
					}
				} else {
					Error ("cannot parse type");
					result = 0;
				}
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			int		expr;

			if (parse_expr (unit, ptokens, &expr)) {
				if (is_token (*ptokens, Token (punctuator), ";")) {
					*ptokens = next_token (*ptokens, 0);
					*out = make_expr_flow (unit, expr);
					Assert (*out >= 0);
					result = 1;
				} else {
					Error ("unexpected token [%s]", *ptokens);
					result = 0;
				}
			} else {
				Error ("cannot parse expr");
				result = 0;
			}
		}
	} else if (is_token (*ptokens, Token (punctuator), "{")) {
		int		inner_scope;

		inner_scope = make_scope (unit, ScopeKind (code), scope_index);
		*out = make_block_flow (unit, inner_scope);
		*ptokens = next_token (*ptokens, 0);
		if (parse_scope (unit, inner_scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				*ptokens = next_token (*ptokens, 0);
				result = 1;
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			Error ("cannot parse code block");
			result = 0;
		}
	} else if (is_token (*ptokens, Token (punctuator), ";")) {
		*ptokens = next_token (*ptokens, 0);
		*out = make_expr_flow (unit, -1);
		Assert (*out >= 0);
		result = 1;
	} else {
		int		expr;

		if (parse_expr (unit, ptokens, &expr)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				*ptokens = next_token (*ptokens, 0);
				*out = make_expr_flow (unit, expr);
				Assert (*out >= 0);
				result = 1;
			} else if (expr < 0) {
				Debug ("MISSING");
				result = 2;
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			Error ("cannot parse expr");
			result = 0;
		}
	}
	return (result);
}

int		parse_tag_decl_flow (struct unit *unit, enum declkind declkind, enum scopekind scopekind, int scope_index, char **ptokens, int *out) {
	int		result;

	*ptokens = next_token (*ptokens, 0);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;

		name = *ptokens;
		*ptokens = next_token (*ptokens, 0);
		if (is_token (*ptokens, Token (punctuator), "{")) {
			int		scope;

			*ptokens = next_token (*ptokens, 0);
			scope = make_scope (unit, scopekind, scope_index);
			if (parse_scope (unit, scope, ptokens)) {
				if (is_token (*ptokens, Token (punctuator), "}")) {
					int		decl;

					decl = make_decl (unit, scope_index, name, make_struct_type (unit, name), declkind);
					get_decl (unit, decl)->body = scope;
					*out = make_decl_flow (unit, decl);
					*ptokens = next_token (*ptokens, 0);
					result = 1;
				} else {
					Error ("unexpected token");
					result = 0;
				}
			} else {
				Error ("cannot parse 'struct' body");
				result = 0;
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

int		parse_unit_scope_flow (struct unit *unit, int scope_index, char **ptokens, int *out) {
	int		result;

	*out = -1;
	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "struct")) {
			result = parse_tag_decl_flow (unit, DeclKind (struct), ScopeKind (struct), scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "union")) {
			result = parse_tag_decl_flow (unit, DeclKind (union), ScopeKind (union), scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "stroke")) {
			result = parse_tag_decl_flow (unit, DeclKind (stroke), ScopeKind (stroke), scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "enum")) {
			result = parse_tag_decl_flow (unit, DeclKind (enum), ScopeKind (enum), scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "bitfield")) {
			result = parse_tag_decl_flow (unit, DeclKind (bitfield), ScopeKind (bitfield), scope_index, ptokens, out);
		} else {
			const char	*name;
			int			type_index;

			name = *ptokens;
			*ptokens = next_token (*ptokens, 0);
			if (parse_type (unit, ptokens, &type_index)) {
				if (type_index >= 0) {
					struct type	*type;

					type = get_type (unit, type_index);
					if (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function)) {
						if (is_token (*ptokens, Token (punctuator), "{")) {
							int		decl;
							int		func_scope;

							decl = make_decl (unit, scope_index, name, type_index, DeclKind (func));
							*out = make_decl_flow (unit, decl);
							*ptokens = next_token (*ptokens, 0);
							func_scope = make_scope (unit, ScopeKind (func), scope_index);
							get_scope (unit, func_scope)->param_scope = type->mod.func.param_scope;
							if (parse_scope (unit, func_scope, ptokens)) {
								if (is_token (*ptokens, Token (punctuator), "}")) {
									*ptokens = next_token (*ptokens, 0);
									get_decl (unit, decl)->body = func_scope;
									result = 1;
								} else {
									Error ("unexpected token");
									result = 0;
								}
							} else {
								Error ("cannot parse code scope");
								result = 0;
							}
						} else {
							Error ("unexpected token");
							result = 0;
						}
					} else if (is_token (*ptokens, Token (punctuator), ";")) {
						int			decl;

						*ptokens = next_token (*ptokens, 0);
						decl = make_decl (unit, scope_index, name, type_index, DeclKind (var));
						*out = make_decl_flow (unit, decl);
						result = 1;
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					Error ("empty type");
					result = 0;
				}
			} else {
				Error ("cannot parse type");
				result = 0;
			}
		}
	} else {
		result = 2;
	}
	return (result);
}

int		parse_struct_scope_flow (struct unit *unit, int scope_index, char **ptokens, int *out) {
	int		result;

	*out = -1;
	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "const")) {
			Error ("reserved hint word");
			result = 0;
		} else if (0 == strcmp (*ptokens, "assert")) {
			Error ("reserved hint word");
			result = 0;
		} else if (0 == strcmp (*ptokens, "assume")) {
			Error ("reserved hint word");
			result = 0;
		} else if (0 == strcmp (*ptokens, "union")) {
			Error ("reserved hint word");
			result = 0;
		} else {
			const char	*name;

			name = *ptokens;
			*ptokens = next_token (*ptokens, 0);
			if (is_token (*ptokens, Token (punctuator), "{")) {
				int		scope;

				*ptokens = next_token (*ptokens, 0);
				scope = make_scope (unit, ScopeKind (struct), scope_index);
				if (parse_scope (unit, scope, ptokens)) {
					if (is_token (*ptokens, Token (punctuator), "}")) {
						*ptokens = next_token (*ptokens, 0);
						*out = make_decl_block_flow (unit, name, scope);
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					Error ("cannot parse struct scope");
					result = 0;
				}
			} else {
				int		type_index;

				if (parse_type (unit, ptokens, &type_index)) {
					if (is_token (*ptokens, Token (punctuator), ";")) {
						int		decl;

						*ptokens = next_token (*ptokens, 0);
						decl = make_decl (unit, scope_index, name, type_index, DeclKind (var));
						*out = make_decl_flow (unit, decl);
						result = 1;
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					Error ("cannot parse type");
					result = 0;
				}
			}
		}
	} else if (is_token (*ptokens, Token (punctuator), "{")) {
		int		scope;

		*ptokens = next_token (*ptokens, 0);
		scope = make_scope (unit, ScopeKind (struct), scope_index);
		if (parse_scope (unit, scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				*ptokens = next_token (*ptokens, 0);
				*out = make_decl_block_flow (unit, 0, scope);
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			Error ("cannot parse struct scope");
			result = 0;
		}
	} else {
		result = 2;
	}
	return (result);
}

int		parse_scope (struct unit *unit, int scope_index, char **ptokens) {
	int				result;
	enum scopekind	scopekind;

	scopekind = get_scope (unit, scope_index)->kind;
	result = 1;
	do {
		int		flow_index;

		switch (scopekind) {
			case ScopeKind (unit): result = parse_unit_scope_flow (unit, scope_index, ptokens, &flow_index); break ;
			case ScopeKind (func):
			case ScopeKind (code): result = parse_code_scope_flow (unit, scope_index, ptokens, &flow_index); break ;
			case ScopeKind (stroke):
			case ScopeKind (union):
			case ScopeKind (struct): result = parse_struct_scope_flow (unit, scope_index, ptokens, &flow_index); break ;
			default: Assert (0);
			// default: Unreachable ();
		}
		if (result == 1) {
			add_flow_to_scope (unit, scope_index, flow_index);
		}
	} while (result == 1);
	return (!!result);
}

void	print_scope (struct unit *unit, int scope_index, int indent, FILE *file);

void	print_scope_flow (struct unit *unit, int flow_index, int indent, enum scopekind scopekind, FILE *file) {
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			struct decl	*decl;

			decl = get_decl (unit, flow->decl.index);
			switch (decl->kind) {
				case DeclKind (var): {
					if (scopekind == ScopeKind (code)) {
						fprintf (file, "%*.svar %s ", indent * 4, "", decl->name);
					} else {
						fprintf (file, "%*.s%s ", indent * 4, "", decl->name);
					}
					print_type (unit, decl->type, file);
					fprintf (file, ";\n");
				} break ;
				case DeclKind (const): {
					fprintf (file, "%*.sconst %s = ", indent * 4, "", decl->name);
					Assert (decl->body >= 0);
					print_expr (unit, decl->body, file);
					fprintf (file, ";\n");
				} break ;
				case DeclKind (func): {
					Assert (scopekind == ScopeKind (unit));
					fprintf (file, "%*.s%s ", indent * 4, "", decl->name);
					print_type (unit, decl->type, file);
					fprintf (file, " {\n");
					Assert (decl->body >= 0);
					print_scope (unit, decl->body, indent + 1, file);
					fprintf (file, "%*.s}\n", indent * 4, "");
				} break ;
				case DeclKind (union):
				case DeclKind (stroke):
				case DeclKind (struct): {
					Assert (scopekind == ScopeKind (unit));
					if (decl->kind == DeclKind (struct)) {
						fprintf (file, "%*.sstruct %s {\n", indent * 4, "", decl->name);
					} else if (decl->kind == DeclKind (union)) {
						fprintf (file, "%*.sunion %s {\n", indent * 4, "", decl->name);
					} else if (decl->kind == DeclKind (stroke)) {
						fprintf (file, "%*.sstroke %s {\n", indent * 4, "", decl->name);
					} else {
						Assert (0);
					}
					Assert (decl->body >= 0);
					print_scope (unit, decl->body, indent + 1, file);
					fprintf (file, "%*.s}\n", indent * 4, "");
				} break ;
				case DeclKind (bitfield):
				case DeclKind (enum): {
					Assert (scopekind == ScopeKind (unit));
					if (decl->kind == DeclKind (enum)) {
						fprintf (file, "%*.senum %s {\n", indent * 4, "", decl->name);
					} else if (decl->kind == DeclKind (bitfield)) {
						fprintf (file, "%*.sbitfield %s {\n", indent * 4, "", decl->name);
					} else {
						Assert (0);
					}
					Assert (decl->body >= 0);
					print_scope (unit, decl->body, indent + 1, file);
					fprintf (file, "%*.s}\n", indent * 4, "");
				} break ;
				default: {
					Error ("unknown decl kind %d", decl->kind);
					Assert (0);
				} break ;
			}
		} break ;
		case FlowType (expr): {
			fprintf (file, "%*.s", indent * 4, "");
			if (flow->expr.index >= 0) {
				print_expr (unit, flow->expr.index, file);
			}
			fprintf (file, ";\n");
		} break ;
		case FlowType (block): {
			fprintf (file, "%*.s{\n", indent * 4, "");
			print_scope (unit, flow->block.scope, indent + 1, file);
			fprintf (file, "%*.s}\n", indent * 4, "");
		} break ;
		case FlowType (if): {
			fprintf (file, "%*.sif ", indent * 4, "");
			print_expr (unit, flow->fif.expr, file);
			fprintf (file, "\n");
			print_scope_flow (unit, flow->fif.flow_body, indent + 1, scopekind, file);
			if (flow->fif.else_body >= 0) {
				fprintf (file, "%*.selse\n", indent * 4, "");
				print_scope_flow (unit, flow->fif.else_body, indent + 1, scopekind, file);
			}
		} break ;
		case FlowType (while): {
			fprintf (file, "%*.swhile ", indent * 4, "");
			print_expr (unit, flow->fwhile.expr, file);
			fprintf (file, "\n");
			print_scope_flow (unit, flow->fwhile.flow_body, indent + 1, scopekind, file);
		} break ;
		case FlowType (dowhile): {
			fprintf (file, "%*.sdo\n", indent * 4, "");
			print_scope_flow (unit, flow->dowhile.flow_body, indent + 1, scopekind, file);
			fprintf (file, "%*.swhile ", indent * 4, "");
			print_expr (unit, flow->dowhile.expr, file);
			fprintf (file, ";\n");
		} break ;
		case FlowType (declblock): {
			if (flow->declblock.name) {
				fprintf (file, "%*.s%s {\n", indent * 4, "", flow->declblock.name);
			} else {
				fprintf (file, "%*.s{\n", indent * 4, "");
			}
			print_scope (unit, flow->declblock.scope, indent + 1, file);
			fprintf (file, "%*.s}\n", indent * 4, "");
		} break ;
		default: {
			Error ("unknown flow type %d", flow->type);
		} break ;
	}
}

void	print_scope (struct unit *unit, int scope_index, int indent, FILE *file) {
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		while (flow) {
			print_scope_flow (unit, get_flow_index (unit, flow), indent, scope->kind, file);
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		}
	}
}

int		check_scope_declarations_for_name_uniqueness (struct unit *unit, int scope_index) {
	int				result;
	struct scope	*scope;
	struct decl		*decl;
	int				names_count = 0;
	const char		*names[2 * 1024];

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		int		index;

		decl = get_decl (unit, scope->decl_begin);
		do {
			Assert (names_count < Array_Count (names));
			names[names_count] = decl->name;
			names_count += 1;
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (decl);
		if (scope->param_scope >= 0) {
			scope = get_scope (unit, scope->param_scope);
			if (scope->decl_begin >= 0) {
				decl = get_decl (unit, scope->decl_begin);
				do {
					Assert (names_count < Array_Count (names));
					names[names_count] = decl->name;
					names_count += 1;
					decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
				} while (decl);
			}
		}
		qsort ((char *) names, names_count, sizeof *names, qs_compare_strings);
		index = 0;
		while (index < names_count - 1 && 0 != strcmp (names[index], names[index + 1])) {
			index += 1;
		}
		if (index >= names_count - 1) {
			result = 1;
		} else {
			Error ("redefinition of '%s'", names[index]);
			result = 0;
		}
	} else {
		result = 1;
	}
	return (result);
}


