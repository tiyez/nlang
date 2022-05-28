

void	init_typestack (struct typestack *typestack) {
	typestack->types_count = 0;
	typestack->head = -1;
	typestack->is_lvalue = 0;
}

struct type	*get_typestack_type (struct typestack *typestack, int index) {
	return (index >= 0 ? typestack->types + index : 0);
}

struct type	*get_typestack_head (struct typestack *typestack) {
	return (get_typestack_type (typestack, typestack->head));
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
		typestack->types[typestack->types_count] = *type;
		type = typestack->types + typestack->types_count;
		switch (type->mod.kind) {
			case TypeMod (pointer): type->mod.ptr.type = typestack->head; break ;
			case TypeMod (function): type->mod.func.type = typestack->head; break ;
			case TypeMod (array): type->mod.array.type = typestack->head; break ;
			default: Unreachable ();
		}
		typestack->head = typestack->types_count;
		typestack->types_count += 1;
		result = 1;
	}
	return (result);
}

int		push_typestack_recursive (struct unit *unit, struct typestack *typestack, int type_index) {
	int			result;
	struct type	*type;

	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (accessor):
		case TypeKind (tag):
		case TypeKind (basic): {
			result = push_typestack (typestack, type);
		} break ;
		case TypeKind (group): {
			result = push_typestack_recursive (unit, typestack, type->group.type);
		} break ;
		case TypeKind (mod): {
			if (push_typestack_recursive (unit, typestack, *get_type_mod_forward_ptr (type))) {
				result = push_typestack (typestack, type);
			} else {
				result = 0;
			}
		} break ;
		case TypeKind (decl): {
			struct decl	*decl;

			decl = get_decl (unit, type->decl.index);
			if (decl->type >= 0) {
				result = push_typestack_recursive (unit, typestack, decl->type);
			} else {
				Error ("decl doesn't have a type");
				result = 0;
			}
		} break ;
		case TypeKind (typeof):
		default: Unreachable ();
	}
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
	switch (type->kind) {
		case TypeKind (tag):
		case TypeKind (basic): {
			result = 1;
		} break ;
		case TypeKind (mod): {
			int		child_type_index;

			child_type_index = make_basic_type (unit, BasicType (void));
			Assert (child_type_index >= 0);
			switch (type->mod.kind) {
				case TypeMod (pointer): type->mod.ptr.type = child_type_index; break ;
				case TypeMod (array): type->mod.array.type = child_type_index; break ;
				case TypeMod (function): type->mod.func.type = child_type_index; break ;
				default: Unreachable ();
			}
			pop_typestack (typestack);
			result = insert_typestack_to_type (unit, child_type_index, typestack);
		} break ;
		case TypeKind (typeof):
		case TypeKind (decl):
		default: Unreachable ();
	}
	return (result);
}








