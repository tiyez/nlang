
int		allocate_typeinfo (struct unit *unit, enum typeinfo_kind kind, uint type_decl_index) {
	int		index;

	if (Prepare_Bucket (unit->typeinfos, 1)) {
		struct typeinfo	*typeinfo;

		typeinfo = Push_Bucket (unit->typeinfos);
		typeinfo->count = 1;
		typeinfo->kind = kind;
		typeinfo->typeinfo = 0;
		typeinfo->members = 0;
		typeinfo->size = 0;
		typeinfo->type_decl_index = type_decl_index;
		index = Get_Bucket_Element_Index (unit->typeinfos, typeinfo);
	} else {
		Error ("cannot prepare typeinfos array");
		index = 0;
	}
	return (index);
}

struct typeinfo	*get_typeinfo (struct unit *unit, uint index) {
	struct typeinfo	*typeinfo;

	Assert (Is_Bucket_Index_Valid (unit->typeinfos, index));
	typeinfo = Get_Bucket_Element (unit->typeinfos, index);
	return (typeinfo);
}

uint	get_typeinfo_index (struct unit *unit, struct typeinfo *typeinfo) {
	return (Get_Bucket_Element_Index (unit->typeinfos, typeinfo));
}

uint	make_typeinfo_basic (struct unit *unit, enum basictype basic, uint type_index) {
	uint	index;

	index = allocate_typeinfo (unit, TypeInfo_Kind (basic), type_index);
	if (index) {
		struct typeinfo	*typeinfo;

		typeinfo = get_typeinfo (unit, index);
		typeinfo->basic = basic;
		typeinfo->size = get_basictype_size (basic);
	}
	return (index);
}

uint	make_typeinfo_tag (struct unit *unit, enum tagtype tagtype, uint decl_index) {
	uint				index;
	enum typeinfo_kind	kind;

	if (tagtype == TagType (struct)) {
		kind = TypeInfo_Kind (struct);
	} else if (tagtype == TagType (enum)) {
		kind = TypeInfo_Kind (enum);
	} else if (tagtype == TagType (union)) {
		kind = TypeInfo_Kind (union);
	} else {
		Unreachable ();
	}
	index = allocate_typeinfo (unit, kind, decl_index);
	return (index);
}

uint	make_typeinfo_mod (struct unit *unit, enum typemod mod, uint type_index) {
	uint				index;
	enum typeinfo_kind	kind;

	if (mod == TypeMod (pointer)) {
		kind = TypeInfo_Kind (pointer);
	} else if (mod == TypeMod (array)) {
		kind = TypeInfo_Kind (array);
	} else if (mod == TypeMod (function)) {
		kind = TypeInfo_Kind (function);
	} else {
		Unreachable ();
	}
	index = allocate_typeinfo (unit, kind, type_index);
	return (index);
}

uint	make_typemember (struct unit *unit, uint decl_index) {
	uint	index;

	if (Prepare_Bucket (unit->typemembers, 1)) {
		struct typemember	*member;

		member = Push_Bucket (unit->typemembers);
		member->typeinfo = 0;
		member->decl_index = decl_index;
		index = Get_Bucket_Element_Index (unit->typemembers, member);
	} else {
		index = 0;
	}
	return (index);
}

struct typemember	*get_typemember (struct unit *unit, uint index) {
	Assert (Is_Bucket_Index_Valid (unit->typemembers, index));
	return (Get_Bucket_Element (unit->typemembers, index));
}

uint	get_typemember_index (struct unit *unit, struct typemember *member) {
	return (Get_Bucket_Element_Index (unit->typemembers, member));
}

int		get_number_of_typemembers (struct unit *unit, uint index) {
	int		count;

	count = 0;
	Assert (Is_Bucket_Index_Valid (unit->typemembers, index));
	while (index && Get_Bucket_Element (unit->typemembers, index)->decl_index) {
		count += 1;
		index = Get_Next_Bucket_Index (unit->typemembers, index);
	}
	Assert (index);
	return (count);
}

