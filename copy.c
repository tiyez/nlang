

uint	make_type_copy (struct unit *unit, struct unit *decl_unit, uint type_index);

void	replace_type_with_copy (struct unit *unit, struct type *new_type, struct unit *decl_unit, uint type_index) {
	struct type	*type;

	type = get_type (decl_unit, type_index);
	new_type->flags = type->flags;
	new_type->kind = type->kind;
	if (type->kind == TypeKind (basic)) {
		new_type->basic = type->basic;
	} else if (type->kind == TypeKind (tag)) {
		Assert (!type->tag.decl);
		new_type->tag = type->tag;
	} else if (type->kind == TypeKind (mod)) {
		new_type->mod = type->mod;
		new_type->mod.forward = make_type_copy (unit, decl_unit, type->mod.forward);
		if (type->mod.kind == TypeMod (pointer)) {
		} else if (type->mod.kind == TypeMod (array)) {
			if (type->mod.expr) {
				new_type->mod.expr = make_expr_copy (unit, decl_unit, type->mod.expr);
			} else {
				new_type->mod.expr = 0;
			}
		} else if (type->mod.kind == TypeMod (function)) {
			if (is_lib_index (type->mod.param_scope)) {
				decl_unit = get_lib (get_lib_index (type->mod.param_scope));
				new_type->mod.param_scope = make_scope_copy (unit, decl_unit, unlib_index (type->mod.param_scope), 0, 0);
			} else {
				new_type->mod.param_scope = make_scope_copy (unit, decl_unit, type->mod.param_scope, 0, 0);
			}
		} else {
			Unreachable ();
		}
	} else if (type->kind == TypeKind (typeof)) {
		new_type->typeof = type->typeof;
		new_type->typeof.expr = make_expr_copy (unit, decl_unit, type->typeof.expr);
	} else {
		Unreachable ();
	}
}

uint	make_type_copy (struct unit *unit, struct unit *decl_unit, uint type_index) {
	uint	index;

	if (Prepare_Bucket (unit->types, 1)) {
		struct type	*new_type;

		new_type = Push_Bucket (unit->types);
		replace_type_with_copy (unit, new_type, decl_unit, type_index);
		index = Get_Bucket_Element_Index (unit->types, new_type);
	} else {
		Error ("cannot prepare bucket for type");
		index = 0;
	}
	return (index);
}

void	replace_expr_with_copy (struct unit *unit, struct expr *new_expr, struct unit *decl_unit, uint expr_index);

uint	make_expr_copy (struct unit *unit, struct unit *decl_unit, uint expr_index) {
	uint	index;

	if (Prepare_Bucket (unit->exprs, 1)) {
		struct expr	*new_expr;

		new_expr = Push_Bucket (unit->exprs);
		Assert (new_expr);
		replace_expr_with_copy (unit, new_expr, decl_unit, expr_index);
		index = get_expr_index (unit, new_expr);
	} else {
		Error ("cannot prepare exprs bucket");
		index = 0;
	}
	return (index);
}

void	replace_expr_with_copy (struct unit *unit, struct expr *new_expr, struct unit *decl_unit, uint expr_index) {
	struct expr	*expr;

	expr = get_expr (decl_unit, expr_index);
	new_expr->type = expr->type;
	if (expr->type == ExprType (op)) {
		new_expr->op.type = expr->op.type;
		if (expr->op.type == OpType (array_subscript) || expr->op.type == OpType (function_call)) {
			new_expr->op.forward = make_expr_copy (unit, decl_unit, expr->op.forward);
			new_expr->op.backward = make_expr_copy (unit, decl_unit, expr->op.backward);
		} else if (expr->op.type == OpType (cast)) {
			new_expr->op.forward = make_expr_copy (unit, decl_unit, expr->op.forward);
			new_expr->op.backward = make_type_copy (unit, decl_unit, expr->op.backward);
		} else {
			if (is_expr_unary (expr)) {
				new_expr->op.forward = make_expr_copy (unit, decl_unit, expr->op.forward);
				new_expr->op.backward = 0;
			} else {
				new_expr->op.backward = make_expr_copy (unit, decl_unit, expr->op.backward);
				new_expr->op.forward = make_expr_copy (unit, decl_unit, expr->op.forward);
			}
		}
	} else if (expr->type == ExprType (constant)) {
		new_expr->constant = expr->constant;
	} else if (expr->type == ExprType (string)) {
		new_expr->string = expr->string;
	} else if (expr->type == ExprType (identifier)) {
		new_expr->iden.name = expr->iden.name;
		if (expr->iden.decl) {
			if (is_lib_index (expr->iden.decl)) {
				new_expr->iden.decl = expr->iden.decl;
			} else if (unit != decl_unit) {
				new_expr->iden.decl = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), expr->iden.decl);
			} else {
				new_expr->iden.decl = expr->iden.decl;
			}
		}
	} else if (expr->type == ExprType (funcparam)) {
		if (expr->funcparam.expr) {
			new_expr->funcparam.expr = make_expr_copy (unit, decl_unit, expr->funcparam.expr);
		} else {
			new_expr->funcparam.expr = 0;
		}
		if (expr->funcparam.next) {
			new_expr->funcparam.next = make_expr_copy (unit, decl_unit, expr->funcparam.next);
		} else {
			new_expr->funcparam.next = 0;
		}
	} else if (expr->type == ExprType (typeinfo)) {
		new_expr->typeinfo.index = expr->typeinfo.index;
		new_expr->typeinfo.type = expr->typeinfo.type;
		if (expr->typeinfo.lib_index) {
			new_expr->typeinfo.lib_index = expr->typeinfo.lib_index;
		} else if (unit != decl_unit) {
			new_expr->typeinfo.lib_index = Get_Bucket_Element_Index (g_libs, decl_unit);
		} else {
			new_expr->typeinfo.lib_index = 0;
		}
	} else if (expr->type == ExprType (macroparam)) {
		new_expr->macroparam = expr->macroparam;
	} else {
		Unreachable ();
	}
}

