

struct type	g_basictypes[] = {
	{ .kind = TypeKind (basic), .basic.type = BasicType (void), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (char), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (uchar), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (wchar), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (byte), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (ubyte), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (short), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (ushort), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (int), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (uint), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (size), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (usize), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (float), },
	{ .kind = TypeKind (basic), .basic.type = BasicType (double), },
};

struct type	*get_type (struct unit *unit, int index) {
	struct type	*type;

	Assert (index >= 0 && index < Get_Array_Count (unit->types));
	type = unit->types + index;
	return (type);
}

int		get_type_index (struct unit *unit, struct type *type) {
	return (Get_Element_Index (unit->types, type));
}

int		make_pointer_type (struct unit *unit, int subtype) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (pointer);
		type->mod.ptr.type = subtype;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_array_type (struct unit *unit, int subtype, int expr) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (array);
		type->mod.array.type = subtype;
		type->mod.array.expr = expr;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_function_type (struct unit *unit, int rettype, int param_scope) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (function);
		type->mod.func.type = rettype;
		type->mod.func.param_scope = param_scope;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_basic_type (struct unit *unit, enum basictype basictype) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (basic);
		type->basic.type = basictype;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_group_type (struct unit *unit, int type_index) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (group);
		type->group.type = type_index;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_tag_type (struct unit *unit, const char *name, enum tagtype tagtype) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (tag);
		type->tag.type = tagtype;
		type->tag.name = name;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_typeof_type (struct unit *unit, int expr_index) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (typeof);
		type->typeof.expr = expr_index;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_decl_type (struct unit *unit, int decl_index) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (decl);
		type->decl.index = decl_index;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

int		make_accessor_type (struct unit *unit, enum tagtype tagtype, const char *tagname) {
	int		index;

	if (Prepare_Array (unit->types, 1)) {
		struct type	*type;

		type = Push_Array (unit->types);
		type->kind = TypeKind (accessor);
		type->accessor.tagtype = tagtype;
		type->accessor.tagname = tagname;
		index = Get_Element_Index (unit->types, type);
	} else {
		index = -1;
	}
	return (index);
}

struct typestate {
	int		group_level;
	int		level;
	int		is_post_basic;
	int		missing_token;
	uint	is_const : 1;
	uint	is_restrict : 1;
	uint	is_volatile : 1;
};

int		get_type_precedence (struct type *type) {
	int		result;

	if (type->kind == TypeKind (mod)) {
		result = 1 + (type->mod.kind == TypeMod (pointer));
	} else {
		result = 0;
	}
	return (result);
}

int		*get_type_mod_forward_ptr (struct type *type) {
	int		*result;

	switch (type->mod.kind) {
		case TypeMod (pointer): result = &type->mod.ptr.type; break ;
		case TypeMod (function): result = &type->mod.func.type; break ;
		case TypeMod (array): result = &type->mod.array.type; break ;
		default: Unreachable ();
	}
	return (result);
}

int		add_type_to_tree (struct unit *unit, int type_index, struct type *type, int head_index, struct type *head) {
	if (get_type_precedence (type) < get_type_precedence (head)) {
		int		*pforward_index;

		Assert (head->kind == TypeKind (mod));
		pforward_index = get_type_mod_forward_ptr (head);
		if (*pforward_index >= 0) {
			*pforward_index = add_type_to_tree (unit, type_index, type, *pforward_index, get_type (unit, *pforward_index));
		} else {
			*pforward_index = type_index;
		}
	} else {
		Assert (type->kind == TypeKind (mod));
		*get_type_mod_forward_ptr (type) = head_index;
		head_index = type_index;
	}
	return (head_index);
}