int		link_typeinfo_scope (struct unit *unit, uint scope_index, uint *out) {
	int				result;
	struct scope	*scope;

	*out = 0;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		struct decl	*decl_member;

		decl_member = get_decl (unit, scope->decl_begin);
		do {
			uint	member_index;

			member_index = make_typemember (unit, get_decl_index (unit, decl_member));
			if (!*out) {
				*out = member_index;
			}
			if (decl_member->type) {
				struct typemember	*member;

				member = get_typemember (unit, member_index);
				member->name = decl_member->name;
				member->typeinfo = decl_member->type;
				member->value = 0;
				member->offset = -1;
				result = 1;
			} else if (scope->kind == ScopeKind (param)) {
				struct typemember	*member;

				member = get_typemember (unit, member_index);
				member->name = decl_member->name;
				member->typeinfo = decl_member->type;
				member->value = 0;
				member->offset = 0;
				result = 1;
			} else if (scope->kind == ScopeKind (tag) && scope->tagtype != TagType (enum)) {
				struct typemember	*member;

				Assert (scope->kind == ScopeKind (tag));
				Assert (scope->tagtype == TagType (struct) || scope->tagtype == TagType (union));
				member = get_typemember (unit, member_index);
				member->name = 0;
				Assert (decl_member->block.scope);
				member->typeinfo = decl_member->block.scope;
				member->value = 0;
				member->offset = 0;
				result = 1;
			} else {
				struct typemember	*member;

				Assert (scope->kind == ScopeKind (tag));
				Assert (scope->tagtype == TagType (enum));
				member = get_typemember (unit, member_index);
				member->name = decl_member->name;
				member->value = decl_member->enumt.expr;
				member->offset = 0;
				result = 1;
			}
			if (decl_member->next) {
				decl_member = get_decl (unit, decl_member->next);
			} else {
				decl_member = 0;
			}
		} while (result && decl_member);
		if (result) {
			uint	member_index;
			int		overvalue = 0;

			make_typemember (unit, 0);
			member_index = *out;
			while (result && member_index) {
				struct typemember	*member;

				member = get_typemember (unit, member_index);
				if (member->name ? 1 : member->typeinfo) {
					struct evalvalue	value = {0};

					if (scope->kind == ScopeKind (tag)) {
						if (scope->tagtype == TagType (enum)) {
							if (member->value) {
								if (eval_const_expr (unit, member->value, &value)) {
									if (value.type == EvalType (basic) && is_basictype_integral (value.basic)) {
										member->value = value.value;
										member->offset = 0;
										overvalue = member->value + 1;
										result = 1;
									} else {
										Link_Error (unit, "non-integral value for enum constant");
										result = 0;
									}
								} else {
									result = 0;
								}
							} else {
								member->value = overvalue;
								member->offset = 0;
								overvalue += 1;
								result = 1;
							}
						} else if (scope->tagtype == TagType (struct) || scope->tagtype == TagType (union)) {
							if (member->name) {
								uint	typeinfo_index;

								if (link_typeinfo (unit, member->typeinfo, &typeinfo_index)) {
									get_typemember (unit, member_index)->typeinfo = typeinfo_index;
									result = 1;
								} else {
									result = 0;
								}
							} else {
								uint	typeinfo_index;

								Assert (member->typeinfo);
								if (link_typeinfo_scope (unit, member->typeinfo, &typeinfo_index)) {
									get_typemember (unit, member_index)->typeinfo = typeinfo_index;
									result = 1;
								} else {
									result = 0;
								}
							}
						} else {
							Unreachable ();
						}
					} else if (scope->kind == ScopeKind (param)) {
						uint	typeinfo_index;

						if (member->typeinfo) {
							if (link_typeinfo (unit, member->typeinfo, &typeinfo_index)) {
								get_typemember (unit, member_index)->typeinfo = typeinfo_index;
								result = 1;
							} else {
								result = 0;
							}
						} else {
							result = 1;
						}
					} else {
						Unreachable ();
					}
					member_index = Get_Next_Bucket_Index (unit->typemembers, member_index);
				} else {
					member_index = 0;
				}
			}
		}
	} else {
		result = 1;
	}
	return (result);
}

