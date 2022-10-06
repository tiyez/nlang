
/*
	TODO: proper self reference check
*/


uint64	find_ordinary_decl_local (struct unit *unit, uint scope_index, const char *name) {
	uint64			decl_index;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->kind == ScopeKind (unit)) {
		decl_index = 0;
	} else {
		int				stop;

		stop = 0;
		decl_index = scope->decl_begin;
		while (!stop && decl_index) {
			struct decl	*decl;

			decl = get_decl (unit, decl_index);
			if (get_decl_index (unit, decl) <= unit->link_decl_index) {
				if (is_ordinal_decl (decl) && decl->name && 0 == strcmp (name, decl->name)) {
					stop = 1;
				} else {
					decl_index = decl->next;
					stop = 0;
				}
			} else {
				decl_index = 0;
				stop = 1;
			}
		}
		if (!decl_index && scope->param_scope) {
			decl_index = find_ordinary_decl_local (unit, scope->param_scope, name);
		}
		if (!decl_index && scope->parent_scope) {
			decl_index = find_ordinary_decl_local (unit, scope->parent_scope, name);
		}
	}
	return (decl_index);
}

uint64	find_ordinary_decl_global (struct unit *unit, const char *name) {
	uint64	decl_index;

	decl_index = find_decl_in_table (unit, name, TagType (invalid), 0);
	return (decl_index);
}

uint64	find_ordinary_decl (struct unit *unit, uint scope_index, const char *name) {
	uint64	decl_index;

	decl_index = find_ordinary_decl_local (unit, scope_index, name);
	if (!decl_index) {
		decl_index = find_ordinary_decl_global (unit, name);
	}
	return (decl_index);
}

uint64	find_decl_type (struct unit *unit, uint scope_index, const char *name, enum tagtype tagtype, int is_typedef) {
	uint64			decl_index;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->kind == ScopeKind (unit)) {
		decl_index = find_decl_in_table (unit, name, tagtype, is_typedef);
	} else {
		int				stop;

		stop = 0;
		decl_index = scope->decl_begin;
		while (!stop && decl_index) {
			struct decl	*decl;

			decl = get_decl (unit, decl_index);
			if (get_decl_index (unit, decl) <= unit->link_decl_index) {
				if (((decl->kind == DeclKind (tag) && decl->tag.type == tagtype) || (is_typedef && decl->kind == DeclKind (define) && decl->define.kind == DefineKind (type))) && 0 == strcmp (name, decl->name)) {
					stop = 1;
				} else {
					decl_index = decl->next;
					stop = 0;
				}
			} else {
				decl_index = 0;
				stop = 0;
			}
		}
		if (!decl_index && scope->parent_scope) {
			decl_index = find_decl_type (unit, scope->parent_scope, name, tagtype, is_typedef);
		}
	}
	return (decl_index);
}

uint	find_struct_member (struct unit *unit, uint scope_index, const char *iden) {
	uint			decl_index;
	struct scope	*scope;
	int				stop;

	stop = 0;
	scope = get_scope (unit, scope_index);
	decl_index = scope->decl_begin;
	while (!stop && decl_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (decl->kind == DeclKind (block)) {
			uint	decl_ind;

			decl_ind = find_struct_member (unit, decl->block.scope, iden);
			if (decl_ind) {
				decl_index = decl_ind;
				stop = 1;
			} else {
				decl_index = decl->next;
				stop = 0;
			}
		} else {
			if (0 == strcmp (decl->name, iden)) {
				stop = 1;
			} else {
				decl_index = decl->next;
				stop = 0;
			}
		}
	}
	return (decl_index);
}

int		link_type (struct unit *unit, uint scope_index, uint type_index, int is_selfref_check);

uint	find_enum_by_name (struct unit *unit, uint scope_index, const char *name) {
	uint			decl_index;
	struct scope	*scope;
	int				stop;

	stop = 0;
	scope = get_scope (unit, scope_index);
	decl_index = scope->decl_begin;
	while (!stop && decl_index) {
		struct decl	*decl;
		const char	*token;

		decl = get_decl (unit, decl_index);
		token = decl->name;
		do {
			if (0 == strcmp (name, token)) {
				Assert (decl->kind == DeclKind (enum));
				stop = 1;
			} else {
				token = next_const_token (token, 0);
				stop = 0;
			}
		} while (!stop && token[-1] == Token (identifier));
		if (!stop) {
			decl_index = decl->next;
		}
	}
	return (decl_index);
}

int		link_enum_expr (struct unit *unit, uint64 decl_index, uint expr_index) {
	int			result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	Assert (expr->type == ExprType (funcparam));
	if (expr->funcparam.expr) {
		if (!expr->funcparam.next) {
			expr = get_expr (unit, expr->funcparam.expr);
			if (expr->type == ExprType (identifier)) {
				struct decl	*decl;
				struct unit	*decl_unit;

				if (is_lib_index (decl_index)) {
					decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl_index));
				} else {
					decl_unit = unit;
				}
				decl = get_decl (decl_unit, unlib_index (decl_index));
				Assert (decl->kind == DeclKind (define));
				Assert (decl->define.kind == DefineKind (accessor));
				Assert (decl->define.accessor.decl);
				if (is_lib_index (decl->define.accessor.decl)) {
					decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl->define.accessor.decl));
				}
				decl = get_decl (decl_unit, unlib_index (decl->define.accessor.decl));
				Assert (decl->kind == DeclKind (tag));
				if (decl->tag.type == TagType (struct)) {
					Todo ();
				} else if (decl->tag.type == TagType (enum)) {
					decl_index = find_enum_by_name (decl_unit, decl->tag.scope, expr->iden.name);
					if (decl_index) {
						if (decl_unit != unit) {
							expr->iden.decl = make_lib_index (Get_Bucket_Element_Index (g_libs, decl_unit), decl_index);
						} else {
							expr->iden.decl = decl_index;
						}
						result = 1;
					} else {
						Link_Error (unit, "cannot find '%s' in the enum %s", expr->iden.name, decl->name);
						result = 0;
					}
				} else {
					Unreachable ();
				}
			} else {
				Link_Error (unit, "invalid accessor parameter %s", g_exprtype[expr->type]);
				result = 0;
			}
		} else {
			Link_Error (unit, "accessor call must have only one parameter %u", expr->funcparam.next);
			result = 0;
		}
	} else {
		Link_Error (unit, "empty accessor call");
		result = 0;
	}
	return (result);
}

int		link_expr (struct unit *unit, uint scope_index, uint expr_index, int is_selfref_check, struct typestack *typestack);

int		is_same_type (struct typestack *leftstack, struct typestack *rightstack, int is_void_acceptable) {
	int			result;
	struct type	*left, *right;

	left = get_typestack_head (leftstack);
	right = get_typestack_head (rightstack);
	if (left->kind == right->kind) {
		if (left->kind == TypeKind (basic)) {
			if (left->basic.type == right->basic.type) {
				result = 1;
			} else if (is_void_acceptable && (left->basic.type == BasicType (void) || right->basic.type == BasicType (void))) {
				result = 1;
			} else {
				result = 0;
			}
		} else if (left->kind == TypeKind (tag)) {
			result = (left->tag.type == right->tag.type && 0 == strcmp (left->tag.name, right->tag.name));
		} else if (left->kind == TypeKind (mod)) {
			struct type	lefttype, righttype;

			lefttype = *left;
			righttype = *right;
			pop_typestack (leftstack);
			pop_typestack (rightstack);
			result = is_same_type (leftstack, rightstack, is_void_acceptable);
			push_typestack (leftstack, &lefttype);
			push_typestack (rightstack, &righttype);
		} else if (left->kind == TypeKind (internal)) {
			result = left->internal.decl == right->internal.decl;
		} else if (left->kind == TypeKind (opaque)) {
			result = left->opaque.decl == right->opaque.decl;
		} else {
			Unreachable ();
		}
	} else {
		result = 0;
	}
	return (result);
}

int		is_implicit_castable (struct unit *unit, struct typestack *leftstack, struct typestack *rightstack, int overwrite_right) {
	int			result;
	struct type	*left;
	struct type	*right;

	left = get_typestack_head (leftstack);
	right = get_typestack_head (rightstack);
	Assert (left);
	Assert (right);
	if (right->kind == TypeKind (internal) || left->kind == TypeKind (internal)) {
		Error ("internal type cast is undefined");
		result = 0;
	} else if (left->kind == TypeKind (basic)) {
		if (right->kind == TypeKind (basic)) {
			enum basictype	common;

			if (BasicType (void) != (common = get_common_arithmetic_basictype (right->basic.type, left->basic.type))) {
				if (overwrite_right) {
					right->basic.type = common;
				}
				result = 1;
			} else {
				Error ("cannot cast %s to %s", get_basictype_name (right->basic.type), get_basictype_name (left->basic.type));
			}
		} else if (right->kind == TypeKind (tag)) {
			Error ("cannot cast %s to %s %s", get_basictype_name (right->basic.type), g_tagname[left->tag.type], left->tag.name);
			result = 0;
		} else if (right->kind == TypeKind (mod)) {
			if (right->mod.kind == TypeMod (pointer)) {
				if (is_basictype_integral (left->basic.type)) {
					result = 1;
				} else {
					Error ("cannot use non-integral type for pointer stuff");
					result = 0;
				}
			} else {
				Error ("cannot implicit cast %s type to basic", g_typemod[right->mod.kind]);
				result = 0;
			}
		} else {
			Unreachable ();
		}
	} else if (left->kind == TypeKind (tag)) {
		if (right->kind == TypeKind (tag)) {
			if (left->tag.type == right->tag.type) {
				if (0 == strcmp (left->tag.name, right->tag.name)) {
					result = 1;
				} else {
					Error ("tag name mismatch");
					result = 0;
				}
			} else {
				Error ("tag type mismatch");
				result = 0;
			}
		} else {
			Error ("cannot implicit cast non-tag to tag type");
			result = 0;
		}
	} else if (left->kind == TypeKind (mod)) {
		if (right->kind == TypeKind (mod)) {
			if (right->mod.kind == TypeMod (array) && left->mod.kind == TypeMod (pointer)) {
				struct type	lefttype, righttype;

				lefttype = *left;
				righttype = *right;
				pop_typestack (leftstack);
				pop_typestack (rightstack);
				if (get_typestack_head (leftstack)->kind == TypeKind (basic) && get_typestack_head (leftstack)->basic.type == BasicType (void)) {
					result = 1;
				} else {
					result = is_same_type (leftstack, rightstack, 0);
				}
				push_typestack (leftstack, &lefttype);
				push_typestack (rightstack, &righttype);
			} else if (left->mod.kind == TypeMod (function)) {
				Error ("illegal function type");
				result = 0;
			} else if (right->mod.kind == TypeMod (function)) {
				Error ("illegal function type");
				result = 0;
			} else if (left->mod.kind == right->mod.kind && left->mod.kind == TypeMod (pointer)) {
				if (is_same_type (leftstack, rightstack, 1)) {
					result = 1;
				} else {
					Error ("type mismatch");
					result = 0;
				}
			} else {
				if (is_same_type (leftstack, rightstack, 0)) {
					result = 1;
				} else {
					Error ("type mismatch");
					result = 0;
				}
			}
		} else if (right->kind == TypeKind (basic)) {
			if (is_basictype_integral (right->basic.type)) {
				result = 1;
			} else {
				Error ("cannot use non-integral type for pointer stuff");
				result = 0;
			}
		} else {
			Error ("cannot implicit cast type");
			result = 0;
		}
	} else {
		Unreachable ();
	}
	if (!result) {
		FILE	*file = stderr;

		fprintf (file, "\nleft type: ");
		print_typestack (unit, leftstack, file);
		fprintf (file, "\nright type: ");
		print_typestack (unit, rightstack, file);
		fprintf (file, "\n\n");
	}
	return (result);
}

