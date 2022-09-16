


void	init_scope (struct scope *scope) {
	scope->tagtype = TagType (invalid);
	scope->parent_scope = 0;
	scope->param_scope = 0;
	scope->decl_begin = 0;
	scope->decl_last = 0;
	scope->flow_begin = 0;
	scope->flow_last = 0;
}

struct scope	*get_scope (struct unit *unit, uint scope_index) {
	struct scope	*scope;

	Assert (Is_Bucket_Index_Valid (unit->scopes, scope_index));
	scope = Get_Bucket_Element (unit->scopes, scope_index);
	return (scope);
}

int		get_scope_index (struct unit *unit, struct scope *scope) {
	return (Get_Bucket_Element_Index (unit->scopes, scope));
}

uint	make_scope (struct unit *unit, enum scopekind kind, uint parent) {
	uint	index;

	if (Prepare_Bucket (unit->scopes, 1)) {
		struct scope	*scope;

		scope = Push_Bucket (unit->scopes);
		init_scope (scope);
		scope->kind = kind;
		scope->tagtype = TagType (invalid);
		scope->parent_scope = parent;
		scope->param_scope = 0;
		scope->type_index = 0;
		index = Get_Bucket_Element_Index (unit->scopes, scope);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_tag_scope (struct unit *unit, enum tagtype tagtype, uint parent) {
	uint	index;

	index = make_scope (unit, ScopeKind (tag), parent);
	if (index) {
		get_scope (unit, index)->tagtype = tagtype;
	}
	return (index);
}

struct flow	*get_flow (struct unit *unit, uint index) {
	struct flow	*flow;

	Assert (Is_Bucket_Index_Valid (unit->flows, index));
	flow = Get_Bucket_Element (unit->flows, index);
	return (flow);
}

uint	get_flow_index (struct unit *unit, struct flow *flow) {
	return (Get_Bucket_Element_Index (unit->flows, flow));
}

uint	make_flow (struct unit *unit, enum flowtype type, int line) {
	uint	index;

	if (Prepare_Bucket (unit->flows, 1)) {
		struct flow		*flow;

		flow = Push_Bucket (unit->flows);
		flow->next = 0;
		flow->line = line;
		flow->type = type;
		index = Get_Bucket_Element_Index (unit->flows, flow);
	} else {
		Error ("cannot prepare flow array");
		index = 0;
	}
	return (index);
}

uint	make_block_flow (struct unit *unit, uint inner_scope, int line) {
	uint	index;

	index = make_flow (unit, FlowType (block), line);
	if (index) {
		get_flow (unit, index)->block.scope = inner_scope;
	}
	return (index);
}

uint	make_expr_flow (struct unit *unit, uint expr, int line) {
	uint	index;

	index = make_flow (unit, FlowType (expr), line);
	if (index) {
		get_flow (unit, index)->expr.index = expr;
	}
	return (index);
}

uint	make_if_flow (struct unit *unit, uint expr, uint body, uint else_body, int line) {
	uint	index;

	index = make_flow (unit, FlowType (if), line);
	if (index) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->fif.expr = expr;
		flow->fif.flow_body = body;
		flow->fif.else_body = else_body;
	}
	return (index);
}

uint	make_constif_flow (struct unit *unit, uint expr, uint body, uint else_body, int line) {
	uint		index;

	index = make_flow (unit, FlowType (constif), line);
	if (index) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->fif.expr = expr;
		flow->fif.flow_body = body;
		flow->fif.else_body = else_body;
	}
	return (index);
}

uint	make_while_flow (struct unit *unit, uint expr, uint body, int line) {
	uint	index;

	index = make_flow (unit, FlowType (while), line);
	if (index) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->fwhile.expr = expr;
		flow->fwhile.flow_body = body;
	}
	return (index);
}

uint	make_dowhile_flow (struct unit *unit, uint expr, uint body, int line) {
	uint	index;

	index = make_flow (unit, FlowType (dowhile), line);
	if (index) {
		struct flow *flow;

		flow = get_flow (unit, index);
		flow->dowhile.expr = expr;
		flow->dowhile.flow_body = body;
	}
	return (index);
}

uint	make_decl_flow (struct unit *unit, uint decl_index, int line) {
	uint	index;

	index = make_flow (unit, FlowType (decl), line);
	if (index) {
		get_flow (unit, index)->decl.index = decl_index;
	}
	return (index);
}

uint	make_init_flow (struct unit *unit, enum inittype inittype, uint body, int line) {
	uint	index;

	index = make_flow (unit, FlowType (init), line);
	if (index) {
		struct flow	*flow;

		flow = get_flow (unit, index);
		flow->init.type = inittype;
		flow->init.body = body;
	}
	return (index);
}

struct decl	*get_decl (struct unit *unit, uint index) {
	struct decl	*decl;

	Assert (Is_Bucket_Index_Valid (unit->decls, index));
	decl = Get_Bucket_Element (unit->decls, index);
	return (decl);
}

uint	get_decl_index (struct unit *unit, struct decl *decl) {
	return (Get_Bucket_Element_Index (unit->decls, decl));
}

uint64	get_decl_gindex (struct unit *unit, struct unit *decl_unit, struct decl *decl) {
	uint64	gindex;

	if (unit != decl_unit) {
		gindex = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), get_decl_index (decl_unit, decl));
	} else {
		gindex = get_decl_index (unit, decl);
	}
	return (gindex);
}

uint	make_decl (struct unit *unit, uint scope_index, const char *name, uint type, enum declkind kind, int line) {
	uint	index;

	if (Prepare_Bucket (unit->decls, 1)) {
		struct scope	*scope;
		struct decl		*decl;

		decl = Push_Bucket (unit->decls);
		if (scope_index) {
			scope = get_scope (unit, scope_index);
			if (scope->decl_last) {
				scope->decl_last = get_decl (unit, scope->decl_last)->next = Get_Bucket_Element_Index (unit->decls, decl);
			} else {
				scope->decl_last = scope->decl_begin = Get_Bucket_Element_Index (unit->decls, decl);
			}
			decl->is_global = scope->kind == ScopeKind (unit);
		}
		decl->next = 0;
		decl->line = line;
		decl->name = name;
		decl->type = type;
		decl->kind = kind;
		decl->filename = unit->filename;
		index = Get_Bucket_Element_Index (unit->decls, decl);
	} else {
		Error ("cannot prepare decl array");
		index = 0;
	}
	return (index);
}

uint	make_var_decl (struct unit *unit, uint scope_index, const char *name, uint type, int line) {
	uint	index;

	Assert (type);
	index = make_decl (unit, scope_index, name, type, DeclKind (var), line);
	get_decl (unit, index)->var.init_scope = 0;
	return (index);
}

