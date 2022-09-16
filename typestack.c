

void	init_typestack (struct typestack *typestack) {
	typestack->types_count = 0;
	typestack->head = -1;
	typestack->value = ValueCategory (rvalue);
}

struct type	*get_typestack_type (struct typestack *typestack, int index) {
	return (index >= 0 ? typestack->types + index : 0);
}

struct type	*get_typestack_type_gen (struct typestack *typestack, uint index) {
	return (index > 0 ? typestack->types + (index - 1) : 0);
}

struct type	*get_typestack_head (struct typestack *typestack) {
	return (get_typestack_type (typestack, typestack->head));
}

struct type *get_typestack_tail (struct typestack *typestack) {
	return (get_typestack_type (typestack, 0));
}

void	empty_typestack (struct typestack *typestack) {
	init_typestack (typestack);
}

int		push_typestack (struct typestack *typestack, struct type *type) {
	int		result;

	if (typestack->head < 0) {
		Assert (type->kind != TypeKind (mod) && type->kind != TypeKind (typeof));
		Assert (typestack->types_count == 0);
		typestack->types[typestack->types_count] = *type;
		typestack->head = typestack->types_count;
		typestack->types_count += 1;
		result = 1;
	} else {
		// Debug ("type kind %s; head kind is %s", g_typekind[type->kind], g_typekind[get_typestack_head (typestack)->kind]);
		Assert (type->kind == TypeKind (mod));
		Assert (typestack->types_count > 0 && typestack->types_count < Max_Type_Depth);
		Assert (typestack->head >= 0);
		typestack->types[typestack->types_count] = *type;
		type = typestack->types + typestack->types_count;
		type->mod.forward = typestack->head;
		typestack->head = typestack->types_count;
		typestack->types_count += 1;
		result = 1;
	}
	return (result);
}

int		push_typestack_recursive (struct unit *unit, struct unit *decl_unit, struct typestack *typestack, uint type_index) {
	int			result;
	struct type	*type;

	type = get_type (decl_unit, type_index);
	if (type->kind == TypeKind (internal) || type->kind == TypeKind (tag) || type->kind == TypeKind (basic)) {
		result = push_typestack (typestack, type);
		if (unit != decl_unit) {
			type = get_typestack_head (typestack);
			if (type->kind == TypeKind (tag)) {
				if (type->tag.decl && !is_lib_index (type->tag.decl)) {
					Assert (decl_unit != g_unit);
					type->tag.decl = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), type->tag.decl);
				}
			} else if (type->kind == TypeKind (internal)) {
				if (type->internal.decl && !is_lib_index (type->internal.decl)) {
					Assert (decl_unit != g_unit);
					type->internal.decl = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), type->internal.decl);
				}
			}
		}
	} else if (type->kind == TypeKind (mod)) {
		if (push_typestack_recursive (unit, decl_unit, typestack, type->mod.forward)) {
			result = push_typestack (typestack, type);
			if (unit != decl_unit) {
				type = get_typestack_head (typestack);
				Assert (type->kind == TypeKind (mod));
				if (type->mod.kind == TypeMod (function)) {
					if (type->mod.param_scope && !is_lib_index (type->mod.param_scope)) {
						Assert (decl_unit != g_unit);
						type->mod.param_scope = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), type->mod.param_scope);
					}
				} else if (type->mod.kind == TypeMod (array)) {
					if (type->mod.expr && !is_lib_index (type->mod.expr)) {
						Assert (decl_unit != g_unit);
						type->mod.expr = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), type->mod.expr);
					}
				}
			}
		} else {
			result = 0;
		}
	} else {
		Unreachable ();
	}
	return (result);
}

int		push_pointer_type_to_typestack (struct typestack *typestack, int is_const) {
	int			result;
	struct type	ptr_type = {0};

	ptr_type.kind = TypeKind (mod);
	ptr_type.flags.is_const = is_const;
	ptr_type.mod.kind = TypeMod (pointer);
	ptr_type.mod.forward = 0;
	result = push_typestack (typestack, &ptr_type);
	return (result);
}

int		push_basictype_to_typestack (struct typestack *typestack, enum basictype basic, int is_const) {
	int			result;
	struct type	type = {0};

	type.kind = TypeKind (basic);
	type.flags.is_const = is_const;
	type.basic.type = basic;
	result = push_typestack (typestack, &type);
	return (result);
}

int		pop_typestack (struct typestack *typestack) {
	int		result;

	if (typestack->head >= 0) {
		Assert (typestack->head + 1 == typestack->types_count);
		typestack->types_count -= 1;
		typestack->head -= 1;
		result = 1;
	} else {
		result = 1;
	}
	return (result);
}