int		link_funcparams (struct unit *unit, uint scope_index, uint64 param_scope_index, uint expr_index) {
	int				result;
	struct scope	*param_scope;
	struct expr		*expr;
	struct unit		*decl_unit;

	if (is_lib_index (param_scope_index)) {
		decl_unit = get_lib (get_lib_index (param_scope_index));
	} else {
		decl_unit = unit;
	}
	param_scope = get_scope (decl_unit, unlib_index (param_scope_index));
	if (param_scope->decl_begin) {
		if (expr_index) {
			struct decl	*decl;
			int			is_continue = 1;

			decl = get_decl (decl_unit, param_scope->decl_begin);
			do {
				push_decl_path (unit, get_decl_gindex (unit, decl_unit, decl));
				if (expr_index && (decl->type || 0 == strcmp (decl->name, "..."))) {
					struct typestack	ctypestack, *typestack = &ctypestack;

					expr = get_expr (unit, expr_index);
					if (expr->type == ExprType (macroparam)) {
						uint	copy_expr;
						struct decl	*decl;

						Assert (!expr->macroparam.expr);
						decl = get_decl (unit, expr->macroparam.decl);
						Assert (decl->kind == DeclKind (param));
						Assert (decl->param.expr);
						copy_expr = make_expr_copy (unit, unit, decl->param.expr, scope_index);
						Assert (copy_expr);
						*expr = *get_expr (unit, copy_expr);
					}
					Assert (expr->type == ExprType (funcparam));
					Assert (expr->funcparam.expr);
					push_expr_path (unit, expr->funcparam.expr);
					init_typestack (typestack, 0);
					if (link_expr (unit, scope_index, expr->funcparam.expr, 0, typestack)) {
						if (decl->type) {
							struct typestack	leftstack;

							init_typestack (&leftstack, 0);
							push_typestack_recursive (unit, decl_unit, &leftstack, decl->type);
							if (get_typestack_head (&leftstack)->kind == TypeKind (mod) && get_typestack_head (&leftstack)->mod.kind == TypeMod (array)) {
								get_typestack_head (&leftstack)->mod.kind = TypeMod (pointer);
							}
							if (get_typestack_head (typestack)->kind == TypeKind (basic)) {
								get_typestack_head (typestack)->basic.type = get_promoted_basictype (get_typestack_head (typestack)->basic.type);
							}
							if (is_typestacks_compatible (&leftstack, typestack)) {
								expr_index = expr->funcparam.next;
								result = 1;
							} else {
								Link_Error (unit, "parameter type mismatch");
								print_left_right_typestacks (unit, &leftstack, typestack);
								Debug ("is sizeof context: %d", typestack->is_sizeof_context);
								result = 0;
							}
						} else {
							expr_index = expr->funcparam.next;
							result = 1;
						}
					} else {
						result = 0;
					}
					pop_path (unit);
				} else if (!decl->type && 0 == strcmp (decl->name, "...")) {
					result = 1;
					is_continue = 0;
				} else {
					Link_Error (unit, "too few arguments");
					result = 0;
				}
				pop_path (unit);
				if (result) {
					if (decl->next) {
						decl = get_decl (decl_unit, decl->next);
					} else if (!decl->type && 0 == strcmp (decl->name, "...")) {
						result = 1;
					} else {
						decl = 0;
					}
				}
			} while (result && decl && is_continue);
			if (result && expr_index) {
				Link_Error (unit, "too many arguments");
				result = 0;
			}
		} else {
			Link_Error (unit, "too few arguments");
			result = 0;
		}
	} else if (expr_index) {
		Link_Error (unit, "too many arguments");
		result = 0;
	} else {
		result = 1;
	}
	return (result);
}

int		link_member_access (struct unit *unit, uint64 decl_index, uint expr_index, struct typestack *typestack) {
	int			result;
	struct decl	*decl;
	struct expr	*expr;
	struct unit	*decl_unit;

	if (is_lib_index (decl_index)) {
		decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl_index));
	} else {
		decl_unit = unit;
	}
	decl = get_decl (decl_unit, unlib_index (decl_index));
	Assert (decl->kind == DeclKind (tag));
	if (decl->tag.type == TagType (struct)) {
		expr = get_expr (unit, expr_index);
		if (expr->type == ExprType (identifier)) {
			uint	member_decl;

			member_decl = find_struct_member (decl_unit, decl->tag.scope, expr->iden.name);
			if (member_decl) {
				decl = get_decl (decl_unit, member_decl);
				if (decl->type) {
					result = push_typestack_recursive (unit, decl_unit, typestack, decl->type);
					if (decl_unit != unit) {
						expr->iden.decl = make_lib_index (get_lib_index (decl_index), member_decl);
					} else {
						expr->iden.decl = member_decl;
					}
					if (get_typestack_head (typestack)->kind == TypeKind (tag) && get_typestack_head (typestack)->tag.type == TagType (enum)) {
						init_typestack (typestack, typestack->is_sizeof_context);
						push_basictype_to_typestack (typestack, BasicType (int), 0);
					}
					typestack->value = ValueCategory (lvalue);
				} else {
					Link_Error (unit, "untyped member '%s' in struct %s", expr->iden.name, decl->name);
					result = 0;
				}
			} else {
				Link_Error (unit, "cannot find '%s' member in struct %s", expr->iden.name, decl->name);
				result = 0;
			}
		} else {
			Link_Error (unit, "invalid member access operand");
			result = 0;
		}
	} else {
		Link_Error (unit, "member access for non-struct value");
		result = 0;
	}
	return (result);
}

int		link_typeinfo (struct unit *unit, uint type_index, uint *out);

int		link_macro_eval (struct unit *unit, uint scope_index, uint decl_index, uint expr_index, struct typestack *typestack);

void	push_const_char_pointer_to_typestack (struct typestack *typestack) {
	struct type	type = {0};

	type.kind = TypeKind (basic);
	type.basic.type = BasicType (char);
	type.flags.is_const = 1;
	push_typestack (typestack, &type);
	memset (&type, 0, sizeof type);
	type.kind = TypeKind (mod);
	type.mod.kind = TypeMod (pointer);
	push_typestack (typestack, &type);
}

