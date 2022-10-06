


struct type	*get_type (struct unit *unit, uint index) {
	struct type	*type;

	Assert (Is_Bucket_Index_Valid (unit->buckets->types, index));
	type = Get_Bucket_Element (unit->buckets->types, index);
	return (type);
}

uint	get_type_index (struct unit *unit, struct type *type) {
	return (Get_Bucket_Element_Index (unit->buckets->types, type));
}

uint	make_pointer_type (struct unit *unit, uint subtype, int is_const) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (pointer);
		type->mod.forward = subtype;
		type->flags.is_const = is_const;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		Error ("cannot prepare array for types");
		index = 0;
	}
	return (index);
}

uint	make_array_type (struct unit *unit, uint subtype, uint expr) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (array);
		type->mod.forward = subtype;
		type->mod.expr = expr;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_function_type (struct unit *unit, uint rettype, uint param_scope) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (mod);
		type->mod.kind = TypeMod (function);
		type->mod.forward = rettype;
		type->mod.param_scope = param_scope;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_basic_type (struct unit *unit, enum basictype basictype, int is_const) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (basic);
		type->basic.type = basictype;
		type->flags.is_const = is_const;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_tag_type (struct unit *unit, const char *name, enum tagtype tagtype) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (tag);
		type->tag.type = tagtype;
		type->tag.name = name;
		type->tag.decl = 0;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

int		is_opaque_tag_decl (struct decl *decl) {
	return (0 == strcmp (decl->name, "_"));
}

uint	make_typeof_type (struct unit *unit, uint expr_index) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (typeof);
		type->typeof.expr = expr_index;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_deftype_type (struct unit *unit, const char *name) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (deftype);
		type->deftype.name = name;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
	}
	return (index);
}