uint	make_alias_decl (struct unit *unit, uint scope_index, const char *name, uint expr_index, int line) {
	uint	index;

	Assert (expr_index);
	index = make_decl (unit, scope_index, name, 0, DeclKind (alias), line);
	get_decl (unit, index)->alias.expr = expr_index;
	return (index);
}

uint	make_const_decl (struct unit *unit, uint scope_index, const char *name, uint type, uint expr, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type, DeclKind (const), line);
	if (index) {
		get_decl (unit, index)->dconst.expr = expr;
	}
	return (index);
}

uint	make_func_decl (struct unit *unit, uint scope_index, const char *name, uint type, uint scope, uint param_scope, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type, DeclKind (func), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->func.scope = scope;
		decl->func.param_scope = param_scope;
	}
	return (index);
}

uint	make_define_macro_decl (struct unit *unit, uint scope_index, const char *name, uint scope, uint param_scope, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (macro);
		decl->define.macro.scope = scope;
		decl->define.macro.param_scope = param_scope;
	}
	return (index);
}

uint	make_tag_decl (struct unit *unit, uint scope_index, const char *name, uint type, enum tagtype tagtype, uint scope, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type, DeclKind (tag), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->tag.type = tagtype;
		decl->tag.scope = scope;
		decl->tag.member_table = 0;
	}
	return (index);
}

uint	make_block_decl (struct unit *unit, uint scope_index, const char *name, uint scope, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, 0, DeclKind (block), line);
	if (index) {
		get_decl (unit, index)->block.scope = scope;
	}
	return (index);
}

uint	make_enum_decl (struct unit *unit, uint scope_index, const char *name, uint expr, uint params, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, 0, DeclKind (enum), line);
	if (index) {
		get_decl (unit, index)->enumt.expr = expr;
		get_decl (unit, index)->enumt.params = params;
	}
	return (index);
}

uint	make_define_accessor_decl (struct unit *unit, uint scope_index, const char *name, enum tagtype tagtype, const char *tagname, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (accessor);
		decl->define.accessor.tagtype = tagtype;
		decl->define.accessor.name = tagname;
		decl->define.accessor.decl = 0;
	}
	return (index);
}

uint	make_param_decl (struct unit *unit, uint scope_index, const char *name, uint type_index, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type_index, DeclKind (param), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->param.expr = 0;
	}
	return (index);
}

uint	make_define_external_decl (struct unit *unit, uint scope_index, const char *name, uint type_index, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type_index, DeclKind (define), line);
	if (index) {
		get_decl (unit, index)->define.kind = DefineKind (external);
	}
	return (index);
}

uint	make_define_type_decl (struct unit *unit, uint scope_index, const char *name, uint type_index, int line) {
	uint	index;

	index = make_decl (unit, scope_index, name, type_index, DeclKind (define), line);
	if (index) {
		get_decl (unit, index)->define.kind = DefineKind (type);
	}
	return (index);
}

uint	make_define_visability_decl (struct unit *unit, uint scope_index, enum visability visability, const char *target, int line) {
	uint	index;

	index = make_decl (unit, scope_index, 0, 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (visability);
		decl->define.visability.type = visability;
		decl->define.visability.target = target;
	}
	return (index);
}

uint	make_define_funcprefix_decl (struct unit *unit, uint scope_index, const char *prefix, const char *target, int line) {
	uint	index;

	index = make_decl (unit, scope_index, 0, 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (funcprefix);
		decl->define.funcprefix.prefix = prefix;
		decl->define.funcprefix.target = target;
	}
	return (index);
}

uint	make_define_builtin_decl (struct unit *unit, uint scope_index, enum builtin builtin, int line) {
	uint	index;

	index = make_decl (unit, scope_index, g_builtin[builtin], 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (builtin);
		decl->define.builtin.type = builtin;
	}
	return (index);
}

uint	make_define_assert_decl (struct unit *unit, uint scope_index, uint expr, int line) {
	uint	index;

	index = make_decl (unit, scope_index, 0, 0, DeclKind (define), line);
	if (index) {
		struct decl	*decl;

		decl = get_decl (unit, index);
		decl->define.kind = DefineKind (assert);
		decl->define.assert.expr = expr;
	}
	return (index);
}

void	add_flow_to_scope (struct unit *unit, uint scope_index, uint flow_index) {
	struct scope	*scope;

	Assert (flow_index);
	scope = get_scope (unit, scope_index);
	if (scope->flow_begin) {
		scope->flow_last = get_flow (unit, scope->flow_last)->next = flow_index;
	} else {
		scope->flow_last = scope->flow_begin = flow_index;
	}
}

void	add_decl_to_scope (struct unit *unit, uint scope_index, uint decl_index) {
	struct scope	*scope;

	Assert (decl_index);
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		scope->decl_last = get_decl (unit, scope->decl_last)->next = decl_index;
	} else {
		scope->decl_last = scope->decl_begin = decl_index;
	}
}