void	set_type_cvr (struct unit *unit, int type_index, struct typestate *state) {
	struct type	*type;

	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (basic): {
			type->basic.is_const = state->is_const;
			type->basic.is_volatile = state->is_volatile;
		} break ;
		case TypeKind (group): {
		} break ;
		case TypeKind (tag): {
			type->tag.is_const = state->is_const;
			type->tag.is_volatile = state->is_volatile;
		} break ;
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): {
					type->mod.ptr.is_const = state->is_const;
					type->mod.ptr.is_volatile = state->is_volatile;
					type->mod.ptr.is_restrict = state->is_restrict;
				} break ;
				case TypeMod (array): {
					type->mod.ptr.is_const = state->is_const;
					type->mod.ptr.is_volatile = state->is_volatile;
					type->mod.ptr.is_restrict = state->is_restrict;
				} break ;
				case TypeMod (function): {
				} break ;
			}
		} break ;
		case TypeKind (typeof): {
			type->typeof.is_const = state->is_const;
			type->typeof.is_volatile = state->is_volatile;
		} break ;
		case TypeKind (decl): {
			type->typeof.is_const = state->is_const;
			type->typeof.is_volatile = state->is_volatile;
		} break ;
		default: Unreachable ();
	}
	state->is_const = 0;
	state->is_volatile = 0;
	state->is_restrict = 0;
}

int		parse_type (struct unit *unit, char **ptokens, int *out);

