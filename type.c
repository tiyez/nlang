

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

struct typestate {
	int		group_level;
	int		level;
	int		is_post_basic;
	uint	is_const : 1;
	uint	is_restrict : 1;
	uint	is_volatile : 1;
};

int		parse_type_rec (struct unit *unit, char **ptokens, int *out, struct typestate *state) {
	int		result;
	int		is_parse_next = 0;

	if ((*ptokens)[-1] == Token (punctuator)) {
		if (0 == strcmp (*ptokens, "*")) {
			struct type	*type;
			int			type_index;

			type_index = make_pointer_type (unit, 0);
			type = get_type (unit, type_index);
			type->mod.ptr.is_const = state->is_const;
			type->mod.ptr.is_volatile = state->is_volatile;
			type->mod.ptr.is_restrict = state->is_restrict;
			state->is_const = 0;
			state->is_volatile = 0;
			state->is_restrict = 0;
			*ptokens = next_token (*ptokens, 0);
			state->level += 1;
			result = parse_type_rec (unit, ptokens, out, state);
			if (result) {
				type = get_type (unit, type_index);
				state->level -= 1;
				Assert (*out);
				Assert (state->is_post_basic);
				type->mod.ptr.type = *out;
				*out = type_index;
				if (state->level == 0) {
					Debug ("parse next [%s]", *ptokens);
					result = parse_type_rec (unit, ptokens, out, state);
				} else {
					Debug ("level: %d", state->level);
				}
			}
		} else if (0 == strcmp (*ptokens, "[")) {
			if (state->is_post_basic) {
				int			expr = -1;

				*ptokens = next_token (*ptokens, 0);
				if (parse_expr (unit, ptokens, &expr)) {
					if (is_token (*ptokens, Token (punctuator), "]")) {
						*out = make_array_type (unit, *out, expr);
						result = 1;
						is_parse_next = 1;
					} else {
						Error ("unexpected token");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				/* it's not array, unexpected token */
				result = 0;
			}
		} else if (0 == strcmp (*ptokens, "(")) {
			if (state->is_post_basic) {
				int				scope_index;

				scope_index = make_scope (unit, ScopeKind (param), -1);
				*out = make_function_type (unit, *out, scope_index);
				/* function */
				result = 1;
				while (result && !is_token (*ptokens, Token (punctuator), ")")) {
					if (is_token (*ptokens, Token (punctuator), "(") || is_token (*ptokens, Token (punctuator), ",")) {
						*ptokens = next_token (*ptokens, 0);
						if ((*ptokens)[-1] == Token (identifier)) {
							const char	*name;
							int		type_index;

							name = *ptokens;
							*ptokens = next_token (*ptokens, 0);
							if (parse_type (unit, ptokens, &type_index)) {
								make_decl (unit, scope_index, name, type_index, DeclKind (const));
							} else {
								result = 0;
							}
						} else {
							Error ("unexpected token");
							result = 0;
						}
					} else {
						Error ("unexpected token");
						result = 0;
					}
				}
				if (result) {
					Assert (is_token (*ptokens, Token (punctuator), ")"));
					is_parse_next = 1;
				}
			} else {
				int		old_level = state->level;

				/* group */
				state->group_level += 1;
				state->level = 0;
				*ptokens = next_token (*ptokens, 0);
				result = parse_type_rec (unit, ptokens, out, state);
				if (result) {
					Assert (state->is_post_basic);
					if (is_token (*ptokens, Token (punctuator), ")")) {
						*ptokens = next_token (*ptokens, 0);
					} else {
						result = parse_type_rec (unit, ptokens, out, state);
						Assert (is_token (*ptokens, Token (punctuator), ")"));
						*ptokens = next_token (*ptokens, 0);
					}
					state->level = old_level;
					state->group_level -= 1;
				}
			}
		} else if (0 == strcmp (*ptokens, ")")) {
			/* must be closing group token */
			if (state->is_post_basic) {
				result = 1;
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			result = 1;
		}
	} else if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "const")) {
			state->is_const = 1;
			result = is_parse_next = 1;
		} else if (0 == strcmp (*ptokens, "restrict")) {
			state->is_restrict = 1;
			result = is_parse_next = 1;
		} else if (0 == strcmp (*ptokens, "volatile")) {
			state->is_volatile = 1;
			result = is_parse_next = 1;
		} else if (0 == strcmp (*ptokens, "typeof")) {
			int		expr_index;

			*ptokens = next_token (*ptokens, 0);
			if (parse_expr (unit, ptokens, &expr_index)) {
				*out = make_typeof_type (unit, expr_index);
				get_type (unit, *out)->typeof.is_const = state->is_const;
				result = 1;
			} else {
				Error ("cannot parse expr for typeof");
				result = 0;
			}
		} else {
			enum basictype	basictype;

			if (!state->is_post_basic) {
				if (0 == strcmp (*ptokens, "struct")) {
					*ptokens = next_token (*ptokens, 0);
					Assert ((*ptokens)[-1] == Token (identifier));
					*out = make_tag_type (unit, *ptokens, TagType (struct));
					get_type (unit, *out)->tag.is_const = state->is_const;
					result = 1;
				} else if (0 == strcmp (*ptokens, "enum")) {
					*ptokens = next_token (*ptokens, 0);
					Assert ((*ptokens)[-1] == Token (identifier));
					*out = make_tag_type (unit, *ptokens, TagType (enum));
					get_type (unit, *out)->tag.is_const = state->is_const;
					result = 1;
				} else if (0 == strcmp (*ptokens, "union")) {
					*ptokens = next_token (*ptokens, 0);
					Assert ((*ptokens)[-1] == Token (identifier));
					*out = make_tag_type (unit, *ptokens, TagType (union));
					get_type (unit, *out)->tag.is_const = state->is_const;
					result = 1;
				} else if (0 == strcmp (*ptokens, "stroke")) {
					*ptokens = next_token (*ptokens, 0);
					Assert ((*ptokens)[-1] == Token (identifier));
					*out = make_tag_type (unit, *ptokens, TagType (stroke));
					get_type (unit, *out)->tag.is_const = state->is_const;
					result = 1;
				} else if (0 == strcmp (*ptokens, "bitfield")) {
					*ptokens = next_token (*ptokens, 0);
					Assert ((*ptokens)[-1] == Token (identifier));
					*out = make_tag_type (unit, *ptokens, TagType (bitfield));
					get_type (unit, *out)->tag.is_const = state->is_const;
					result = 1;
				} else if (is_basictype (*ptokens, &basictype)) {
					*out = make_basic_type (unit, basictype);
					get_type (unit, *out)->basic.is_const = state->is_const;
					result = 1;
				} else {
					Error ("unexpected token [%s]", *ptokens);
					result = 0;
				}
				if (result) {
					if (state->level) {
						*ptokens = next_token (*ptokens, 0);
					} else {
						is_parse_next = 1;
					}
					state->is_post_basic = 1;
				}
			} else {
				result = 1;
			}
		}
	}
	if (result && is_parse_next) {
		*ptokens = next_token (*ptokens, 0);
		result = parse_type_rec (unit, ptokens, out, state);
	}
	return (result);
}