uint	make_scope_copy (struct unit *unit, struct unit *decl_unit, uint scope_index, uint parent_scope, uint param_scope);

uint	make_flow_copy (struct unit *unit, struct unit *decl_unit, uint scope_index, uint flow_index) {
	struct flow	*flow, *new_flow;

	Assert (scope_index);
	Assert (flow_index);
	flow = get_flow (decl_unit, flow_index);
	new_flow = get_flow (unit, make_flow (unit, flow->type, flow->line));
	Assert (new_flow);
	if (flow->type == FlowType (if)) {
		new_flow->fif.expr = make_expr_copy (unit, decl_unit, flow->fif.expr);
		new_flow->fif.flow_body = make_flow_copy (unit, decl_unit, scope_index, flow->fif.flow_body);
		new_flow->fif.else_body = make_flow_copy (unit, decl_unit, scope_index, flow->fif.else_body);
	} else if (flow->type == FlowType (block)) {
		new_flow->block.scope = make_scope_copy (unit, decl_unit, flow->block.scope, scope_index, get_scope (unit, scope_index)->param_scope);
	} else if (flow->type == FlowType (expr)) {
		new_flow->expr.index = make_expr_copy (unit, decl_unit, flow->expr.index);
	} else {
		Unreachable ();
	}
	return (get_flow_index (unit, new_flow));
}

uint	make_decl_copy (struct unit *unit, struct unit *decl_unit, uint decl_index);

uint	make_scope_copy (struct unit *unit, struct unit *decl_unit, uint scope_index, uint parent_scope, uint param_scope) {
	uint			new_scope_index;
	struct scope	*scope, *new_scope;

	scope = get_scope (decl_unit, scope_index);
	new_scope_index = make_scope (unit, scope->kind, parent_scope);
	Assert (new_scope_index);
	new_scope = get_scope (unit, new_scope_index);
	new_scope->param_scope = param_scope;
	if (scope->kind == ScopeKind (param)) {
		uint	decl_index;

		decl_index = scope->decl_begin;
		while (decl_index) {
			uint	new_decl_index;

			new_decl_index = make_decl_copy (unit, decl_unit, decl_index);
			add_decl_to_scope (unit, new_scope_index, new_decl_index);
			decl_index = get_decl (decl_unit, decl_index)->next;
		}
	} else if (scope->kind == ScopeKind (macro)) {
		uint	flow_index;

		flow_index = scope->flow_begin;
		while (flow_index) {
			uint	new_flow_index;

			new_flow_index = make_flow_copy (unit, decl_unit, scope_index, flow_index);
			add_flow_to_scope (unit, new_scope_index, new_flow_index);
			flow_index = get_flow (decl_unit, flow_index)->next;
		}
	} else {
		Unreachable ();
	}
	return (new_scope_index);
}

uint	make_decl_copy (struct unit *unit, struct unit *decl_unit, uint decl_index) {
	uint		new_decl_index;
	struct decl	*decl;
	struct decl	*new_decl;

	decl = get_decl (decl_unit, decl_index);
	new_decl_index = make_decl (unit, 0, decl->name, 0, decl->kind, decl->line);
	Assert (new_decl_index);
	new_decl = get_decl (unit, new_decl_index);
	if (decl->type) {
		new_decl->type = make_type_copy (unit, decl_unit, decl->type);
	}
	if (decl->kind == DeclKind (const)) {
		Assert (decl->dconst.expr);
		new_decl->dconst.expr = make_expr_copy (unit, decl_unit, decl->dconst.expr);
	} else if (decl->kind == DeclKind (param)) {
		Assert (!decl->param.expr);
		new_decl->param.expr = 0;
	} else if (decl->kind == DeclKind (define)) {
		if (decl->define.kind == DefineKind (macro)) {
			new_decl->define.macro.param_scope = make_scope_copy (unit, decl_unit, decl->define.macro.param_scope, 0, 0);
			new_decl->define.macro.scope = make_scope_copy (unit, decl_unit, decl->define.macro.scope, 0, new_decl->define.macro.param_scope);
		} else {
			Unreachable ();
		}
	} else {
		Unreachable ();
	}
	return (new_decl_index);
}