int		insert_typestack_to_type (struct unit *unit, int type_index, struct typestack *typestack) {
	int			result;
	struct type	*type;

	type = get_type (unit, type_index);
	*type = *get_typestack_head (typestack);
	if (type->kind == TypeKind (internal) || type->kind == TypeKind (tag) || type->kind == TypeKind (basic)) {
		result = 1;
	} else if (type->kind == TypeKind (mod)) {
		uint	child_type_index;

		child_type_index = make_basic_type (unit, BasicType (void), 0);
		Assert (child_type_index);
		type->mod.forward = child_type_index;
		pop_typestack (typestack);
		result = insert_typestack_to_type (unit, child_type_index, typestack);
	} else {
		Unreachable ();
	}
	return (result);
}

void	print_left_right_typestacks (struct unit *unit, struct typestack *left, struct typestack *right) {
	fprintf (stderr, "\nleft type: ");
	print_typestack (unit, left, stderr);
	fprintf (stderr, "\nright type: ");
	print_typestack (unit, right, stderr);
	fprintf (stderr, "\n");
}

void	print_type_typestack (struct unit *unit, struct typestack *typestack) {
	fprintf (stderr, "\ntype: ");
	print_typestack (unit, typestack, stderr);
	fprintf (stderr, "\n");
}

