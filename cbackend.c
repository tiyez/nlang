

const char	*get_tag_name (int unit_index, enum tagtype tagtype, const char *tagname) {
	static char	buffer[128];

	snprintf (buffer, sizeof buffer, "%s u%d%c_%s", g_tagname[tagtype], unit_index, g_tagname[tagtype][0], tagname);
	return (buffer);
}

const char	*get_enum_name (int unit_index, const char *tagname, const char *name) {
	static char	buffer[128];

	snprintf (buffer, sizeof buffer, "u%dec_%s__%s", unit_index, tagname, name);
	return (buffer);
}

const char	*get_enum_table_name (int unit_index, const char *enum_name, const char *table_name) {
	static char	buffer[128];

	snprintf (buffer, sizeof buffer, "u%det_%s__%s", unit_index, enum_name, table_name);
	return (buffer);
}

const char	*get_global_decl_name (int unit_index, const char *name) {
	static char	buffer[128];

	snprintf (buffer, sizeof buffer, "u%d_%s", unit_index, name);
	return (buffer);
}

const char	*get_local_decl_name (const char *name) {
	static char	buffer[128];

	snprintf (buffer, sizeof buffer, "_%s", name);
	return (buffer);
}

int		cbackend_param_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer);

void	cbackend_typemod (struct unit *unit, struct type **types, int index, const char *iden, const char *prefix, struct cbuffer *buffer) {
	if (index < 0) {
		if (iden) {
			if (prefix) {
				write_string (buffer->wr, prefix);
				write_string (buffer->wr, " ");
			}
			write_string (buffer->wr, iden);
		}
	} else {
		struct type	*type;

		type = types[index];
		Assert (type->kind == TypeKind (mod));
		if (type->mod.kind == TypeMod (pointer)) {
			write_string (buffer->wr, "*");
			if (type->flags.is_const) {
				write_string (buffer->wr, "const ");
			}
			cbackend_typemod (unit, types, index - 1, iden, prefix, buffer);
		} else {
			if (index > 0 && types[index - 1]->mod.kind == TypeMod (pointer)) {
				write_string (buffer->wr, "(");
				cbackend_typemod (unit, types, index - 1, iden, prefix, buffer);
				write_string (buffer->wr, ")");
			} else {
				cbackend_typemod (unit, types, index - 1, iden, prefix, buffer);
			}
			if (type->mod.kind == TypeMod (array)) {
				if (type->mod.count) {
					write_format (buffer->wr, "[%llu]", type->mod.count);
				} else {
					write_string (buffer->wr, "[]");
				}
			} else if (type->mod.kind == TypeMod (function)) {
				if (type->mod.param_scope) {
					struct unit	*unit_decl;

					if (is_lib_index (type->mod.param_scope)) {
						unit_decl = get_lib (get_lib_index (type->mod.param_scope));
					} else {
						unit_decl = unit;
					}
					write_string (buffer->wr, " (");
					cbackend_param_scope (unit_decl, unlib_index (type->mod.param_scope), buffer);
					write_string (buffer->wr, ")");
				} else {
					write_string (buffer->wr, " (void)");
				}
			} else Unreachable ();
		}
	}
}

