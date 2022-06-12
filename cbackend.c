

int		c_backend_translate_tag_name (enum tagtype tagtype, const char *tagname, struct cbuffer *buffer) {
	int		result;

	result = write_format (buffer->wr, "%s %c_%s", g_tagname[tagtype], g_tagname[tagtype][0], tagname);
	return (result);
}

int		c_backend_translate_macro_name (const char *name, struct cbuffer *buffer) {
	int		result;

	result = write_format (buffer->wr, "M_%s", name);
	return (result);
}

int		c_backend_translate_enum_name (const char *tagname, const char *name, struct cbuffer *buffer) {
	int		result;

	result = write_format (buffer->wr, "e_%s_%s", tagname, name);
	return (result);
}

int		c_backend_translate_const_name (const char *name, struct cbuffer *buffer) {
	int		result;

	result = write_format (buffer->wr, "CONST_%s", name);
	return (result);
}


int		c_backend_translate_param_scope (struct unit *unit, int scope_index, struct cbuffer *buffer);

void	c_backend_translate_typemod (struct unit *unit, struct type **types, int index, const char *iden, struct cbuffer *buffer) {
	if (index < 0) {
		if (iden) {
			write_string (buffer->wr, iden);
		}
	} else {
		struct type	*type;

		type = types[index];
		if (type->kind == TypeKind (group)) {
			c_backend_translate_typemod (unit, types, index - 1, iden, buffer);
		} else {
			Assert (type->kind == TypeKind (mod));
			if (type->mod.kind == TypeMod (pointer)) {
				write_string (buffer->wr, "*");
				c_backend_translate_typemod (unit, types, index - 1, iden, buffer);
			} else {
				if (index > 0 && types[index - 1]->mod.kind == TypeMod (pointer)) {
					write_string (buffer->wr, "(");
					c_backend_translate_typemod (unit, types, index - 1, iden, buffer);
					write_string (buffer->wr, ")");
				} else {
					c_backend_translate_typemod (unit, types, index - 1, iden, buffer);
				}
				if (type->mod.kind == TypeMod (array)) {
					write_string (buffer->wr, "[");
					if (type->mod.array.expr >= 0) {
						c_backend_translate_expr (unit, type->mod.array.expr, buffer);
					}
					write_string (buffer->wr, "]");
				} else if (type->mod.kind == TypeMod (function)) {
					write_string (buffer->wr, " (");
					if (type->mod.func.param_scope >= 0) {
						c_backend_translate_param_scope (unit, type->mod.func.param_scope, buffer);
					}
					write_string (buffer->wr, ")");
				} else Unreachable ();
			}
		}
	}
}

int		c_backend_translate_type (struct unit *unit, int type_index, const char *iden, struct cbuffer *buffer) {
	struct type	*types[Max_Type_Depth], *type;
	int			types_count, index;

	types_count = 0;
	type = get_type (unit, type_index);
	while (type->kind == TypeKind (mod) || type->kind == TypeKind (group)) {
		Assert (types_count < Array_Count (types));
		types[types_count] = type;
		types_count += 1;
		if (type->kind == TypeKind (group)) {
			type = get_type (unit, type->group.type);
		} else {
			type = get_type (unit, *get_type_mod_forward_ptr (type));
		}
	}
	Assert (types_count < Array_Count (types));
	types[types_count] = type;
	types_count += 1;
	index = 0;
	type = types[types_count - 1];
	switch (type->kind) {
		case TypeKind (basic): {
			if (type->basic.is_const) {
				write_string (buffer->wr, "const ");
			}
			if (type->basic.is_volatile) {
				write_string (buffer->wr, "volatile ");
			}
			switch (type->basic.type) {
				case BasicType (void): write_string (buffer->wr, "void "); break ;
				case BasicType (char): write_string (buffer->wr, "char "); break ;
				case BasicType (uchar): write_string (buffer->wr, "unsigned char "); break ;
				case BasicType (wchar): write_string (buffer->wr, "wchar_t "); break ;
				case BasicType (byte): write_string (buffer->wr, "char "); break ;
				case BasicType (ubyte): write_string (buffer->wr, "unsigned char "); break ;
				case BasicType (short): write_string (buffer->wr, "short "); break ;
				case BasicType (ushort): write_string (buffer->wr, "unsigned short "); break ;
				case BasicType (int): write_string (buffer->wr, "int "); break ;
				case BasicType (uint): write_string (buffer->wr, "unsigned int "); break ;
				case BasicType (size): write_string (buffer->wr, "long long "); break ;
				case BasicType (usize): write_string (buffer->wr, "unsigned long long "); break ;
				case BasicType (float): write_string (buffer->wr, "float "); break ;
				case BasicType (double): write_string (buffer->wr, "double "); break ;
				default: Unreachable ();
			}
		} break ;
		case TypeKind (decl): {
			struct decl	*decl;

			if (type->decl.is_const) {
				write_string (buffer->wr, "const ");
			}
			if (type->decl.is_volatile) {
				write_string (buffer->wr, "volatile ");
			}
			decl = get_decl (unit, type->decl.index);
			switch (decl->kind) {
				case DeclKind (tag): {
					c_backend_translate_tag_name (decl->tag.type, decl->name, buffer);
					write_string (buffer->wr, " ");
				} break ;
				default: Unreachable ();
			}
		} break ;
		default: Debug ("kind: %d", type->kind); Unreachable ();
	}
	types_count -= 1;
	c_backend_translate_typemod (unit, types, types_count - 1, iden, buffer);
	return (1);
}

