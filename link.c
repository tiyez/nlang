
/*
	TODO: proper self reference check
*/


uint64	find_ordinary_decl (struct unit *unit, uint scope_index, const char *name) {
	uint64			decl_index;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->kind == ScopeKind (unit)) {
		decl_index = find_decl_in_table (unit, name, TagType (invalid), 0);
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
			decl_index = find_ordinary_decl (unit, scope->param_scope, name);
		}
		if (!decl_index && scope->parent_scope) {
			decl_index = find_ordinary_decl (unit, scope->parent_scope, name);
		}
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

		decl = get_decl (unit, decl_index);
		if (0 == strcmp (name, decl->name)) {
			Assert (decl->kind == DeclKind (enum));
			stop = 1;
		} else {
			decl_index = decl->next;
			stop = 0;
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

					push_expr_path (unit, expr_index);
					expr = get_expr (unit, expr_index);
					if (expr->type == ExprType (macroparam)) {
						uint	copy_expr;
						struct decl	*decl;

						Assert (!expr->macroparam.expr);
						decl = get_decl (unit, expr->macroparam.decl);
						Assert (decl->kind == DeclKind (param));
						Assert (decl->param.expr);
						copy_expr = make_expr_copy (unit, unit, decl->param.expr);
						Assert (copy_expr);
						*expr = *get_expr (unit, copy_expr);
					}
					Assert (expr->type == ExprType (funcparam));
					Assert (expr->funcparam.expr);
					init_typestack (typestack);
					if (link_expr (unit, scope_index, expr->funcparam.expr, 0, typestack)) {
						if (decl->type) {
							struct typestack	leftstack;

							init_typestack (&leftstack);
							push_typestack_recursive (unit, decl_unit, &leftstack, decl->type);
							if (is_implicit_castable (unit, &leftstack, typestack, 0)) {
								expr_index = expr->funcparam.next;
								result = 1;
							} else {
								Link_Error (unit, "parameter type mismatch");
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
						init_typestack (typestack);
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

					head = get_typestack_head (typestack);
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
										init_typestack (typestack);
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

						decl_index = head->internal.decl;
						init_typestack (typestack);
						macro_instance = make_decl_copy (unit, decl_unit, unlib_index (decl_index));
						if (link_macro_eval (unit, scope_index, macro_instance, expr->op.backward, typestack)) {
							memset (expr, 0, sizeof *expr);
							expr->type = ExprType (macrocall);
							expr->macrocall.decl = decl_index;
							expr->macrocall.instance = macro_instance;
							result = 1;
						} else {
							result = 0;
						}
					} else if (decl->define.kind == DefineKind (builtin)) {
						Unreachable ();
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

				init_typestack (&rightstack);
				if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, &rightstack)) {
					if (is_type_integral (get_typestack_head (&rightstack))) {
						if (is_pointer_type (get_typestack_head (typestack))) {
							pop_typestack (typestack);
							result = 1;
						} else {
							Link_Error (unit, "one of the array subscript operands must be pointer");
							result = 0;
						}
					} else if (is_pointer_type (get_typestack_head (typestack))) {
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
					init_typestack (typestack);
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

					head = get_typestack_head (typestack);
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

								init_typestack (rightstack);
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
			is_selfref_check = 0;
			if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
				if (typestack->value == ValueCategory (lvalue)) {
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
			if (link_type (unit, scope_index, expr->op.forward, 1)) {
				init_typestack (typestack);
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
				init_typestack (typestack);
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
							init_typestack (typestack);
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
				} else if (is_pointer_type (head)) {
					init_typestack (typestack);
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
				if (is_pointer_type (get_typestack_head (typestack))) {
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

				init_typestack (&rightstack);
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
									init_typestack (typestack);
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
					} else if (is_pointer_type (left) || is_pointer_type (right)) {
						if (is_pointer_type (left) && is_pointer_type (right)) {
							if (!is_assignment_optype (expr->op.type)) {
								if (is_comparison_optype (expr->op.type)) {
									init_typestack (typestack);
									push_basictype_to_typestack (typestack, BasicType (int), 0);
									typestack->value = ValueCategory (rvalue);
									result = 1;
								} else if (expr->op.type == OpType (subtract)) {
									init_typestack (typestack);
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

							if (is_pointer_type (left)) {
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
												init_typestack (typestack);
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

				init_typestack (&rightstack);
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
						} else if (is_pointer_type (left) && is_pointer_type (right)) {
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
						} else if (is_pointer_type (left) && right->kind == TypeKind (basic)) {
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

				init_typestack (&rightstack);
				if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, &rightstack)) {
					struct type	*left, *right;

					left = get_typestack_head (typestack);
					right = get_typestack_head (&rightstack);
					if (is_pointer_type (left) || (left->kind == TypeKind (basic) && is_basictype_integral (left->basic.type))) {
						if (is_pointer_type (right) || (right->kind == TypeKind (basic) && is_basictype_integral (right->basic.type))) {
							init_typestack (typestack);
							push_basictype_to_typestack (typestack, BasicType (int), 0);
							typestack->value = ValueCategory (rvalue);
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

				init_typestack (&rightstack);
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
			Debug ("op %s", g_opname[expr->op.type]);
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
		init_typestack (typestack);
		push_basictype_to_typestack (typestack, expr->constant.type, 0);
		typestack->value = ValueCategory (rvalue);
		result = 1;
	} else if (expr->type == ExprType (identifier)) {
		init_typestack (typestack);
		if (0 == strcmp (expr->iden.name, "__Filename")) {
			expr->type = ExprType (string);
			expr->string.token = unit->filename;
			push_const_char_pointer_to_typestack (typestack);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else if (0 == strcmp (expr->iden.name, "__Function")) {
			expr->type = ExprType (string);
			expr->string.token = unit->function_name;
			push_const_char_pointer_to_typestack (typestack);
			typestack->value = ValueCategory (rvalue);
			result = 1;
		} else {
			uint64	decl_index;
			struct decl	*decl;
			struct unit	*decl_unit;

			decl_index = find_ordinary_decl (unit, scope_index, expr->iden.name);
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
				Link_Error (unit, "undeclared identifier '%s'", expr->iden.name);
				result = 0;
			}
			if (result) {
				if (decl->kind == DeclKind (const)) {
					if (decl->is_linked) {
						result = 1;
					} else {
						if (link_decl (decl_unit, decl_unit->scope, get_decl_index (decl_unit, decl))) {
							result = 1;
						} else {
							result = 0;
						}
					}
					if (result) {
						uint	copy_expr;

						copy_expr = make_expr_copy (unit, decl_unit, decl->dconst.expr);
						Assert (copy_expr);
						expr->type = ExprType (op);
						expr->op.type = OpType (group);
						expr->op.forward = copy_expr;
						update_typestack_recursive (unit, copy_expr, typestack);
					}
				} else if (decl->type) {
					result = push_typestack_recursive (unit, decl_unit, typestack, decl->type);
					if (get_typestack_head (typestack)->kind == TypeKind (mod) && get_typestack_head (typestack)->mod.kind == TypeMod (function)) {
						Assert (decl->kind == DeclKind (func) || decl->kind == DeclKind (define));
						push_pointer_type_to_typestack (typestack, 0);
						typestack->value = ValueCategory (rvalue);
					} else {
						if (get_typestack_head (typestack)->kind == TypeKind (tag) && get_typestack_head (typestack)->tag.type == TagType (enum)) {
							init_typestack (typestack);
							push_basictype_to_typestack (typestack, BasicType (int), 0);
						}
						typestack->value = ValueCategory (lvalue);
					}
				} else if (decl->kind == DeclKind (define)) {
					struct type	type = {0};

					type.kind = TypeKind (internal);
					type.internal.decl = decl_index;
					result = push_typestack (typestack, &type);
					typestack->value = ValueCategory (rvalue);
				} else if ((unit == decl_unit) && decl->kind == DeclKind (alias)) {
					replace_expr_with_copy (unit, expr, decl_unit, decl->alias.expr);
					result = link_expr (unit, scope_index, get_expr_index (unit, expr), is_selfref_check, typestack);
				} else {
					Link_Error (unit, "untyped decl %s", decl->name);
					result = 0;
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
				init_typestack (typestack);
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
		expr_copy = make_expr_copy (unit, unit, decl->param.expr);
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

		init_typestack (typestack);
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
				decl = get_decl (decl_unit, unlib_index (decl_index));
			} else {
				decl_unit = unit;
				decl = get_decl (decl_unit, decl_index);
			}
			Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (type));
			Assert (decl->type);
			cloned_type = make_type_copy (unit, decl_unit, decl->type);
			if (link_type (unit, scope_index, cloned_type, is_selfref_check)) {
				get_type (unit, cloned_type)->flags.is_group = 1;
				*type = *get_type (unit, cloned_type);
				result = 1;
			} else {
				result = 0;
			}
		} else {
			Link_Error (unit, "undeclared type '%s'", type->deftype.name);
			result = 0;
		}
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
		init_typestack (typestack);
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
				result = link_type (unit, scope_index, decl->type, 1);
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

int		link_decl (struct unit *unit, uint scope_index, uint decl_index);
int		link_code_scope (struct unit *unit, uint scope_index);

int		link_code_scope_flow (struct unit *unit, uint scope_index, uint flow_index) {
	int					result;
	struct flow			*flow;
	struct typestack	typestack;

	init_typestack (&typestack);
	push_flow_path (unit, scope_index, flow_index);
	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		unit->link_decl_index = flow->decl.index;
		result = link_decl (unit, scope_index, flow->decl.index);
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
				result = link_code_scope (unit, decl->func.scope);
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
		if (decl->enumt.expr) {
			if (decl->is_in_process == 0) {
				struct typestack	typestack;

				decl->is_in_process = 1;
				init_typestack (&typestack);
				result = link_expr (unit, scope_index, decl->enumt.expr, 1, &typestack);
				/* todo: check if type is ok for enum */
				decl->is_in_process = 0;
			} else {
				Link_Error (unit, "self referencing");
				result = 0;
			}
		} else {
			/* todo: link enum table */
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
		result = link_decl (unit, scope_index, flow->decl.index);
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

		init_typestack (&typestack);
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
					init_scope->type_index = make_type_copy (unit, decl_unit, decl->type);
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

int		link_decl (struct unit *unit, uint scope_index, uint decl_index) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (!decl->is_linked) {
		unit->filename = decl->filename;
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
			} else if (decl->define.kind == DefineKind (macro)) {
				result = 1;
			} else if (decl->define.kind == DefineKind (builtin)) {
				result = 1;
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
				if (flow->type == FlowType (decl)) {
					unit->link_decl_index = flow->decl.index;
					result = link_decl (unit, scope_index, flow->decl.index);
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