int		link_typeinfo (struct unit *unit, uint type_index, uint *out) {
	int			result;
	struct type	*type;

	*out = 0;
	type = get_type (unit, type_index);
	if (type->kind == TypeKind (basic)) {
		struct typeinfo	*typeinfo;

		*out = make_typeinfo_basic (unit, type->basic.type, type_index);
		typeinfo = get_typeinfo (unit, *out);
		typeinfo->qualifiers = 0;
		typeinfo->qualifiers |= (type->flags.is_const ? TypeQualifier (const) : 0);
		typeinfo->qualifiers |= (type->flags.is_volatile ? TypeQualifier (volatile) : 0);
		result = 1;
	} else if (type->kind == TypeKind (mod)) {
		uint	inner_typeinfo_index;

		*out = make_typeinfo_mod (unit, type->mod.kind, type_index);
		if (link_typeinfo (unit, type->mod.forward, &inner_typeinfo_index)) {
			struct typeinfo	*typeinfo;

			typeinfo = get_typeinfo (unit, *out);
			typeinfo->typeinfo = inner_typeinfo_index;
			if (type->mod.kind == TypeMod (array)) {
				typeinfo->size = type->mod.expr;
				typeinfo->qualifiers = 0;
				typeinfo->qualifiers |= (type->flags.is_const ? TypeQualifier (const) : 0);
				typeinfo->qualifiers |= (type->flags.is_volatile ? TypeQualifier (volatile) : 0);
				typeinfo->qualifiers |= (type->flags.is_restrict ? TypeQualifier (restrict) : 0);
				result = 1;
			} else if (type->mod.kind == TypeMod (pointer)) {
				typeinfo->qualifiers = 0;
				typeinfo->qualifiers |= (type->flags.is_const ? TypeQualifier (const) : 0);
				typeinfo->qualifiers |= (type->flags.is_volatile ? TypeQualifier (volatile) : 0);
				typeinfo->qualifiers |= (type->flags.is_restrict ? TypeQualifier (restrict) : 0);
				result = 1;
			} else if (type->mod.kind == TypeMod (function)) {
				uint	member_table;

				result = link_typeinfo_scope (unit, type->mod.param_scope, &member_table);
				if (result) {
					typeinfo = get_typeinfo (unit, *out);
					typeinfo->members = member_table;
				}
			} else {
				Unreachable ();
			}
		} else {
			result = 0;
		}
	} else if (type->kind == TypeKind (typeof)) {
		Unreachable ();
	} else if (type->kind == TypeKind (tag)) {
		uint		decl_index;
		struct decl	*decl;

		Assert (type->tag.decl);
		decl_index = type->tag.decl;
		decl = get_decl (unit, decl_index);
		Assert (decl->kind == DeclKind (tag));
		*out = make_typeinfo_tag (unit, decl->tag.type, decl_index);
		if (!decl->tag.member_table) {
			result = link_typeinfo_scope (unit, decl->tag.scope, &decl->tag.member_table);
		} else {
			result = 1;
		}
		if (result) {
			struct typeinfo	*typeinfo;

			typeinfo = get_typeinfo (unit, *out);
			typeinfo->members = decl->tag.member_table;
			typeinfo->tagname = decl->name;
			typeinfo->qualifiers = 0;
			typeinfo->qualifiers |= (type->flags.is_const ? TypeQualifier (const) : 0);
			typeinfo->qualifiers |= (type->flags.is_volatile ? TypeQualifier (volatile) : 0);
		}
	} else {
		Unreachable ();
	}
	return (result);
}