int		c_backend_translate_typename (struct unit *unit, int type_index, struct cbuffer *buffer) {
	return (c_backend_translate_type (unit, type_index, 0, buffer));
}

int		c_backend_translate_expr (struct unit *unit, int expr_index, struct cbuffer *buffer) {
	int			result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	switch (expr->type) {
		case ExprType (op): {
			switch (expr->op.type) {
				case OpType (group): {
					write_string (buffer->wr, "(");
					if (c_backend_translate_expr (unit, expr->op.forward, buffer)) {
						result = 1;
						write_string (buffer->wr, ")");
					} else {
						result = 0;
					}
				} break ;
				case OpType (function_call): {
					struct expr	*forward;

					forward = get_expr (unit, expr->op.forward);
					if (forward->type == ExprType (identifier) && forward->iden.decl >= 0 && get_decl (unit, forward->iden.decl)->kind == DeclKind (accessor)) {
						struct decl	*accessor, *enumer;
						struct expr	*backward;

						accessor = get_decl (unit, forward->iden.decl);
						backward = get_expr (unit, expr->op.backward);
						Assert (backward->type == ExprType (funcparam));
						Assert (backward->funcparam.next < 0);
						Assert (backward->funcparam.expr >= 0);
						backward = get_expr (unit, backward->funcparam.expr);
						Assert (backward->type == ExprType (identifier));
						enumer = get_decl (unit, backward->iden.decl);
						c_backend_translate_enum_name (accessor->accessor.name, enumer->name, buffer);
						result = 1;
					} else {
						if (c_backend_translate_expr (unit, expr->op.forward, buffer)) {
							if (expr->op.backward >= 0) {
								write_string (buffer->wr, " (");
								if (c_backend_translate_expr (unit, expr->op.backward, buffer)) {
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
					}
				} break ;
				case OpType (array_subscript): {
					if (c_backend_translate_expr (unit, expr->op.forward, buffer)) {
						if (expr->op.backward >= 0) {
							write_string (buffer->wr, "[");
							if (c_backend_translate_expr (unit, expr->op.backward, buffer)) {
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
				} break ;
				case OpType (cast): {
					write_string (buffer->wr, "(");
					if (c_backend_translate_typename (unit, expr->op.backward, buffer)) {
						write_string (buffer->wr, ") ");
						result = c_backend_translate_expr (unit, expr->op.forward, buffer);
					} else {
						result = 0;
					}
				} break ;
				default: {
					if (is_expr_unary (expr)) {
						if (is_expr_postfix (expr)) {
							result = c_backend_translate_expr (unit, expr->op.forward, buffer);
							write_string (buffer->wr, g_opentries[expr->op.type].token);
						} else {
							write_string (buffer->wr, g_opentries[expr->op.type].token);
							result = c_backend_translate_expr (unit, expr->op.forward, buffer);
						}
					} else {
						if (c_backend_translate_expr (unit, expr->op.backward, buffer)) {
							if (is_expr_postfix (expr)) {
								write_string (buffer->wr, g_opentries[expr->op.type].token);
							} else {
								write_format (buffer->wr, " %s ", g_opentries[expr->op.type].token);
							}
							result = c_backend_translate_expr (unit, expr->op.forward, buffer);
						} else {
							result = 0;
						}
					}
				} break ;
			}
		} break ;
		case ExprType (constant): {
			if (is_basictype_integral (expr->constant.type)) {
				if (is_basictype_signed (expr->constant.type)) {
					result = write_format (buffer->wr, "%zd", expr->constant.value);
				} else {
					result = write_format (buffer->wr, "%zu", expr->constant.uvalue);
				}
			} else {
				result = write_format (buffer->wr, "%f", expr->constant.fvalue);
			}
		} break ;
		case ExprType (identifier): {
			if (expr->iden.decl >= 0) {
				struct decl	*decl;

				decl = get_decl (unit, expr->iden.decl);
				if (decl->kind == DeclKind (const)) {
					result = c_backend_translate_const_name (decl->name, buffer);
				} else if (decl->kind == DeclKind (macro)) {
					result = c_backend_translate_macro_name (decl->name, buffer);
				} else {
					result = write_string (buffer->wr, decl->name);
				}
			} else {
				result = write_string (buffer->wr, expr->iden.name);
			}
		} break ;
		case ExprType (funcparam): {
			if (c_backend_translate_expr (unit, expr->funcparam.expr, buffer)) {
				if (expr->funcparam.next >= 0) {
					write_string (buffer->wr, ", ");
					result = c_backend_translate_expr (unit, expr->funcparam.next, buffer);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} break ;
		case ExprType (typeinfo): {
			if (expr->typeinfo.decl >= 0) {
				struct decl	*decl;

				decl = get_decl (unit, expr->typeinfo.decl);
				Assert (decl->kind == DeclKind (typeinfo));
				Assert (decl->type >= 0);
				write_format (buffer->wr, "(g_typeinfos[%d])", decl->type);
				result = 1;
			} else {
				Error ("no decl for typeinfo");
				result = 0;
			}
		} break ;
		case ExprType (macroparam): {
			result = write_format (buffer->wr, "(%s)", expr->macroparam.name);
		} break ;
		case ExprType (string): {
			char	unescaped[4 * 1024];
			usize	size;

			if (unescape_string (&expr->string.token, unescaped, sizeof unescaped, &size)) {
				write_string (buffer->wr, "\"");
				write_string_n (buffer->wr, unescaped, size);
				write_string (buffer->wr, "\"");
				result = 1;
			} else {
				Error ("string too long");
				result = 0;
			}
		} break ;
		default: Debug ("type %d", expr->type); Unreachable ();
	}
	return (result);
}











int		c_backend_translate_tag_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;
	struct decl		*decl;

	scope = get_scope (unit, scope_index);
	Assert (scope);
	Assert (scope->kind == ScopeKind (tag));
	if (scope->decl_begin >= 0) {
		decl = get_decl (unit, scope->decl_begin);
		Assert (decl);
		do {
			switch (decl->kind) {
				case DeclKind (var): {
					write_c_new_line (buffer);
					result = c_backend_translate_type (unit, decl->type, decl->name, buffer);
					write_string (buffer->wr, ";");
				} break ;
				case DeclKind (block): {
					Unreachable ();
				} break ;
				case DeclKind (enum): {
					struct type	*type;

					Assert (scope->tagtype == TagType (enum));
					Assert (scope->type_index >= 0);
					type = get_type (unit, scope->type_index);
					Assert (type->kind == TypeKind (tag));
					Assert (type->tag.type == TagType (enum));
					write_c_new_line (buffer);
					c_backend_translate_enum_name (type->tag.name, decl->name, buffer);
					if (decl->enumt.expr >= 0) {
						write_string (buffer->wr, " = ");
						result = c_backend_translate_expr (unit, decl->enumt.expr, buffer);
					} else {
						result = 1;
					}
					write_string (buffer->wr, ",");
				} break ;
				default: Unreachable ();
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		c_backend_translate_tag_decl (struct unit *unit, int decl_index, struct cbuffer *buffer) {
	int			result;
	struct decl *decl;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (tag));
	write_c_new_line (buffer);
	c_backend_translate_tag_name (decl->tag.type, decl->name, buffer);
	write_string (buffer->wr, " {");
	write_c_indent_up (buffer);
	result = c_backend_translate_tag_scope (unit, decl->tag.scope, buffer);
	write_c_indent_down (buffer);
	write_c_new_line (buffer);
	write_string (buffer->wr, "};");
	return (result);
}

int		c_backend_translate_decl (struct unit *unit, int decl_index, struct cbuffer *buffer);

int		c_backend_translate_code_flow (struct unit *unit, int scope_index, int flow_index, struct cbuffer *buffer) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			write_c_new_line (buffer);
			if (c_backend_translate_decl (unit, flow->decl.index, buffer)) {
				write_string (buffer->wr, ";");
				result = 1;
			} else {
				result = 0;
			}
		} break ;
		case FlowType (expr): {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " ");
			} else {
				write_c_new_line (buffer);
			}
			if (scope_index >= 0) {
				struct scope	*scope;

				scope = get_scope (unit, scope_index);
				if (scope->kind == ScopeKind (func) && flow->next < 0 && is_functype_returnable (unit, scope->type_index)) {
					write_string (buffer->wr, "return ");
				}
			}
			result = c_backend_translate_expr (unit, flow->expr.index, buffer);
			write_string (buffer->wr, ";");
		} break ;
		case FlowType (block): {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " {");
			} else {
				write_c_new_line (buffer);
				write_string (buffer->wr, "{");
			}
			buffer->is_flow_stack = 0;
			write_c_indent_up (buffer);
			result = c_backend_translate_code_scope (unit, flow->block.scope, buffer);
			write_c_indent_down (buffer);
			write_c_new_line (buffer);
			write_string (buffer->wr, "}");
			result = 1;
		} break ;
		case FlowType (if): {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " if (");
			} else {
				write_c_new_line (buffer);
				write_string (buffer->wr, "if (");
			}
			if (c_backend_translate_expr (unit, flow->fif.expr, buffer)) {
				write_string (buffer->wr, ")");
				buffer->is_flow_stack = 1;
				if (c_backend_translate_code_flow (unit, -1, flow->fif.flow_body, buffer)) {
					if (flow->fif.else_body >= 0) {
						write_c_new_line (buffer);
						write_string (buffer->wr, "else");
						buffer->is_flow_stack = 1;
						result = c_backend_translate_code_flow (unit, -1, flow->fif.else_body, buffer);
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
		} break ;
		case FlowType (while): {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " while (");
			} else {
				write_c_new_line (buffer);
				write_string (buffer->wr, "while (");
			}
			if (c_backend_translate_expr (unit, flow->fwhile.expr, buffer)) {
				write_string (buffer->wr, ")");
				buffer->is_flow_stack = 1;
				result = c_backend_translate_code_flow (unit, -1, flow->fwhile.flow_body, buffer);
				buffer->is_flow_stack = 0;
			} else {
				result = 0;
			}
		} break ;
		case FlowType (dowhile): {
			if (buffer->is_flow_stack) {
				write_string (buffer->wr, " do");
			} else {
				write_c_new_line (buffer);
				write_string (buffer->wr, "do");
			}
			buffer->is_flow_stack = 1;
			if (c_backend_translate_code_flow (unit, -1, flow->fwhile.flow_body, buffer)) {
				write_c_new_line (buffer);
				write_string (buffer->wr, "while (");
				result = c_backend_translate_expr (unit, flow->fwhile.expr, buffer);
				write_string (buffer->wr, ");");
				buffer->is_flow_stack = 0;
			} else {
				result = 0;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		c_backend_translate_code_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = c_backend_translate_code_flow (unit, scope_index, get_flow_index (unit, flow), buffer);
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	}
	return (result);
}

int		c_backend_translate_macro_flow (struct unit *unit, int scope_index, int flow_index, struct cbuffer *buffer) {
	int		result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (if): {
			write_string (buffer->wr, "(");
			if (c_backend_translate_expr (unit, flow->fif.expr, buffer)) {
				write_string (buffer->wr, ") ? (");
				if (c_backend_translate_macro_flow (unit, scope_index, flow->fif.flow_body, buffer)) {
					write_string (buffer->wr, ") : (");
					if (c_backend_translate_macro_flow (unit, scope_index, flow->fif.else_body, buffer)) {
						write_string (buffer->wr, ")");
						result = 1;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		case FlowType (block): {
			write_string (buffer->wr, "(");
			result = c_backend_translate_macro_scope (unit, flow->block.scope, buffer);
			write_string (buffer->wr, ")");
		} break ;
		case FlowType (expr): {
			result = c_backend_translate_expr (unit, flow->expr.index, buffer);
		} break ;
		default: Unreachable ();
	}
	if (result && flow->next >= 0) {
		write_string (buffer->wr, ", ");
	}
	return (result);
}

int		c_backend_translate_macro_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = c_backend_translate_macro_flow (unit, scope_index, get_flow_index (unit, flow), buffer);
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	}
	return (result);
}

int		c_backend_translate_func_prototype (struct unit *unit, int decl_index, struct cbuffer *buffer) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (func));
	write_c_new_line (buffer);
	if (c_backend_translate_type (unit, decl->type, decl->name, buffer)) {
		write_string (buffer->wr, ";");
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		c_backend_translate_decl (struct unit *unit, int decl_index, struct cbuffer *buffer) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	switch (decl->kind) {
		case DeclKind (var): {
			result = c_backend_translate_type (unit, decl->type, decl->name, buffer);
		} break ;
		case DeclKind (func): {
			write_c_new_line (buffer);
			if (c_backend_translate_type (unit, decl->type, decl->name, buffer)) {
				write_string (buffer->wr, " {");
				write_c_indent_up (buffer);
				result = c_backend_translate_code_scope (unit, decl->func.scope, buffer);
				write_c_indent_down (buffer);
				write_c_new_line (buffer);
				write_string (buffer->wr, "}");
			} else {
				result = 0;
			}
		} break ;
		case DeclKind (tag): {
			result = c_backend_translate_tag_decl (unit, decl_index, buffer);
		} break ;
		case DeclKind (block): {
			Todo ();
		} break ;
		case DeclKind (accessor): {
		} break ;
		case DeclKind (const): {
			write_c_new_line (buffer);
			write_string (buffer->wr, "#define ");
			c_backend_translate_const_name (decl->name, buffer);
			write_string (buffer->wr, " (");
			if (c_backend_translate_expr (unit, decl->dconst.expr, buffer)) {
				write_string (buffer->wr, ")");
				result = 1;
			} else {
				result = 0;
			}
		} break ;
		case DeclKind (macro): {
			struct scope	*scope;

			write_c_new_line (buffer);
			write_string (buffer->wr, "#define ");
			c_backend_translate_macro_name (decl->name, buffer);
			write_string (buffer->wr, "(");
			scope = get_scope (unit, decl->macro.param_scope);
			if (scope->decl_begin >= 0) {
				struct decl	*param;

				param = get_decl (unit, scope->decl_begin);
				do {
					if (param->next >= 0) {
						write_format (buffer->wr, "%s, ", param->name);
						param = get_decl (unit, param->next);
					} else {
						write_string (buffer->wr, param->name);
						param = 0;
					}
				} while (param);
			}
			write_string (buffer->wr, ") (");
			result = c_backend_translate_macro_scope (unit, decl->macro.scope, buffer);
			write_string (buffer->wr, ")");
		} break ;
		case DeclKind (external): {
			write_c_new_line (buffer);
			write_string (buffer->wr, "extern ");
			result = c_backend_translate_type (unit, decl->type, decl->name, buffer);
			write_string (buffer->wr, ";");
		} break ;
		case DeclKind (enum):
		default: Unreachable ();
	}
	return (result);
}

int		c_backend_translate_typeinfo (struct unit *unit, struct cbuffer *buffer) {
	int		result;

	write_c_new_line (buffer);
	c_backend_translate_tag_name (TagType (struct), "typeinfo", buffer);
	write_string (buffer->wr, " const g_typeinfos[];");
	if (Get_Array_Count (unit->typemembers) > 0) {
		int		index;

		write_c_new_line (buffer);
		c_backend_translate_tag_name (TagType (struct), "typemember", buffer);
		write_string (buffer->wr, " const g_typemembers[] = {");
		write_c_indent_up (buffer);
		index = 0;
		while (index < Get_Array_Count (unit->typemembers)) {
			struct typemember	*member;

			member = unit->typemembers + index;
			write_c_new_line (buffer);
			if (member->name == 0) {
				write_string (buffer->wr, "{ 0, 0, 0, 0 },");
			} else if (member->typeinfo >= 0) {
				write_format (buffer->wr, "{ \"%s\", g_typeinfos + %d, %d, %d },", member->name, member->typeinfo, member->value, member->offset);
			} else {
				write_format (buffer->wr, "{ \"%s\", 0, %d, %d },", member->name, member->value, member->offset);
			}
			index += 1;
		}
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "};");
	}
	if (Get_Array_Count (unit->typeinfos) > 0) {
		int		index;

		write_c_new_line (buffer);
		c_backend_translate_tag_name (TagType (struct), "typeinfo", buffer);
		write_string (buffer->wr, " const g_typeinfos[] = {");
		write_c_indent_up (buffer);
		index = 0;
		while (index < Get_Array_Count (unit->typeinfos)) {
			struct typeinfo	*typeinfo;

			typeinfo = unit->typeinfos + index;
			write_c_new_line (buffer);
			write_format (buffer->wr, "{ %zu, %d, %d, ", typeinfo->size, typeinfo->count, typeinfo->kind);
			switch (typeinfo->kind) {
				case TypeInfo_Kind (basic): {
					write_format (buffer->wr, "%d, 0, 0, 0 },", typeinfo->basic);
				} break ;
				case TypeInfo_Kind (struct):
				case TypeInfo_Kind (enum):
				case TypeInfo_Kind (union): {
					write_format (buffer->wr, "0, \"%s\", 0, g_typemembers + %d },", typeinfo->tagname, typeinfo->members);
				} break ;
				case TypeInfo_Kind (pointer):
				case TypeInfo_Kind (array): {
					write_format (buffer->wr, "0, 0, g_typeinfos + %d, 0 },", typeinfo->typeinfo);
				} break ;
				case TypeInfo_Kind (function): {
					if (typeinfo->members >= 0) {
						write_format (buffer->wr, "0, 0, g_typeinfos + %d, g_typemembers + %d },", typeinfo->typeinfo, typeinfo->members);
					} else {
						write_format (buffer->wr, "0, 0, g_typeinfos + %d, 0 },", typeinfo->typeinfo);
					}
				} break ;
			}
			index += 1;
		}
		write_c_indent_down (buffer);
		write_c_new_line (buffer);
		write_string (buffer->wr, "};");
	}
	result = 1;
	return (result);
}

int		c_backend_translate_unit_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (decl->kind == DeclKind (macro)) {
				result = c_backend_translate_decl (unit, get_decl_index (unit, decl), buffer);
			} else if (decl->kind == DeclKind (const)) {
				result = c_backend_translate_decl (unit, get_decl_index (unit, decl), buffer);
			} else {
				result = 1;
			}
			if (decl->next >= 0) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
		decl = get_decl (unit, scope->decl_begin);
		if (result) do {
			if (decl->kind == DeclKind (tag)) {
				result = c_backend_translate_decl (unit, get_decl_index (unit, decl), buffer);
			} else {
				result = 1;
			}
			if (decl->next >= 0) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
		decl = get_decl (unit, scope->decl_begin);
		if (result) do {
			if (decl->kind == DeclKind (func)) {
				result = c_backend_translate_func_prototype (unit, get_decl_index (unit, decl), buffer);
			} else if (decl->kind == DeclKind (external)) {
				result = c_backend_translate_decl (unit, get_decl_index (unit, decl), buffer);
			} else {
				result = 1;
			}
			if (decl->next >= 0) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
		result = result && c_backend_translate_typeinfo (unit, buffer);
		decl = get_decl (unit, scope->decl_begin);
		if (result) do {
			if (decl->kind == DeclKind (func)) {
				result = c_backend_translate_decl (unit, get_decl_index (unit, decl), buffer);
			} else {
				result = 1;
			}
			if (decl->next >= 0) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		c_backend_translate_func_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;

	result = 0;
	return (result);
}

int		c_backend_translate_param_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			Assert (decl->kind == DeclKind (param));
			if (decl->type >= 0) {
				result = c_backend_translate_type (unit, decl->type, decl->name, buffer);
			} else if (0 == strcmp (decl->name, "...")) {
				write_string (buffer->wr, "...");
			} else {
				Error ("unexpected param decl");
				result = 0;
			}
			if (decl->next >= 0) {
				write_string (buffer->wr, ", ");
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		c_backend_translate_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	switch (scope->kind) {
		case ScopeKind (unit): result = c_backend_translate_unit_scope (unit, scope_index, buffer); break ;
		case ScopeKind (func): result = c_backend_translate_func_scope (unit, scope_index, buffer); break ;
		case ScopeKind (code): result = c_backend_translate_code_scope (unit, scope_index, buffer); break ;
		case ScopeKind (tag): result = c_backend_translate_tag_scope (unit, scope_index, buffer); break ;
		case ScopeKind (param): result = c_backend_translate_param_scope (unit, scope_index, buffer); break ;
		default: Unreachable ();
	}
	return (result);
}

int		c_backend_translate_unit (struct unit *unit, struct cbuffer *buffer) {
	int		result;

	result = c_backend_translate_scope (unit, unit->root_scope, buffer);
	return (result);
}