int		parse_type (struct unit *unit, char **ptokens, int *out) {
	struct typestate	state = {0};
	int					result;

	*out = -1;
	result = parse_type_rec (unit, ptokens, out, &state);
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
			if (type->tag.type == TagType (struct)) {
				fprintf (file, "struct %s", type->tag.name);
			} else if (type->tag.type == TagType (enum)) {
				fprintf (file, "enum %s", type->tag.name);
			} else if (type->tag.type == TagType (union)) {
				fprintf (file, "union %s", type->tag.name);
			} else if (type->tag.type == TagType (stroke)) {
				fprintf (file, "stroke %s", type->tag.name);
			} else if (type->tag.type == TagType (bitfield)) {
				fprintf (file, "bitfield %s", type->tag.name);
			} else {
				Unreachable ();
			}
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
					if (get_type (unit, type->mod.ptr.type)->kind == TypeKind (mod) && get_type (unit, type->mod.ptr.type)->mod.kind != TypeMod (pointer)) {
						fprintf (file, "(");
						print_type (unit, type->mod.ptr.type, file);
						fprintf (file, ")");
					} else {
						print_type (unit, type->mod.ptr.type, file);
					}
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

/*
struct typestack {
	struct type	types[Max_Type_Depth];
	int			types_count;
	int			head;
};

void	init_typestack (struct typestack *typestack) {
	typestack->types_count = 0;
	typestack->head = -1;
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

	if (typestack->head >= 0) {
		Assert (type->kind != TypeKind (mod) && type->kind != TypeKind (typeof));
		Assert (typestack->types_count == 0);
		typestack->types[typestack->types_count] = *type;
		typestack->head = typestack->types_count;
		typestack->types_count += 1;
		result = 1;
	} else {
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

int		make_typestack_from_expr (struct unit *unit, struct typestack *typestack, int expr_index) {
	int				result;
	struct expr		*expr;

	expr = get_expr (unit, expr_index);
	switch (expr->type) {
		case ExprType (op): {
			switch (expr->op.type) {
				case OpType (member_access): {

				} break ;
				case OpType (indirect_access): {

				} break ;
				default: {
					if (is_expr_unary (expr)) {
						if (make_typestack_from_expr (unit, typestack, expr->op.forward)) {
							switch (expr->op.type) {
								case OpType (unary_plus):
								case OpType (unary_minus):
								case OpType (bitwise_not):
								case OpType (function_call): {
									result = 1;
								} break ;

								case OpType (array_subscript): {
									struct typestack		cright_typestack, *right_typestack = &cright_typestack;

									init_typestack (right_typestack);
									if (make_typestack_from_expr (unit, right_typestack, expr->op.backward)) {
										struct type		*head = get_typestack_head (typestack);
										struct type		*right = get_typestack_head (right_typestack);

										if (head->kind == TypeKind (mod) && (head->mod.kind == TypeMod (pointer) || head->mod.kind == TypeMod (array))) {
											pop_typestack (typestack);
										} else if (right->kind == TypeKind (mod) && (right->mod.kind == TypeMod (pointer) || right->mod.kind == TypeMod (array))) {
											pop_typestack (right_typestack);
											*typestack = *right_typestack;
										} else {
											Error ("no pointer or array type operand for array subscript op");
											result = 0;
										}
									}
								} break ;
								case OpType (logical_not): {
									empty_typestack (typestack);
									push_typestack (typestack, &g_basictypes[BasicType (int)]);
								} break ;
								case OpType (indirect): {
									struct type	*head;

									head = get_typestack_head (typestack);
									if (head->kind == TypeKind (mod) && (head->mod.kind == TypeMod (pointer) || head->mod.kind == TypeMod (array))) {
										pop_typestack (typestack);
									} else {
										Error ("invalid operand type for indirection op");
										result = 0;
									}
								} break ;
								case OpType (cast): {
									empty_typestack (typestack);
									push_typestack_rec (typestack, get_type (unit, expr->op.backward));
								} break ;
								case OpType (address_of): {
									struct type		*head;

									head = get_typestack_head (typestack);
									if (head->kind == TypeKind (mod) && head->mod.kind == TypeMod (array)) {
										int		forward;

										forward = head->mod.array.type;
										head->mod.kind = TypeMod (pointer);
										head->mod.ptr.type = forward;
									} else {
										struct type		ptrtype = {0};

										ptrtype.kind = TypeKind (mod);
										ptrtype.mod.kind = TypeMod (pointer);
										push_typestack (typestack, &ptrtype);
									}
								} break ;
								default: Assert (0);
							}
						} else {
							result = 0;
						}
					} else {
						if (make_typestack_from_expr (unit, typestack, expr->op.backward)) {
							struct typestack		cright_typestack, *right_typestack = &cright_typestack;

							init_typestack (right_typestack);
							if (make_typestack_from_expr (unit, right_typestack, expr->op.forward)) {

							}
						}
					}
				} break ;
			}
		} break ;
	}
	return (result);
}
*/