int		parse_code_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*out = 0;
	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "if")) {
			uint	expr, body;

			*out = make_if_flow (unit, 0, 0, 0, unit->pos.line);
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr)) {
				if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
					uint	else_body;

					else_body = 0;
					if (is_token (*ptokens, Token (identifier), "else")) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (parse_code_scope_flow (unit, scope_index, ptokens, &else_body)) {
							result = 1;
						} else {
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
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "constif")) {
			uint	expr, body;

			*out = make_constif_flow (unit, 0, 0, 0, unit->pos.line);
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr)) {
				if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
					uint	else_body;

					else_body = 0;
					if (is_token (*ptokens, Token (identifier), "else")) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (parse_code_scope_flow (unit, scope_index, ptokens, &else_body)) {
							result = 1;
						} else {
							result = 0;
						}
					} else {
						result = 1;
					}
					if (result) {
						struct flow	*flow;

						flow = get_flow (unit, *out);
						flow->constif.expr = expr;
						flow->constif.flow_body = body;
						flow->constif.else_body = else_body;
						result = 1;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "while")) {
			uint	expr, body;

			*out = make_while_flow (unit, 0, 0, unit->pos.line);
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr)) {
				if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
					struct flow	*fwhile;

					fwhile = get_flow (unit, *out);
					fwhile->fwhile.expr = expr;
					fwhile->fwhile.flow_body = body;
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "do")) {
			uint	expr, body;

			*out = make_dowhile_flow (unit, 0, 0, unit->pos.line);
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_code_scope_flow (unit, scope_index, ptokens, &body)) {
				if (is_token (*ptokens, Token (identifier), "while")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					if (parse_expr (unit, ptokens, &expr)) {
						if (is_token (*ptokens, Token (punctuator), ";")) {
							struct flow	*dowhile;

							*ptokens = next_token (*ptokens, &unit->pos);
							dowhile = get_flow (unit, *out);
							dowhile->dowhile.expr = expr;
							dowhile->dowhile.flow_body = body;
							result = 1;
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "var")) {
			const char	*name;
			uint		type_index;
			int			line;

			line = unit->pos.line;
			*ptokens = next_token (*ptokens, &unit->pos);
			if ((*ptokens)[-1] == Token (identifier)) {
				name = *ptokens;
				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_type (unit, ptokens, &type_index)) {
					if (type_index) {
						if (is_token (*ptokens, Token (punctuator), ";")) {
							struct type	*type;

							type = get_type (unit, type_index);
							if (!(type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function))) {
								int		decl;

								*ptokens = next_token (*ptokens, &unit->pos);
								decl = make_var_decl (unit, scope_index, name, type_index, line);
								if (decl) {
									*out = make_decl_flow (unit, decl, line);
									Assert (*out);
									result = 1;
								} else {
									Parse_Error (*ptokens, unit->pos, "cannot create decl");
									result = 0;
								}
							} else {
								Parse_Error (*ptokens, unit->pos, "function declaration is forbidden in the code scope");
								result = 0;
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "empty type");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "alias")) {
			const char	*name;
			uint		expr_index;
			int			line;

			line = unit->pos.line;
			*ptokens = next_token (*ptokens, &unit->pos);
			if ((*ptokens)[-1] == Token (identifier)) {
				name = *ptokens;
				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_expr (unit, ptokens, &expr_index)) {
					if (is_token (*ptokens, Token (punctuator), ";")) {
						if (expr_index) {
							uint	decl_index;

							*ptokens = next_token (*ptokens, &unit->pos);
							decl_index = make_alias_decl (unit, scope_index, name, expr_index, line);
							*out = make_decl_flow (unit, decl_index, line);
							result = 1;
						} else {
							Parse_Error (*ptokens, unit->pos, "empty alias");
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
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			uint	expr;
			int		line;

			line = unit->pos.line;
			if (parse_expr (unit, ptokens, &expr)) {
				if (is_token (*ptokens, Token (punctuator), ";")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					*out = make_expr_flow (unit, expr, line);
					Assert (*out);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token [%s]; expecting ';'", *ptokens);
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "cannot parse expr");
				result = 0;
			}
		}
	} else if (is_token (*ptokens, Token (punctuator), "{")) {
		uint	inner_scope;

		inner_scope = make_scope (unit, ScopeKind (code), scope_index);
		*out = make_block_flow (unit, inner_scope, unit->pos.line);
		*ptokens = next_token (*ptokens, &unit->pos);
		if (parse_scope (unit, inner_scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "cannot parse code block");
			result = 0;
		}
	} else if (is_token (*ptokens, Token (punctuator), ";")) {
		*ptokens = next_token (*ptokens, &unit->pos);
		*out = make_expr_flow (unit, 0, unit->pos.line);
		Assert (*out);
		result = 1;
	} else {
		uint	expr;
		int		line;

		line = unit->pos.line;
		if (parse_expr (unit, ptokens, &expr)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				*ptokens = next_token (*ptokens, &unit->pos);
				*out = make_expr_flow (unit, expr, line);
				Assert (*out);
				result = 1;
			} else if (!expr) {
				result = 2;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "cannot parse expr");
			result = 0;
		}
	}
	return (result);
}

int		parse_tag_decl_flow (struct unit *unit, enum tagtype tagtype, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;
		uint		param_scope;
		int			line;

		line = unit->pos.line;
		param_scope = 0;
		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if (tagtype == TagType (enum) && is_token (*ptokens, Token (punctuator), "(")) {
			result = parse_function_param_scope (unit, ptokens, &param_scope, 0);
		} else {
			result = 1;
		}
		if (result && is_token (*ptokens, Token (punctuator), "{")) {
			uint	scope;

			*ptokens = next_token (*ptokens, &unit->pos);
			scope = make_tag_scope (unit, tagtype, scope_index);
			if (parse_scope (unit, scope, ptokens)) {
				if (is_token (*ptokens, Token (punctuator), "}")) {
					uint	decl;
					uint	type_index;

					type_index = make_tag_type (unit, name, tagtype);
					decl = make_tag_decl (unit, scope_index, name, type_index, tagtype, scope, line);
					get_decl (unit, decl)->tag.param_scope = param_scope;
					get_scope (unit, scope)->type_index = type_index;
					get_scope (unit, scope)->param_scope = param_scope;
					get_type (unit, type_index)->tag.decl = decl;
					*out = make_decl_flow (unit, decl, line);
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token '%s'", *ptokens);
					result = 0;
				}
			} else {
				result = 0;
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

int		parse_accessor_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;
	int		line;

	Assert (is_token (*ptokens, Token (identifier), "accessor"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char		*name, *tagname;
		uint			decl_index, tag_decl;
		enum tagtype	tagtype;

		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if ((*ptokens)[-1] == Token (identifier) && is_tagtype (*ptokens, &tagtype)) {
			*ptokens = next_token (*ptokens, &unit->pos);
			if ((*ptokens)[-1] == Token (identifier)) {
				tagname = *ptokens;
				*ptokens = next_token (*ptokens, &unit->pos);
				if (is_token (*ptokens, Token (punctuator), ";")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					decl_index = make_define_accessor_decl (unit, scope_index, name, tagtype, tagname, line);
					*out = make_decl_flow (unit, decl_index, line);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
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

int		parse_const_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;
	int		line;

	Assert (is_token (*ptokens, Token (identifier), "const"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;

		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if (is_token (*ptokens, Token (punctuator), "=")) {
			uint	expr;

			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr)) {
				if (is_token (*ptokens, Token (punctuator), ";")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					if (expr) {
						uint	decl;

						decl = make_const_decl (unit, scope_index, name, 0, expr, line);
						*out = make_decl_flow (unit, decl, line);
						result = 1;
					} else {
						Parse_Error (*ptokens, unit->pos, "empty expression");
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
			Parse_Error (*ptokens, unit->pos, "unexpected token");
			result = 0;
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_external_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;
	int		line;

	Assert (is_token (*ptokens, Token (identifier), "external"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		enum tagtype	tagtype;

		if (is_tagtype (*ptokens, &tagtype)) {
			if (parse_tag_decl_flow (unit, tagtype, scope_index, ptokens, out)) {
				get_decl (unit, get_flow (unit, *out)->decl.index)->tag.is_external = 1;
				result = 1;
			} else {
				result = 0;
			}
		} else {
			const char	*name;
			uint		type_index;

			name = *ptokens;
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_type (unit, ptokens, &type_index)) {
				if (is_token (*ptokens, Token (punctuator), ";")) {
					uint	decl;

					decl = make_define_external_decl (unit, scope_index, name, type_index, line);
					*out = make_decl_flow (unit, decl, line);
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				}
			} else {
				result = 0;
			}
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_type_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int			result;
	const char	*name;
	int			line;

	Assert (is_token (*ptokens, Token (identifier), "type"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		uint	type_index;

		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if (parse_type (unit, ptokens, &type_index)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				uint	decl;

				decl = make_define_type_decl (unit, scope_index, name, type_index, line);
				*out = make_decl_flow (unit, decl, line);
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_visability_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int			result;
	const char	*name;
	int			line;

	Assert (is_token (*ptokens, Token (identifier), "visability"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		uint			type_index;
		const char		*target;

		target = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if ((*ptokens)[-1] == Token (identifier)) {
			enum visability	visability;

			if (is_token (*ptokens, Token (identifier), "public")) {
				visability = Visability (public);
				result = 1;
			} else if (is_token (*ptokens, Token (identifier), "private")) {
				visability = Visability (private);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
			if (result) {
				*ptokens = next_token (*ptokens, &unit->pos);
				if (is_token (*ptokens, Token (punctuator), ";")) {
					uint	decl;

					decl = make_define_visability_decl (unit, scope_index, visability, target, line);
					*out = make_decl_flow (unit, decl, line);
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
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

int		parse_funcprefix_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int			result;
	const char	*name;
	int			line;

	Assert (is_token (*ptokens, Token (identifier), "funcprefix"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*target;

		target = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if ((*ptokens)[-1] == Token (string)) {
			const char	*prefix;

			prefix = *ptokens;
			*ptokens = next_token (*ptokens, &unit->pos);
			if (is_token (*ptokens, Token (punctuator), ";")) {
				uint	decl;

				decl = make_define_funcprefix_decl (unit, scope_index, prefix, target, line);
				*out = make_decl_flow (unit, decl, line);
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
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

int		parse_builtin_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int			result;
	const char	*name;
	int			line;

	Assert (is_token (*ptokens, Token (identifier), "builtin"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if ((*ptokens)[-1] == Token (identifier)) {
		int		index;

		index = 0;
		while (index < Array_Count (g_builtin) && 0 != strcmp (*ptokens, g_builtin[index])) {
			index += 1;
		}
		if (index < Array_Count (g_builtin)) {
			*ptokens = next_token (*ptokens, &unit->pos);
			if (is_token (*ptokens, Token (punctuator), ";")) {
				uint	decl;

				decl = make_define_builtin_decl (unit, scope_index, index, line);
				*out = make_decl_flow (unit, decl, line);
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unrecognized token");
				result = 0;
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "unrecognized builtin name");
			result = 0;
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_assert_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int			result;
	const char	*name;
	int			line;
	uint		expr;

	Assert (is_token (*ptokens, Token (identifier), "assert"));
	line = unit->pos.line;
	*ptokens = next_token (*ptokens, &unit->pos);
	if (parse_expr (unit, ptokens, &expr)) {
		if (is_token (*ptokens, Token (punctuator), ";")) {
			uint	decl;

			decl = make_define_assert_decl (unit, scope_index, expr, line);
			*out = make_decl_flow (unit, decl, line);
			*ptokens = next_token (*ptokens, &unit->pos);
			result = 1;
		} else {
			Parse_Error (*ptokens, unit->pos, "unrecognized token");
			result = 0;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		parse_manifest_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*out = 0;
	*ptokens = next_token (*ptokens, &unit->pos);
	if (is_token (*ptokens, Token (punctuator), "{")) {
		*ptokens = next_token (*ptokens, &unit->pos);
		do {
			if ((*ptokens)[-1] == Token (identifier)) {
				if (0 == strcmp (*ptokens, "libs")) {
					if (!unit->manifest.libs) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							*ptokens = next_token (*ptokens, &unit->pos);
							begin = g_tokenizer.current;
							do {
								if ((*ptokens)[-1] == Token (string)) {
									char	*colon_token;

									result = copy_token (&g_tokenizer, *ptokens);
									*ptokens = next_token (*ptokens, &unit->pos);
									if (is_token (*ptokens, Token (punctuator), "{")) {
										*ptokens = next_token (*ptokens, &unit->pos);
										do {
											if ((*ptokens)[-1] == Token (identifier)) {
												if (copy_token (&g_tokenizer, *ptokens)) {
													*ptokens = next_token (*ptokens, &unit->pos);
													if ((*ptokens)[-1] == Token (preprocessing_number)) {
														if (copy_token (&g_tokenizer, *ptokens)) {
															*ptokens = next_token (*ptokens, &unit->pos);
															if (is_token (*ptokens, Token (punctuator), ";")) {
																colon_token = *ptokens;
																*ptokens = next_token (*ptokens, &unit->pos);
																result = 1;
															} else {
																Parse_Error (*ptokens, unit->pos, "unexpected token");
																result = 0;
															}
														} else {
															Parse_Error (*ptokens, unit->pos, "cannot copy token");
															result = 0;
														}
													} else {
														Parse_Error (*ptokens, unit->pos, "unexpected token");
														result = 0;
													}
												} else {
													Parse_Error (*ptokens, unit->pos, "cannot copy token");
													result = 0;
												}
											} else {
												Parse_Error (*ptokens, unit->pos, "unexpected token");
												result = 0;
											}
										} while (result && !is_token (*ptokens, Token (punctuator), "}"));
										if (result) {
											result = copy_token (&g_tokenizer, colon_token);
											*ptokens = next_token (*ptokens, &unit->pos);
										}
									} else if (is_token (*ptokens, Token (punctuator), ";")) {
										result = copy_token (&g_tokenizer, *ptokens);
										*ptokens = next_token (*ptokens, &unit->pos);
									} else {
										Parse_Error (*ptokens, unit->pos, "unexpected token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.libs = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest libs");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "sources")) {
					if (!unit->manifest.sources) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.sources = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest sources");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "options")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					if (is_token (*ptokens, Token (punctuator), "{")) {
						char	*begin;

						begin = g_tokenizer.current;
						if (unit->manifest.options) {
							const char	*options;

							options = unit->manifest.options;
							do {
								result = copy_token (&g_tokenizer, options);
								options = next_const_token (options, 0);
							} while (result && options[-1]);
						} else {
							result = 1;
						}
						if (result) {
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (identifier)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if ((*ptokens)[-1] == Token (preprocessing_number)) {
											if (copy_token (&g_tokenizer, *ptokens)) {
												*ptokens = next_token (*ptokens, &unit->pos);
												if (is_token (*ptokens, Token (punctuator), ";")) {
													*ptokens = next_token (*ptokens, &unit->pos);
													result = 1;
												} else {
													Parse_Error (*ptokens, unit->pos, "unexpected token");
													result = 0;
												}
											} else {
												Parse_Error (*ptokens, unit->pos, "cannot copy token");
												result = 0;
											}
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.options = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "expose")) {
					*ptokens = next_token (*ptokens, &unit->pos);
					if (!unit->manifest.is_expose_all && !unit->manifest.expose) {
						if (is_token (*ptokens, Token (identifier), "all")) {
							*ptokens = next_token (*ptokens, &unit->pos);
							if (is_token (*ptokens, Token (punctuator), ";")) {
								*ptokens = next_token (*ptokens, &unit->pos);
								unit->manifest.is_expose_all = 1;
								result = 1;
							} else {
								Parse_Error (*ptokens, unit->pos, "unexpected token");
								result = 0;
							}
						} else if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (identifier)) {
									if (is_tagtype (*ptokens, 0)) {
										if (copy_token (&g_tokenizer, *ptokens)) {
											*ptokens = next_token (*ptokens, &unit->pos);
											if ((*ptokens)[-1] == Token (identifier)) {
												result = 1;
											} else {
												Parse_Error (*ptokens, unit->pos, "unexpected token");
												result = 0;
											}
										} else {
											Parse_Error (*ptokens, unit->pos, "cannot copy token");
											result = 0;
										}
									} else {
										result = 1;
									}
									if (result) {
										if (copy_token (&g_tokenizer, *ptokens)) {
											*ptokens = next_token (*ptokens, &unit->pos);
											if (is_token (*ptokens, Token (punctuator), ";")) {
												*ptokens = next_token (*ptokens, &unit->pos);
												result = 1;
											} else {
												Parse_Error (*ptokens, unit->pos, "unexpected token");
												result = 0;
											}
										} else {
											Parse_Error (*ptokens, unit->pos, "cannot copy token");
											result = 0;
										}
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.expose = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefiniton of manifest expose");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_include_paths")) {
					if (!unit->manifest.cc_include_paths) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_include_paths = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_includes")) {
					if (!unit->manifest.cc_includes) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_includes = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_libpaths")) {
					if (!unit->manifest.cc_libpaths) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_libpaths = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_libs")) {
					if (!unit->manifest.cc_libs) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_libs = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_flags")) {
					if (!unit->manifest.cc_flags) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_flags = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else if (0 == strcmp (*ptokens, "cc_linker_flags")) {
					if (!unit->manifest.cc_linker_flags) {
						*ptokens = next_token (*ptokens, &unit->pos);
						if (is_token (*ptokens, Token (punctuator), "{")) {
							char	*begin;

							begin = g_tokenizer.current;
							*ptokens = next_token (*ptokens, &unit->pos);
							do {
								if ((*ptokens)[-1] == Token (string)) {
									if (copy_token (&g_tokenizer, *ptokens)) {
										*ptokens = next_token (*ptokens, &unit->pos);
										if (is_token (*ptokens, Token (punctuator), ";")) {
											*ptokens = next_token (*ptokens, &unit->pos);
											result = 1;
										} else {
											Parse_Error (*ptokens, unit->pos, "unexpected token");
											result = 0;
										}
									} else {
										Parse_Error (*ptokens, unit->pos, "cannot copy token");
										result = 0;
									}
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token");
									result = 0;
								}
							} while (result && !is_token (*ptokens, Token (punctuator), "}"));
							if (result) {
								end_tokenizer (&g_tokenizer, 0);
								unit->manifest.cc_linker_flags = get_next_from_tokenizer (&g_tokenizer, begin);
								*ptokens = next_token (*ptokens, &unit->pos);
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "redefinition of manifest cc_include_paths");
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "unknown manifest entry '%s'", *ptokens);
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} while (result && !is_token (*ptokens, Token (punctuator), "}"));
		if (result) {
			Assert (is_token (*ptokens, Token (punctuator), "}"));
			*ptokens = next_token (*ptokens, &unit->pos);
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		skip_manifest (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*ptokens = next_token (*ptokens, &unit->pos);
	if (is_token (*ptokens, Token (punctuator), "{")) {
		int		count;

		count = 0;
		do {
			if ((*ptokens)[-1] == Token (punctuator)) {
				count += 0 == strcmp (*ptokens, "{");
				count -= 0 == strcmp (*ptokens, "}");
			}
			*ptokens = next_token (*ptokens, &unit->pos);
		} while ((*ptokens)[-1] && count > 0);
		if (count <= 0) {
			result = 1;
		} else {
			Parse_Error (*ptokens, unit->pos, "unexpected end");
			result = 0;
		}
	} else {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_macro_decl_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out);

int		parse_unit_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*out = 0;
	if ((*ptokens)[-1] == Token (identifier)) {
		enum tagtype	tagtype;

		if (is_tagtype (*ptokens, &tagtype)) {
			result = parse_tag_decl_flow (unit, tagtype, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "const")) {
			result = parse_const_decl_flow (unit, scope_index, ptokens, out);
		} else {
			const char	*name;
			uint		type_index;
			int			line;

			line = unit->pos.line;
			name = *ptokens;
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_type (unit, ptokens, &type_index)) {
				if (type_index) {
					struct type	*type;

					type = get_type (unit, type_index);
					if (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function)) {
						if (is_token (*ptokens, Token (punctuator), "{")) {
							uint	decl;
							uint	func_scope;

							decl = make_func_decl (unit, scope_index, name, type_index, 0, type->mod.param_scope, line);
							*out = make_decl_flow (unit, decl, line);
							*ptokens = next_token (*ptokens, &unit->pos);
							func_scope = make_scope (unit, ScopeKind (func), scope_index);
							get_scope (unit, func_scope)->param_scope = type->mod.param_scope;
							get_scope (unit, func_scope)->type_index = type_index;
							if (parse_scope (unit, func_scope, ptokens)) {
								if (is_token (*ptokens, Token (punctuator), "}")) {
									*ptokens = next_token (*ptokens, &unit->pos);
									get_decl (unit, decl)->func.scope = func_scope;
									result = 1;
								} else {
									Parse_Error (*ptokens, unit->pos, "unexpected token '%s' at %d", *ptokens, get_current_line ());
									result = 0;
								}
							} else {
								result = 0;
							}
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else if (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (array) && is_token (*ptokens, Token (punctuator), "{")) {
						uint		decl;
						uint		init_scope;

						*ptokens = next_token (*ptokens, &unit->pos);
						decl = make_var_decl (unit, scope_index, name, type_index, line);
						*out = make_decl_flow (unit, decl, line);
						init_scope = make_scope (unit, ScopeKind (init), scope_index);
						get_scope (unit, init_scope)->type_index = type->mod.forward;
						if (parse_scope (unit, init_scope, ptokens)) {
							if (is_token (*ptokens, Token (punctuator), "}")) {
								*ptokens = next_token (*ptokens, &unit->pos);
								get_decl (unit, decl)->var.init_scope = init_scope;
								result = 1;
							} else {
								Parse_Error (*ptokens, unit->pos, "unexpected token");
								result = 0;
							}
						} else {
							result = 0;
						}
					} else if (is_token (*ptokens, Token (punctuator), ";")) {
						uint		decl;

						*ptokens = next_token (*ptokens, &unit->pos);
						decl = make_var_decl (unit, scope_index, name, type_index, line);
						*out = make_decl_flow (unit, decl, line);
						result = 1;
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "empty type");
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "cannot parse type");
				result = 0;
			}
		}
	} else if (is_token (*ptokens, Token (punctuator), "#")) {
		*ptokens = next_token (*ptokens, &unit->pos);
		if (0 == strcmp (*ptokens, "accessor")) {
			result = parse_accessor_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "external")) {
			result = parse_external_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "macro")) {
			result = parse_macro_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "type")) {
			result = parse_type_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "funcprefix")) {
			result = parse_funcprefix_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "builtin")) {
			result = parse_builtin_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "assert")) {
			result = parse_assert_decl_flow (unit, scope_index, ptokens, out);
		} else if (0 == strcmp (*ptokens, "manifest")) {
			if (unit->flags[Flag (entry)]) {
				result = parse_manifest_decl_flow (unit, scope_index, ptokens, out);
			} else {
				result = skip_manifest (unit, scope_index, ptokens, out);
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "unexpected token");
			result = 0;
		}
	} else {
		result = 2;
	}
	return (result);
}

int		parse_struct_tag_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*out = 0;
	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "assert")) {
			Parse_Error (*ptokens, unit->pos, "reserved hint word");
			result = 0;
		} else if (0 == strcmp (*ptokens, "assume")) {
			Parse_Error (*ptokens, unit->pos, "reserved hint word");
			result = 0;
		} else {
			const char	*name;
			int		line;

			line = unit->pos.line;
			name = *ptokens;
			*ptokens = next_token (*ptokens, &unit->pos);
			if (is_token (*ptokens, Token (punctuator), "{")) {
				uint			scope;
				enum tagtype	tagtype;

				if (0 == strcmp (name, "struct")) {
					tagtype = TagType (struct);
					name = 0;
					result = 1;
				} else if (0 == strcmp (name, "union")) {
					tagtype = TagType (union);
					name = 0;
					result = 1;
				} else {
					tagtype = TagType (struct);
					result = 1;
				}
				if (result) {
					*ptokens = next_token (*ptokens, &unit->pos);
					scope = make_tag_scope (unit, tagtype, scope_index);
					if (parse_scope (unit, scope, ptokens)) {
						if (is_token (*ptokens, Token (punctuator), "}")) {
							uint	decl;

							*ptokens = next_token (*ptokens, &unit->pos);
							decl = make_block_decl (unit, scope_index, name, scope, line);
							*out = make_decl_flow (unit, decl, line);
							result = 1;
						} else {
							Parse_Error (*ptokens, unit->pos, "unexpected token");
							result = 0;
						}
					} else {
						Parse_Error (*ptokens, unit->pos, "cannot parse %s scope", g_tagname [tagtype]);
						result = 0;
					}
				}
			} else {
				uint	type_index;

				if (parse_type (unit, ptokens, &type_index)) {
					if (is_token (*ptokens, Token (punctuator), ";")) {
						uint	decl;

						*ptokens = next_token (*ptokens, &unit->pos);
						decl = make_var_decl (unit, scope_index, name, type_index, line);
						*out = make_decl_flow (unit, decl, line);
						result = 1;
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "cannot parse type");
					result = 0;
				}
			}
		}
	} else {
		result = 2;
	}
	return (result);
}

int		parse_enum_tag_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	*out = 0;
	if ((*ptokens)[-1] == Token (identifier)) {
		const char	*name;
		uint		expr;
		int			is_params;
		uint		params;
		int			line;

		line = unit->pos.line;
		is_params = 0;
		params = 0;
		name = *ptokens;
		*ptokens = next_token (*ptokens, &unit->pos);
		if (is_token (*ptokens, Token (punctuator), "=") && !get_scope (unit, scope_index)->param_scope) {
			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr)) {
				if (expr) {
					if (is_token (*ptokens, Token (punctuator), ";")) {
						*ptokens = next_token (*ptokens, &unit->pos);
						result = 1;
					} else {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					}
				} else {
					Parse_Error (*ptokens, unit->pos, "empty expression");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (is_token (*ptokens, Token (punctuator), ",")) {
			is_params = 1;
			expr = 0;
			result = 1;
		} else if (is_token (*ptokens, Token (punctuator), ";")) {
			*ptokens = next_token (*ptokens, &unit->pos);
			expr = 0;
			result = 1;
		} else {
			Parse_Error (*ptokens, unit->pos, "unexpected token");
			result = 0;
		}
		if (result && is_params) {
			uint	prev_params;

			prev_params = 0;
			while (result && (*ptokens)[-1] && !is_token (*ptokens, Token (punctuator), ";")) {
				if (is_token (*ptokens, Token (punctuator), ",")) {
					uint	param_index;

					param_index = 0;
					*ptokens = next_token (*ptokens, &unit->pos);
					if (parse_expr (unit, ptokens, &param_index)) {
						if (param_index) {
							if (prev_params) {
								prev_params = get_expr (unit, prev_params)->funcparam.next = make_expr_funcparam (unit, param_index, 0);
							} else {
								prev_params = params = make_expr_funcparam (unit, param_index, 0);
							}
							result = 1;
						} else {
							Parse_Error (*ptokens, unit->pos, "empty enum parameter");
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
			if (result) {
				Assert (is_token (*ptokens, Token (punctuator), ";"));
				*ptokens = next_token (*ptokens, &unit->pos);
			}
		}
		if (result) {
			uint	decl;

			decl = make_enum_decl (unit, scope_index, name, expr, params, line);
			*out = make_decl_flow (unit, decl, line);
		}
	} else {
		result = 2;
	}
	return (result);
}

int		parse_init_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out) {
	int		result;

	if (is_token (*ptokens, Token (punctuator), "{")) {
		uint	init_scope;
		int		line;

		line = unit->pos.line;
		*ptokens = next_token (*ptokens, &unit->pos);
		init_scope = make_scope (unit, ScopeKind (init), scope_index);
		if (parse_scope (unit, init_scope, ptokens)) {
			if (is_token (*ptokens, Token (punctuator), "}")) {
				*ptokens = next_token (*ptokens, &unit->pos);
				*out = make_init_flow (unit, InitType (list), init_scope, line);
				result = 1;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (is_token (*ptokens, Token (punctuator), "}")) {
		result = 2;
	} else {
		uint	expr;
		int		line;

		line = unit->pos.line;
		if (parse_expr (unit, ptokens, &expr)) {
			if (is_token (*ptokens, Token (punctuator), ";")) {
				if (expr) {
					*ptokens = next_token (*ptokens, &unit->pos);
					*out = make_init_flow (unit, InitType (expr), expr, line);
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "empty expr");
					result = 0;
				}
			} else {
				Parse_Error (*ptokens, unit->pos, "unrecognized token");
				result = 0;
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "could not parse expr");
			result = 0;
		}
	}
	return (result);
}

int		parse_macro_scope_flow (struct unit *unit, uint scope_index, char **ptokens, uint *out);

int		parse_scope (struct unit *unit, uint scope_index, char **ptokens) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->kind == ScopeKind (func)) {
		struct scope	*scope;
		struct type		*type;

		scope = get_scope (unit, scope_index);
		type = get_type (unit, scope->type_index);
		Assert (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function));
		type = get_type (unit, type->mod.forward);
		if (!(type->kind == TypeKind (basic) && type->basic.type == BasicType (void))) {
			uint	type_index;
			uint	decl_index;
			uint	flow_index;

			type_index = make_type_copy (unit, unit, get_type_index (unit, type));
			decl_index = make_var_decl (unit, scope_index, "result", type_index, unit->pos.line);
			flow_index = make_decl_flow (unit, decl_index, unit->pos.line);
			add_flow_to_scope (unit, scope_index, flow_index);
		}
	}
	result = 1;
	do {
		uint	flow_index;

		flow_index = 0;
		if (scope->kind == ScopeKind (unit)) {
			result = parse_unit_scope_flow (unit, scope_index, ptokens, &flow_index);
		} else if (scope->kind == ScopeKind (func) || scope->kind == ScopeKind (code)) {
			result = parse_code_scope_flow (unit, scope_index, ptokens, &flow_index);
		} else if (scope->kind == ScopeKind (tag)) {
			if (scope->tagtype == TagType (struct) || scope->tagtype == TagType (union)) {
				result = parse_struct_tag_scope_flow (unit, scope_index, ptokens, &flow_index);
			} else if (scope->tagtype == TagType (enum)) {
				result = parse_enum_tag_scope_flow (unit, scope_index, ptokens, &flow_index);
			} else {
				Unreachable ();
			}
		} else if (scope->kind == ScopeKind (macro)) {
			result = parse_macro_scope_flow (unit, scope_index, ptokens, &flow_index);
		} else if (scope->kind == ScopeKind (init)) {
			result = parse_init_scope_flow (unit, scope_index, ptokens, &flow_index);
		} else {
			Unreachable ();
		}
		if (result == 1) {
			if (flow_index) {
				add_flow_to_scope (unit, scope_index, flow_index);
			}
		}
	} while (result == 1);
	return (!!result);
}

void	print_scope (struct unit *unit, uint scope_index, int indent, FILE *file);

void	print_scope_flow (struct unit *unit, uint flow_index, int indent, enum scopekind scopekind, FILE *file) {
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		struct decl	*decl;

		decl = get_decl (unit, flow->decl.index);
		if (decl->kind == DeclKind (var)) {
			if (scopekind == ScopeKind (code) || scopekind == ScopeKind (func)) {
				fprintf (file, "%*.svar %s ", indent * 4, "", decl->name);
			} else {
				fprintf (file, "%*.s%s ", indent * 4, "", decl->name);
			}
			print_type (unit, decl->type, file);
			fprintf (file, ";\n");
		} else if (decl->kind == DeclKind (const)) {
			fprintf (file, "%*.sconst %s = ", indent * 4, "", decl->name);
			Assert (decl->dconst.expr);
			print_expr (unit, decl->dconst.expr, file);
			fprintf (file, ";\n");
		} else if (decl->kind == DeclKind (func)) {
			Assert (scopekind == ScopeKind (unit));
			fprintf (file, "%*.s%s ", indent * 4, "", decl->name);
			print_type (unit, decl->type, file);
			fprintf (file, " {\n");
			Assert (decl->func.scope);
			print_scope (unit, decl->func.scope, indent + 1, file);
			fprintf (file, "%*.s}\n", indent * 4, "");
		} else if (decl->kind == DeclKind (tag)) {
			Assert (scopekind == ScopeKind (unit));
			if (decl->tag.type == TagType (struct)) {
				fprintf (file, "%*.sstruct %s {\n", indent * 4, "", decl->name);
			} else if (decl->tag.type == TagType (union)) {
				fprintf (file, "%*.sunion %s {\n", indent * 4, "", decl->name);
			} else if (decl->tag.type == TagType (enum)) {
				fprintf (file, "%*.senum %s {\n", indent * 4, "", decl->name);
			} else {
				Unreachable ();
			}
			Assert (decl->tag.scope);
			print_scope (unit, decl->tag.scope, indent + 1, file);
			fprintf (file, "%*.s}\n", indent * 4, "");
		} else if (decl->kind == DeclKind (block)) {
			fprintf (file, "%*.s%s {\n", indent * 4, "", g_tagname [get_scope (unit, decl->block.scope)->tagtype]);
			print_scope (unit, decl->block.scope, indent + 1, file);
			fprintf (file, "%*.s}\n", indent * 4, "");
		} else if (decl->kind == DeclKind (enum)) {
			if (decl->enumt.expr) {
				fprintf (file, "%*.s%s = ", indent * 4, "", decl->name);
				print_expr (unit, decl->enumt.expr, file);
				fprintf (file, ";\n");
			} else {
				fprintf (file, "%*.s%s;\n", indent * 4, "", decl->name);
			}
		} else if (decl->kind == DeclKind (define)) {
			if (decl->define.kind == DefineKind (accessor)) {
				fprintf (file, "%*.s#accessor %s %s %s;\n", indent * 4, "", decl->name, g_tagname[decl->define.accessor.tagtype], decl->define.accessor.name);
			} else if (decl->define.kind == DefineKind (macro)) {
				struct scope	*scope;

				fprintf (file, "%*.s#macro %s (", indent * 4, "", decl->name);
				scope = get_scope (unit, decl->define.macro.param_scope);
				if (scope->decl_begin) {
					struct decl		*param;

					param = get_decl (unit, scope->decl_begin);
					do {
						if (param->next) {
							fprintf (file, "%s, ", param->name);
							param = get_decl (unit, param->next);
						} else {
							fprintf (file, "%s", param->name);
							param = 0;
						}
					} while (param);
				}
				fprintf (file, ") {\n");
				print_scope (unit, decl->define.macro.scope, indent + 1, file);
				fprintf (file, "%.*s}\n", indent * 4, "");
			} else if (decl->define.kind == DefineKind (external)) {
				fprintf (file, "%*.s#external %s ", indent * 4, "", decl->name);
				print_type (unit, decl->type, file);
				fprintf (file, ";\n");
			} else if (decl->define.kind == DefineKind (type)) {
				fprintf (file, "%*.s#type %s ", indent * 4, "", decl->name);
				print_type (unit, decl->type, file);
				fprintf (file, ";\n");
			} else if (decl->define.kind == DefineKind (visability)) {
				fprintf (file, "%*.s#visability %s %s;\n", indent * 4, "", decl->define.visability.target, g_visability[decl->define.visability.type]);
			} else if (decl->define.kind == DefineKind (funcprefix)) {
				fprintf (file, "%*.s#funcprefix %s \"%s\";\n", indent * 4, "", decl->define.funcprefix.target, decl->define.funcprefix.prefix);
			} else if (decl->define.kind == DefineKind (builtin)) {
				fprintf (file, "%*.s#builtin %s;\n", indent * 4, "", g_builtin[decl->define.builtin.type]);
			} else {
				Unreachable ();
			}
		} else if (decl->kind == DeclKind (alias)) {
			fprintf (file, "%*.salias %s ", indent * 4, "", decl->name);
			print_expr (unit, decl->alias.expr, file);
			fprintf (file, ";\n");
		} else {
			Unreachable ();
		}
	} else if (flow->type == FlowType (expr)) {
		fprintf (file, "%*.s", indent * 4, "");
		if (flow->expr.index) {
			print_expr (unit, flow->expr.index, file);
		}
		fprintf (file, ";\n");
	} else if (flow->type == FlowType (block)) {
		fprintf (file, "%*.s{\n", indent * 4, "");
		print_scope (unit, flow->block.scope, indent + 1, file);
		fprintf (file, "%*.s}\n", indent * 4, "");
	} else if (flow->type == FlowType (if)) {
		fprintf (file, "%*.sif ", indent * 4, "");
		print_expr (unit, flow->fif.expr, file);
		fprintf (file, "\n");
		print_scope_flow (unit, flow->fif.flow_body, indent + 1, scopekind, file);
		if (flow->fif.else_body) {
			fprintf (file, "%*.selse\n", indent * 4, "");
			print_scope_flow (unit, flow->fif.else_body, indent + 1, scopekind, file);
		}
	} else if (flow->type == FlowType (while)) {
		fprintf (file, "%*.swhile ", indent * 4, "");
		print_expr (unit, flow->fwhile.expr, file);
		fprintf (file, "\n");
		print_scope_flow (unit, flow->fwhile.flow_body, indent + 1, scopekind, file);
	} else if (flow->type == FlowType (dowhile)) {
		fprintf (file, "%*.sdo\n", indent * 4, "");
		print_scope_flow (unit, flow->dowhile.flow_body, indent + 1, scopekind, file);
		fprintf (file, "%*.swhile ", indent * 4, "");
		print_expr (unit, flow->dowhile.expr, file);
		fprintf (file, ";\n");
	} else {
		Unreachable ();
	}
}

void	print_scope (struct unit *unit, uint scope_index, int indent, FILE *file) {
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		while (flow) {
			print_scope_flow (unit, get_flow_index (unit, flow), indent, scope->kind, file);
			flow = flow->next ? get_flow (unit, flow->next) : 0;
		}
	}
}

int		qs_compare_strings (const void *l, const void *r) {
	return (strcmp (*(const char **) l, *(const char **) r));
}

int		check_scope_tag_declarations_for_name_uniqueness (struct unit *unit, uint scope_index, enum tagtype tagtype) {
	int				result;
	struct scope	*scope;
	struct decl		*decl;
	int				names_count = 0;
	const char		*names[2 * 1024];

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		int		index;

		decl = get_decl (unit, scope->decl_begin);
		do {
			Assert (names_count < Array_Count (names));
			if (decl->kind == DeclKind (tag) && decl->tag.type == tagtype) {
				names[names_count] = decl->name;
				names_count += 1;
			}
			decl = decl->next ? get_decl (unit, decl->next) : 0;
		} while (decl);
		qsort ((char *) names, names_count, sizeof *names, qs_compare_strings);
		index = 0;
		while (index < names_count - 1 && 0 != strcmp (names[index], names[index + 1])) {
			index += 1;
		}
		if (index >= names_count - 1) {
			result = 1;
		} else {
			Error ("redefinition of %s tag '%s'", g_tagname[tagtype], names[index]);
			result = 0;
		}
	} else {
		result = 1;
	}
	return (result);
}

int		is_ordinal_decl (struct decl *decl) {
	return !(decl->kind == DeclKind (tag) || (decl->kind == DeclKind (define) && !g_definekind_ordinal[decl->define.kind]));
}

void	collect_scope_decl_names (struct unit *unit, uint scope_index, const char **pnames, int *pcount) {
	struct scope	*scope;
	uint			decl_index;

	scope = get_scope (unit, scope_index);
	decl_index = scope->decl_begin;
	while (decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (is_ordinal_decl (decl)) {
			if (decl->name) {
				pnames[*pcount] = decl->name;
				*pcount += 1;
			} else {
				Debug ("decl without name: %s", g_declkind[decl->kind]);
				Assert (decl->kind == DeclKind (block));
				collect_scope_decl_names (unit, decl->block.scope, pnames, pcount);
			}
		}
		decl_index = decl->next;
	}
}

int		check_scope_ordinary_declarations_for_name_uniqueness (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;
	struct decl		*decl;
	int				names_count = 0;
	const char		*names[2 * 1024];

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		int		index;

		collect_scope_decl_names (unit, scope_index, names, &names_count);
		if (scope->param_scope) {
			collect_scope_decl_names (unit, scope->param_scope, names, &names_count);
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

int		check_scope_declarations_for_name_uniqueness (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->kind == ScopeKind (unit)) {
		result = check_scope_tag_declarations_for_name_uniqueness (unit, scope_index, TagType (struct)) &&
				check_scope_tag_declarations_for_name_uniqueness (unit, scope_index, TagType (union)) &&
				check_scope_tag_declarations_for_name_uniqueness (unit, scope_index, TagType (enum)) &&
				check_scope_ordinary_declarations_for_name_uniqueness (unit, scope_index);
	} else {
		result = check_scope_ordinary_declarations_for_name_uniqueness (unit, scope_index);
	}
	return (result);
}

int		count_decls_in_scope (struct unit *unit, uint scope_index) {
	struct scope	*scope;
	int				count;
	uint			decl_index;

	scope = get_scope (unit, scope_index);
	count = 0;
	decl_index = scope->decl_begin;
	while (decl_index) {
		count += 1;
		decl_index = get_decl (unit, decl_index)->next;
	}
	return (count);
}



