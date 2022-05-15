


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
					/* translate expr */
					write_string (buffer->wr, "]");
				} else if (type->mod.kind == TypeMod (function)) {
					write_string (buffer->wr, "(");
					/* translate expr */
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
					switch (decl->tag.type) {
						case TagType (struct): write_format (buffer->wr, "struct s_%s ", decl->name);
						case TagType (union): write_format (buffer->wr, "struct u_%s ", decl->name);
						case TagType (stroke): write_format (buffer->wr, "struct k_%s ", decl->name);
						case TagType (enum): write_format (buffer->wr, "struct e_%s ", decl->name);
						case TagType (bitfield): write_format (buffer->wr, "struct b_%s ", decl->name);
						default: Unreachable ();
					}
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
		case ExprType (decl): {
			struct decl	*decl;

			decl = get_decl (unit, expr->decl.index);
			result = write_string (buffer->wr, decl->name);
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
			Unreachable (); /* TODO; */
		} break ;
		default: Unreachable ();
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
			if (c_backend_translate_type (unit, decl->type, decl->name, buffer)) {
				result = c_backend_translate_scope (unit, decl->func.scope, buffer);
			} else {
				result = 0;
			}
		} break ;
		case DeclKind (tag): {

		} break ;
		case DeclKind (block):
		case DeclKind (enum):
		case DeclKind (const):
		default: Unreachable ();
	}
	return (result);
}

int		c_backend_translate_unit_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		if (flow->type == FlowType (decl)) {
			result = c_backend_translate_decl (unit, flow->decl.index, buffer);
		} else {
			Error ("unexpected flow type");
			result = 0;
		}
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

int		c_backend_translate_code_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;

	result = 0;
	return (result);
}

int		c_backend_translate_tag_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;

	result = 0;
	return (result);
}

int		c_backend_translate_param_scope (struct unit *unit, int scope_index, struct cbuffer *buffer) {
	int		result;

	result = 0;
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






