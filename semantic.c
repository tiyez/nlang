]
int		analyze_code_flow (struct unit *unit, int flow_index, int scope_index) {
	int		result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): result = 1; break ;
		case FlowType (expr): {
			result = analyze_expr (unit, flow->expr.index, scope_index);
		} break ;
		case FlowType (block): {
			result = analyze_code_scope (unit, flow->block.scope);
		} break ;
		case FlowType (if): {
			if (analyze_expr (unit, flow->fif.expr, scope_index)) {
				struct typestack	ctypestack, *typestack = &ctypestack;

				init_typestack (typestack);
				if (make_typestack_from_expr (unit, typestack, flow->fif.expr)) {
					if (is_type_implicitly_convertable_to_bool (get_typestack_head (typestack))) {
						if (analyze_code_flow (unit, flow->fif.flow_body, scope_index)) {
							if (flow->fif.else_body >= 0) {
								result = analyze_code_flow (unit, flow->fif.else_body, scope_index);
							} else {
								result = 1;
							}
						} else {
							result = 0;
						}
					} else {
						Error ("value of expression in 'if' condition is not convertable to bool");
						result = 0;
					}
				} else {
					Error ("cannot create typestack");
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		case FlowType (while): {
			if (analyze_expr (unit, flow->fwhile.expr, scope_index)) {
				struct typestack	ctypestack, *typestack = &ctypestack;

				init_typestack (typestack);
				if (make_typestack_from_expr (unit, typestack, flow->fwhile.expr)) {
					if (is_type_implicitly_convertable_to_bool (get_typestack_head (typestack))) {
						if (analyze_code_flow (unit, flow->fwhile.flow_body, scope_index)) {
							result = 1;
						} else {
							result = 0;
						}
					} else {
						Error ("value of expression in 'while' condition is not convertable to bool");
						result = 0;
					}
				} else {
					Error ("cannot create typestack");
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		case FlowType (dowhile): {
			if (analyze_expr (unit, flow->dowhile.expr, scope_index)) {
				struct typestack	ctypestack, *typestack = &ctypestack;

				init_typestack (typestack);
				if (make_typestack_from_expr (unit, typestack, flow->dowhile.expr)) {
					if (is_type_implicitly_convertable_to_bool (get_typestack_head (typestack))) {
						if (analyze_code_flow (unit, flow->dowhile.flow_body, scope_index)) {
							result = 1;
						} else {
							result = 0;
						}
					} else {
						Error ("value of expression in 'do while' condition is not convertable to bool");
						result = 0;
					}
				} else {
					Error ("cannot create typestack");
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		default: Assert (0);
		// default: Unreachable ();
	}
	return (result);
}

int		analyze_code_scope (struct unit *unit, int scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin >= 0) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				result = analyze_code_flow (unit, get_flow_index (unit, flow), scope_index);
				flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		} else {
			result = 1;
		}
	} else {
		Error ("redefinition");
		result = 0;
	}
	return (result);
}

int		analyze_struct_scope (struct unit *unit, int scope_index) {
	return (0);
}

int		analyze_union_scope (struct unit *unit, int scope_index) {
	return (0);
}

int		analyze_stroke_scope (struct unit *unit, int scope_index) {
	return (0);
}

int		analyze_enum_scope (struct unit *unit, int scope_index) {
	return (0);
}

int		analyze_bitfield_scope (struct unit *unit, int scope_index) {
	return (0);
}

//
int		analyze_decl_var (struct unit *unit, int decl_index) {
	return (1);
}

int		analyze_decl_const (struct unit *unit, int decl_index) {
	return (1);
}

int		analyze_decl_func (struct unit *unit, int decl_index) {
	int		result;
	struct decl	*decl;
	struct type	*type;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (func));
	Assert (decl->type >= 0);
	type = get_type (unit, decl->type);
	Assert (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (function));
	Assert (type->mod.func.param_scope >= 0);
	if (check_scope_declarations_for_name_uniqueness (unit, type->mod.func.param_scope)) {
		Assert (decl->func.scope >= 0);
		result = analyze_code_scope (unit, decl->func.scope);
	} else {
		Error ("redefinition in the function parameters");
		result = 0;
	}
	return (result);
}

int		analyze_decl_tag (struct unit *unit, int decl_index) {
	return (0);
}
//

int		analyze_unit_scope (struct unit *unit, int scope_index) {
	int		result;
	struct scope	*scope;
	struct flow		*flow;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		flow = get_flow (unit, scope->flow_begin);
		do {
			if (flow->type == FlowType (decl)) {
				struct decl	*decl;

				Assert (flow->decl.index >= 0);
				switch (get_decl (unit, flow->decl.index)->kind) {
					case DeclKind (var): result = analyze_decl_var (unit, flow->decl.index);
					case DeclKind (const): result = analyze_decl_const (unit, flow->decl.index);
					case DeclKind (func): result = analyze_decl_func (unit, flow->decl.index);
					case DeclKind (tag): result = analyze_decl_tag (unit, flow->decl.index);
					default: Unreachable ();
				}
			} else {
				Error ("in the unit scope there must be only declarations");
				result = 0;
			}
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	}
	return (result);
}

int		analyze_unit (struct unit *unit) {
	int		result;

	result = analyze_unit_scope (unit, unit->root_scope);
	return (result);
}