uint	make_internal_type (struct unit *unit, uint decl) {
	uint	index;

	if (Prepare_Bucket (unit->buckets->types, 1)) {
		struct type	*type;

		type = Push_Bucket (unit->buckets->types);
		type->kind = TypeKind (internal);
		type->internal.decl = decl;
		index = Get_Bucket_Element_Index (unit->buckets->types, type);
	} else {
		index = 0;
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

	if (type->kind == TypeKind (mod) && !type->flags.is_group) {
		result = 1 + (type->mod.kind == TypeMod (pointer));
	} else {
		result = 0;
	}
	return (result);
}

uint	add_type_to_tree (struct unit *unit, uint type_index, struct type *type, uint head_index, struct type *head) {
	if (get_type_precedence (type) < get_type_precedence (head)) {
		Assert (head->kind == TypeKind (mod));
		if (head->mod.forward) {
			head->mod.forward = add_type_to_tree (unit, type_index, type, head->mod.forward, get_type (unit, head->mod.forward));
		} else {
			head->mod.forward = type_index;
		}
	} else {
		Assert (type->kind == TypeKind (mod));
		type->mod.forward = head_index;
		head_index = type_index;
	}
	return (head_index);
}

void	set_type_cvr (struct unit *unit, uint type_index, struct typestate *state) {
	struct type	*type;

	type = get_type (unit, type_index);
	type->flags.is_const = state->is_const;
	type->flags.is_volatile = state->is_volatile;
	type->flags.is_restrict = state->is_restrict;
	state->is_const = 0;
	state->is_volatile = 0;
	state->is_restrict = 0;
}

int		parse_function_param_scope (struct unit *unit, char **ptokens, uint *out_scope, int is_vaarg) {
	int		is_continue = 1;
	int		result;
	int		line;

	line = unit->pos.line;
	*out_scope = make_scope (unit, ScopeKind (param), 0);
	result = 1;
	do {
		*ptokens = next_token (*ptokens, &unit->pos);
		if ((*ptokens)[-1] == Token (identifier)) {
			const char	*name;
			uint		type_index;

			name = *ptokens;
			*ptokens = next_token (*ptokens, &unit->pos);
			if (0 == strcmp (name, "void") && is_token (*ptokens, Token (punctuator), ")")) {
				if (!get_scope (unit, *out_scope)->decl_begin) {
					is_continue = 0;
					result = 1;
				} else {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				}
			} else if (parse_type (unit, ptokens, &type_index)) {
				make_param_decl (unit, *out_scope, name, type_index, line);
				if (is_token (*ptokens, Token (punctuator), ")")) {
					is_continue = 0;
				} else if (!is_token (*ptokens, Token (punctuator), ",")) {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else if (is_token (*ptokens, Token (punctuator), ")")) {
			is_continue = 0;
		} else if (is_vaarg && is_token (*ptokens, Token (punctuator), "...")) {
			make_param_decl (unit, *out_scope, "...", 0, line);
			*ptokens = next_token (*ptokens, &unit->pos);
			if (is_token (*ptokens, Token (punctuator), ")")) {
				is_continue = 0;
			} else {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			}
		} else {
			Parse_Error (*ptokens, unit->pos, "unexpected token");
			result = 0;
		}
	} while (result && is_continue);
	if (result) {
		Assert (is_token (*ptokens, Token (punctuator), ")"));
		*ptokens = next_token (*ptokens, &unit->pos);
	}
	return (result);
}

int		parse_type (struct unit *unit, char **ptokens, uint *out);

int		parse_type_ (struct unit *unit, char **ptokens, uint *phead, struct typestate *state) {
	int		result;

	if ((*ptokens)[-1] == Token (punctuator)) {
		if (state->is_post_basic) {
			if (0 == strcmp (*ptokens, "(")) {
				uint	func_index;
				uint	scope_index;

				if (parse_function_param_scope (unit, ptokens, &scope_index, 1)) {
					func_index = make_function_type (unit, 0, scope_index);
					set_type_cvr (unit, func_index, state);
					Assert (*phead);
					*phead = add_type_to_tree (unit, func_index, get_type (unit, func_index), *phead, get_type (unit, *phead));
				} else {
					result = 0;
				}
			} else if (0 == strcmp (*ptokens, "[")) {
				uint	expr;

				expr = 0;
				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_expr (unit, ptokens, &expr)) {
					if (is_token (*ptokens, Token (punctuator), "]")) {
						int		array_index;

						*ptokens = next_token (*ptokens, &unit->pos);
						array_index = make_array_type (unit, 0, expr);
						set_type_cvr (unit, array_index, state);
						Assert (*phead);
						*phead = add_type_to_tree (unit, array_index, get_type (unit, array_index), *phead, get_type (unit, *phead));
						result = 1;
					} else if (is_active_buckets (unit)) {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					} else {
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
				uint	pointer_index;

				pointer_index = make_pointer_type (unit, 0, 0);
				set_type_cvr (unit, pointer_index, state);
				if (*phead) {
					*phead = add_type_to_tree (unit, pointer_index, get_type (unit, pointer_index), *phead, get_type (unit, *phead));
				} else {
					*phead = pointer_index;
				}
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else if (0 == strcmp (*ptokens, "(")) {
				uint	inner_head;

				*ptokens = next_token (*ptokens, &unit->pos);
				if (parse_type (unit, ptokens, &inner_head)) {
					if (is_token (*ptokens, Token (punctuator), ")")) {
						if (inner_head) {
							*ptokens = next_token (*ptokens, &unit->pos);
							state->is_post_basic = 1;
							set_type_cvr (unit, inner_head, state);
							get_type (unit, inner_head)->flags.is_group = 1;
							if (*phead) {
								*phead = add_type_to_tree (unit, inner_head, get_type (unit, inner_head), *phead, get_type (unit, *phead));
							} else {
								*phead = inner_head;
							}
							result = 1;
						} else if (is_active_buckets (unit)) {
							Parse_Error (*ptokens, unit->pos, "empty type group");
							result = 0;
						} else {
							result = 0;
						}
					} else if (is_active_buckets (unit)) {
						Parse_Error (*ptokens, unit->pos, "unexpected token");
						result = 0;
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			} else if (is_active_buckets (unit)) {
				Parse_Error (*ptokens, unit->pos, "unexpected token");
				result = 0;
			} else {
				result = 0;
			}
		}
	} else if (!state->is_post_basic && (*ptokens)[-1] == Token (identifier)) {
		uint	basic_index;

		basic_index = 0;
		if (0 == strcmp (*ptokens, "const")) {
			state->is_const = 1;
			*ptokens = next_token (*ptokens, &unit->pos);
			result = 1;
		} else if (0 == strcmp (*ptokens, "restrict")) {
			state->is_restrict = 1;
			*ptokens = next_token (*ptokens, &unit->pos);
			result = 1;
		} else if (0 == strcmp (*ptokens, "volatile")) {
			state->is_volatile = 1;
			*ptokens = next_token (*ptokens, &unit->pos);
			result = 1;
		} else if (0 == strcmp (*ptokens, "typeof")) {
			uint	expr_index;
			uint	typeof_index;

			*ptokens = next_token (*ptokens, &unit->pos);
			if (parse_expr (unit, ptokens, &expr_index)) {
				basic_index = make_typeof_type (unit, expr_index);
				result = 1;
			} else {
				result = 0;
			}
		} else {
			enum basictype	basictype;
			enum tagtype	tagtype;

			if (is_tagtype (*ptokens, &tagtype)) {
				*ptokens = next_token (*ptokens, &unit->pos);
				if ((*ptokens)[-1] == Token (identifier)) {
					basic_index = make_tag_type (unit, *ptokens, tagtype);
					*ptokens = next_token (*ptokens, &unit->pos);
					result = 1;
				} else if (is_active_buckets (unit)) {
					Parse_Error (*ptokens, unit->pos, "unexpected token");
					result = 0;
				} else {
					result = 0;
				}
			} else if (is_basictype (*ptokens, &basictype)) {
				basic_index = make_basic_type (unit, basictype, 0);
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			} else {
				basic_index = make_deftype_type (unit, *ptokens);
				*ptokens = next_token (*ptokens, &unit->pos);
				result = 1;
			}
		}
		if (result && basic_index) {
			state->is_post_basic = 1;
			set_type_cvr (unit, basic_index, state);
			if (*phead) {
				*phead = add_type_to_tree (unit, basic_index, get_type (unit, basic_index), *phead, get_type (unit, *phead));
			} else {
				*phead = basic_index;
			}
		}
	} else if (state->is_post_basic) {
		state->missing_token = 1;
		result = 1;
	} else if (is_active_buckets (unit)) {
		Parse_Error (*ptokens, unit->pos, "unexpected token");
		result = 0;
	} else {
		result = 0;
	}
	return (result);
}

int		parse_type (struct unit *unit, char **ptokens, uint *out) {
	struct typestate	state = {0};
	int					result;

	*out = 0;
	state.missing_token = 0;
	result = 1;
	while (result && !state.missing_token) {
		result = parse_type_ (unit, ptokens, out, &state);
	}
	if (result && *out) {
		if (get_type (unit, *out)->flags.is_group) {
			if (is_active_buckets (unit)) {
				Parse_Error (*ptokens, unit->pos, "root grouping of type is forbidden");
				result = 0;
			} else {
				result = 0;
			}
		} else {
			result = 1;
		}
	}
	return (result);
}

int		try_to_parse_type (struct unit *unit, char *tokens) {
	int				result;
	struct position	pos;
	uint			index;
	struct buckets	*buckets;

	buckets = unit->buckets;
	unit->buckets = &unit->temp_buckets;
	pos = unit->pos;
	result = parse_type (unit, &tokens, &index);
	unit->pos = pos;
	unit->buckets = buckets;
	if (buckets != &unit->temp_buckets) {
		clear_buckets (&unit->temp_buckets);
	}
	return (result);
}

int		try_to_parse_type_ended_by_token (struct unit *unit, char *tokens, int token_type, const char *token_value) {
	int				result;
	struct position	pos;
	uint			index;
	struct buckets	*buckets;

	buckets = unit->buckets;
	unit->buckets = &unit->temp_buckets;
	pos = unit->pos;
	result = parse_type (unit, &tokens, &index);
	unit->pos = pos;
	unit->buckets = buckets;
	if (buckets != &unit->temp_buckets) {
		clear_buckets (&unit->temp_buckets);
	}
	if (result) {
		result = (tokens[-1] == token_type && 0 == strcmp (tokens, token_value));
	}
	return (result);
}

void	print_type_gen (struct unit *unit, void *owner, uint head, FILE *file, struct type *(*get_type) (void *owner, uint index)) {
	struct type	*type;
	int			is_grouped;

	Assert (head);
	type = get_type (owner, head);
	is_grouped = type->flags.is_group;
	if (is_grouped) {
		fprintf (file, "(");
	}
	if (type->kind == TypeKind (basic)) {
		if (type->flags.is_const) {
			fprintf (file, "const ");
		}
		fprintf (file, "%s", get_basictype_name (type->basic.type));
	} else if (type->kind == TypeKind (tag)) {
		if (type->flags.is_const) {
			fprintf (file, "const ");
		}
		fprintf (file, "%s %s", g_tagname[type->tag.type], type->tag.name);
	} else if (type->kind == TypeKind (typeof)) {
		if (type->flags.is_const) {
			fprintf (file, "const ");
		}
		if (type->typeof.expr) {
			fprintf (file, "typeof ");
			print_expr (unit, type->typeof.expr, file);
		} else {
			fprintf (file, "(typeof ())");
		}
	} else if (type->kind == TypeKind (deftype)) {
		if (type->flags.is_const) {
			fprintf (file, "const ");
		}
		fprintf (file, type->deftype.name);
	} else if (type->kind == TypeKind (opaque)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		if (type->flags.is_const) {
			fprintf (file, "const ");
		}
		if (is_lib_index (type->opaque.decl)) {
			decl_unit = get_lib (get_lib_index (type->opaque.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (type->opaque.decl));
		Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (opaque));
		fprintf (file, decl->name);
	} else if (type->kind == TypeKind (internal)) {
		if (type->internal.decl) {
			fprintf (file, "%s", get_decl (unit, type->internal.decl)->name);
		} else {
			fprintf (file, "<internal>");
		}
	} else if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (pointer)) {
			if (type->flags.is_const) {
				fprintf (file, "const ");
			}
			if (type->flags.is_volatile) {
				fprintf (file, "volatile ");
			}
			if (type->flags.is_restrict) {
				fprintf (file, "restrict ");
			}
			fprintf (file, "*");
			Assert (type->mod.forward);
			print_type_gen (unit, owner, type->mod.forward, file, get_type);
		} else if (type->mod.kind == TypeMod (array)) {
			print_type_gen (unit, owner, type->mod.forward, file, get_type);
			if (type->mod.expr) {
				fprintf (file, "[");
				print_expr (unit, type->mod.expr, file);
				fprintf (file, "]");
			} else {
				fprintf (file, "[]");
			}
		} else if (type->mod.kind == TypeMod (function)) {
			print_type_gen (unit, owner, type->mod.forward, file, get_type);
			if (type->mod.param_scope) {
				struct scope	*scope;
				struct decl		*decl;

				fprintf (file, " (");
				scope = get_scope (unit, type->mod.param_scope);
				decl = scope->decl_begin < 0 ? 0 : get_decl (unit, scope->decl_begin);
				while (decl) {
					if (decl->type) {
						fprintf (file, "%s ", decl->name);
						print_type_gen (unit, owner, decl->type, file, get_type);
					} else {
						Assert (0 == strcmp (decl->name, "..."));
						Assert (decl->next < 0);
						fprintf (file, "...");
					}
					if (decl->next) {
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
		} else {
			Unreachable ();
		}
	} else {
		Unreachable ();
	}
	if (is_grouped) {
		fprintf (file, ")");
	}
}

void	print_type (struct unit *unit, uint head, FILE *file) {
	print_type_gen (unit, unit, head, file, get_type);
}

struct type	*get_typestack_type_gen (struct typestack *typestack, uint index);

void	print_typestack (struct unit *unit, struct typestack *typestack, FILE *file) {
	if (typestack->types_count >= 0) {
		print_type_gen (unit, typestack, typestack->head + 1, file, get_typestack_type_gen);
	}
}

void	print_type_tree (struct unit *unit, int head, FILE *file) {
	struct type	*type;

	Assert (head);
	type = get_type (unit, head);
	if (type->flags.is_group) {
		fprintf (file, "(");
	}
	if (type->kind == TypeKind (basic)) {
		fprintf (file, "%s", get_basictype_name (type->basic.type));
	} else if (type->kind == TypeKind (tag)) {
		fprintf (file, "%s %s", g_tagname[type->tag.type], type->tag.name);
	} else if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (pointer)) {
			fprintf (file, "* -> ");
			print_type_tree (unit, type->mod.forward, file);
		} else if (type->mod.kind == TypeMod (array)) {
			fprintf (file, "[] -> ");
			print_type_tree (unit, type->mod.forward, file);
		} else if (type->mod.kind == TypeMod (function)) {
			fprintf (file, "() -> ");
			print_type_tree (unit, type->mod.forward, file);
		} else {
			Unreachable ();
		}
	} else if (type->kind == TypeKind (typeof)) {
		fprintf (file, "typeof");
	} else if (type->kind == TypeKind (internal)) {
		fprintf (file, "internal");
	} else {
		Unreachable ();
	}
	if (type->flags.is_group) {
		fprintf (file, ")");
	}
}

int		is_functype_returnable (struct unit *unit, uint type_index) {
	struct type	*type;

	Assert (type_index);
	type = get_type (unit, type_index);
	Assert (type->kind == TypeKind (mod));
	Assert (type->mod.kind == TypeMod (function));
	Assert (type->mod.forward);
	type = get_type (unit, type->mod.forward);
	return (!(type->kind == TypeKind (basic) && type->basic.type == BasicType (void)));
}

void	get_cvr (struct type *type, int cvr[3]) {
	cvr[0] = type->flags.is_const;
	cvr[1] = type->flags.is_volatile;
	cvr[2] = type->flags.is_restrict;
}

int		is_same_cvr (struct type *left, struct type *right) {
	int		left_cvr[3], right_cvr[3];

	get_cvr (left, left_cvr);
	get_cvr (right, right_cvr);
	return (0 == memcmp (left_cvr, right_cvr, sizeof left_cvr));
}

int		is_type_integral (struct type *type) {
	return ((type->kind == TypeKind (basic) && is_basictype_integral (type->basic.type)) || (type->kind == TypeKind (tag) && type->tag.type == TagType (enum)));
}

int		is_pointer_type (struct type *type, int is_sizeof_context) {
	return (type->kind == TypeKind (mod) && (type->mod.kind == TypeMod (pointer) || (is_sizeof_context && type->mod.kind == TypeMod (array))));
}









