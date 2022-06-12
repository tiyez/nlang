
int		allocate_typeinfo (struct unit *unit, enum typeinfo_kind kind, int type_decl_index) {
	int		index;

	if (Prepare_Array (unit->typeinfos, 1)) {
		struct typeinfo	*typeinfo;

		typeinfo = Push_Array (unit->typeinfos);
		typeinfo->count = 1;
		typeinfo->kind = kind;
		typeinfo->typeinfo = -1;
		typeinfo->members = -1;
		typeinfo->size = -1;
		typeinfo->type_decl_index = type_decl_index;
		index = Get_Element_Index (unit->typeinfos, typeinfo);
	} else {
		Error ("cannot prepare typeinfos array");
		index = -1;
	}
	return (index);
}

struct typeinfo	*get_typeinfo (struct unit *unit, int index) {
	struct typeinfo	*typeinfo;

	Assert (index >= 0 && index < Get_Array_Count (unit->typeinfos));
	typeinfo = unit->typeinfos + index;
	return (typeinfo);
}

int		get_typeinfo_index (struct unit *unit, struct typeinfo *typeinfo) {
	return (Get_Element_Index (unit->typeinfos, typeinfo));
}

int		make_typeinfo_basic (struct unit *unit, enum basictype basic, int type_index) {
	int		index;

	index = allocate_typeinfo (unit, TypeInfo_Kind (basic), type_index);
	if (index >= 0) {
		struct typeinfo	*typeinfo;

		typeinfo = get_typeinfo (unit, index);
		typeinfo->basic = basic;
		typeinfo->size = get_basictype_size (basic);
	}
	return (index);
}

int		make_typeinfo_tag (struct unit *unit, enum tagtype tagtype, int decl_index) {
	int					index;
	enum typeinfo_kind	kind;

	switch (tagtype) {
		case TagType (struct): kind = TypeInfo_Kind (struct); break ;
		case TagType (enum): kind = TypeInfo_Kind (enum); break ;
		case TagType (union): kind = TypeInfo_Kind (union); break ;
		default: Unreachable ();
	}
	index = allocate_typeinfo (unit, kind, decl_index);
	return (index);
}

int		make_typeinfo_mod (struct unit *unit, enum typemod mod, int type_index) {
	int					index;
	enum typeinfo_kind	kind;

	switch (mod) {
		case TypeMod (pointer): kind = TypeInfo_Kind (pointer); break ;
		case TypeMod (array): kind = TypeInfo_Kind (array); break ;
		case TypeMod (function): kind = TypeInfo_Kind (function); break ;
		default: Unreachable ();
	}
	index = allocate_typeinfo (unit, kind, type_index);
	return (index);
}

int		make_typemember (struct unit *unit, int decl_index) {
	int		index;

	if (Prepare_Array (unit->typemembers, 1)) {
		struct typemember	*member;

		member = Push_Array (unit->typemembers);
		member->typeinfo = -1;
		member->decl_index = decl_index;
		index = Get_Element_Index (unit->typemembers, member);
	} else {
		index = -1;
	}
	return (index);
}

struct typemember	*get_typemember (struct unit *unit, int index) {
	Assert (index >= 0 && index < Get_Array_Count (unit->typemembers));
	return (unit->typemembers + index);
}

int		get_typemember_index (struct unit *unit, struct typemember *member) {
	return (Get_Element_Index (unit->typemembers, member));
}

int		get_number_of_typemembers (struct unit *unit, int index) {
	int		count;

	count = 0;
	Assert (index >= 0 && index < Get_Array_Count (unit->typemembers));
	while (unit->typemembers[index + count].decl_index >= 0) {
		count += 1;
	}
	Assert (index + count >= 0 && index + count < Get_Array_Count (unit->typemembers));
	return (count);
}

int		link_typeinfo_scope (struct unit *unit, int scope_index, int *out) {
	int				result;
	struct scope	*scope;

	*out = -1;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl_member;

		decl_member = get_decl (unit, scope->decl_begin);
		do {
			int		member_typeinfo;
			int		member_index;

			member_index = make_typemember (unit, get_decl_index (unit, decl_member));
			if (*out < 0) {
				*out = member_index;
			}
			if (decl_member->type >= 0) {
				if (link_typeinfo (unit, decl_member->type, &member_typeinfo)) {
					struct typemember	*member;

					member = get_typemember (unit, member_index);
					member->name = decl_member->name;
					member->typeinfo = member_typeinfo;
					result = 1;
				} else {
					result = 0;
				}
			} else {
				struct typemember	*member;

				member = get_typemember (unit, member_index);
				member->name = decl_member->name;
				result = 1;
			}
			if (decl_member->next >= 0) {
				decl_member = get_decl (unit, decl_member->next);
			} else {
				decl_member = 0;
			}
		} while (result && decl_member);
		if (result) {
			make_typemember (unit, -1);
		}
	} else {
		result = 1;
	}
	return (result);
}

int		link_typeinfo (struct unit *unit, int type_index, int *out) {
	int			result;
	struct type	*type;

	*out = -1;
	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (basic): {
			*out = make_typeinfo_basic (unit, type->basic.type, type_index);
			result = 1;
		} break ;
		case TypeKind (mod): {
			int		typeinfo_index, inner_typeinfo_index;

			typeinfo_index = make_typeinfo_mod (unit, type->mod.kind, type_index);
			if (link_typeinfo (unit, *get_type_mod_forward_ptr (type), &inner_typeinfo_index)) {
				struct typeinfo	*typeinfo;

				typeinfo = get_typeinfo (unit, typeinfo_index);
				typeinfo->typeinfo = inner_typeinfo_index;
				if (type->mod.kind == TypeMod (array)) {
					typeinfo->size = type->mod.array.expr;
					result = 1;
				} else if (type->mod.kind == TypeMod (function)) {
					int		member_table;

					result = link_typeinfo_scope (unit, type->mod.func.param_scope, &member_table);
					if (result) {
						typeinfo = get_typeinfo (unit, typeinfo_index);
						typeinfo->members = member_table;
					}
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} break ;
		case TypeKind (group): {
			result = link_typeinfo (unit, type->group.type, out);
		} break ;
		case TypeKind (typeof): {
			Unreachable ();
		} break ;
		case TypeKind (tag):
		case TypeKind (decl): {
			int		decl_index;
			struct decl	*decl;

			if (type->kind == TypeKind (tag)) {
				decl_index = type->tag.decl;
			} else if (type->kind == TypeKind (decl)) {
				decl_index = type->decl.index;
			} else {
				Unreachable ();
			}
			decl = get_decl (unit, decl_index);
			Assert (decl->kind == DeclKind (tag));
			*out = make_typeinfo_tag (unit, decl->tag.type, decl_index);
			if (decl->tag.member_table < 0) {
				result = link_typeinfo_scope (unit, decl->tag.scope, &decl->tag.member_table);
			} else {
				result = 1;
			}
			if (result) {
				struct typeinfo	*typeinfo;

				typeinfo = get_typeinfo (unit, *out);
				typeinfo->members = decl->tag.member_table;
				typeinfo->tagname = decl->name;
			}
		} break ;
	}
	return (result);
}