int		link_expr (struct unit *unit, uint scope_index, uint expr_index, int is_selfref_check, struct typestack *typestack) {
	int		result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (group)) {
			result = link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack);
		} else if (expr->op.type == OpType (function_call)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				if (typestack->types_count == 1 && get_typestack_head (typestack)->kind == TypeKind (internal)) {
					struct type	*head;
					struct decl	*decl;
					struct unit	*decl_unit;
					uint64		accessor_decl;

					head = get_typestack_head (typestack);
					accessor_decl = head->internal.decl;
					if (is_lib_index (head->internal.decl)) {
						decl_unit = get_lib (get_lib_index (head->internal.decl));
					} else {
						decl_unit = unit;
					}
					decl = get_decl (decl_unit, unlib_index (head->internal.decl));
					Assert (decl->kind == DeclKind (define));
					if (decl->define.kind == DefineKind (accessor)) {
						struct expr	*right_expr;

						right_expr = get_expr (unit, expr->op.backward);
						Assert (right_expr->type == ExprType (funcparam));
						if (!right_expr->funcparam.next) {
							if (right_expr->funcparam.expr) {
								right_expr = get_expr (unit, right_expr->funcparam.expr);
								if (right_expr->type == ExprType (identifier)) {
									uint64	decl_index;

									Assert (decl->define.accessor.decl);
									if (is_lib_index (decl->define.accessor.decl)) {
										decl_unit = get_lib (get_lib_index (decl->define.accessor.decl));
									}
									decl = get_decl (decl_unit, unlib_index (decl->define.accessor.decl));
									Assert (decl->kind == DeclKind (tag));
									Assert (decl->tag.type == TagType (enum));
									decl_index = find_enum_by_name (decl_unit, decl->tag.scope, right_expr->iden.name);
									if (decl_index) {
										memset (expr, 0, sizeof *expr);
										expr->type = ExprType (enum);
										if (decl_unit != unit) {
											expr->enumt.lib_index = Get_Bucket_Element_Index (g_libs, decl_unit);
										} else {
											expr->enumt.lib_index = 0;
										}
										expr->enumt.decl = decl_index;
										expr->enumt.enum_decl = get_decl_index (decl_unit, decl);
										expr->enumt.accessor_decl = accessor_decl;
										init_typestack (typestack, typestack->is_sizeof_context);
										/* todo: get basictype from enum constant */
										push_basictype_to_typestack (typestack, BasicType (int), 0);
										typestack->value = ValueCategory (rvalue);
										result = 1;
									} else {
										Link_Error (unit, "undeclared enum constant '%s' in enum %s", expr->iden.name, decl->name);
										result = 0;
									}
								} else {
									Link_Error (unit, "the parameter of accessor must be an identifier");
									result = 0;
								}
							} else {
								Link_Error (unit, "no parameter for accessor");
								result = 0;
							}
						} else {
							Link_Error (unit, "too many parameters for accessor");
							result = 0;
						}
					} else if (decl->define.kind == DefineKind (macro)) {
						uint	macro_instance;
						uint64	decl_index;
						uint	old_link_foreground_unit;

						decl_index = head->internal.decl;
						init_typestack (typestack, typestack->is_sizeof_context);
						macro_instance = make_decl_copy (unit, decl_unit, unlib_index (decl_index), scope_index);
						old_link_foreground_unit = unit->link_foreground_unit;
						unit->link_foreground_unit = get_lib_index (head->internal.decl);
						if (link_macro_eval (unit, scope_index, macro_instance, expr->op.backward, typestack)) {
							unit->link_foreground_unit = old_link_foreground_unit;
							memset (expr, 0, sizeof *expr);
							expr->type = ExprType (macrocall);
							expr->macrocall.decl = decl_index;
							expr->macrocall.instance = macro_instance;
							result = 1;
						} else {
							result = 0;
						}
					} else if (decl->define.kind == DefineKind (builtin)) {
						struct expr	*right_exprs[4] = {0};

						if (expr->op.backward) {
							int		num_of_params;
							struct expr	*current;
							int		index;

							num_of_params = g_builtin_params_num[decl->define.builtin.type];
							current = get_expr (unit, expr->op.backward);
							Assert (num_of_params > 0);
							index = 0;
							do {
								Assert (current->type == ExprType (funcparam));
								if (current->funcparam.expr) {
									right_exprs[index] = get_expr (unit, current->funcparam.expr);
									index += 1;
									if (index < num_of_params) {
										if (current->funcparam.next) {
											current = get_expr (unit, current->funcparam.next);
											result = 1;
										} else {
											Link_Error (unit, "the %s builtin function must have %d parameters", g_builtin[decl->define.builtin.type], num_of_params);
											result = 0;
										}
									} else if (current->funcparam.next) {
										Link_Error (unit, "calling %s builtin function with more than %d parameter%s", g_builtin[decl->define.builtin.type], num_of_params, num_of_params == 1 ? "" : "s");
										result = 0;
									} else {
										result = 1;
									}
								} else {
									Link_Error (unit, "calling %s builtin function with empty parameter", g_builtin[decl->define.builtin.type]);
									result = 0;
								}
							} while (result && index < num_of_params);
						} else {
							Link_Error (unit, "calling %s builtin function without parameters", g_builtin[decl->define.builtin.type]);
							result = 0;
						}
						if (result) {
							struct expr	*right_expr;

							right_expr = right_exprs[0];
							if (decl->define.builtin.type == Builtin (flag)) {
								if (right_expr->type == ExprType (identifier)) {
									int		flag_index;

									flag_index = 0;
									while (flag_index < Array_Count (g_flag) && 0 != strcmp (g_flag[flag_index], right_expr->iden.name)) {
										flag_index += 1;
									}
									if (flag_index < Array_Count (unit->flags)) {
										expr->type = ExprType (constant);
										expr->constant.type = BasicType (int);
										expr->constant.value = unit->flags[flag_index];
										init_typestack (typestack, typestack->is_sizeof_context);
										push_basictype_to_typestack (typestack, expr->constant.type, 0);
										result = 1;
									} else {
										Link_Error (unit, "unknown parameter for %s builtin function", g_builtin[decl->define.builtin.type]);
										result = 0;
									}
								} else {
									Link_Error (unit, "parameter of %s must be an identifier", g_builtin[decl->define.builtin.type]);
									result = 0;
								}
							} else if (decl->define.builtin.type == Builtin (platform)) {
								if (right_expr->type == ExprType (identifier)) {
									int		platform_index;

									platform_index = 0;
									while (platform_index < Array_Count (g_platform_name) && 0 != strcmp (g_platform_name[platform_index], right_expr->iden.name)) {
										platform_index += 1;
									}
									if (platform_index < Array_Count (g_platform_name)) {
										expr->type = ExprType (constant);
										expr->constant.type = BasicType (int);
										expr->constant.value = platform_index == g_platform;
										init_typestack (typestack, typestack->is_sizeof_context);
										push_basictype_to_typestack (typestack, expr->constant.type, 0);
										result = 1;
									} else {
										Link_Error (unit, "unknown parameter for %s builtin function", g_builtin[decl->define.builtin.type]);
										result = 0;
									}
								} else {
									Link_Error (unit, "parameter of %s must be an identifier", g_builtin[decl->define.builtin.type]);
									result = 0;
								}
							} else if (decl->define.builtin.type == Builtin (option)) {
								if (right_expr->type == ExprType (identifier)) {
									if (unit->manifest.options) {
										char	*value;

										value = (char *) get_value_from_options (unit->manifest.options, right_expr->iden.name);
										if (value) {
											struct exprvalue	exprvalue = {0};

											if (parse_constant_value (unit, value, &exprvalue)) {
												expr->type = ExprType (constant);
												expr->constant = exprvalue;
												init_typestack (typestack, typestack->is_sizeof_context);
												push_basictype_to_typestack (typestack, expr->constant.type, 0);
												result = 1;
											} else {
												Link_Error (unit, "cannot parse constant value");
												result = 0;
											}
										} else {
											Link_Error (unit, "undeclared option '%s'", right_expr->iden.name);
											result = 0;
										}
									} else {
										Link_Error (unit, "no options are declared in this unit");
										result = 0;
									}
								} else {
									Link_Error (unit, "parameter of %s must be an identifier", g_builtin[decl->define.builtin.type]);
									result = 0;
								}
							} else if (decl->define.builtin.type == Builtin (value_property)) {
								if (right_expr->type == ExprType (identifier)) {
									struct typestack	rightstack = {0};

									init_typestack (&rightstack, 0);
									rightstack.is_sizeof_context = 1;
									if (link_expr (unit, scope_index, get_expr_index (unit, right_exprs[1]), 0, &rightstack)) {
										struct type	*type;
										int			value;

										type = get_typestack_head (&rightstack);
										if (0 == strcmp (right_expr->iden.name, "category")) {
											value = rightstack.value;
											result = 1;
										} else if (0 == strcmp (right_expr->iden.name, "__declkind")) {
											struct decl	*decl;

											/* todo: get decl from typestack */
											decl = 0;
											if (decl->kind == DeclKind (var)) {
												value = Internal_DeclKind (var);
											} else if (decl->kind == DeclKind (const)) {
												value = Internal_DeclKind (const);
											} else if (decl->kind == DeclKind (alias)) {
												value = Internal_DeclKind (alias);
											} else if (decl->kind == DeclKind (tag)) {
												if (decl->tag.type == TagType (enum)) {
													value = Internal_DeclKind (enum_tag);
												} else if (decl->tag.type == TagType (struct)) {
													value = Internal_DeclKind (struct_tag);
												} else if (decl->tag.type == TagType (union)) {
													value = Internal_DeclKind (union_tag);
												} else {
													Unreachable ();
												}
											} else if (decl->kind == DeclKind (func)) {
												value = Internal_DeclKind (func);
											} else if (decl->kind == DeclKind (param)) {
												value = Internal_DeclKind (param);
											} else if (decl->kind == DeclKind (block)) {
												value = Internal_DeclKind (block);
											} else if (decl->kind == DeclKind (enum)) {
												value = Internal_DeclKind (enum);
											} else if (decl->kind == DeclKind (define)) {
												if (decl->define.kind == DefineKind (macro)) {
													value = Internal_DeclKind (macro);
												} else if (decl->define.kind == DefineKind (accessor)) {
													value = Internal_DeclKind (accessor);
												} else if (decl->define.kind == DefineKind (external)) {
													value = Internal_DeclKind (external);
												} else if (decl->define.kind == DefineKind (type)) {
													value = Internal_DeclKind (type);
												} else if (decl->define.kind == DefineKind (builtin)) {
													value = Internal_DeclKind (builtin);
												} else {
													value = Internal_DeclKind (unknown);
												}
											} else {
												Unreachable ();
											}
											result = 1;
										} else if (0 == strcmp (right_expr->iden.name, "typekind")) {
											if (type->kind == TypeKind (basic)) {
												value = Internal_TypeKind (basic);
											} else if (type->kind == TypeKind (tag)) {
												if (type->tag.type == TagType (struct)) {
													value = Internal_TypeKind (struct);
												} else if (type->tag.type == TagType (enum)) {
													value = Internal_TypeKind (enum);
												} else if (type->tag.type == TagType (union)) {
													value = Internal_TypeKind (union);
												} else {
													Unreachable ();
												}
											} else if (type->kind == TypeKind (mod)) {
												if (type->mod.kind == TypeMod (pointer)) {
													value = Internal_TypeKind (pointer);
												} else if (type->mod.kind == TypeMod (array)) {
													value = Internal_TypeKind (array);
												} else if (type->mod.kind == TypeMod (function)) {
													value = Internal_TypeKind (function);
												} else {
													Unreachable ();
												}
											} else {
												value = Internal_TypeKind (unknown);
											}
											result = 1;
										} else {
											Link_Error (unit, "unknown value property %s in first parameter of %s builtin function", right_expr->iden.name, g_builtin[decl->define.builtin.type]);
											result = 0;
										}
										if (result) {
											expr->type = ExprType (constant);
											expr->constant.type = BasicType (int);
											expr->constant.value = value;
											init_typestack (typestack, typestack->is_sizeof_context);
											push_basictype_to_typestack (typestack, expr->constant.type, 0);
										}
									} else {
										result = 0;
									}
								} else {
									Link_Error (unit, "first parameter of %s must be an identifier", g_builtin[decl->define.builtin.type]);
									result = 0;
								}
							} else if (decl->define.builtin.type == Builtin (debug_print_expr_type)) {
								int		old_sizeof_context;

								old_sizeof_context = typestack->is_sizeof_context;
								typestack->is_sizeof_context = 1;
								if (link_expr (unit, scope_index, get_expr_index (unit, right_expr), 0, typestack)) {
									typestack->is_sizeof_context = old_sizeof_context;
									fprintf (stderr, "__Debug_Print_Expr_Type (");
									print_expr (unit, get_expr_index (unit, right_expr), stderr);
									fprintf (stderr, ") -> ");
									print_typestack (unit, typestack, stderr);
									fprintf (stderr, "\n");
									expr->type = ExprType (constant);
									expr->constant.type = BasicType (int);
									expr->constant.value = 0;
									init_typestack (typestack, typestack->is_sizeof_context);
									push_basictype_to_typestack (typestack, expr->constant.type, 0);
									result = 1;
								} else {
									result = 0;
								}
							} else {
								Unreachable ();
							}
						}
					} else {
						Unreachable ();
					}
				} else {
					struct type	*head;

					head = get_typestack_head (typestack);
					if (head->kind == TypeKind (mod) && head->mod.kind == TypeMod (pointer)) {
						pop_typestack (typestack);
						head = get_typestack_head (typestack);
						if (head->kind == TypeKind (mod) && head->mod.kind == TypeMod (function)) {
							if (link_funcparams (unit, scope_index, head->mod.param_scope, expr->op.backward)) {
								pop_typestack (typestack);
								typestack->value = ValueCategory (rvalue);
								result = 1;
							} else {
								result = 0;
							}
						} else {
							Link_Error (unit, "left-hand side operand of the %s is not a pointer to function", g_opname[expr->op.type]);
							print_type_typestack (unit, typestack);
							result = 0;
						}
					} else {
						Link_Error (unit, "left-hand side operand of the %s is not a pointer to function", g_opname[expr->op.type]);
						print_type_typestack (unit, typestack);
						result = 0;
					}
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (array_subscript)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				struct typestack	rightstack;

				init_typestack (&rightstack, 0);
				if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, &rightstack)) {
					if (is_type_integral (get_typestack_head (&rightstack))) {
						if (is_pointer_type (get_typestack_head (typestack), typestack->is_sizeof_context)) {
							pop_typestack (typestack);
							result = 1;
						} else {
							Link_Error (unit, "one of the array subscript operands must be pointer");
							result = 0;
						}
					} else if (is_pointer_type (get_typestack_head (typestack), typestack->is_sizeof_context)) {
						if (is_type_integral (get_typestack_head (typestack))) {
							pop_typestack (&rightstack);
							*typestack = rightstack;
							result = 1;
						} else {
							Link_Error (unit, "one of the array subscript operands must be integral");
							result = 0;
						}
					} else {
						Link_Error (unit, "one of the array subscript operands must be integral");
						result = 0;
					}
					if (result) {
						typestack->value = ValueCategory (lvalue);
					} else {
						print_left_right_typestacks (unit, typestack, &rightstack);
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (cast)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				if (link_type (unit, scope_index, expr->op.backward, 1)) {
					init_typestack (typestack, typestack->is_sizeof_context);
					if (push_typestack_recursive (unit, unit, typestack, expr->op.backward)) {
						typestack->value = ValueCategory (rvalue);
						result = 1;
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (indirect_access) || expr->op.type == OpType (member_access)) {
			if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
				if (typestack->types_count == 1 && get_typestack_head (typestack)->kind == TypeKind (internal)) {
					struct type	*head;
					struct decl	*decl;
					struct unit	*decl_unit;
					uint64		accessor_decl;

					head = get_typestack_head (typestack);
					accessor_decl = head->internal.decl;
					if (is_lib_index (head->internal.decl)) {
						decl_unit = Get_Bucket_Element (g_libs, get_lib_index (head->internal.decl));
					} else {
						decl_unit = unit;
					}
					decl = get_decl (decl_unit, unlib_index (head->internal.decl));
					Assert (decl->kind == DeclKind (define));
					if (decl->define.kind == DefineKind (accessor)) {
						struct expr	*right_expr;

						right_expr = get_expr (unit, expr->op.forward);
						Assert (right_expr);
						Assert (right_expr->type == ExprType (identifier));
						if (is_lib_index (decl->define.accessor.decl)) {
							decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl->define.accessor.decl));
						}
						decl = get_decl (decl_unit, unlib_index (decl->define.accessor.decl));
						Assert (decl->kind == DeclKind (tag) && decl->tag.type == TagType (enum));
						if (0 == strcmp (right_expr->iden.name, "name")) {
							pop_typestack (typestack);
							push_const_char_pointer_to_typestack (typestack);
							push_pointer_type_to_typestack (typestack, 1);
							typestack->value = ValueCategory (rvalue);
							memset (expr, 0, sizeof *expr);
							expr->type = ExprType (table);
							if (unit != decl_unit) {
								expr->table.lib_index = Get_Bucket_Element_Index (g_libs, decl_unit);
							} else {
								expr->table.lib_index = 0;
							}
							expr->table.decl = 0;
							expr->table.tag_decl = get_decl_index (decl_unit, decl);
							expr->table.accessor_decl = accessor_decl;
							result = 1;
						} else if (decl->tag.param_scope) {
							uint64	decl_index;

							decl_index = find_ordinary_decl (decl_unit, decl->tag.param_scope, right_expr->iden.name);
							if (decl_index) {
								Assert (!is_lib_index (decl_index));
								pop_typestack (typestack);
								push_typestack_recursive (unit, decl_unit, typestack, get_decl (decl_unit, decl_index)->type);
								push_pointer_type_to_typestack (typestack, 1);
								typestack->value = ValueCategory (rvalue);
								memset (expr, 0, sizeof *expr);
								expr->type = ExprType (table);
								if (unit != decl_unit) {
									expr->table.lib_index = Get_Bucket_Element_Index (g_libs, decl_unit);
								} else {
									expr->table.lib_index = 0;
								}
								expr->table.decl = decl_index;
								expr->table.tag_decl = get_decl_index (decl_unit, decl);
								expr->table.accessor_decl = accessor_decl;
								result = 1;
							} else {
								Link_Error (unit, "undefined enum table parameter '%s' in enum %s", right_expr->iden.name, decl->name);
								result = 0;
							}
						} else {
							Link_Error (unit, "undefined accessor enum member '%s'", right_expr->iden.name);
							result = 0;
						}
					} else {
						Unreachable ();
					}
				} else {
					struct type	*head;

					head = get_typestack_head (typestack);
					if (expr->op.type == OpType (indirect_access)) {
						if (head->kind == TypeKind (mod) && (head->mod.kind == TypeMod (pointer) || head->mod.kind == TypeMod (array))) {
							pop_typestack (typestack);
							head = get_typestack_head (typestack);
							typestack->value = ValueCategory (lvalue);
							result = 1;
						} else {
							Link_Error (unit, "indirect access on non-pointer value");
							print_type_typestack (unit, typestack);
							result = 0;
						}
					} else {
						result = 1;
					}
					if (result) {
						if (head->kind == TypeKind (tag) && head->tag.type == TagType (struct)) {
							if (typestack->value == ValueCategory (lvalue)) {
								struct typestack	ctypestack, *rightstack = &ctypestack;
								struct type			*type;

								init_typestack (rightstack, 0);
								Assert (head->tag.decl);
								result = link_member_access (unit, head->tag.decl, expr->op.forward, rightstack);
								*typestack = *rightstack;
								typestack->value = ValueCategory (lvalue);
							} else {
								Link_Error (unit, "left-hand side of member access operator is not lvalue");
								print_type_typestack (unit, typestack);
								result = 0;
							}
						} else {
							Link_Error (unit, "member access on non-struct value");
							print_type_typestack (unit, typestack);
							result = 0;
						}
					}
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (address_of)) {
			int		old_sizeof_context;

			old_sizeof_context = typestack->is_sizeof_context;
			typestack->is_sizeof_context = 1;
			is_selfref_check = 0;
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				typestack->is_sizeof_context = old_sizeof_context;
				if (get_typestack_head (typestack)->kind == TypeKind (mod) && get_typestack_head (typestack)->mod.kind == TypeMod (array)) {
					push_pointer_type_to_typestack (typestack, 0);
					typestack->value = ValueCategory (rvalue);
				} else if (typestack->value == ValueCategory (lvalue)) {
					push_pointer_type_to_typestack (typestack, 0);
					typestack->value = ValueCategory (rvalue);
					result = 1;
				} else {
					Link_Error (unit, "address_of operator on non-lvalue expression");
					print_type_typestack (unit, typestack);
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (typesizeof) || expr->op.type == OpType (typealignof)) {
			if (link_type (unit, scope_index, expr->op.backward, 1)) {
				init_typestack (typestack, typestack->is_sizeof_context);
				push_basictype_to_typestack (typestack, BasicType (usize), 0);
				typestack->value = ValueCategory (rvalue);
				result = 1;
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
			int		old_sizeof_context;

			old_sizeof_context = typestack->is_sizeof_context;
			typestack->is_sizeof_context = 1;
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				typestack->is_sizeof_context = old_sizeof_context;
				init_typestack (typestack, typestack->is_sizeof_context);
				push_basictype_to_typestack (typestack, BasicType (usize), 0);
				typestack->value = ValueCategory (rvalue);
				result = 1;
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (unary_plus) || expr->op.type == OpType (unary_minus) || expr->op.type == OpType (bitwise_not)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				struct type	*head;

				head = get_typestack_head (typestack);
				if (head->kind == TypeKind (basic)) {
					if (head->basic.type != BasicType (void)) {
						if (is_basictype_integral (head->basic.type)) {
							head->basic.type = get_promoted_basictype (head->basic.type);
							result = 1;
						} else if (expr->op.type == OpType (bitwise_not)) {
							Link_Error (unit, "floating point numbers are not allowed in bitwise_not operator");
							print_type_typestack (unit, typestack);
							result = 0;
						} else {
							result = 1;
						}
						if (result) {
							typestack->value = ValueCategory (rvalue);
						}
					} else {
						Link_Error (unit, "void basic type in %s operator", g_opname[expr->op.type]);
						print_type_typestack (unit, typestack);
						result = 0;
					}
				} else {
					Link_Error (unit, "only basic types are allowed for %s operators", g_opname[expr->op.type]);
					print_type_typestack (unit, typestack);
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (logical_not)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				struct type	*head;

				head = get_typestack_head (typestack);
				if (head->kind == TypeKind (basic)) {
					if (head->basic.type != BasicType (void)) {
						if (is_basictype_integral (head->basic.type)) {
							init_typestack (typestack, typestack->is_sizeof_context);
							push_basictype_to_typestack (typestack, BasicType (int), 0);
							typestack->value = ValueCategory (rvalue);
							result = 1;
						} else {
							Link_Error (unit, "floating point numbers are not allowed in logical_not operator");
							print_type_typestack (unit, typestack);
							result = 0;
						}
					} else {
						Link_Error (unit, "void basic type in logical_not operator");
						print_type_typestack (unit, typestack);
						result = 0;
					}
				} else if (is_pointer_type (head, typestack->is_sizeof_context)) {
					init_typestack (typestack, typestack->is_sizeof_context);
					push_basictype_to_typestack (typestack, BasicType (int), 0);
					typestack->value = ValueCategory (rvalue);
					result = 1;
				} else {
					Link_Error (unit, "only basic or pointer types are allowed for logical_not operators");
					print_type_typestack (unit, typestack);
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (indirect)) {
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				if (is_pointer_type (get_typestack_head (typestack), typestack->is_sizeof_context)) {
					pop_typestack (typestack);
					typestack->value = ValueCategory (lvalue);
					result = 1;
				} else {
					Link_Error (unit, "only pointer types are allowed for indirect operator");
					print_type_typestack (unit, typestack);
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (is_arithmetic_optype (expr->op.type) || is_arithmethic_assignment_optype (expr->op.type)) {
			if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
				struct typestack	rightstack = {0};

				init_typestack (&rightstack, 0);
				if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, &rightstack)) {
					struct type	*left;
					struct type	*right;

					left = get_typestack_head (typestack);
					right = get_typestack_head (&rightstack);
					if (left->kind == TypeKind (basic) && right->kind == TypeKind (basic)) {
						if (left->basic.type != BasicType (void) && right->basic.type != BasicType (void)) {
							if (is_bitwise_optype (expr->op.type) || is_bitwise_assignment_optype (expr->op.type)) {
								if (is_basictype_integral (left->basic.type) && is_basictype_integral (right->basic.type)) {
									result = 1;
								} else {
									Link_Error (unit, "cannot apply %s operator for floating-point operands", g_opname[expr->op.type]);
									print_left_right_typestacks (unit, typestack, &rightstack);
									result = 0;
								}
							} else {
								result = 1;
							}
							if (result) {
								if (is_comparison_optype (expr->op.type)) {
									init_typestack (typestack, typestack->is_sizeof_context);
									push_basictype_to_typestack (typestack, BasicType (int), 0);
								} else {
									if (!is_assignment_optype (expr->op.type)) {
										left->basic.type = get_common_arithmetic_basictype (left->basic.type, right->basic.type);
									}
								}
								typestack->value = ValueCategory (rvalue);
							}
						} else {
							Link_Error (unit, "cannot apply %s operator when one of the operands has void type", g_opname[expr->op.type]);
							print_left_right_typestacks (unit, typestack, &rightstack);
							result = 0;
						}
					} else if (is_pointer_type (left, typestack->is_sizeof_context) || is_pointer_type (right, rightstack.is_sizeof_context)) {
						if (is_pointer_type (left, typestack->is_sizeof_context) && is_pointer_type (right, rightstack.is_sizeof_context)) {
							if (!is_assignment_optype (expr->op.type)) {
								if (is_comparison_optype (expr->op.type)) {
									init_typestack (typestack, typestack->is_sizeof_context);
									push_basictype_to_typestack (typestack, BasicType (int), 0);
									typestack->value = ValueCategory (rvalue);
									result = 1;
								} else if (expr->op.type == OpType (subtract)) {
									init_typestack (typestack, typestack->is_sizeof_context);
									push_basictype_to_typestack (typestack, BasicType (size), 0);
									typestack->value = ValueCategory (rvalue);
									result = 1;
								} else {
									Link_Error (unit, "cannot apply %s operator when both operands are pointers", g_opname[expr->op.type]);
									print_left_right_typestacks (unit, typestack, &rightstack);
									result = 0;
								}
							} else {
								Link_Error (unit, "cannot apply %s operator for pointer arithmetic", g_opname[expr->op.type]);
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							}
						} else {
							struct type	*ptr, *integ;

							if (is_pointer_type (left, typestack->is_sizeof_context)) {
								ptr = left;
								integ = right;
								result = 1;
							} else if (is_assignment_optype (expr->op.type)) {
								Link_Error (unit, "cannot apply %s operator - resulting type is not basic", g_opname[expr->op.type]);
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							} else {
								ptr = right;
								integ = left;
								result = 1;
							}
							if (result) {
								if (integ->kind == TypeKind (basic) && is_basictype_integral (integ->basic.type)) {
									if (is_comparison_optype (expr->op.type)) {
										struct expr	*right_expr;

										if (integ == left) {
											right_expr = get_constant_expr_from_parent (unit, expr->op.backward);
										} else {
											right_expr = get_constant_expr_from_parent (unit, expr->op.forward);
										}
										if (right_expr) {
											Assert (is_basictype_integral (right_expr->constant.type));
											if (right_expr->constant.uvalue == 0) {
												init_typestack (typestack, typestack->is_sizeof_context);
												push_basictype_to_typestack (typestack, BasicType (int), 0);
												typestack->value = ValueCategory (rvalue);
												result = 1;
											} else {
												Link_Error (unit, "cannot apply %s operator for pointer arithmetic with non-null integral value", g_opname[expr->op.type]);
												print_left_right_typestacks (unit, typestack, &rightstack);
												result = 0;
											}
										} else {
											Link_Error (unit, "cannot apply %s operator for pointer arithmetic with non-constant integral value", g_opname[expr->op.type]);
											print_left_right_typestacks (unit, typestack, &rightstack);
											result = 0;
										}
									} else {
										if (ptr == right) {
											*typestack = rightstack;
										}
										typestack->value = ValueCategory (rvalue);
										result = 1;
									}
								} else {
									Link_Error (unit, "cannot apply %s operator for pointer arithmetic, one of the operand must be an integral value", g_opname[expr->op.type]);
									print_left_right_typestacks (unit, typestack, &rightstack);
									result = 0;
								}
							}
						}
					} else {
						Link_Error (unit, "cannot apply %s operator for non-basic values", g_opname[expr->op.type]);
						print_left_right_typestacks (unit, typestack, &rightstack);
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (assign)) {
			if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
				struct typestack	rightstack;

				init_typestack (&rightstack, 0);
				if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, &rightstack)) {
					if (typestack->value == ValueCategory (lvalue)) {
						struct type	*left, *right;

						left = get_typestack_head (typestack);
						right = get_typestack_head (&rightstack);
						if (left->kind == TypeKind (basic) && right->kind == TypeKind (basic)) {
							if (left->basic.type != BasicType (void) && right->basic.type != BasicType (void)) {
								typestack->value = ValueCategory (rvalue);
								result = 1;
							} else {
								Link_Error (unit, "cannot apply %s operator when one of the operands has void type", g_opname[expr->op.type]);
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							}
						} else if (is_pointer_type (left, typestack->is_sizeof_context) && is_pointer_type (right, rightstack.is_sizeof_context)) {
							if (is_same_type (typestack, &rightstack, 1)) {
								typestack->value = ValueCategory (rvalue);
								result = 1;
							} else {
								Link_Error (unit, "cannot apply %s operator - types of operands are not compatible", g_opname[expr->op.type]);
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							}
						} else if (left->kind == right->kind && left->kind == TypeKind (tag) && left->tag.type == right->tag.type) {
							if (left->tag.decl == right->tag.decl) {
								typestack->value = ValueCategory (rvalue);
								result = 1;
							} else {
								Link_Error (unit, "cannot assign value of %s %s to object of type %s %s", g_tagname[right->tag.type], right->tag.name, g_tagname[left->tag.type], left->tag.name);
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							}
						} else if (is_pointer_type (left, typestack->is_sizeof_context) && right->kind == TypeKind (basic)) {
							struct expr	*right_expr;

							right_expr = get_expr (unit, expr->op.forward);
							while (right_expr->type == ExprType (op) && right_expr->op.type == OpType (group)) {
								right_expr = get_expr (unit, right_expr->op.forward);
							}
							if (right_expr->type == ExprType (constant)) {
								if (is_basictype_integral (right_expr->constant.type)) {
									if (right_expr->constant.uvalue == 0) {
										typestack->value = ValueCategory (rvalue);
										result = 1;
									} else {
										Link_Error (unit, "cannot assign non-null basic type to pointer");
										print_left_right_typestacks (unit, typestack, &rightstack);
										result = 0;
									}
								} else {
									Link_Error (unit, "cannot assign non-integral basic type to pointer");
									print_left_right_typestacks (unit, typestack, &rightstack);
									result = 0;
								}
							} else {
								Link_Error (unit, "cannot assign non-null basic type to pointer");
								print_left_right_typestacks (unit, typestack, &rightstack);
								result = 0;
							}
						} else {
							Link_Error (unit, "cannot apply %s operator - types of operands are not compatible", g_opname[expr->op.type]);
							print_left_right_typestacks (unit, typestack, &rightstack);
							result = 0;
						}
					} else {
						Link_Error (unit, "left-hand side expression is not lvalue");
						print_left_right_typestacks (unit, typestack, &rightstack);
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (logical_and) || expr->op.type == OpType (logical_or)) {
			if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
				struct typestack	rightstack = {0};

				init_typestack (&rightstack, 0);
				if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, &rightstack)) {
					struct type	*left, *right;

					left = get_typestack_head (typestack);
					right = get_typestack_head (&rightstack);
					if (is_pointer_type (left, typestack->is_sizeof_context) || (left->kind == TypeKind (basic) && is_basictype_integral (left->basic.type))) {
						if (is_pointer_type (right, rightstack.is_sizeof_context) || (right->kind == TypeKind (basic) && is_basictype_integral (right->basic.type))) {
							init_typestack (typestack, typestack->is_sizeof_context);
							push_basictype_to_typestack (typestack, BasicType (int), 0);
							typestack->value = ValueCategory (rvalue);
							result = 1;
						} else {
							Link_Error (unit, "right-hand side operands must have a pointer or integral type");
							print_left_right_typestacks (unit, typestack, &rightstack);
							result = 0;
						}
					} else {
						Link_Error (unit, "left-hand side operands must have a pointer or integral type");
						print_left_right_typestacks (unit, typestack, &rightstack);
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (left_shift) || expr->op.type == OpType (right_shift) || expr->op.type == OpType (left_shift_assign) || expr->op.type == OpType (right_shift_assign)) {
			if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
				struct typestack	rightstack = {0};

				init_typestack (&rightstack, 0);
				if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, &rightstack)) {
					struct type	*left, *right;

					left = get_typestack_head (typestack);
					right = get_typestack_head (&rightstack);
					if (left->kind == TypeKind (basic) && is_basictype_integral (left->basic.type)) {
						if (right->kind == TypeKind (basic) && is_basictype_integral (right->basic.type)) {
							typestack->value = ValueCategory (rvalue);
							result = 1;
						} else {
							Link_Error (unit, "cannot apply %s operator - right-hand side operand is not integral", g_opname[expr->op.type]);
							print_left_right_typestacks (unit, typestack, &rightstack);
							result = 0;
						}
					} else {
						Link_Error (unit, "cannot apply %s operator - left-hand side operand is not integral", g_opname[expr->op.type]);
						print_left_right_typestacks (unit, typestack, &rightstack);
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else {
			Unreachable ();
		}
	} else if (expr->type == ExprType (funcparam)) {
#if 0
		Assert (expr->funcparam.expr >= 0);
		init_typestack (typestack);
		if (link_expr (unit, scope_index, expr->funcparam.expr, is_selfref_check, typestack)) {
			if (expr->funcparam.next >= 0) {
				result = link_expr (unit, scope_index, expr->funcparam.next, is_selfref_check, typestack);
			} else {
				result = 1;
			}
		} else {
			result = 0;
		}
#endif
		Unreachable ();
	} else if (expr->type == ExprType (constant)) {
		init_typestack (typestack, typestack->is_sizeof_context);
		push_basictype_to_typestack (typestack, expr->constant.type, 0);
		typestack->value = ValueCategory (rvalue);
		result = 1;
	} else if (expr->type == ExprType (identifier)) {
		init_typestack (typestack, typestack->is_sizeof_context);
		if (0 == strcmp (expr->iden.name, "__Filename")) {
			expr->type = ExprType (string);
			expr->string.token = unit->filename;
			push_const_char_pointer_to_typestack (typestack);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else if (0 == strcmp (expr->iden.name, "__Filepath")) {
			expr->type = ExprType (string);
			expr->string.token = unit->filepath;
			push_const_char_pointer_to_typestack (typestack);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else if (0 == strcmp (expr->iden.name, "__Function")) {
			expr->type = ExprType (string);
			expr->string.token = unit->function_name;
			push_const_char_pointer_to_typestack (typestack);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else if (0 == strcmp (expr->iden.name, "__Line")) {
			expr->type = ExprType (constant);
			expr->constant.type = BasicType (int);
			expr->constant.value = unit->pos.line;
			push_basictype_to_typestack (typestack, BasicType (int), 0);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else {
			uint64	decl_index;
			struct decl	*decl;
			struct unit	*decl_unit;

			decl_index = find_ordinary_decl_local (unit, scope_index, expr->iden.name);
			if (!decl_index && unit->link_foreground_unit) {
				decl_index = find_decl_in_table (Get_Bucket_Element (g_libs, unit->link_foreground_unit), expr->iden.name, TagType (invalid), 0);
				if (!is_lib_index (decl_index)) {
					decl_index = make_lib_index (unit->link_foreground_unit, decl_index);
				}
			}
			if (!decl_index) {
				decl_index = find_ordinary_decl_global (unit, expr->iden.name);
			}
			if (decl_index) {
				if (is_lib_index (decl_index)) {
					decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl_index));
				} else {
					decl_unit = unit;
				}
				decl = get_decl (decl_unit, unlib_index (decl_index));
				if ((is_selfref_check && decl->is_in_process == 0) || !is_selfref_check || decl->kind == DeclKind (func)) {
					expr->iden.decl = decl_index;
					result = 1;
				} else {
					Link_Error (unit, "self referencing declaration '%s'", decl->name);
					result = 0;
				}
			} else {
				print_included_libs (unit);
				print_symbols_included_from (unit, 1);
				Link_Error (unit, "undeclared identifier '%s'", expr->iden.name);
				result = 0;
			}
			if (result) {
				if (decl->is_linked) {
					result = 1;
				} else {
					if (link_decl (decl_unit, decl_unit->scope, get_decl_index (decl_unit, decl), 0)) {
						result = 1;
					} else {
						result = 0;
					}
				}
				if (result) {
					if (decl->kind == DeclKind (const)) {
						uint	copy_expr;

						copy_expr = make_expr_copy (unit, decl_unit, decl->dconst.expr, scope_index);
						Assert (copy_expr);
						expr->type = ExprType (op);
						expr->op.type = OpType (group);
						expr->op.forward = copy_expr;
						update_typestack_recursive (unit, copy_expr, typestack);
					} else if (decl->type) {
						result = push_typestack_recursive (unit, decl_unit, typestack, decl->type);
						if (decl->kind == DeclKind (param) && is_pointer_type (get_typestack_head (typestack), 1)) {
							get_typestack_head (typestack)->mod.kind = TypeMod (pointer);
						}
					} else if (decl->kind == DeclKind (define)) {
						struct type	type = {0};

						type.kind = TypeKind (internal);
						type.internal.decl = decl_index;
						result = push_typestack (typestack, &type);
						typestack->value = ValueCategory (rvalue);
					} else if ((unit == decl_unit) && decl->kind == DeclKind (alias)) {
						replace_expr_with_copy (unit, expr, decl_unit, decl->alias.expr, scope_index);
						result = link_expr (unit, scope_index, get_expr_index (unit, expr), is_selfref_check, typestack);
					} else {
						Link_Error (unit, "untyped decl %s", decl->name);
						result = 0;
					}
				}
				if (result) {
					if (get_typestack_head (typestack)->kind == TypeKind (mod) && get_typestack_head (typestack)->mod.kind == TypeMod (function)) {
						Assert (decl->kind == DeclKind (func) || decl->kind == DeclKind (define));
						push_pointer_type_to_typestack (typestack, 0);
						typestack->value = ValueCategory (rvalue);
					} else if (get_typestack_head (typestack)->kind == TypeKind (mod) && get_typestack_head (typestack)->mod.kind == TypeMod (array)) {
						if (typestack->is_sizeof_context) {
							typestack->value = ValueCategory (rvalue);
						} else {
							get_typestack_head (typestack)->mod.kind = TypeMod (pointer);
							typestack->value = ValueCategory (rvalue);
						}
					} else {
						if (get_typestack_head (typestack)->kind == TypeKind (tag) && get_typestack_head (typestack)->tag.type == TagType (enum)) {
							init_typestack (typestack, typestack->is_sizeof_context);
							push_basictype_to_typestack (typestack, BasicType (int), 0);
						}
						typestack->value = ValueCategory (lvalue);
					}
				}
			}
		}
	} else if (expr->type == ExprType (typeinfo)) {
		if (link_type (unit, scope_index, expr->typeinfo.type, 1)) {
			uint	typeinfo_index;

			if (link_typeinfo (unit, expr->typeinfo.type, &typeinfo_index)) {
				struct type	type = {0};

				Assert (typeinfo_index);
				expr->typeinfo.index = typeinfo_index;
				type.kind = TypeKind (tag);
				type.tag.type = TagType (struct);
				type.tag.name = "typeinfo";
				type.tag.decl = g_typeinfo_struct_decl;
				init_typestack (typestack, typestack->is_sizeof_context);
				result = push_typestack (typestack, &type);
				typestack->value = ValueCategory (lvalue);
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (expr->type == ExprType (macroparam)) {
		struct decl	*decl;
		uint	expr_copy;

		Assert (expr->macroparam.decl);
		decl = get_decl (unit, expr->macroparam.decl);
		Assert (decl->kind == DeclKind (param));
		Assert (decl->param.expr);
		expr_copy = make_expr_copy (unit, unit, decl->param.expr, scope_index);
		Assert (expr_copy);
		expr->type = ExprType (op);
		expr->op.type = OpType (group);
		expr->op.forward = expr_copy;
		result = link_expr (unit, scope_index, expr_copy, is_selfref_check, typestack);
	} else if (expr->type == ExprType (string)) {
		push_const_char_pointer_to_typestack (typestack);
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		link_type (struct unit *unit, uint scope_index, uint type_index, int is_selfref_check) {
	int			result;
	struct type	*type;

	type = get_type (unit, type_index);
	if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (pointer)) {
			result = link_type (unit, scope_index, type->mod.forward, 0);
		} else if (type->mod.kind == TypeMod (function)) {
			if (link_type (unit, scope_index, type->mod.forward, 0)) {
				if (!is_lib_index (type->mod.param_scope)) {
					result = link_param_scope (unit, scope_index, type->mod.param_scope);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else if (type->mod.kind == TypeMod (array)) {
			result = link_type (unit, scope_index, type->mod.forward, 0);
		} else {
			Unreachable ();
		}
	} else if (type->kind == TypeKind (basic)) {
		result = 1;
	} else if (type->kind == TypeKind (tag)) {
		uint64	decl_index;

		decl_index = find_decl_type (unit, scope_index, type->tag.name, type->tag.type, 0);
		if (decl_index) {
			if (is_selfref_check) {
				struct decl	*decl;
				struct unit	*decl_unit;

				if (is_lib_index (decl_index)) {
					decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl_index));
				} else {
					decl_unit = unit;
				}
				decl = get_decl (decl_unit, unlib_index (decl_index));
				if (decl->is_in_process == 0) {
					type->tag.decl = decl_index;
					result = 1;
				} else {
					Link_Error (unit, "self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
					result = 0;
				}
			} else {
				type->tag.decl = decl_index;
				result = 1;
			}
		} else {
			Link_Error (unit, "undeclared %s tag '%s'", g_tagname[type->tag.type], type->tag.name);
			result = 0;
		}
	} else if (type->kind == TypeKind (typeof)) {
		struct typestack	ctypestack, *typestack = &ctypestack;

		init_typestack (typestack, 0);
		if (link_expr (unit, scope_index, type->typeof.expr, is_selfref_check, typestack)) {
			result = insert_typestack_to_type (unit, type_index, typestack);
		} else {
			result = 0;
		}
	} else if (type->kind == TypeKind (deftype)) {
		uint64	decl_index;

		decl_index = find_decl_type (unit, scope_index, type->deftype.name, TagType (invalid), 1);
		if (decl_index) {
			struct decl	*decl;
			struct unit	*decl_unit;
			uint		cloned_type;

			if (is_lib_index (decl_index)) {
				decl_unit = Get_Bucket_Element (g_libs, get_lib_index (decl_index));
			} else {
				decl_unit = unit;
			}
			decl = get_decl (decl_unit, unlib_index (decl_index));
			Assert (decl->kind == DeclKind (define));
			if (decl->define.kind == DefineKind (opaque)) {
				const char	*name;

				name = type->deftype.name;
				type->kind = TypeKind (opaque);
				type->opaque.decl = decl_index;
				result = 1;
			} else {
				Assert (decl->define.kind == DefineKind (type));
				Assert (decl->type);
				replace_type_with_copy (unit, type, decl_unit, decl->type, scope_index);
				type->flags.is_group = 1;
				if (link_type (unit, scope_index, type_index, is_selfref_check)) {
					result = 1;
				} else {
					result = 0;
				}
			}
		} else {
			Link_Error (unit, "undeclared type '%s'", type->deftype.name);
			result = 0;
		}
	} else if (type->kind == TypeKind (opaque)) {
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		link_decl_var (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	Assert (decl->type);
	if (decl->is_in_process == 0) {
		push_type_path (unit, decl->type);
		decl->is_in_process = 1;
		result = link_type (unit, scope_index, decl->type, 1);
		decl->is_in_process = 0;
		pop_path (unit);
		if (result && decl->var.init_scope) {
			struct type	*type;

			type = get_type (unit, decl->type);
			Assert (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (array));
			Assert (type->mod.forward);
			get_scope (unit, decl->var.init_scope)->type_index = type->mod.forward;
			result = link_init_scope (unit, decl->var.init_scope);
		}
		decl->is_linked = 1;
	} else {
		Link_Error (unit, "type loop referencing");
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_const (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	if (decl->is_in_process == 0) {
		struct typestack	ctypestack, *typestack = &ctypestack;

		decl->is_in_process = 1;
		init_typestack (typestack, 0);
		push_expr_path (unit, decl->dconst.expr);
		result = link_expr (unit, scope_index, decl->dconst.expr, 1, typestack);
		pop_path (unit);
		decl->is_in_process = 0;
		decl->is_linked = 1;
	} else {
		Link_Error (unit, "type loop referencing");
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_param_scope (struct unit *unit, uint scope_index, uint param_scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, param_scope_index);
	if (scope->decl_begin) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (decl->type) {
				push_decl_path (unit, get_decl_index (unit, decl));
				push_type_path (unit, decl->type);
				decl->is_in_process = 1;
				result = link_type (unit, scope_index, decl->type, 1);
				decl->is_in_process = 0;
				decl->is_linked = 1;
				pop_path (unit);
				pop_path (unit);
			}
			decl = decl->next ? get_decl (unit, decl->next) : 0;
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		link_decl (struct unit *unit, uint scope_index, uint decl_index, int is_global);
int		link_code_scope (struct unit *unit, uint scope_index);

int		link_code_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int					result;
	struct flow			*flow;
	struct typestack	typestack;

	init_typestack (&typestack, 0);
	push_flow_path (unit, scope_index, flow_index);
	flow = get_flow (unit, flow_index);
	unit->pos.line = flow->line;
	if (flow->type == FlowType (decl)) {
		unit->link_decl_index = flow->decl.index;
		result = link_decl (unit, scope_index, flow->decl.index, 0);
	} else if (flow->type == FlowType (expr)) {
		if (flow->expr.index) {
			push_expr_path (unit, flow->expr.index);
			result = link_expr (unit, scope_index, flow->expr.index, 0, &typestack);
			pop_path (unit);
		} else {
			result = 1;
		}
	} else if (flow->type == FlowType (block)) {
		result = link_code_scope (unit, flow->block.scope);
	} else if (flow->type == FlowType (if)) {
		push_expr_path (unit, flow->fif.expr);
		if (link_expr (unit, scope_index, flow->fif.expr, 0, &typestack)) {
			/* check if type is ok for if statement */
			result = 1;
		} else {
			result = 0;
		}
		pop_path (unit);
		if (result) {
			if (link_code_scope_flow (unit, scope_index, flow->fif.flow_body)) {
				if (flow->fif.else_body) {
					result = link_code_scope_flow (unit, scope_index, flow->fif.else_body);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		}
	} else if (flow->type == FlowType (static_if)) {
		push_expr_path (unit, flow->static_if.expr);
		if (link_expr (unit, scope_index, flow->static_if.expr, 0, &typestack)) {
			struct evalvalue	value = {0};

			/* check if type is ok for if statement */
			if (eval_const_expr (unit, flow->static_if.expr, &value)) {
				int		is_true;

				if (value.type == EvalType (basic)) {
					if (is_basictype_integral (value.basic)) {
						if (is_basictype_signed (value.basic)) {
							is_true = value.value != 0;
						} else {
							is_true = value.uvalue != 0;
						}
						result = 1;
					} else if (value.basic == BasicType (void)) {
						Link_Error (unit, "value for static_if condition has void type");
						result = 0;
					} else {
						Link_Error (unit, "cannot convert floating point number to boolean in static_if condition");
						result = 0;
					}
				} else if (value.type == EvalType (string)) {
					is_true = value.string != 0;
					result = 1;
				} else if (value.type == EvalType (typeinfo_pointer)) {
					is_true = value.typeinfo_index != 0;
					result = 1;
				} else if (value.type == EvalType (typemember_pointer)) {
					is_true = value.typemember_index != 0;
					result = 1;
				} else {
					Link_Error (unit, "invalid value for static_if condition");
					result = 0;
				}
				if (result) {
					uint	next;

					next = flow->next;
					if (is_true) {
						*flow = *get_flow (unit, flow->static_if.flow_body);
						flow->next = next;
						result = link_code_scope_flow (unit, scope_index, flow_index);
					} else {
						if (flow->static_if.else_body) {
							*flow = *get_flow (unit, flow->static_if.else_body);
							flow->next = next;
							result = link_code_scope_flow (unit, scope_index, flow_index);
						} else {
							memset (flow, 0, sizeof *flow);
							flow->type = FlowType (expr);
							flow->expr.index = 0;
							result = 1;
						}
					}
				}
			} else {
				Link_Error (unit, "cannot evaluate constant expression");
				result = 0;
			}
		} else {
			result = 0;
		}
		pop_path (unit);
	} else if (flow->type == FlowType (while)) {
		push_expr_path (unit, flow->fwhile.expr);
		if (link_expr (unit, scope_index, flow->fwhile.expr, 0, &typestack)) {
			/* check if type is ok for while statement */
			result = 1;
		} else {
			result = 0;
		}
		pop_path (unit);
		if (result) {
			result = link_code_scope_flow (unit, scope_index, flow->fwhile.flow_body);
		}
	} else if (flow->type == FlowType (dowhile)) {
		if (link_code_scope_flow (unit, scope_index, flow->dowhile.flow_body)) {
			push_expr_path (unit, flow->dowhile.expr);
			result = link_expr (unit, scope_index, flow->dowhile.expr, 0, &typestack);
			pop_path (unit);
			/* check if type is ok for dowhile statement */
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (assert) || flow->type == FlowType (static_assert)) {
		push_expr_path (unit, flow->dowhile.expr);
		if (link_expr (unit, scope_index, flow->assert.expr, 0, &typestack)) {
			if (is_pointer_type (get_typestack_head (&typestack), 0) || is_type_integral (get_typestack_head (&typestack))) {
				if (flow->type == FlowType (static_assert)) {
					struct evalvalue	value = {0};

					if (eval_const_expr (unit, flow->assert.expr, &value)) {
						int		is_true;

						if (value.type == EvalType (basic)) {
							if (is_basictype_integral (value.basic)) {
								if (is_basictype_signed (value.basic)) {
									is_true = value.value != 0;
								} else {
									is_true = value.uvalue != 0;
								}
								result = 1;
							} else if (value.basic == BasicType (void)) {
								Link_Error (unit, "value for static_assert condition has void type");
								result = 0;
							} else {
								Link_Error (unit, "cannot convert floating point number to boolean in static_assert condition");
								result = 0;
							}
						} else if (value.type == EvalType (string)) {
							is_true = value.string != 0;
							result = 1;
						} else if (value.type == EvalType (typeinfo_pointer)) {
							is_true = value.typeinfo_index != 0;
							result = 1;
						} else if (value.type == EvalType (typemember_pointer)) {
							is_true = value.typemember_index != 0;
							result = 1;
						} else {
							Link_Error (unit, "invalid value for static_assert condition");
							result = 0;
						}
						if (result) {
							if (is_true) {
								result = 1;
							} else {
								Link_Error (unit, "Assertion failed");
								fprintf (stderr, "Asserted expression: ");
								print_expr (unit, flow->assert.expr, stderr);
								fprintf (stderr, "\n");
								result = 0;
							}
						}
					} else {
						Link_Error (unit, "cannot evaluate constant expression of static_assert statement");
						result = 0;
					}
				} else {
					result = 1;
				}
			} else {
				Link_Error (unit, "assertion condition must have an intergral or pointer type");
				result = 0;
			}
		} else {
			result = 0;
		}
		pop_path (unit);
	} else if (flow->type == FlowType (unreachable)) {
		result = 1;
	} else {
		Unreachable ();
	}
	pop_path (unit);
	return (result);
}

int		link_code_scope (struct unit *unit, uint scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				int flow_index = get_flow_index (unit, flow);
				result = link_code_scope_flow (unit, scope_index, flow_index);
				flow = flow->next ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		} else {
			result = 1;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_decl_func (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_function_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	unit->function_name = decl->name;
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		push_type_path (unit, decl->type);
		result = link_type (unit, scope_index, decl->type, 1);
		pop_path (unit);
		if (result) {
			if (link_param_scope (unit, scope_index, decl->func.param_scope)) {
				result = 1;
			} else {
				result = 0;
			}
		}
		decl->is_in_process = 0;
		decl->is_linked = 1;
	} else {
		Link_Error (unit, "self referencing");
		result = 0;
	}
	unit->function_name = "<none>";
	pop_path (unit);
	return (result);
}

int		link_enum_table (struct unit *unit, uint scope_index, uint params) {
	int				result;
	struct scope	*scope;
	uint			decl_index;

	scope = get_scope (unit, scope_index);
	Assert (scope->param_scope);
	scope = get_scope (unit, scope->param_scope);
	Assert (scope->decl_begin);
	decl_index = scope->decl_begin;
	if (decl_index && params) {
		do {
			struct expr			*expr;
			struct decl			*decl;
			struct typestack	typestack = {0};

			expr = get_expr (unit, params);
			Assert (expr->type == ExprType (funcparam));
			Assert (expr->funcparam.expr);
			decl = get_decl (unit, decl_index);
			Assert (decl->kind == DeclKind (param));
			Assert (decl->type);
			init_typestack (&typestack, 0);
			if (link_expr (unit, scope_index, expr->funcparam.expr, 1, &typestack)) {
				struct typestack	leftstack = {0};

				init_typestack (&leftstack, 0);
				push_typestack_recursive (unit, unit, &leftstack, decl->type);
				if (is_typestacks_compatible (&leftstack, &typestack)) {
					result = 1;
				} else {
					Link_Error (unit, "types are not compatible in enum table");
					print_left_right_typestacks (unit, &leftstack, &typestack);
				}
			} else {
				result = 0;
			}
			decl_index = decl->next;
			params = expr->funcparam.next;
		} while (result && decl_index && params);
		if (result) {
			if (decl_index) {
				Link_Error (unit, "too few arguments for enum table");
				result = 0;
			} else if (params) {
				Link_Error (unit, "too many arguments for enum table");
				result = 0;
			} else {
				result = 1;
			}
		}
	} else {
		Link_Error (unit, "too few arguments for enum table");
		result = 0;
	}
	return (result);
}

int		link_enum_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int			result;
	struct flow	*flow;

	push_flow_path (unit, scope_index, flow_index);
	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		struct decl	*decl;

		unit->link_decl_index = flow->decl.index;
		decl = get_decl (unit, flow->decl.index);
		Assert (decl->kind == DeclKind (enum));
		if (get_scope (unit, scope_index)->param_scope) {
			if (decl->enumt.params) {
				result = link_enum_table (unit, scope_index, decl->enumt.params);
			} else {
				Link_Error (unit, "enum constant doesn't declare parameters in the parametric enum scope");
				result = 0;
			}
		} else if (decl->enumt.params) {
			Link_Error (unit, "enum constant declaration has parameters when the scope doesn't have any");
			result = 0;
		} else {
			result = 1;
		}
	} else {
		Unreachable ();
	}
	pop_path (unit);
	return (result);
}

int		link_enum_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_enum_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		link_struct_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int			result;
	struct flow	*flow;

	push_flow_path (unit, scope_index, flow_index);
	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		unit->link_decl_index = flow->decl.index;
		result = link_decl (unit, scope_index, flow->decl.index, 0);
	} else {
		Unreachable ();
	}
	pop_path (unit);
	return (result);
}

int		link_struct_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_struct_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

enum inittype	get_init_type (struct type *type) {
	enum inittype	result;

	if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (array)) {
			result = InitType (array);
		} else {
			result = InitType (expr);
		}
	} else if (type->kind == TypeKind (tag) && (type->tag.type == TagType (struct) || type->tag.type == TagType (union))) {
		result = InitType (struct);
	} else {
		result = InitType (expr);
	}
	return (result);
}

int		link_init_scope (struct unit *unit, uint scope_index);

int		link_init_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int			result;
	struct flow	*flow;
	struct type	*type;

	flow = get_flow (unit, flow_index);
	Assert (flow->type == FlowType (init));
	Assert (get_scope (unit, scope_index)->type_index);
	type = get_type (unit, get_scope (unit, scope_index)->type_index);
	if (flow->init.type == InitType (expr)) {
		struct typestack	typestack;

		init_typestack (&typestack, 0);
		result = link_expr (unit, scope_index, flow->init.body, 1, &typestack);
	} else {
		Assert (flow->init.type == InitType (list));
		flow->init.type = get_init_type (type);
		if (flow->init.type == InitType (array)) {
			Assert (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (array));
			get_scope (unit, flow->init.body)->type_index = type->mod.forward;
			result = link_init_scope (unit, flow->init.body);
		} else if (flow->init.type == InitType (struct)) {
			struct decl		*decl;
			struct scope	*scope, *init_scope;
			struct flow		*init_flow;
			struct unit		*decl_unit;

			Assert (type->kind == TypeKind (tag) && (type->tag.type == TagType (struct) || type->tag.type == TagType (union)));
			if (is_lib_index (type->tag.decl)) {
				decl_unit = Get_Bucket_Element (g_libs, get_lib_index (type->tag.decl));
			} else {
				decl_unit = unit;
			}
			decl = get_decl (decl_unit, unlib_index (type->tag.decl));
			Assert (decl->kind == DeclKind (tag) && decl->tag.type == type->tag.type);
			Assert (decl->tag.scope);
			scope = get_scope (decl_unit, decl->tag.scope);
			Assert (scope->decl_begin);
			decl = get_decl (decl_unit, scope->decl_begin);
			Assert (flow->init.body);
			init_scope = get_scope (unit, flow->init.body);
			Assert (init_scope->kind == ScopeKind (init));
			Assert (init_scope->flow_begin);
			init_flow = get_flow (unit, init_scope->flow_begin);
			result = 1;
			while (result && decl && init_flow) {
				Assert (decl->type);
				if (decl_unit != unit) {
					/* !memory: free copy of old iteration */
					init_scope->type_index = make_type_copy (unit, decl_unit, decl->type, scope_index);
				} else {
					init_scope->type_index = decl->type;
				}
				result = link_init_scope_flow (unit, flow->init.body, get_flow_index (unit, init_flow));
				if (init_flow->next) {
					init_flow = get_flow (unit, init_flow->next);
				} else {
					init_flow = 0;
				}
				if (decl->next) {
					decl = get_decl (decl_unit, decl->next);
				} else {
					decl = 0;
				}
			}
			if (result && !decl && init_flow) {
				Link_Error (unit, "too many init entries");
				result = 0;
			}
		} else {
			Link_Error (unit, "init type mismatch %d", flow->init.type);
			print_type (unit, get_type_index (unit, type), stderr);
			fprintf (stderr, "\n");
			Assert (type->kind == TypeKind (tag));
			result = 0;
		}
	}
	return (result);
}

int		link_init_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;
	uint			flow_index;

	scope = get_scope (unit, scope_index);
	flow_index = scope->flow_begin;
	result = 1;
	while (result && flow_index) {
		result = link_init_scope_flow (unit, scope_index, flow_index);
		flow_index = get_flow (unit, flow_index)->next;
	}
	return (result);
}

int		link_decl_tag (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_tag_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	if (check_scope_declarations_for_name_uniqueness (unit, decl->tag.scope)) {
		if (decl->is_in_process == 0) {
			decl->is_in_process = 1;
			if (decl->tag.type == TagType (struct) || decl->tag.type == TagType (union)) {
				result = link_struct_scope (unit, decl->tag.scope);
			} else if (decl->tag.type == TagType (enum)) {
				result = link_enum_scope (unit, decl->tag.scope);
			} else {
				Link_Error (unit, "unknown tag type");
				result = 0;
			}
			decl->is_in_process = 0;
			decl->is_linked = 1;
		} else {
			Link_Error (unit, "self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
			result = 0;
		}
	} else {
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_block (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	decl->is_in_process = 1;
	result = link_struct_scope (unit, decl->block.scope);
	decl->is_in_process = 0;
	decl->is_linked = 1;
	pop_path (unit);
	return (result);
}

int		link_decl_accessor (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;
	uint64		tagdecl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	tagdecl = find_decl_type (unit, scope_index, decl->define.accessor.name, decl->define.accessor.tagtype, 0);
	if (tagdecl) {
		decl->define.accessor.decl = tagdecl;
		decl->is_linked = 1;
		result = 1;
	} else {
		Link_Error (unit, "cannot link accessor");
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_external (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	decl->is_in_process = 1;
	if (link_type (unit, scope_index, decl->type, 1)) {
		decl->is_in_process = 0;
		decl->is_linked = 1;
		result = 1;
	} else {
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_visability (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;
	uint		target_decl;
	enum visability	visability;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	target_decl = find_ordinary_decl (unit, scope_index, decl->define.visability.target);
	visability = decl->define.visability.type;
	if (target_decl) {
		decl = get_decl (unit, target_decl);
		if (decl->kind == DeclKind (func)) {
			decl->func.visability = visability;
			result = 1;
		} else if (decl->kind == DeclKind (var)) {
			decl->var.visability = visability;
			result = 1;
		} else {
			Link_Error (unit, "visability defined for non-function and non-variable object");
			result = 0;
		}
	} else {
		result = 1;
	}
	decl->is_linked = 1;
	pop_path (unit);
	return (result);
}

int		link_decl_funcprefix (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl, *target_decl;
	uint		target_decl_index;
	const char	*prefix;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	target_decl_index = find_ordinary_decl (unit, scope_index, decl->define.funcprefix.target);
	prefix = decl->define.funcprefix.prefix;
	if (target_decl_index) {
		target_decl = get_decl (unit, target_decl_index);
		if (target_decl->kind == DeclKind (func)) {
			target_decl->func.prefix = prefix;
			decl->is_linked = 1;
			result = 1;
		} else {
			Link_Error (unit, "funcprefix defined for non-function object");
			result = 0;
		}
	} else {
		result = 1;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_type (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	decl->is_in_process = 1;
	if (link_type (unit, scope_index, decl->type, 1)) {
		decl->is_in_process = 0;
		decl->is_linked = 1;
		result = 1;
	} else {
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl_assert (struct unit *unit, uint scope_index, uint decl_index) {
	int					result;
	struct decl			*decl;
	struct typestack	typestack = {0};

	push_decl_path (unit, decl_index);
	decl = get_decl (unit, decl_index);
	init_typestack (&typestack, 0);
	decl->is_in_process = 1;
	if (link_expr (unit, scope_index, decl->define.assert.expr, 1, &typestack)) {
		struct evalvalue	value = {0};

		decl->is_in_process = 0;
		decl->is_linked = 1;
		if (eval_const_expr (unit, decl->define.assert.expr, &value)) {
			if (value.type == EvalType (basic)) {
				if (is_basictype_integral (value.basic)) {
					if (value.uvalue) {
						result = 1;
					} else {
						Link_Error (unit, "Assertion failed");
						fprintf (stderr, "Asserted expression: ");
						print_expr (unit, decl->define.assert.expr, stderr);
						fprintf (stderr, "\n");
						result = 0;
					}
				} else {
					Link_Error (unit, "assert condition has non-intergal basic type");
					result = 0;
				}
			} else if (value.type == EvalType (string)) {
				if (value.string) {
					result = 1;
				} else {
					Link_Error (unit, "assertion failed");
					result = 0;
				}
			} else if (value.type == EvalType (typeinfo_pointer)) {
				if (value.typeinfo_index) {
					result = 1;
				} else {
					Link_Error (unit, "assertion failed");
					result = 0;
				}
			} else if (value.type == EvalType (typemember_pointer)) {
				if (value.typemember_index) {
					result = 1;
				} else {
					Link_Error (unit, "assertion failed");
					result = 0;
				}
			} else {
				Link_Error (unit, "assert condition has non-intergal basic type");
				result = 0;
			}
		} else {
			Link_Error (unit, "cannot evaluate constant expression");
			result = 0;
		}
	} else {
		result = 0;
	}
	pop_path (unit);
	return (result);
}

int		link_decl (struct unit *unit, uint scope_index, uint decl_index, int is_global) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (!decl->is_linked) {
		unit->filepath = decl->filepath;
		if (decl->kind == DeclKind (var)) {
			result = link_decl_var (unit, scope_index, decl_index);
		} else if (decl->kind == DeclKind (const)) {
			result = link_decl_const (unit, scope_index, decl_index);
		} else if (decl->kind == DeclKind (func)) {
			result = link_decl_func (unit, scope_index, decl_index);
		} else if (decl->kind == DeclKind (tag)) {
			result = link_decl_tag (unit, scope_index, decl_index);
		} else if (decl->kind == DeclKind (block)) {
			result = link_decl_block (unit, scope_index, decl_index);
		} else if (decl->kind == DeclKind (define)) {
			if (decl->define.kind == DefineKind (accessor)) {
				result = link_decl_accessor (unit, scope_index, decl_index);
			} else if (decl->define.kind == DefineKind (external)) {
				result = link_decl_external (unit, scope_index, decl_index);
			} else if (decl->define.kind == DefineKind (visability)) {
				result = link_decl_visability (unit, scope_index, decl_index);
			} else if (decl->define.kind == DefineKind (funcprefix)) {
				result = link_decl_funcprefix (unit, scope_index, decl_index);
			} else if (decl->define.kind == DefineKind (type)) {
				result = link_decl_type (unit, scope_index, decl_index);
			} else if (decl->define.kind == DefineKind (opaque)) {
				result = 1;
			} else if (decl->define.kind == DefineKind (macro)) {
				result = 1;
			} else if (decl->define.kind == DefineKind (builtin)) {
				result = 1;
			} else if (decl->define.kind == DefineKind (assert)) {
				result = link_decl_assert (unit, scope_index, decl_index);
			} else {
				Unreachable ();
			}
		} else if (decl->kind == DeclKind (alias)) {
			result = 1;
		} else {
			Unreachable ();
		}
	} else {
		result = 1;
	}
	if (result && is_global && decl->kind == DeclKind (func)) {
		unit->function_name = decl->name;
		result = link_code_scope (unit, decl->func.scope);
	}
	return (result);
}

int		link_unit_scope (struct unit *unit, uint scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				push_flow_path (unit, scope_index, get_flow_index (unit, flow));
				unit->pos.line = flow->line;
				if (flow->type == FlowType (decl)) {
					unit->link_decl_index = flow->decl.index;
					result = link_decl (unit, scope_index, flow->decl.index, 1);
				} else {
					Link_Error (unit, "unexpected flow type in unit scope");
					result = 0;
				}
				pop_path (unit);
				flow = flow->next ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		} else {
			result = 1;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_unit (struct unit *unit) {
	int		result;

	g_path = &unit->paths;
	push_unit_path (unit);
	result = link_unit_scope (unit, unit->scope);
	pop_path (unit);
	g_path = 0;
	return (result);
}