int		parse_type_ (struct unit *unit, char **ptokens, int *phead, struct typestate *state) {
	int		result;

	if ((*ptokens)[-1] == Token (punctuator)) {
		if (state->is_post_basic) {
			if (0 == strcmp (*ptokens, "(")) {
				int				scope_index;
				int				is_continue = 1;
				int				func_index;

				scope_index = make_scope (unit, ScopeKind (param), -1);
				func_index = make_function_type (unit, -1, scope_index);
				set_type_cvr (unit, func_index, state);
				result = 1;
				do {
					*ptokens = next_token (*ptokens, 0);
					if ((*ptokens)[-1] == Token (identifier)) {
						const char	*name;
						int		type_index;

						name = *ptokens;
						*ptokens = next_token (*ptokens, 0);
						if (parse_type (unit, ptokens, &type_index)) {
							make_decl (unit, scope_index, name, type_index, DeclKind (param));
							if (is_token (*ptokens, Token (punctuator), ")")) {
								is_continue = 0;
							} else if (!is_token (*ptokens, Token (punctuator), ",")) {
								Error ("unexpected token");
								result = 0;
							} else {
								result = 1;
							}
						} else {
							result = 0;
						}
					} else if (is_token (*ptokens, Token (punctuator), ")")) {
						is_continue = 0;
					} else {
						Error ("unexpected token %s", *ptokens);
						result = 0;
					}
				} while (result && is_continue);
				if (result) {
					Assert (is_token (*ptokens, Token (punctuator), ")"));
					Assert (*phead >= 0);
					*phead = add_type_to_tree (unit, func_index, get_type (unit, func_index), *phead, get_type (unit, *phead));
					*ptokens = next_token (*ptokens, 0);
				}
			} else if (0 == strcmp (*ptokens, "[")) {
				int			expr = -1;

				*ptokens = next_token (*ptokens, 0);
				if (parse_expr (unit, ptokens, &expr)) {
					if (is_token (*ptokens, Token (punctuator), "]")) {
						int		array_index;

						*ptokens = next_token (*ptokens, 0);
						array_index = make_array_type (unit, -1, expr);
						set_type_cvr (unit, array_index, state);
						Assert (*phead >= 0);
						*phead = add_type_to_tree (unit, array_index, get_type (unit, array_index), *phead, get_type (unit, *phead));
						result = 1;
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				state->missing_token = 1;
				result = 1;
			}
		} else {
			if (0 == strcmp (*ptokens, "*")) {
				int		pointer_index;

				pointer_index = make_pointer_type (unit, -1);
				set_type_cvr (unit, pointer_index, state);
				if (*phead >= 0) {
					*phead = add_type_to_tree (unit, pointer_index, get_type (unit, pointer_index), *phead, get_type (unit, *phead));
				} else {
					*phead = pointer_index;
				}
				*ptokens = next_token (*ptokens, 0);
				result = 1;
			} else if (0 == strcmp (*ptokens, "(")) {
				int		group_index, inner_head;

				group_index = make_group_type (unit, -1);
				set_type_cvr (unit, group_index, state);
				*ptokens = next_token (*ptokens, 0);
				if (parse_type (unit, ptokens, &inner_head)) {
					if (is_token (*ptokens, Token (punctuator), ")")) {
						if (inner_head >= 0) {
							*ptokens = next_token (*ptokens, 0);
							state->is_post_basic = 1;
							get_type (unit, group_index)->group.type = inner_head;
							if (*phead >= 0) {
								*phead = add_type_to_tree (unit, group_index, get_type (unit, group_index), *phead, get_type (unit, *phead));
							} else {
								*phead = group_index;
							}
							result = 1;
						} else {
							Error ("empty type group");
							result = 0;
						}
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Error ("unexpected token");
				result = 0;
			}
		}
	} else if (!state->is_post_basic && (*ptokens)[-1] == Token (identifier)) {
		int		basic_index;

		basic_index = -1;
		if (0 == strcmp (*ptokens, "const")) {
			state->is_const = 1;
			result = 1;
		} else if (0 == strcmp (*ptokens, "restrict")) {
			state->is_restrict = 1;
			result = 1;
		} else if (0 == strcmp (*ptokens, "volatile")) {
			state->is_volatile = 1;
			result = 1;
		} else if (0 == strcmp (*ptokens, "typeof")) {
			int		expr_index;
			int		typeof_index;

			*ptokens = next_token (*ptokens, 0);
			if (parse_expr (unit, ptokens, &expr_index)) {
				basic_index = make_typeof_type (unit, expr_index);
				result = 1;
			} else {
				Error ("cannot parse expr for typeof");
				result = 0;
			}
		} else {
			enum basictype	basictype;
			enum tagtype	tagtype;

			if (is_tagtype (*ptokens, &tagtype)) {
				*ptokens = next_token (*ptokens, 0);
				if ((*ptokens)[-1] == Token (identifier)) {
					basic_index = make_tag_type (unit, *ptokens, tagtype);
					result = 1;
				} else {
					Error ("unexpected token");
					result = 0;
				}
			} else if (is_basictype (*ptokens, &basictype)) {
				basic_index = make_basic_type (unit, basictype);
				result = 1;
			} else {
				Error ("unexpected token [%s]", *ptokens);
				result = 0;
			}
		}
		if (result && basic_index >= 0) {
			state->is_post_basic = 1;
			set_type_cvr (unit, basic_index, state);
			if (*phead >= 0) {
				*phead = add_type_to_tree (unit, basic_index, get_type (unit, basic_index), *phead, get_type (unit, *phead));
			} else {
				*phead = basic_index;
			}
		}
		if (result) {
			*ptokens = next_token (*ptokens, 0);
		}
	} else if (state->is_post_basic) {
		state->missing_token = 1;
		result = 1;
	} else {
		Error ("unexpected token");
		result = 0;
	}
	return (result);
}

int		parse_type (struct unit *unit, char **ptokens, int *out) {
	struct typestate	state = {0};
	int					result;

	*out = -1;
	state.missing_token = 0;
	result = 1;
	while (result && !state.missing_token) {
		result = parse_type_ (unit, ptokens, out, &state);
	}
	return (result);

}

void	print_type (struct unit *unit, int head, FILE *file) {
	struct type	*type;

	Assert (head >= 0);
	type = get_type (unit, head);
	switch (type->kind) {
		case TypeKind (basic): {
			if (type->basic.is_const) {
				fprintf (file, "const ");
			}
			fprintf (file, "%s", get_basictype_name (type->basic.type));
		} break ;
		case TypeKind (tag): {
			if (type->tag.is_const) {
				fprintf (file, "const ");
			}
			fprintf (file, "%s %s", g_tagname[type->tag.type], type->tag.name);
		} break ;
		case TypeKind (typeof): {
			if (type->typeof.is_const) {
				fprintf (file, "const ");
			}
			if (type->typeof.expr >= 0) {
				fprintf (file, "typeof ");
				print_expr (unit, type->typeof.expr, file);
			} else {
				fprintf (file, "(typeof ())");
			}
		} break ;
		case TypeKind (decl): {
			struct decl	*decl;

			decl = get_decl (unit, type->decl.index);
			print_type (unit, decl->type, file);
		} break ;
		case TypeKind (group): {
			fprintf (file, "(");
			print_type (unit, type->group.type, file);
			fprintf (file, ")");
		} break ;
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): {
					if (type->mod.ptr.is_const) {
						fprintf (file, "const ");
					}
					if (type->mod.ptr.is_volatile) {
						fprintf (file, "volatile ");
					}
					if (type->mod.ptr.is_restrict) {
						fprintf (file, "restrict ");
					}
					fprintf (file, "*");
					Assert (type->mod.ptr.type);
					print_type (unit, type->mod.ptr.type, file);
				} break ;
				case TypeMod (array): {
					print_type (unit, type->mod.array.type, file);
					if (type->mod.array.expr >= 0) {
						fprintf (file, "[");
						print_expr (unit, type->mod.array.expr, file);
						fprintf (file, "]");
					} else {
						fprintf (file, "[]");
					}
				} break ;
				case TypeMod (function): {
					print_type (unit, type->mod.func.type, file);
					if (type->mod.func.param_scope >= 0) {
						struct scope	*scope;
						struct decl		*decl;

						fprintf (file, " (");
						scope = get_scope (unit, type->mod.func.param_scope);
						decl = scope->decl_begin < 0 ? 0 : get_decl (unit, scope->decl_begin);
						while (decl) {
							fprintf (file, "%s ", decl->name);
							print_type (unit, decl->type, file);
							if (decl->next >= 0) {
								fprintf (file, ", ");
								decl = get_decl (unit, decl->next);
							} else {
								decl = 0;
							}
						}
						fprintf (file, ")");
					} else {
						fprintf (file, " ()");
					}
				} break ;
			}
		} break ;
	}
}

void	print_type_tree (struct unit *unit, int head, FILE *file) {
	struct type	*type;

	Assert (head >= 0);
	type = get_type (unit, head);
	switch (type->kind) {
		case TypeKind (basic): {
			fprintf (file, "%s", get_basictype_name (type->basic.type));
		} break ;
		case TypeKind (group): {
			fprintf (file, "(");
			print_type_tree (unit, type->group.type, file);
			fprintf (file, ")");
		} break ;
		case TypeKind (tag): {
			fprintf (file, "%s %s", g_tagname[type->tag.type], type->tag.name);
		} break ;
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): {
					fprintf (file, "* -> ");
					print_type_tree (unit, type->mod.ptr.type, file);
				} break ;
				case TypeMod (array): {
					fprintf (file, "[] -> ");
					print_type_tree (unit, type->mod.array.type, file);
				} break ;
				case TypeMod (function): {
					fprintf (file, "() -> ");
					print_type_tree (unit, type->mod.func.type, file);
				} break ;
				default: Unreachable ();
			}
		} break ;
		case TypeKind (typeof): {
			fprintf (file, "typeof");
		} break ;
		case TypeKind (decl): {
			fprintf (file, "decl");
		} break ;
	}
}

int		is_functype_returnable (struct unit *unit, int type_index) {
	struct type	*type;

	Assert (type_index >= 0);
	type = get_type (unit, type_index);
	Assert (type->kind == TypeKind (mod));
	Assert (type->mod.kind == TypeMod (function));
	Assert (type->mod.func.type >= 0);
	type = get_type (unit, type->mod.func.type);
	while (type->kind == TypeKind (group)) {
		type = get_type (unit, type->group.type);
	}
	return (!(type->kind == TypeKind (basic) && type->basic.type == BasicType (void)));
}




