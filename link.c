
/*
	TODO: proper self reference check
*/


int		g_link_decl_index;

int		find_ordinary_decl (struct unit *unit, int scope_index, const char *name) {
	int		decl_index;
	struct scope	*scope;

	decl_index = -1;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;
		int			result;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (scope->kind == ScopeKind (unit) || get_decl_index (unit, decl) <= g_link_decl_index) {
				if (decl->kind == DeclKind (tag) && (decl->tag.type == TagType (enum) || decl->tag.type == TagType (bitfield))) {
					decl_index = find_ordinary_decl (unit, decl->tag.scope, name);
				} else if (decl->kind != DeclKind (tag) && 0 == strcmp (name, decl->name)) {
					decl_index = get_decl_index (unit, decl);
				}
				result = 1;
			} else {
				result = 0;
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl_index < 0 && decl);
	}
	if (decl_index < 0 && scope->param_scope >= 0) {
		decl_index = find_ordinary_decl (unit, scope->param_scope, name);
	}
	if (decl_index < 0 && scope->parent_scope >= 0) {
		decl_index = find_ordinary_decl (unit, scope->parent_scope, name);
	}
	return (decl_index);
}

int		find_decl_tag (struct unit *unit, int scope_index, const char *name, enum tagtype tagtype) {
	int		decl_index;
	struct scope	*scope;

	decl_index = -1;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;
		int			result;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (scope->kind == ScopeKind (unit) || get_decl_index (unit, decl) <= g_link_decl_index) {
				if (decl->kind == DeclKind (tag) && decl->tag.type == tagtype && 0 == strcmp (name, decl->name)) {
					decl_index = get_decl_index (unit, decl);
				}
				result = 1;
			} else {
				result = 0;
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl_index < 0 && decl);
	}
	if (decl_index < 0 && scope->parent_scope >= 0) {
		decl_index = find_decl_tag (unit, scope->parent_scope, name, tagtype);
	}
	return (decl_index);
}

int		link_type (struct unit *unit, int scope_index, int type_index, int is_selfref_check);