void	update_typestack (struct unit *unit, struct expr *expr, struct typestack *typestack, struct typestack *rightstack) {
	struct type	*left, *right;

	left = get_typestack_head (typestack);
	if (rightstack) {
		right = get_typestack_head (rightstack);
	} else {
		right = 0;
	}
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (group)) {
		} else if (expr->op.type == OpType (function_call)) {
			Assert (left);
			Assert (left->kind == TypeKind (mod) && left->mod.kind == TypeMod (pointer));
			pop_typestack (typestack);
			left = get_typestack_head (typestack);
			Assert (left);
			Assert (left->kind == TypeKind (mod) && left->mod.kind == TypeMod (function));
			pop_typestack (typestack);
			Assert (get_typestack_head (typestack));
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (array_subscript)) {
			Assert (left);
			Assert (right);
			if (is_type_integral (left)) {
				Assert (is_pointer_type (right));
				*typestack = *rightstack;
			} else {
				Assert (is_pointer_type (left));
				Assert (is_type_integral (right));
			}
			pop_typestack (typestack);
			Assert (get_typestack_head (typestack));
			typestack->value = ValueCategory (lvalue);
		} else if (expr->op.type == OpType (cast)) {
			Assert (left);
			init_typestack (typestack);
			if (push_typestack_recursive (unit, unit, typestack, expr->op.backward)) {
				Assert (get_typestack_head (typestack));
				typestack->value = ValueCategory (rvalue);
			} else {
				Unreachable ();
			}
		} else if (expr->op.type == OpType (indirect_access) || expr->op.type == OpType (member_access)) {
			Assert (left);
			Assert (right);
			if (expr->op.type == OpType (indirect_access)) {
				Assert (is_pointer_type (left));
				pop_typestack (typestack);
				left = get_typestack_head (typestack);
				typestack->value = ValueCategory (lvalue);
			}
			Assert (left->kind == TypeKind (tag) && left->tag.type == TagType (struct));
			Assert (typestack->value == ValueCategory (lvalue));
			Assert (right);
			*typestack = *rightstack;
			typestack->value = ValueCategory (lvalue);
			Assert (get_typestack_head (typestack));
		} else if (expr->op.type == OpType (address_of)) {
			Assert (left);
			Assert (typestack->value == ValueCategory (lvalue));
			push_pointer_type_to_typestack (typestack, 0);
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (typesizeof) || expr->op.type == OpType (typealignof)) {
			init_typestack (typestack);
			push_basictype_to_typestack (typestack, BasicType (usize), 0);
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
			init_typestack (typestack);
			push_basictype_to_typestack (typestack, BasicType (usize), 0);
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (unary_plus) || expr->op.type == OpType (unary_minus) || expr->op.type == OpType (bitwise_not)) {
			Assert (left);
			Assert (left->kind == TypeKind (basic));
			Assert (left->basic.type == BasicType (void));
			if (is_basictype_integral (left->basic.type)) {
				left->basic.type = get_promoted_basictype (left->basic.type);
			} else {
				Assert (expr->op.type != OpType (bitwise_not));
			}
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (logical_not)) {
			Assert (left);
			if (left->kind == TypeKind (basic)) {
				Assert (left->basic.type != BasicType (void));
				Assert (is_basictype_integral (left->basic.type));
				init_typestack (typestack);
				push_basictype_to_typestack (typestack, BasicType (int), 0);
				typestack->value = ValueCategory (rvalue);
			} else {
				Assert (is_pointer_type (left));
				init_typestack (typestack);
				push_basictype_to_typestack (typestack, BasicType (int), 0);
				typestack->value = ValueCategory (rvalue);
			}
		} else if (expr->op.type == OpType (indirect)) {
			Assert (left);
			Assert (is_pointer_type (left));
			pop_typestack (typestack);
			typestack->value = ValueCategory (lvalue);
			Assert (get_typestack_head (typestack));
		} else if (is_arithmetic_optype (expr->op.type) || is_arithmethic_assignment_optype (expr->op.type)) {
			Assert (left);
			Assert (right);
			if (left->kind == TypeKind (basic) && right->kind == TypeKind (basic)) {
				Assert (left->basic.type != BasicType (void) && right->basic.type != BasicType (void));
				if (is_bitwise_optype (expr->op.type) || is_bitwise_assignment_optype (expr->op.type)) {
					Assert (is_basictype_integral (left->basic.type) && is_basictype_integral (right->basic.type));
				}
				if (is_comparison_optype (expr->op.type)) {
					init_typestack (typestack);
					push_basictype_to_typestack (typestack, BasicType (int), 0);
				} else {
					if (!is_assignment_optype (expr->op.type)) {
						left->basic.type = get_common_arithmetic_basictype (left->basic.type, right->basic.type);
					}
				}
				typestack->value = ValueCategory (rvalue);
			} else {
				Assert (is_pointer_type (left) || is_pointer_type (right));
				if (is_pointer_type (left) && is_pointer_type (right)) {
					Assert (!is_assignment_optype (expr->op.type));
					if (is_comparison_optype (expr->op.type)) {
						init_typestack (typestack);
						push_basictype_to_typestack (typestack, BasicType (int), 0);
						typestack->value = ValueCategory (rvalue);
					} else {
						Assert (expr->op.type == OpType (subtract));
						init_typestack (typestack);
						push_basictype_to_typestack (typestack, BasicType (size), 0);
						typestack->value = ValueCategory (rvalue);
					}
				} else {
					struct type	*ptr, *integ;

					if (is_pointer_type (left)) {
						ptr = left;
						integ = right;
					} else {
						Assert (!is_assignment_optype (expr->op.type));
						ptr = right;
						integ = left;
					}
					Assert (integ->kind == TypeKind (basic) && is_basictype_integral (integ->basic.type));
					if (is_comparison_optype (expr->op.type)) {
						init_typestack (typestack);
						push_basictype_to_typestack (typestack, BasicType (int), 0);
						typestack->value = ValueCategory (rvalue);
					} else {
						if (ptr == right) {
							*typestack = *rightstack;
						}
						typestack->value = ValueCategory (rvalue);
					}
				}
			}
		} else if (expr->op.type == OpType (assign)) {
			Assert (left);
			Assert (right);
			Assert (typestack->value == ValueCategory (lvalue));
			if (left->kind == TypeKind (basic) && right->kind == TypeKind (basic)) {
				Assert (left->basic.type != BasicType (void) && right->basic.type != BasicType (void));
				typestack->value = ValueCategory (rvalue);
			} else if (is_pointer_type (left) && is_pointer_type (right)) {
				Assert (is_same_type (typestack, rightstack, 1));
				typestack->value = ValueCategory (rvalue);
			} else if (left->kind == right->kind && left->kind == TypeKind (tag) && left->tag.type == right->tag.type) {
				Assert (left->tag.decl == right->tag.decl);
				typestack->value = ValueCategory (rvalue);
			} else {
				Assert (is_pointer_type (left) && right->kind == TypeKind (basic));
				typestack->value = ValueCategory (rvalue);
			}
		} else if (expr->op.type == OpType (logical_and) || expr->op.type == OpType (logical_or)) {
			Assert (left);
			Assert (right);
			Assert (is_pointer_type (left) || (left->kind == TypeKind (basic) && is_basictype_integral (left->basic.type)));
			Assert (is_pointer_type (right) || (right->kind == TypeKind (basic) && is_basictype_integral (right->basic.type)));
			init_typestack (typestack);
			push_basictype_to_typestack (typestack, BasicType (int), 0);
			typestack->value = ValueCategory (rvalue);
		} else if (expr->op.type == OpType (left_shift) || expr->op.type == OpType (right_shift) || expr->op.type == OpType (left_shift_assign) || expr->op.type == OpType (right_shift_assign)) {
			Assert (left);
			Assert (right);
			Assert (left->kind == TypeKind (basic) && is_basictype_integral (left->basic.type));
			Assert (right->kind == TypeKind (basic) && is_basictype_integral (right->basic.type));
			typestack->value = ValueCategory (rvalue);
		} else {
			Unreachable ();
		}
	} else if (expr->type == ExprType (constant)) {
		init_typestack (typestack);
		push_basictype_to_typestack (typestack, expr->constant.type, 0);
		typestack->value = ValueCategory (rvalue);
	} else if (expr->type == ExprType (identifier)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		init_typestack (typestack);
		if (is_lib_index (expr->iden.decl)) {
			decl_unit = get_lib (get_lib_index (expr->iden.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (expr->iden.decl));
		if (decl->type) {
			push_typestack_recursive (unit, decl_unit, typestack, decl->type);
			left = get_typestack_head (typestack);
			if (left->kind == TypeKind (mod) && left->mod.kind == TypeMod (function)) {
				Assert (decl->kind == DeclKind (func) || decl->kind == DeclKind (define));
				push_pointer_type_to_typestack (typestack, 0);
				typestack->value = ValueCategory (rvalue);
			} else if (left->kind == TypeKind (mod) && left->mod.kind == TypeMod (array)) {
				if (typestack->is_sizeof_context) {
					typestack->value = ValueCategory (lvalue);
				} else {
					left->mod.kind = TypeMod (pointer);
					typestack->value = ValueCategory (rvalue);
				}
			} else {
				if (left->kind == TypeKind (tag) && left->tag.type == TagType (enum)) {
					init_typestack (typestack);
					push_basictype_to_typestack (typestack, BasicType (int), 0);
				}
				typestack->value = ValueCategory (lvalue);
			}
		} else {
			struct type	type = {0};

			Assert (decl->kind == DeclKind (define));
			type.kind = TypeKind (internal);
			type.internal.decl = expr->iden.decl;
			push_typestack (typestack, &type);
			typestack->value = ValueCategory (rvalue);
		}
	} else if (expr->type == ExprType (typeinfo)) {
		struct type	type = {0};

		type.kind = TypeKind (tag);
		type.tag.type = TagType (struct);
		type.tag.name = "typeinfo";
		type.tag.decl = g_typeinfo_struct_decl;
		init_typestack (typestack);
		push_typestack (typestack, &type);
		typestack->value = ValueCategory (lvalue);
	} else if (expr->type == ExprType (string)) {
		init_typestack (typestack);
		push_const_char_pointer_to_typestack (typestack);
		typestack->value = ValueCategory (rvalue);
	} else if (expr->type == ExprType (macrocall)) {
		struct decl	*decl;

		init_typestack (typestack);
		Assert (expr->macrocall.instance);
		decl = get_decl (unit, expr->macrocall.instance);
		Assert (decl->type);
		push_typestack_recursive (unit, unit, typestack, decl->type);
	} else if (expr->type == ExprType (enum)) {
		init_typestack (typestack);
		/* todo: get basictype from enum constant */
		push_basictype_to_typestack (typestack, BasicType (int), 0);
		typestack->value = ValueCategory (rvalue);
	} else if (expr->type == ExprType (table)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (expr->table.decl) {
			if (expr->table.lib_index) {
				decl_unit = get_lib (expr->table.lib_index);
			} else {
				decl_unit = unit;
			}
			decl = get_decl (decl_unit, expr->table.decl);
			Assert (decl->type);
			init_typestack (typestack);
			push_typestack_recursive (unit, decl_unit, typestack, decl->type);
			Assert (get_typestack_head (typestack));
			push_pointer_type_to_typestack (typestack, 0);
			typestack->value = ValueCategory (rvalue);
		} else {
			init_typestack (typestack);
			push_const_char_pointer_to_typestack (typestack);
			push_pointer_type_to_typestack (typestack, 0);
			typestack->value = ValueCategory (rvalue);
		}
	} else {
		Unreachable ();
	}
}

void	update_typestack_recursive (struct unit *unit, uint expr_index, struct typestack *typestack) {
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (typesizeof) || expr->op.type == OpType (typealignof) || expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
			update_typestack (unit, expr, typestack, 0);
		} else if (is_expr_unary (expr)) {
			update_typestack_recursive (unit, expr->op.forward, typestack);
			update_typestack (unit, expr, typestack, 0);
		} else {
			struct typestack	rightstack = {0};

			update_typestack_recursive (unit, expr->op.backward, typestack);
			init_typestack (&rightstack);
			update_typestack_recursive (unit, expr->op.forward, &rightstack);
			update_typestack (unit, expr, typestack, &rightstack);
		}
	} else {
		update_typestack (unit, expr, typestack, 0);
	}
}