int		cbackend_type (struct unit *unit, uint type_index, const char *iden, const char *prefix, struct cbuffer *buffer) {
	struct type	*types[Max_Type_Depth], *type;
	int			types_count;

	types_count = 0;
	type = get_type (unit, type_index);
	while (type->kind == TypeKind (mod)) {
		Assert (types_count < Array_Count (types));
		types[types_count] = type;
		types_count += 1;
		type = get_type (unit, type->mod.forward);
	}
	Assert (types_count < Array_Count (types));
	types[types_count] = type;
	types_count += 1;
	type = types[types_count - 1];
	if (type->kind == TypeKind (basic)) {
		if (type->flags.is_const) {
			write_string (buffer->wr, "const ");
		}
		if (type->flags.is_volatile) {
			write_string (buffer->wr, "volatile ");
		}
		write_string (buffer->wr, g_basictype_c_name[type->basic.type]);
		write_string (buffer->wr, " ");
	} else if (type->kind == TypeKind (tag)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (type->flags.is_const) {
			write_string (buffer->wr, "const ");
		}
		if (type->flags.is_volatile) {
			write_string (buffer->wr, "volatile ");
		}
		Assert (type->tag.decl);
		if (is_lib_index (type->tag.decl)) {
			decl_unit = get_lib (get_lib_index (type->tag.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (type->tag.decl));
		if (decl->kind == DeclKind (tag)) {
			if (decl->tag.is_external) {
				write_format (buffer->wr, "%s %s ", g_tagname[decl->tag.type], decl->name);
			} else {
				write_string (buffer->wr, get_tag_name (decl_unit->flags[Flag (unit_index)], decl->tag.type, decl->name));
				write_string (buffer->wr, " ");
			}
		} else {
			Unreachable ();
		}
	} else if (type->kind == TypeKind (opaque)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (type->flags.is_const) {
			write_string (buffer->wr, "const ");
		}
		if (type->flags.is_volatile) {
			write_string (buffer->wr, "volatile ");
		}
		Assert (type->opaque.decl);
		if (is_lib_index (type->opaque.decl)) {
			decl_unit = get_lib (get_lib_index (type->opaque.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (type->opaque.decl));
		write_string (buffer->wr, decl->name);
	} else {
		Unreachable ();
	}
	types_count -= 1;
	cbackend_typemod (unit, types, types_count - 1, iden, prefix, buffer);
	return (1);
}

int		cbackend_typename (struct unit *unit, uint type_index, struct cbuffer *buffer) {
	return (cbackend_type (unit, type_index, 0, 0, buffer));
}

int		cbackend_expr (struct unit *unit, uint expr_index, struct cbuffer *buffer) {
	int			result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (group)) {
			write_string (buffer->wr, "(");
			if (cbackend_expr (unit, expr->op.forward, buffer)) {
				result = 1;
				write_string (buffer->wr, ")");
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (function_call)) {
			if (cbackend_expr (unit, expr->op.forward, buffer)) {
				if (expr->op.backward) {
					write_string (buffer->wr, " (");
					if (cbackend_expr (unit, expr->op.backward, buffer)) {
						write_string (buffer->wr, ")");
						result = 1;
					} else {
						result = 0;
					}
				} else {
					write_string (buffer->wr, " ()");
					result = 1;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (array_subscript)) {
			if (cbackend_expr (unit, expr->op.forward, buffer)) {
				if (expr->op.backward) {
					write_string (buffer->wr, "[");
					if (cbackend_expr (unit, expr->op.backward, buffer)) {
						write_string (buffer->wr, "]");
						result = 1;
					} else {
						result = 0;
					}
				} else {
					write_string (buffer->wr, "[]");
					result = 1;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (cast)) {
			write_string (buffer->wr, "(");
			if (cbackend_typename (unit, expr->op.backward, buffer)) {
				write_string (buffer->wr, ") ");
				result = cbackend_expr (unit, expr->op.forward, buffer);
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (member_access)) {
			if (cbackend_expr (unit, expr->op.backward, buffer)) {
				write_string (buffer->wr, g_opentries[expr->op.type].token);
				result = cbackend_expr (unit, expr->op.forward, buffer);
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
			write_string (buffer->wr, g_opentries[expr->op.type].token);
			write_string (buffer->wr, " ");
			result = cbackend_expr (unit, expr->op.forward, buffer);
		} else {
			if (is_expr_unary (expr)) {
				if (is_expr_postfix (expr)) {
					result = cbackend_expr (unit, expr->op.forward, buffer);
					write_string (buffer->wr, g_opentries[expr->op.type].token);
				} else {
					write_string (buffer->wr, g_opentries[expr->op.type].token);
					result = cbackend_expr (unit, expr->op.forward, buffer);
				}
			} else {
				if (cbackend_expr (unit, expr->op.backward, buffer)) {
					if (is_expr_postfix (expr)) {
						write_string (buffer->wr, g_opentries[expr->op.type].token);
					} else {
						write_format (buffer->wr, " %s ", g_opentries[expr->op.type].token);
					}
					result = cbackend_expr (unit, expr->op.forward, buffer);
				} else {
					result = 0;
				}
			}
		}
	} else if (expr->type == ExprType (constant)) {
		if (is_basictype_integral (expr->constant.type)) {
			if (is_basictype_signed (expr->constant.type)) {
				if (write_format (buffer->wr, "%zd", expr->constant.value)) {
					if (is_basictype_with_l_suffix (expr->constant.type)) {
						result = write_string (buffer->wr, "l");
					} else if (is_basictype_with_ll_suffix (expr->constant.type)) {
						result = write_string (buffer->wr, "ll");
					} else {
						result = 1;
					}
				}
			} else {
				if (write_format (buffer->wr, "%zu", expr->constant.uvalue)) {
					if (is_basictype_with_l_suffix (expr->constant.type)) {
						result = write_string (buffer->wr, "ul");
					} else if (is_basictype_with_ll_suffix (expr->constant.type)) {
						result = write_string (buffer->wr, "ull");
					} else {
						result = 1;
					}
				} else {
					result = 0;
				}
			}
		} else {
			if (write_format (buffer->wr, "%f", expr->constant.fvalue)) {
				if (expr->constant.type == BasicType (float)) {
					result = write_string (buffer->wr, "f");
				} else if (expr->constant.type == BasicType (longdouble)) {
					result = write_string (buffer->wr, "l");
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		}
	} else if (expr->type == ExprType (identifier)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		Assert (expr->iden.decl);
		if (is_lib_index (expr->iden.decl)) {
			decl_unit = Get_Bucket_Element (g_libs, get_lib_index (expr->iden.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (expr->iden.decl));
		if (decl->is_global) {
			if (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (external)) {
				result = write_string (buffer->wr, decl->name);
			} else {
				result = write_string (buffer->wr, get_global_decl_name (decl_unit->flags[Flag (unit_index)], decl->name));
			}
		} else {
			result = write_string (buffer->wr, get_local_decl_name (decl->name));
		}
	} else if (expr->type == ExprType (funcparam)) {
		if (cbackend_expr (unit, expr->funcparam.expr, buffer)) {
			if (expr->funcparam.next) {
				write_string (buffer->wr, ", ");
				result = cbackend_expr (unit, expr->funcparam.next, buffer);
			} else {
				result = 1;
			}
		} else {
			result = 0;
		}
	} else if (expr->type == ExprType (typeinfo)) {
		if (expr->typeinfo.index) {
			struct decl	*decl;
			struct unit	*decl_unit;
			int			ordindex;

			if (expr->typeinfo.lib_index) {
				decl_unit = get_lib (expr->typeinfo.lib_index);
			} else {
				decl_unit = unit;
			}
			ordindex = Get_Bucket_OrdIndex (decl_unit->buckets->typeinfos, expr->typeinfo.index);
			Assert (ordindex >= 0);
			write_format (buffer->wr, "(%s[%d])", get_global_decl_name (decl_unit->flags[Flag (unit_index)], "typeinfos__"), ordindex);
			result = 1;
		} else {
			Error ("no decl for typeinfo");
			result = 0;
		}
	} else if (expr->type == ExprType (string)) {
		char	unescaped[4 * 1024];
		usize	size;

		if (unescape_string (expr->string.token, unescaped, sizeof unescaped, &size)) {
			write_string (buffer->wr, "\"");
			write_string_n (buffer->wr, unescaped, size);
			write_string (buffer->wr, "\"");
			result = 1;
		} else {
			Error ("string too long");
			result = 0;
		}
	} else if (expr->type == ExprType (enum)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (expr->enumt.lib_index) {
			decl_unit = get_lib (expr->enumt.lib_index);
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, expr->enumt.enum_decl);
		result = write_string (buffer->wr, get_enum_name (decl_unit->flags[Flag (unit_index)], decl->name, get_decl (decl_unit, expr->enumt.decl)->name));
	} else if (expr->type == ExprType (table)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (expr->enumt.lib_index) {
			decl_unit = get_lib (expr->enumt.lib_index);
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, expr->enumt.enum_decl);
		if (expr->enumt.decl) {
			result = write_string (buffer->wr, get_enum_table_name (decl_unit->flags[Flag (unit_index)], decl->name, get_decl (decl_unit, expr->enumt.decl)->name));
		} else {
			result = write_string (buffer->wr, get_enum_table_name (decl_unit->flags[Flag (unit_index)], decl->name, "name"));
		}
	} else if (expr->type == ExprType (macrocall)) {
		struct decl	*decl;

		decl = get_decl (unit, expr->macrocall.instance);
		Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (macro));
		write_string (buffer->wr, "(");
		result = cbackend_macro_scope (unit, decl->define.macro.scope, buffer);
		write_string (buffer->wr, ")");
	} else {
		Debug ("expr type %s", g_exprtype[expr->type]);
		Unreachable ();
	}
	return (result);
}

int		cbackend_tag_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			decl_index;
	int				enum_offset;

	Assert (scope_index);
	scope = get_scope (unit, scope_index);
	Assert (scope->kind == ScopeKind (tag));
	decl_index = scope->decl_begin;
	enum_offset = 0;
	result = 1;
	while (result && decl_index) {
		struct decl		*decl;

		decl = get_decl (unit, decl_index);
		if (decl->kind == DeclKind (var)) {
			write_c_new_line (buffer);
			result = cbackend_type (unit, decl->type, get_local_decl_name (decl->name), 0, buffer);
			write_string (buffer->wr, ";");
		} else if (decl->kind == DeclKind (block)) {
			write_c_new_line (buffer);
			write_format (buffer->wr, "%s {", g_tagname[get_scope (unit, decl->block.scope)->tagtype]);
			write_c_indent_up (buffer);
			result = cbackend_tag_scope (unit, decl->block.scope, buffer);
			write_c_indent_down (buffer);
			write_c_new_line (buffer);
			if (decl->name) {
				write_format (buffer->wr, "} %s;", get_local_decl_name (decl->name));
			} else {
				write_string (buffer->wr, "};");
			}
		} else if (decl->kind == DeclKind (enum)) {
			struct type	*type;

			Assert (scope->tagtype == TagType (enum));
			Assert (scope->type_index);
			type = get_type (unit, scope->type_index);
			Assert (type->kind == TypeKind (tag));
			Assert (type->tag.type == TagType (enum));
			write_c_new_line (buffer);
			write_string (buffer->wr, get_enum_name (unit->flags[Flag (unit_index)], type->tag.name, decl->name));
			write_format (buffer->wr, " = %d,", enum_offset);
			enum_offset += 1;
			result = 1;
		}
		decl_index = decl->next;
	}
	return (result);
}

int		cbackend_tag_decl (struct unit *unit, uint decl_index, struct cbuffer *buffer) {
	int			result;
	struct decl *decl;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (tag));
	if (!decl->tag.is_external) {
		write_c_new_line (buffer);
		write_string (buffer->wr, get_tag_name (unit->flags[Flag (unit_index)], decl->tag.type, decl->name));
		write_string (buffer->wr, " {");
		write_c_indent_up (buffer);
		result = cbackend_tag_scope (unit, decl->tag.scope, buffer);
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "};");
	}
	return (result);
}

int		cbackend_decl (struct unit *unit, uint decl_index, struct cbuffer *buffer);

int		cbackend_code_flow (struct unit *unit, uint scope_index, uint flow_index, struct cbuffer *buffer) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		if (cbackend_decl (unit, flow->decl.index, buffer)) {
			write_string (buffer->wr, ";");
			result = 1;
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (expr)) {
		if (buffer->is_flow_stack) {
			write_string (buffer->wr, " ");
		} else {
			write_c_new_line (buffer);
		}
		if (flow->expr.index) {
			result = cbackend_expr (unit, flow->expr.index, buffer);
		}
		write_string (buffer->wr, ";");
	} else if (flow->type == FlowType (block)) {
		if (buffer->is_flow_stack) {
			write_string (buffer->wr, " {");
		} else {
			write_c_new_line (buffer);
			write_string (buffer->wr, "{");
		}
		buffer->is_flow_stack = 0;
		write_c_indent_up (buffer);
		result = cbackend_code_scope (unit, flow->block.scope, buffer);
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "}");
		result = 1;
	} else if (flow->type == FlowType (if)) {
		if (buffer->is_flow_stack) {
			write_string (buffer->wr, " if (");
		} else {
			write_c_new_line (buffer);
			write_string (buffer->wr, "if (");
		}
		if (cbackend_expr (unit, flow->fif.expr, buffer)) {
			write_string (buffer->wr, ")");
			buffer->is_flow_stack = 1;
			if (cbackend_code_flow (unit, 0, flow->fif.flow_body, buffer)) {
				if (flow->fif.else_body) {
					write_c_new_line (buffer);
					write_string (buffer->wr, "else");
					buffer->is_flow_stack = 1;
					result = cbackend_code_flow (unit, 0, flow->fif.else_body, buffer);
				} else {
					result = 1;
				}
				buffer->is_flow_stack = 0;
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (while)) {
		if (buffer->is_flow_stack) {
			write_string (buffer->wr, " while (");
		} else {
			write_c_new_line (buffer);
			write_string (buffer->wr, "while (");
		}
		if (cbackend_expr (unit, flow->fwhile.expr, buffer)) {
			write_string (buffer->wr, ")");
			buffer->is_flow_stack = 1;
			result = cbackend_code_flow (unit, 0, flow->fwhile.flow_body, buffer);
			buffer->is_flow_stack = 0;
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (dowhile)) {
		if (buffer->is_flow_stack) {
			write_string (buffer->wr, " do");
		} else {
			write_c_new_line (buffer);
			write_string (buffer->wr, "do");
		}
		buffer->is_flow_stack = 1;
		if (cbackend_code_flow (unit, 0, flow->fwhile.flow_body, buffer)) {
			write_c_new_line (buffer);
			write_string (buffer->wr, "while (");
			result = cbackend_expr (unit, flow->fwhile.expr, buffer);
			write_string (buffer->wr, ");");
			buffer->is_flow_stack = 0;
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (assert)) {
		char	unescaped_filepath[256];
		char	unescaped_string[1024];
		usize	size;

		if (unescape_string (unit->filepath, unescaped_filepath, sizeof unescaped_filepath, &size)) {
			if (unescape_string (flow->assert.string, unescaped_string, sizeof unescaped_string, &size)) {
				if (buffer->is_flow_stack) {
					write_string (buffer->wr, " ");
				} else {
					write_c_new_line (buffer);
				}
				write_string (buffer->wr, "Assert (");
				result = cbackend_expr (unit, flow->assert.expr, buffer);
				write_format (buffer->wr, ", \"%s\", \"%s\", %d);", unescaped_string, unescaped_filepath, flow->line);
			} else {
				Error ("cannot unescape string");
				result = 0;
			}
		} else {
			Error ("cannot unescape filepath");
			result = 0;
		}
	} else if (flow->type == FlowType (unreachable)) {
		char	unescaped_filepath[256];
		usize	size;

		if (unescape_string (unit->filepath, unescaped_filepath, sizeof unescaped_filepath, &size)) {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " ");
			} else {
				write_c_new_line (buffer);
			}
			write_string (buffer->wr, "Unreachable (");
			result = cbackend_expr (unit, flow->assert.expr, buffer);
			write_format (buffer->wr, ", \"%s\", %d);", unescaped_filepath, flow->line);
		} else {
			Error ("cannot unescape filepath");
			result = 0;
		}
	} else if (flow->type == FlowType (static_assert)) {
		write_string (buffer->wr, "(1)");
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		cbackend_code_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	scope = get_scope (unit, scope_index);
	flow_index = scope->flow_begin;
	result = 1;
	while (result && flow_index) {
		result = cbackend_code_flow (unit, scope_index, flow_index, buffer);
		flow_index = get_flow (unit, flow_index)->next;
	}
	if (result && scope->kind == ScopeKind (func) && is_functype_returnable (unit, scope->type_index)) {
		write_c_new_line (buffer);
		write_format (buffer->wr, "return (%s);", get_local_decl_name ("result"));
	}
	return (result);
}

int		cbackend_macro_flow (struct unit *unit, uint scope_index, uint flow_index, struct cbuffer *buffer) {
	int		result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (if)) {
		write_string (buffer->wr, "((");
		if (cbackend_expr (unit, flow->fif.expr, buffer)) {
			write_string (buffer->wr, ") ? (");
			if (cbackend_macro_flow (unit, scope_index, flow->fif.flow_body, buffer)) {
				write_string (buffer->wr, ") : (");
				if (cbackend_macro_flow (unit, scope_index, flow->fif.else_body, buffer)) {
					write_string (buffer->wr, "))");
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (block)) {
		write_string (buffer->wr, "(");
		result = cbackend_macro_scope (unit, flow->block.scope, buffer);
		write_string (buffer->wr, ")");
	} else if (flow->type == FlowType (expr)) {
		result = cbackend_expr (unit, flow->expr.index, buffer);
	} else if (flow->type == FlowType (assert)) {
		char	unescaped_filepath[256];
		char	unescaped_string[1024];
		usize	size;

		if (unescape_string (unit->filepath, unescaped_filepath, sizeof unescaped_filepath, &size)) {
			if (unescape_string (flow->assert.string, unescaped_string, sizeof unescaped_string, &size)) {
				write_string (buffer->wr, "Assert (");
				result = cbackend_expr (unit, flow->assert.expr, buffer);
				write_format (buffer->wr, ", \"%s\", \"%s\", %d)", unescaped_string, unescaped_filepath, flow->line);
			} else {
				Error ("cannot unescape string");
				result = 0;
			}
		} else {
			Error ("cannot unescape filepath");
			result = 0;
		}
	} else if (flow->type == FlowType (static_assert)) {
		write_string (buffer->wr, "(1)");
		result = 1;
	} else if (flow->type == FlowType (unreachable)) {
		char	unescaped_filepath[256];
		usize	size;

		if (unescape_string (unit->filepath, unescaped_filepath, sizeof unescaped_filepath, &size)) {
			write_string (buffer->wr, "Unreachable (");
			result = cbackend_expr (unit, flow->assert.expr, buffer);
			write_format (buffer->wr, ", \"%s\", %d)", unescaped_filepath, flow->line);
		} else {
			Error ("cannot unescape filepath");
			result = 0;
		}
	} else {
		Unreachable ();
	}
	if (result && flow->next) {
		write_string (buffer->wr, ", ");
	}
	return (result);
}

int		cbackend_macro_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	scope = get_scope (unit, scope_index);
	flow_index = scope->flow_begin;
	result = 1;
	while (result && flow_index) {
		result = cbackend_macro_flow (unit, scope_index, flow_index, buffer);
		flow_index = get_flow (unit, flow_index)->next;
	}
	return (result);
}

int		cbackend_func_prototype (struct unit *unit, uint decl_index, struct cbuffer *buffer) {
	int			result;
	struct decl	*decl;
	const char	*name;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (func));
	write_c_new_line (buffer);
	if (decl->func.visability == Visability (private)) {
		if (unit->flags[Flag (lib)] || 0 != strcmp ("main", decl->name)) {
			write_string (buffer->wr, "static ");
			name = get_global_decl_name (unit->flags[Flag (unit_index)], decl->name);
		} else {
			name = decl->name;
		}
	} else {
		name = decl->name;
	}
	if (cbackend_type (unit, decl->type, name, decl->func.prefix, buffer)) {
		write_string (buffer->wr, ";");
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		cbackend_var_prototype (struct unit *unit, uint decl_index, struct cbuffer *buffer) {
	int			result;
	struct decl	*decl;
	const char	*name;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (var));
	write_c_new_line (buffer);
	if (decl->var.visability == Visability (private)) {
		if (unit->flags[Flag (lib)] || 0 != strcmp ("main", decl->name)) {
			write_string (buffer->wr, "static ");
			name = get_global_decl_name (unit->flags[Flag (unit_index)], decl->name);
		} else {
			name = decl->name;
		}
	} else {
		name = decl->name;
	}
	if (cbackend_type (unit, decl->type, name, 0, buffer)) {
		write_string (buffer->wr, ";");
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

uint	get_param_by_index (struct unit *unit, uint params, int index) {
	int		result;

	while (params && index > 0) {
		struct expr	*expr;

		expr = get_expr (unit, params);
		Assert (expr->type == ExprType (funcparam));
		params = expr->funcparam.next;
		index -= 1;
	}
	if (params) {
		result = get_expr (unit, params)->funcparam.expr;
	} else {
		result = 0;
	}
	return (result);
}

int		cbackend_enum_table (struct unit *unit, uint enum_decl_index, int is_prototype, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			decl_index;
	struct decl		*enum_decl;
	uint			param_index;
	uint			count_decls;

	enum_decl = get_decl (unit, enum_decl_index);
	count_decls = count_decls_in_scope (unit, enum_decl->tag.scope);
	Assert (enum_decl->kind == DeclKind (tag) && enum_decl->tag.type == TagType (enum));
	if (enum_decl->tag.param_scope) {
		scope = get_scope (unit, enum_decl->tag.param_scope);
		Assert (scope->decl_begin);
		decl_index = scope->decl_begin;
		param_index = 0;
		result = 1;
		while (result && decl_index) {
			struct decl	*decl;
			uint		array_type_index;

			decl = get_decl (unit, decl_index);
			write_c_new_line (buffer);
			write_string (buffer->wr, "static ");
			array_type_index = make_array_type (unit, decl->type, count_decls);
			if (cbackend_type (unit, array_type_index, get_enum_table_name (unit->flags[Flag (unit_index)], enum_decl->name, decl->name), 0, buffer)) {
				if (is_prototype) {
					write_string (buffer->wr, ";");
					result = 1;
				} else {
					struct scope	*enum_scope;
					uint			decl_index;

					write_string (buffer->wr, " = {");
					write_c_indent_up (buffer);
					enum_scope = get_scope (unit, enum_decl->tag.scope);
					Assert (enum_scope->decl_begin);
					decl_index = enum_scope->decl_begin;
					do {
						struct decl	*decl;
						uint		expr_index;

						decl = get_decl (unit, decl_index);
						Assert (decl->kind == DeclKind (enum));
						write_c_new_line (buffer);
						expr_index = get_param_by_index (unit, decl->enumt.params, param_index);
						Assert (expr_index);
						if (cbackend_expr (unit, expr_index, buffer)) {
							write_string (buffer->wr, ",");
							result = 1;
						} else {
							result = 0;
						}
						decl_index = decl->next;
					} while (result && decl_index);
					if (result) {
						write_c_indent_down (buffer);
						write_c_new_line (buffer);
						write_string (buffer->wr, "};");
					}
				}
			} else {
				result = 0;
			}
			param_index += 1;
			decl_index = decl->next;
		}
	} else {
		result = 1;
	}
	if (result) {
		uint	array_type_index;

		write_c_new_line (buffer);
		write_string (buffer->wr, "static ");
		array_type_index = make_array_type (unit, make_pointer_type (unit, make_basic_type (unit, BasicType (char), 1), 1), count_decls);
		if (cbackend_type (unit, array_type_index, get_enum_table_name (unit->flags[Flag (unit_index)], enum_decl->name, "name"), 0, buffer)) {
			if (is_prototype) {
				write_string (buffer->wr, ";");
				result = 1;
			} else {
				struct scope	*enum_scope;
				uint			decl_index;

				write_string (buffer->wr, " = {");
				write_c_indent_up (buffer);
				enum_scope = get_scope (unit, enum_decl->tag.scope);
				Assert (enum_scope->decl_begin);
				decl_index = enum_scope->decl_begin;
				do {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					Assert (decl->kind == DeclKind (enum));
					write_c_new_line (buffer);
					write_format (buffer->wr, "\"%s\",", decl->name);
					result = 1;
					decl_index = decl->next;
				} while (result && decl_index);
				if (result) {
					write_c_indent_down (buffer);
					write_c_new_line (buffer);
					write_string (buffer->wr, "};");
				}
			}
		} else {
			result = 0;
		}
	}
	return (result);
}

int		cbackend_init_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer);

int		cbackend_init_scope_flow (struct unit *unit, uint flow_index, struct cbuffer *buffer) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	write_c_new_line (buffer);
	if (flow->init.type == InitType (struct) || flow->init.type == InitType (array)) {
		write_string (buffer->wr, "{");
		write_c_indent_up (buffer);
		result = cbackend_init_scope (unit, flow->init.body, buffer);
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "},");
	} else {
		Assert (flow->init.type == InitType (expr));
		result = cbackend_expr (unit, flow->init.body, buffer);
		write_string (buffer->wr, ",");
	}
	return (result);
}

int		cbackend_init_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	scope = get_scope (unit, scope_index);
	flow_index = scope->flow_begin;
	result = 1;
	while (result && flow_index) {
		result = cbackend_init_scope_flow (unit, flow_index, buffer);
		flow_index = get_flow (unit, flow_index)->next;
	}
	return (result);
}

int		cbackend_decl (struct unit *unit, uint decl_index, struct cbuffer *buffer) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	unit->filepath = decl->filepath;
	if (decl->kind == DeclKind (var)) {
		const char	*name;

		write_c_new_line (buffer);
		if (decl->is_global) {
			if (decl->var.visability == Visability (private)) {
				write_string (buffer->wr, "static ");
				name = get_global_decl_name (unit->flags[Flag (unit_index)], decl->name);
			} else {
				name = decl->name;
			}
		} else {
			name = get_local_decl_name (decl->name);
		}
		if (cbackend_type (unit, decl->type, name, 0, buffer)) {
			if (decl->var.init_scope) {
				write_string (buffer->wr, " = {");
				write_c_indent_up (buffer);
				result = cbackend_init_scope (unit, decl->var.init_scope, buffer);
				write_c_indent_down (buffer);
				write_c_new_line (buffer);
				write_string (buffer->wr, "}");
			} else {
				result = 1;
			}
		} else {
			result = 0;
		}
	} else if (decl->kind == DeclKind (alias)) {
		result = 1;
	} else if (decl->kind == DeclKind (func)) {
		const char	*name;

		write_c_new_line (buffer);
		if (decl->func.visability == Visability (private)) {
			if (unit->flags[Flag (lib)] || 0 != strcmp ("main", decl->name)) {
				write_string (buffer->wr, "static ");
				name = get_global_decl_name (unit->flags[Flag (unit_index)], decl->name);
			} else {
				name = decl->name;
			}
		} else {
			name = decl->name;
		}
		if (cbackend_type (unit, decl->type, name, decl->func.prefix, buffer)) {
			write_string (buffer->wr, " {");
			write_c_indent_up (buffer);
			result = cbackend_code_scope (unit, decl->func.scope, buffer);
			write_c_indent_down (buffer);
			write_c_new_line (buffer);
			write_string (buffer->wr, "}");
		} else {
			result = 0;
		}
	} else if (decl->kind == DeclKind (tag)) {
		result = cbackend_tag_decl (unit, decl_index, buffer);
	} else if (decl->kind == DeclKind (block)) {
		Todo ();
	} else if (decl->kind == DeclKind (define)) {
		if (decl->define.kind == DefineKind (external)) {
			write_c_new_line (buffer);
			write_string (buffer->wr, "extern ");
			result = cbackend_type (unit, decl->type, decl->name, 0, buffer);
			write_string (buffer->wr, ";");
		} else {
			result = 1;
		}
	} else if (decl->kind == DeclKind (const)) {
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		cbackend_typeinfo (struct unit *unit, struct cbuffer *buffer) {
	int		result;
	uint	typeinfo_index;
	uint	typemember_index;
	int		unit_index;
	int		tag_unit_index;

	tag_unit_index = get_lib (get_lib_index (g_typeinfo_struct_decl))->flags[Flag (unit_index)];
	unit_index = unit->flags[Flag (unit_index)];
	typeinfo_index = Get_Bucket_First_Index (unit->buckets->typeinfos);
	typemember_index = Get_Bucket_First_Index (unit->buckets->typemembers);
	if (typeinfo_index) {
		write_c_new_line (buffer);
		write_string (buffer->wr, "static ");
		write_string (buffer->wr, get_tag_name (tag_unit_index, TagType (struct), "typeinfo"));
		write_format (buffer->wr, " const %s[%zu];", get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_Count (unit->buckets->typeinfos));
	}
	if (typemember_index) {
		write_c_new_line (buffer);
		write_string (buffer->wr, "static ");
		write_string (buffer->wr, get_tag_name (tag_unit_index, TagType (struct), "typemember"));
		write_format (buffer->wr, " const %s[%zu] = {", get_global_decl_name (unit_index, "typemembers__"), Get_Bucket_Count (unit->buckets->typemembers));
		write_c_indent_up (buffer);
		do {
			struct typemember	*member;

			member = Get_Bucket_Element (unit->buckets->typemembers, typemember_index);
			write_c_new_line (buffer);
			if (member->name == 0 && member->typeinfo) {
				// write_format (buffer->wr, "{ 0, 0, g_typemembers + %d, 0, 0 },", member->typeinfo);
				write_format (buffer->wr, "{ 0, %s + %d, 0, 0, 0 },", get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_OrdIndex (unit->buckets->typeinfos, member->typeinfo));
			} else if (member->name == 0) {
				write_string (buffer->wr, "{ 0, 0, 0, 0, 0 },");
			} else if (member->typeinfo) {
				write_format (buffer->wr, "{ \"%s\", %s + %d, 0, %d, %d },", member->name, get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_OrdIndex (unit->buckets->typeinfos, member->typeinfo), member->value, member->offset);
			} else {
				write_format (buffer->wr, "{ \"%s\", 0, 0, %d, %d },", member->name, member->value, member->offset);
			}
			typemember_index = Get_Next_Bucket_Index (unit->buckets->typemembers, typemember_index);
		} while (typemember_index);
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "};");
	}
	if (typeinfo_index) {
		write_c_new_line (buffer);
		write_string (buffer->wr, "static ");
		write_string (buffer->wr, get_tag_name (tag_unit_index, TagType (struct), "typeinfo"));
		write_format (buffer->wr, " const %s[%zu] = {", get_global_decl_name (unit_index, "typeinfos__"),Get_Bucket_Count (unit->buckets->typeinfos));
		write_c_indent_up (buffer);
		do {
			struct typeinfo	*typeinfo;

			typeinfo = Get_Bucket_Element (unit->buckets->typeinfos, typeinfo_index);
			write_c_new_line (buffer);
			write_format (buffer->wr, "{ %zu, %d, %d, %d, ", typeinfo->size, typeinfo->count, typeinfo->qualifiers, typeinfo->kind);
			if (typeinfo->kind == TypeInfo_Kind (basic)) {
				write_format (buffer->wr, "%d, 0, 0, 0 },", typeinfo->basic);
			} else if (typeinfo->kind == TypeInfo_Kind (struct) || typeinfo->kind == TypeInfo_Kind (enum) || typeinfo->kind == TypeInfo_Kind (union)) {
				write_format (buffer->wr, "0, \"%s\", 0, %s + %d },", typeinfo->tagname, get_global_decl_name (unit_index, "typemembers__"), Get_Bucket_OrdIndex (unit->buckets->typemembers, typeinfo->members));
			} else if (typeinfo->kind == TypeInfo_Kind (pointer) || typeinfo->kind == TypeInfo_Kind (array)) {
				write_format (buffer->wr, "0, 0, %s + %d, 0 },", get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_OrdIndex (unit->buckets->typeinfos, typeinfo->typeinfo));
			} else if (typeinfo->kind == TypeInfo_Kind (function)) {
				if (typeinfo->members) {
					write_format (buffer->wr, "0, 0, %s + %d, %s + %d },", get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_OrdIndex (unit->buckets->typeinfos, typeinfo->typeinfo), get_global_decl_name (unit_index, "typemembers__"),Get_Bucket_OrdIndex (unit->buckets->typemembers, typeinfo->members));
				} else {
					write_format (buffer->wr, "0, 0, %s + %d, 0 },", get_global_decl_name (unit_index, "typeinfos__"), Get_Bucket_OrdIndex (unit->buckets->typeinfos, typeinfo->typeinfo));
				}
			} else {
				Unreachable ();
			}
			typeinfo_index = Get_Next_Bucket_Index (unit->buckets->typeinfos, typeinfo_index);
		} while (typeinfo_index);
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "};");
	}
	result = 1;
	return (result);
}

int		cbackend_tags (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			decl_index;

	scope = get_scope (unit, scope_index);
	decl_index = scope->decl_begin;
	result = 1;
	while (result && decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (decl->kind == DeclKind (tag)) {
			result = cbackend_decl (unit, decl_index, buffer);
		} else {
			result = 1;
		}
		decl_index = decl->next;
	}
	return (result);
}

int		cbackend_prototypes (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	uint			decl_index;

	decl_index = get_scope (unit, scope_index)->decl_begin;
	result = 1;
	while (result && decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (decl->kind == DeclKind (func)) {
			result = cbackend_func_prototype (unit, get_decl_index (unit, decl), buffer);
		} else if (decl->kind == DeclKind (var)) {
			result = cbackend_var_prototype (unit, get_decl_index (unit, decl), buffer);
		} else if (decl->kind == DeclKind (tag) && decl->tag.type == TagType (enum)) {
			result = cbackend_enum_table (unit, get_decl_index (unit, decl), 1, buffer);
		} else {
			result = 1;
		}
		decl_index = decl->next;
	}
	return (result);
}

int		cbackend_definitions (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	uint			decl_index;

	decl_index = get_scope (unit, scope_index)->decl_begin;
	result = 1;
	while (result && decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (decl->kind == DeclKind (func)) {
			result = cbackend_decl (unit, get_decl_index (unit, decl), buffer);
		} else if (decl->kind == DeclKind (var)) {
			result = cbackend_decl (unit, get_decl_index (unit, decl), buffer);
			write_string (buffer->wr, ";");
		} else if (decl->kind == DeclKind (tag) && decl->tag.type == TagType (enum)) {
			result = cbackend_enum_table (unit, get_decl_index (unit, decl), 0, buffer);
		} else {
			result = 1;
		}
		decl_index = decl->next;
	}
	return (result);
}

int		cbackend_unit_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;

	if (cbackend_tags (unit, scope_index, buffer)) {
		if (cbackend_prototypes (unit, scope_index, buffer)) {
			if (cbackend_typeinfo (unit, buffer)) {
				if (cbackend_definitions (unit, scope_index, buffer)) {
					result = 1;
				} else {
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
	return (result);
}

int		cbackend_param_scope (struct unit *unit, uint scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	uint			decl_index;

	scope = get_scope (unit, scope_index);
	decl_index = scope->decl_begin;
	result = 1;
	while (result && decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		Assert (decl->kind == DeclKind (param));
		if (decl->type) {
			result = cbackend_type (unit, decl->type, get_local_decl_name (decl->name), 0, buffer);
		} else if (0 == strcmp (decl->name, "...")) {
			write_string (buffer->wr, "...");
		} else {
			Error ("unexpected param decl");
			result = 0;
		}
		if (decl->next) {
			write_string (buffer->wr, ", ");
		}
		decl_index = decl->next;
	}
	return (result);
}

int		cbackend_unit (struct unit *unit, struct cbuffer *buffer) {
	int		result;

	result = cbackend_unit_scope (unit, unit->scope, buffer);
	return (result);
}

int		cbackend_include (struct unit *unit, const char *include, struct cbuffer *buffer) {
	write_c_new_line (buffer);
	if (include[0] == '@') {
		write_format (buffer->wr, "#include <%s>", include + 1);
	} else {
		write_format (buffer->wr, "#include \"..\\%s\"", include);
	}
	return (1);
}

int		cbackend_define (struct unit *unit, const char *name, struct cbuffer *buffer) {
	write_c_new_line (buffer);
	write_format (buffer->wr, "#define %s", name);
	return (1);
}