int		link_expr (struct unit *unit, int scope_index, int expr_index, int is_selfref_check) {
	int		result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	switch (expr->type) {
		case ExprType (op): {
			switch (expr->op.type) {
				case OpType (function_call): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check)) {
						if (expr->op.backward >= 0) {
							result = link_expr (unit, scope_index, expr->op.backward, is_selfref_check);
						} else {
							result = 1;
						}
					} else {
						result = 0;
					}
				} break ;
				case OpType (array_subscript): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check)) {
						result = link_expr (unit, scope_index, expr->op.backward, is_selfref_check);
					} else {
						result = 0;
					}
				} break ;
				case OpType (cast): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check)) {
						result = link_type (unit, scope_index, expr->op.backward, 1);
					} else {
						result = 0;
					}
				} break ;
				default: {
					if (is_expr_unary (expr)) {
						if (expr->op.type == OpType (address_of)) {
							is_selfref_check = 0;
						} else if (expr->op.type == OpType (group)) {
							is_selfref_check = 1;
						}
						result = link_expr (unit, scope_index, expr->op.forward, is_selfref_check);
					} else {
						if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check)) {
							result = link_expr (unit, scope_index, expr->op.forward, is_selfref_check);
						} else {
							result = 0;
						}
					}
				} break ;
			}
		} break ;
		case ExprType (funcparam): {
			if (link_expr (unit, scope_index, expr->funcparam.expr, is_selfref_check)) {
				if (expr->funcparam.next >= 0) {
					result = link_expr (unit, scope_index, expr->funcparam.next, is_selfref_check);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} break ;
		case ExprType (decl):
		case ExprType (constant): {
			result = 1;
		} break ;
		case ExprType (identifier): {
			int		decl_index;

			decl_index = find_ordinary_decl (unit, scope_index, expr->identifier);
			if (decl_index >= 0) {
				if (is_selfref_check) {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					if (decl->is_in_process == 0) {
						expr->type = ExprType (decl);
						expr->decl.index = decl_index;
						result = 1;
					} else {
						Error ("self referencing declaration '%s'", decl->name);
						result = 0;
					}
				} else {
					expr->type = ExprType (decl);
					expr->decl.index = decl_index;
					result = 1;
				}
			} else {
				Error ("undeclared identifier '%s'", expr->identifier);
				result = 0;
			}
		} break ;
		case ExprType (typeinfo): {
			result = link_type (unit, scope_index, expr->typeinfo.type, 1);
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_type (struct unit *unit, int scope_index, int type_index, int is_selfref_check) {
	int		result;
	struct type	*type;

	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): result = link_type (unit, scope_index, type->mod.ptr.type, 0); break ;
				case TypeMod (function): result = link_type (unit, scope_index, type->mod.func.type, 0); break ;
				case TypeMod (array): result = link_type (unit, scope_index, type->mod.array.type, 0); break ;
				default: Unreachable ();
			}
		} break ;
		case TypeKind (basic): {
			result = 1;
		} break ;
		case TypeKind (tag): {
			int		decl_index;

			decl_index = find_decl_tag (unit, scope_index, type->tag.name, type->tag.type);
			if (decl_index >= 0) {
				Debug ("resolving %s tag '%s' with selfref check %d", g_tagname[type->tag.type], type->tag.name, is_selfref_check);
				if (is_selfref_check) {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					if (decl->is_in_process == 0) {
						type->kind = TypeKind (decl);
						type->decl.index = decl_index;
						result = 1;
					} else {
						Error ("self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
						result = 0;
					}
				} else {
					type->kind = TypeKind (decl);
					type->decl.index = decl_index;
					result = 1;
				}
			} else {
				Error ("undeclared %s tag '%s'", g_tagname[type->tag.type], type->tag.name);
				result = 0;
			}
		} break ;
		case TypeKind (typeof): {
			result = link_expr (unit, scope_index, type->typeof.expr, 1);
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_decl_var (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	Assert (decl->type >= 0);
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		result = link_type (unit, scope_index, decl->type, 1);
		decl->is_in_process = 0;
	} else {
		Error ("type loop referencing");
		result = 0;
	}
	return (result);
}

int		link_decl_const (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		result = link_expr (unit, scope_index, decl->dconst.expr, 1);
		decl->is_in_process = 0;
	} else {
		Error ("type loop referencing");
		result = 0;
	}
	return (result);
}

int		link_param_scope (struct unit *unit, int scope_index, int param_scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, param_scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			result = link_type (unit, scope_index, decl->type, 1);
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		link_decl (struct unit *unit, int scope_index, int decl_index);
int		link_code_scope (struct unit *unit, int scope_index);

int		link_code_scope_flow (struct unit *unit, int scope_index, int flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			g_link_decl_index = flow->decl.index;
			result = link_decl (unit, scope_index, flow->decl.index);
		} break ;
		case FlowType (expr): {
			result = link_expr (unit, scope_index, flow->expr.index, 1);
		} break ;
		case FlowType (block): {
			result = link_code_scope (unit, flow->block.scope);
		} break ;
		case FlowType (if): {
			if (link_expr (unit, scope_index, flow->fif.expr, 1)) {
				if (link_code_scope_flow (unit, scope_index, flow->fif.flow_body)) {
					if (flow->fif.else_body >= 0) {
						result = link_code_scope_flow (unit, scope_index, flow->fif.else_body);
					} else {
						result = 1;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		case FlowType (while): {
			if (link_expr (unit, scope_index, flow->fwhile.expr, 1)) {
				result = link_code_scope_flow (unit, scope_index, flow->fwhile.flow_body);
			} else {
				result = 0;
			}
		} break ;
		case FlowType (dowhile): {
			if (link_code_scope_flow (unit, scope_index, flow->dowhile.flow_body)) {
				result = link_expr (unit, scope_index, flow->dowhile.expr, 1);
			} else {
				result = 0;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_code_scope (struct unit *unit, int scope_index) {
	int				result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin >= 0) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				result = link_code_scope_flow (unit, scope_index, get_flow_index (unit, flow));
				flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		} else {
			result = 1;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_decl_func (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		if (link_type (unit, scope_index, decl->type, 1)) {
			if (link_param_scope (unit, scope_index, decl->func.param_scope)) {
				result = link_code_scope (unit, decl->func.scope);
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
		decl->is_in_process = 0;
	} else {
		Error ("self referencing");
		result = 0;
	}
	return (result);
}

int		link_enum_scope_flow (struct unit *unit, int scope_index, int flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			struct decl	*decl;

			g_link_decl_index = flow->decl.index;
			decl = get_decl (unit, flow->decl.index);
			Assert (decl->kind == DeclKind (enum));
			if (decl->enumt.expr >= 0) {
				if (decl->is_in_process == 0) {
					decl->is_in_process = 1;
					result = link_expr (unit, scope_index, decl->enumt.expr, 1);
					decl->is_in_process = 0;
				} else {
					Error ("self referencing");
					result = 0;
				}
			} else {
				result = 1;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_enum_scope (struct unit *unit, int scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_enum_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		link_struct_scope_flow (struct unit *unit, int scope_index, int flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			g_link_decl_index = flow->decl.index;
			result = link_decl (unit, scope_index, flow->decl.index);
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_struct_scope (struct unit *unit, int scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_struct_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		link_decl_tag (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (check_scope_declarations_for_name_uniqueness (unit, decl->tag.scope)) {
		if (decl->is_in_process == 0) {
			decl->is_in_process = 1;
			if (decl->tag.type == TagType (struct) || decl->tag.type == TagType (union) || decl->tag.type == TagType (stroke)) {
				result = link_struct_scope (unit, decl->tag.scope);
			} else if (decl->tag.type == TagType (enum) || decl->tag.type == TagType (bitfield)) {
				result = link_enum_scope (unit, decl->tag.scope);
			} else {
				Error ("unknown tag type");
				result = 0;
			}
			decl->is_in_process = 0;
		} else {
			Error ("self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
			result = 0;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_decl_block (struct unit *unit, int scope_index, int decl_index) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	result = link_struct_scope (unit, decl->block.scope);
	return (result);
}

int		link_decl (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	switch (decl->kind) {
		case DeclKind (var): result = link_decl_var (unit, scope_index, decl_index); break ;
		case DeclKind (const): result = link_decl_const (unit, scope_index, decl_index); break ;
		case DeclKind (func): result = link_decl_func (unit, scope_index, decl_index); break ;
		case DeclKind (tag): result = link_decl_tag (unit, scope_index, decl_index); break ;
		case DeclKind (block): result = link_decl_block (unit, scope_index, decl_index); break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_unit_scope (struct unit *unit, int scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin >= 0) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				if (flow->type == FlowType (decl)) {
					g_link_decl_index = flow->decl.index;
					result = link_decl (unit, scope_index, flow->decl.index);
				} else {
					Error ("unexpected flow type in unit scope");
					result = 0;
				}
				flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_unit (struct unit *unit) {
	int		result;

	result = link_unit_scope (unit, unit->root_scope);
	return (result);
}








