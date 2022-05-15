




int		typecheck_type (struct unit *unit, int type_index) {
	int		result;
	struct type	*type;

	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): result = typecheck_type (unit, type->mod.ptr.type); break ;
				case TypeMod (array): {
					struct typestack	ctypestack, *typestack = &ctypestack;

					init_typestack (typestack);
					if (typecheck_expr (unit, type->mod.array.expr, typestack)) {
						struct type	*head;

						head = get_typestack_head (typestack);
						if (head && head->kind == TypeKind (basic) && is_basictype_integral (head->basic.type)) {
							result = 1;
						} else {
							Error ("array size expression is not integral");
							result = 0;
						}
					} else {
						result = 0;
					}
				} break ;
				case TypeMod (function): {

				} break ;
			}
		} break ;
		case TypeKind (typeof): {
			struct typestack	ctypestack, *typestack = &ctypestack;

			init_typestack (typestack);
			if (typecheck_expr (unit, type->typeof.expr, typestack)) {
				result = insert_typestack_to_type (unit, type_index, typestack);
			} else {
				result = 0;
			}
		} break ;
		case TypeKind (basic):
		case TypeKind (decl): {
			result = 1;
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		typecheck_decl_var (struct unit *unit, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	result = typecheck_type (unit, decl->type);
	return (result);
}

int		typecheck_decl (struct unit *unit, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	switch (decl->kind) {
		DeclKind (var): result = typecheck_decl_var (unit, decl_index); break ;
		DeclKind (const): result = typecheck_decl_const (unit, decl_index); break ;
		DeclKind (func): result = typecheck_decl_func (unit, decl_index); break ;
		DeclKind (tag): result = typecheck_decl_tag (unit, decl_index); break ;
		DeclKind (block): result = typecheck_decl_block (unit, decl_index); break ;
		DeclKind (enum): result = typecheck_decl_enum (unit, decl_index); break ;
		default: Unreachable ();
	}
	return (result);
}

int		typecheck_scope (struct unit *unit, int scope_index) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			if (flow->type == FlowType (decl)) {
				result = typecheck_decl (unit, scope_index, flow->decl.index);
			} else {
				Error ("unexpected flow type");
				result = 0;
			}
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		typecheck_unit (struct unit *unit) {
	int		result;

	result = typecheck_scope (unit, unit->root_scope);
	return (result);
}




