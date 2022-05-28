
/*
	TODO: proper self reference check
*/


int		g_link_decl_index;

int		find_ordinary_decl (struct unit *unit, int scope_index, const char *name) {
	int		decl_index;
	struct scope	*scope;

	decl_index = -1;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;
		int			result;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (scope->kind == ScopeKind (unit) || get_decl_index (unit, decl) <= g_link_decl_index) {
				if (decl->kind != DeclKind (tag) && 0 == strcmp (name, decl->name)) {
					decl_index = get_decl_index (unit, decl);
				}
				result = 1;
			} else {
				result = 0;
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl_index < 0 && decl);
	}
	if (decl_index < 0 && scope->param_scope >= 0) {
		decl_index = find_ordinary_decl (unit, scope->param_scope, name);
	}
	if (decl_index < 0 && scope->parent_scope >= 0) {
		decl_index = find_ordinary_decl (unit, scope->parent_scope, name);
	}
	return (decl_index);
}

int		find_decl_tag (struct unit *unit, int scope_index, const char *name, enum tagtype tagtype) {
	int		decl_index;
	struct scope	*scope;

	decl_index = -1;
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;
		int			result;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (scope->kind == ScopeKind (unit) || get_decl_index (unit, decl) <= g_link_decl_index) {
				if (decl->kind == DeclKind (tag) && decl->tag.type == tagtype && 0 == strcmp (name, decl->name)) {
					decl_index = get_decl_index (unit, decl);
				}
				result = 1;
			} else {
				result = 0;
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl_index < 0 && decl);
	}
	if (decl_index < 0 && scope->parent_scope >= 0) {
		decl_index = find_decl_tag (unit, scope->parent_scope, name, tagtype);
	}
	return (decl_index);
}

int		find_struct_member (struct unit *unit, int scope_index, const char *iden) {
	int		result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		result = -1;
		decl = get_decl (unit, scope->decl_begin);
		do {
			if (0 == strcmp (decl->name, iden)) {
				result = get_decl_index (unit, decl);
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result < 0 && decl);
	} else {
		result = -1;
	}
	return (result);
}

int		link_type (struct unit *unit, int scope_index, int type_index, int is_selfref_check);

int		find_enum_by_name (struct unit *unit, int scope_index, const char *name) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		result = -1;
		decl = get_decl (unit, scope->decl_begin);
		do {
			if (0 == strcmp (name, decl->name)) {
				Assert (decl->kind == DeclKind (enum));
				result = get_decl_index (unit, decl);
			}
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result < 0 && decl);
	} else {
		result = -1;
	}
	return (result);
}

int		link_enum_expr (struct unit *unit, int scope_index, int decl_index, int expr_index) {
	int		result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	Assert (expr->type == ExprType (funcparam));
	if (expr->funcparam.expr >= 0) {
		if (expr->funcparam.next < 0) {
			expr = get_expr (unit, expr->funcparam.expr);
			if (expr->type == ExprType (identifier)) {
				struct decl	*decl;

				decl = get_decl (unit, decl_index);
				Assert (decl->kind == DeclKind (accessor));
				Assert (decl->accessor.decl >= 0);
				decl = get_decl (unit, decl->accessor.decl);
				Assert (decl->kind == DeclKind (tag));
				switch (decl->tag.type) {
					case TagType (struct): {
						Todo ();
					} break ;
					case TagType (enum): {
						decl_index = find_enum_by_name (unit, decl->tag.scope, expr->identifier);
						if (decl_index >= 0) {
							expr->type = ExprType (decl);
							expr->decl.index = decl_index;
							result = 1;
						} else {
							Error ("cannot find '%s' in the enum %s", expr->identifier, decl->name);
							result = 0;
						}
					} break ;
					default: Unreachable ();
				}
			} else {
				Error ("invalid accessor parameter");
				result = 0;
			}
		} else {
			Error ("accessor call must have only one parameter");
			result = 0;
		}
	} else {
		Error ("empty accessor call");
		result = 0;
	}
	return (result);
}

int		link_expr (struct unit *unit, int scope_index, int expr_index, int is_selfref_check, struct typestack *typestack);

enum basictype	get_promoted_integer (enum basictype type) {
	switch (type) {
		case BasicType (char):
		case BasicType (uchar):
		case BasicType (wchar):
		case BasicType (byte):
		case BasicType (ubyte):
		case BasicType (short):
		case BasicType (ushort):
		case BasicType (int): {
			type = BasicType (int);
		} break ;
		case BasicType (uint): break ;
		case BasicType (size): break ;
		case BasicType (usize): break ;
		default: {
			Unreachable ();
		}
	}
	return (type);
}

enum basictype	get_common_basictype (enum basictype left, enum basictype right) {
	enum basictype	result;

	if (left == BasicType (double) || right == BasicType (double)) {
		result = BasicType (double);
	} else if (left == BasicType (float) || right == BasicType (float)) {
		result = BasicType (float);
	} else if (is_basictype_integral (left) && is_basictype_integral (right)) {
		left = get_promoted_integer (left);
		right = get_promoted_integer (right);
		if (left == right) {
			result = 1;
		} else if (is_basictype_signed (left) == is_basictype_signed (right)) {
			if (left < right) {
				result = right;
			} else {
				result = left;
			}
		} else {
			if (is_basictype_signed (left)) {
				if (right >= left) {
					result = right;
				} else if (get_basictype_size (left) > get_basictype_size (right)) {
					result = left;
				} else {
					Assert (is_basictype_signed (left));
					Assert (is_basictype_unsigned (left + 1));
					Assert (get_basictype_size (left) == get_basictype_size (left + 1));
					result = left + 1;
				}
			} else {
				if (left >= right) {
					result = left;
				} else if (get_basictype_size (right) > get_basictype_size (left)) {
					result = right;
				} else {
					Assert (is_basictype_signed (right));
					Assert (is_basictype_unsigned (right + 1));
					Assert (get_basictype_size (right) == get_basictype_size (right + 1));
					result = right + 1;
				}
			}
		}
	} else {
		result = BasicType (void);
	}
	return (result);
}

int		is_same_type (struct typestack *leftstack, struct typestack *rightstack) {
	int			result;
	struct type	*left, *right;

	left = get_typestack_head (leftstack);
	right = get_typestack_head (rightstack);
	if (left->kind == right->kind) {
		if (left->kind == TypeKind (basic)) {
			result = (left->basic.type == right->basic.type);
		} else if (left->kind == TypeKind (tag)) {
			result = (left->tag.type == right->tag.type && 0 == strcmp (left->tag.name, right->tag.name));
		} else if (left->kind == TypeKind (mod)) {
			struct type	lefttype, righttype;

			lefttype = *left;
			righttype = *right;
			pop_typestack (leftstack);
			pop_typestack (rightstack);
			result = is_same_type (leftstack, rightstack);
			push_typestack (leftstack, &lefttype);
			push_typestack (rightstack, &righttype);
		} else if (left->kind == TypeKind (accessor)) {
			result = 1;
		} else {
			Unreachable ();
		}
	} else {
		result = 0;
	}
	return (result);
}

int		is_implicit_castable (struct typestack *leftstack, struct typestack *rightstack) {
	int		result;
	struct type	*left;
	struct type	*right;

	left = get_typestack_head (leftstack);
	right = get_typestack_head (rightstack);
	Assert (left);
	Assert (right);
	if (right->kind == TypeKind (accessor) || left->kind == TypeKind (accessor)) {
		Error ("accessor type cast is undefined");
		result = 0;
	} else switch (left->kind) {
		case TypeKind (basic): {
			switch (right->kind) {
				case TypeKind (basic): {
					if (BasicType (void) != get_common_basictype (right->basic.type, left->basic.type)) {
						result = 1;
					} else {
						Error ("cannot cast %s to %s", get_basictype_name (right->basic.type), get_basictype_name (left->basic.type));
					}
				} break ;
				case TypeKind (tag): {
					Error ("cannot cast %s to %s %s", get_basictype_name (right->basic.type), g_tagname[left->tag.type], left->tag.name);
					result = 0;
				} break ;
				case TypeKind (mod): {
					Error ("cannot implicit cast pointer type to basic");
					result = 0;
				} break ;
				default: Unreachable ();
			}
		} break ;
		case TypeKind (tag): {
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
		} break ;
		case TypeKind (mod): {
			if (right->kind == TypeKind (mod)) {
				if (left->mod.kind == TypeMod (array) && right->mod.kind != TypeMod (function)) {
					struct type	lefttype, righttype;

					lefttype = *left;
					righttype = *right;
					pop_typestack (leftstack);
					pop_typestack (rightstack);
					result = is_same_type (leftstack, rightstack);
					push_typestack (leftstack, &lefttype);
					push_typestack (rightstack, &righttype);
				} else if (left->mod.kind == TypeMod (function)) {
					Error ("illegal function type");
					result = 0;
				} else if (right->mod.kind == TypeMod (function)) {
					Error ("illegal function type");
					result = 0;
				} else {
					result = is_same_type (leftstack, rightstack);
					if (!result) {
						Error ("type mismatch");
					}
				}
			} else {
				Error ("cannot implicit cast type");
				result = 0;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_funcparams (struct unit *unit, int scope_index, int param_scope_index, int expr_index) {
	int		result;
	struct scope	*param_scope;
	struct expr		*expr;

	param_scope = get_scope (unit, param_scope_index);
	if (param_scope->decl_begin >= 0) {
		if (expr_index >= 0) {
			struct decl	*decl;

			decl = get_decl (unit, param_scope->decl_begin);
			do {
				if (expr_index >= 0) {
					struct typestack	ctypestack, *typestack = &ctypestack;

					expr = get_expr (unit, expr_index);
					Assert (expr->type == ExprType (funcparam));
					Assert (expr->funcparam.expr >= 0);
					init_typestack (typestack);
					if (link_expr (unit, scope_index, expr->funcparam.expr, 0, typestack)) {
						struct typestack	leftstack;

						init_typestack (&leftstack);
						push_typestack_recursive (unit, &leftstack, decl->type);
						if (is_implicit_castable (&leftstack, typestack)) {
							expr_index = expr->funcparam.next;
							result = 1;
						} else {
							Error ("parameter type mismatch");
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					Error ("too few arguments");
					result = 0;
				}
				decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
			} while (result && decl);
			if (result && expr_index >= 0) {
				Error ("too many arguments");
				result = 0;
			}
		} else {
			Error ("too few arguments");
			result = 0;
		}
	} else if (expr_index >= 0) {
		Error ("too many arguments");
		result = 0;
	} else {
		result = 1;
	}
	return (result);
}

int		link_member_access (struct unit *unit, int decl_index, int expr_index, struct typestack *typestack) {
	int		result;
	struct decl	*decl;
	struct expr	*expr;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (tag));
	if (decl->tag.type == TagType (struct)) {
		expr = get_expr (unit, expr_index);

		if (expr->type == ExprType (identifier)) {
			int		member_decl;

			member_decl = find_struct_member (unit, decl->tag.scope, expr->identifier);
			if (member_decl >= 0) {
				decl = get_decl (unit, member_decl);
				if (decl->type >= 0) {
					result = push_typestack_recursive (unit, typestack, decl->type);
					expr->type = ExprType (decl);
					expr->decl.index = member_decl;
				} else {
					Error ("untyped member '%s' in struct %s", expr->identifier, decl->name);
					result = 0;
				}
			} else {
				Error ("cannot find '%s' member in struct %s", expr->identifier, decl->name);
				result = 0;
			}
		} else {
			Error ("invalid member access operand");
			result = 0;
		}
	} else {
		Error ("member access for non-struct value");
		result = 0;
	}
	return (result);
}

int		link_expr (struct unit *unit, int scope_index, int expr_index, int is_selfref_check, struct typestack *typestack) {
	int		result;
	struct expr	*expr;

	expr = get_expr (unit, expr_index);
	switch (expr->type) {
		case ExprType (op): {
			switch (expr->op.type) {
				case OpType (function_call): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
						if (typestack->types_count == 1 && get_typestack_head (typestack)->kind == TypeKind (accessor)) {
							struct type	*head;

							result = link_enum_expr (unit, scope_index, get_expr (unit, expr->op.forward)->decl.index, expr->op.backward);
							head = get_typestack_head (typestack);
							head->kind = TypeKind (basic);
							head->basic.type = BasicType (int);
						} else {
							struct type	*head;

							head = get_typestack_head (typestack);
							if (head->kind == TypeKind (mod) && head->mod.kind == TypeMod (pointer)) {
								pop_typestack (typestack);
								head = get_typestack_head (typestack);
							}
							if (head->kind == TypeKind (mod) && head->mod.kind == TypeMod (function)) {
								if (link_funcparams (unit, scope_index, head->mod.func.param_scope, expr->op.backward)) {
									pop_typestack (typestack);
									result = 1;
								} else {
									result = 0;
								}
							} else {
								Error ("operand of the _ () is not a pointer to function");
								result = 0;
							}
						}
					} else {
						result = 0;
					}
				} break ;
				case OpType (array_subscript): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
						struct typestack	rightstack;

						init_typestack (&rightstack);
						result = link_expr (unit, scope_index, expr->op.backward, is_selfref_check, &rightstack);
						if (get_typestack_head (&rightstack)->kind == TypeKind (basic) && is_basictype_integral (get_typestack_head (&rightstack)->basic.type)) {
							if (get_typestack_head (typestack)->kind == TypeKind (mod) && (get_typestack_head (typestack)->mod.kind == TypeMod (pointer) || get_typestack_head (typestack)->mod.kind == TypeMod (array))) {
								pop_typestack (typestack);
								result = 1;
							} else {
								Error ("one of the array subscript operands must be pointer");
								result = 0;
							}
						} else if (get_typestack_head (&rightstack)->kind == TypeKind (mod) && (get_typestack_head (&rightstack)->mod.kind == TypeMod (pointer) || get_typestack_head (&rightstack)->mod.kind == TypeMod (array))) {
							if (get_typestack_head (typestack)->kind == TypeKind (basic) && is_basictype_integral (get_typestack_head (typestack)->basic.type)) {
								pop_typestack (&rightstack);
								*typestack = rightstack;
								result = 1;
							} else {
								Error ("one of the array subscript operands must be integral");
								result = 0;
							}
						} else {
							Error ("one of the array subscript operands must be integral");
							result = 0;
						}
						/* check if rightstack is integral */
					} else {
						result = 0;
					}
				} break ;
				case OpType (cast): {
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
						result = link_type (unit, scope_index, expr->op.backward, 1);
					} else {
						result = 0;
					}
				} break ;
				case OpType (indirect_access):
				case OpType (member_access): {
					if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
						if (expr->op.type == OpType (indirect_access)) {
							if (get_typestack_head (typestack)->kind == TypeKind (mod) && get_typestack_head (typestack)->mod.kind == TypeMod (pointer)) {
								Debug ("pop pointer mod");
								pop_typestack (typestack);
								result = 1;
							} else {
								Error ("indirect access on non-pointer value");
								result = 0;
							}
						} else {
							result = 1;
						}
						if (result) {
							if (get_typestack_head (typestack)->kind == TypeKind (tag) && get_typestack_head (typestack)->tag.type == TagType (struct)) {
								struct typestack	ctypestack, *rightstack = &ctypestack;
								struct type			*type;

								init_typestack (rightstack);
								type = get_typestack_head (typestack);
								Assert (type->tag.decl >= 0);
								result = link_member_access (unit, type->tag.decl, expr->op.forward, rightstack);
								*typestack = *rightstack;
							} else {
								Error ("member access on non-struct value; %s %s", g_typekind[get_typestack_head (typestack)->kind], g_typemod[get_typestack_head (typestack)->mod.kind]);
								result = 0;
							}
						}
					} else {
						result = 0;
					}
				} break ;
				case OpType (address_of): {
					is_selfref_check = 0;
					if (link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack)) {
						struct type	ptr = {0};

						/* check if lvalue */
						ptr.kind = TypeKind (mod);
						ptr.mod.kind = TypeMod (pointer);
						push_typestack (typestack, &ptr);
						result = 1;
					} else {
						result = 0;
					}
				} break ;
				default: {
					if (is_expr_unary (expr)) {
						if (expr->op.type == OpType (address_of)) {
							is_selfref_check = 0;
						} else if (expr->op.type == OpType (group)) {
							is_selfref_check = 1;
						}
						result = link_expr (unit, scope_index, expr->op.forward, is_selfref_check, typestack);
					} else {
						if (link_expr (unit, scope_index, expr->op.backward, is_selfref_check, typestack)) {
							struct typestack	ctypestack, *rightstack = &ctypestack;

							init_typestack (rightstack);
							result = link_expr (unit, scope_index, expr->op.forward, is_selfref_check, rightstack);
						} else {
							result = 0;
						}
					}
				} break ;
			}
		} break ;
		case ExprType (funcparam): {
			if (link_expr (unit, scope_index, expr->funcparam.expr, is_selfref_check, typestack)) {
				if (expr->funcparam.next >= 0) {
					result = link_expr (unit, scope_index, expr->funcparam.next, is_selfref_check, typestack);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} break ;
		case ExprType (decl): {
			struct decl	*decl;

			decl = get_decl (unit, expr->decl.index);
			if (decl->type >= 0) {
				result = push_typestack_recursive (unit, typestack, decl->type);
				if (decl->kind == DeclKind (func)) {
					struct type	ptr = {0};

					ptr.kind = TypeKind (mod);
					ptr.mod.kind = TypeMod (pointer);
					push_typestack (typestack, &ptr);
					typestack->is_lvalue = 0;
				} else {
					typestack->is_lvalue = 1;
				}
			} else {
				Error ("untyped decl %s", decl->name);
				result = 0;
			}
		} break ;
		case ExprType (constant): {
			struct type	type = {0};

			type.kind = TypeKind (basic);
			type.basic.type = expr->constant.type;
			result = push_typestack (typestack, &type);
			typestack->is_lvalue = 0;
		} break ;
		case ExprType (identifier): {
			int		decl_index;

			decl_index = find_ordinary_decl (unit, scope_index, expr->identifier);
			if (decl_index >= 0) {
				if (is_selfref_check) {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					if (decl->is_in_process == 0) {
						expr->type = ExprType (decl);
						expr->decl.index = decl_index;
						if (decl->type >= 0) {
							result = push_typestack_recursive (unit, typestack, decl->type);
							if (decl->kind == DeclKind (func)) {
								struct type	ptr = {0};

								ptr.kind = TypeKind (mod);
								ptr.mod.kind = TypeMod (pointer);
								push_typestack (typestack, &ptr);
								typestack->is_lvalue = 0;
							} else {
								typestack->is_lvalue = 1;
							}
						} else {
							Error ("untyped decl %s", decl->name);
							result = 0;
						}
					} else {
						Error ("self referencing declaration '%s'", decl->name);
						result = 0;
					}
				} else {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					expr->type = ExprType (decl);
					expr->decl.index = decl_index;
					if (decl->type >= 0) {
						result = push_typestack_recursive (unit, typestack, decl->type);
						if (decl->kind == DeclKind (func)) {
							struct type	ptr = {0};

							ptr.kind = TypeKind (mod);
							ptr.mod.kind = TypeMod (pointer);
							push_typestack (typestack, &ptr);
							typestack->is_lvalue = 0;
						} else {
							typestack->is_lvalue = 1;
						}
					} else {
						Error ("untyped decl %s", decl->name);
						result = 0;
					}
					result = 1;
				}
			} else {
				Error ("undeclared identifier '%s'", expr->identifier);
				result = 0;
			}
		} break ;
		case ExprType (typeinfo): {
			result = link_type (unit, scope_index, expr->typeinfo.type, 1);
		} break ;
		default:
		Unreachable ();
	}
	return (result);
}

int		link_type (struct unit *unit, int scope_index, int type_index, int is_selfref_check) {
	int		result;
	struct type	*type;

	type = get_type (unit, type_index);
	switch (type->kind) {
		case TypeKind (mod): {
			switch (type->mod.kind) {
				case TypeMod (pointer): result = link_type (unit, scope_index, type->mod.ptr.type, 0); break ;
				case TypeMod (function): result = link_type (unit, scope_index, type->mod.func.type, 0); break ;
				case TypeMod (array): result = link_type (unit, scope_index, type->mod.array.type, 0); break ;
				default: Unreachable ();
			}
		} break ;
		case TypeKind (basic): {
			result = 1;
		} break ;
		case TypeKind (group): {
			result = link_type (unit, scope_index, type->group.type, is_selfref_check);
		} break ;
		case TypeKind (tag): {
			int		decl_index;

			decl_index = find_decl_tag (unit, scope_index, type->tag.name, type->tag.type);
			if (decl_index >= 0) {
				Debug ("resolving %s tag '%s' with selfref check %d", g_tagname[type->tag.type], type->tag.name, is_selfref_check);
				if (is_selfref_check) {
					struct decl	*decl;

					decl = get_decl (unit, decl_index);
					if (decl->is_in_process == 0) {
						type->kind = TypeKind (decl);
						type->decl.index = decl_index;
						result = 1;
					} else {
						Error ("self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
						result = 0;
					}
				} else {
					type->kind = TypeKind (decl);
					type->decl.index = decl_index;
					result = 1;
				}
			} else {
				Error ("undeclared %s tag '%s'", g_tagname[type->tag.type], type->tag.name);
				result = 0;
			}
		} break ;
		case TypeKind (typeof): {
			struct typestack	ctypestack, *typestack = &ctypestack;

			init_typestack (typestack);
			if (link_expr (unit, scope_index, type->typeof.expr, 1, typestack)) {
				result = insert_typestack_to_type (unit, type_index, typestack);
			} else {
				result = 0;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_decl_var (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	Assert (decl->type >= 0);
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		result = link_type (unit, scope_index, decl->type, 1);
		decl->is_in_process = 0;
	} else {
		Error ("type loop referencing");
		result = 0;
	}
	return (result);
}

int		link_decl_const (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (decl->is_in_process == 0) {
		struct typestack	ctypestack, *typestack = &ctypestack;

		decl->is_in_process = 1;
		init_typestack (typestack);
		result = link_expr (unit, scope_index, decl->dconst.expr, 1, typestack);
		decl->is_in_process = 0;
		if (result) {
			Assert (decl->type < 0);
			decl->type = make_basic_type (unit, BasicType (void));
			result = insert_typestack_to_type (unit, decl->type, typestack);
		}
	} else {
		Error ("type loop referencing");
		result = 0;
	}
	return (result);
}

int		link_param_scope (struct unit *unit, int scope_index, int param_scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, param_scope_index);
	if (scope->decl_begin >= 0) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			result = link_type (unit, scope_index, decl->type, 1);
			decl = decl->next >= 0 ? get_decl (unit, decl->next) : 0;
		} while (result && decl);
	} else {
		result = 1;
	}
	return (result);
}

int		link_decl (struct unit *unit, int scope_index, int decl_index);
int		link_code_scope (struct unit *unit, int scope_index);

int		link_code_scope_flow (struct unit *unit, int scope_index, int flow_index, int is_flow_body) {
	int			result;
	struct flow	*flow;
	struct typestack typestack;

	init_typestack (&typestack);
	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			g_link_decl_index = flow->decl.index;
			result = link_decl (unit, scope_index, flow->decl.index);
		} break ;
		case FlowType (expr): {
			if (flow->expr.index >= 0) {
				Debug ("linking: expr");
				result = link_expr (unit, scope_index, flow->expr.index, 1, &typestack);
				Debug ("linking: expr end");
			}
		} break ;
		case FlowType (block): {
			result = link_code_scope (unit, flow->block.scope);
		} break ;
		case FlowType (if): {
			if (link_expr (unit, scope_index, flow->fif.expr, 1, &typestack)) {
				/* check if type is ok for if statement */
				if (link_code_scope_flow (unit, scope_index, flow->fif.flow_body, 1)) {
					if (flow->fif.else_body >= 0) {
						result = link_code_scope_flow (unit, scope_index, flow->fif.else_body, 1);
					} else {
						result = 1;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} break ;
		case FlowType (while): {
			if (link_expr (unit, scope_index, flow->fwhile.expr, 1, &typestack)) {
				/* check if type is ok for while statement */
				result = link_code_scope_flow (unit, scope_index, flow->fwhile.flow_body, 1);
			} else {
				result = 0;
			}
		} break ;
		case FlowType (dowhile): {
			if (link_code_scope_flow (unit, scope_index, flow->dowhile.flow_body, 1)) {
				result = link_expr (unit, scope_index, flow->dowhile.expr, 1, &typestack);
				/* check if type is ok for dowhile statement */
			} else {
				result = 0;
			}
		} break ;
		default: Unreachable ();
	}
	if (flow->next < 0 && !is_flow_body && get_scope (unit, scope_index)->kind == ScopeKind (func)) {
		struct scope	*scope;
		struct type		*type;

		scope = get_scope (unit, scope_index);
		Assert (scope->type_index >= 0);
		type = get_type (unit, scope->type_index);
		Assert (type->kind == TypeKind (mod));
		Assert (type->mod.kind == TypeMod (function));
		type = get_type (unit, type->mod.func.type);
		if (type->kind == TypeKind (basic) && type->basic.type == BasicType (void)) {
			result = 1;
		} else if (flow->type == FlowType (expr) && flow->expr.index >= 0) {
			struct typestack	rightstack;

			init_typestack (&rightstack);
			if (push_typestack_recursive (unit, &rightstack, get_type_index (unit, type))) {
				if (is_implicit_castable (&rightstack, &typestack)) {
					result = 1;
				} else {
					Error ("return type mismatch");
					result = 0;
				}
			} else {
				Error ("cannot push function's return type");
				result = 0;
			}
		} else {
			Error ("function must return value");
			result = 0;
		}
	}
	return (result);
}

int		link_code_scope (struct unit *unit, int scope_index) {
	int				result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin >= 0) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				Debug ("linking: flow");
				result = link_code_scope_flow (unit, scope_index, get_flow_index (unit, flow), 0);
				flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		} else {
			result = 1;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_decl_func (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (decl->is_in_process == 0) {
		decl->is_in_process = 1;
		if (link_type (unit, scope_index, decl->type, 1)) {
			if (link_param_scope (unit, scope_index, decl->func.param_scope)) {
				result = link_code_scope (unit, decl->func.scope);
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
		decl->is_in_process = 0;
	} else {
		Error ("self referencing");
		result = 0;
	}
	return (result);
}

int		link_enum_scope_flow (struct unit *unit, int scope_index, int flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			struct decl	*decl;

			g_link_decl_index = flow->decl.index;
			decl = get_decl (unit, flow->decl.index);
			Assert (decl->kind == DeclKind (enum));
			if (decl->enumt.expr >= 0) {
				if (decl->is_in_process == 0) {
					struct typestack	typestack;

					decl->is_in_process = 1;
					init_typestack (&typestack);
					Debug ("here");
					result = link_expr (unit, scope_index, decl->enumt.expr, 1, &typestack);
					Debug ("here end");
					/* check if type is ok for enum */
					decl->is_in_process = 0;
				} else {
					Error ("self referencing");
					result = 0;
				}
			} else {
				result = 1;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_enum_scope (struct unit *unit, int scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_enum_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		link_struct_scope_flow (struct unit *unit, int scope_index, int flow_index) {
	int			result;
	struct flow	*flow;

	flow = get_flow (unit, flow_index);
	switch (flow->type) {
		case FlowType (decl): {
			g_link_decl_index = flow->decl.index;
			result = link_decl (unit, scope_index, flow->decl.index);
		} break ;
		default: Unreachable ();
	}
	return (result);
}

int		link_struct_scope (struct unit *unit, int scope_index) {
	int				result;
	struct scope	*scope;

	scope = get_scope (unit, scope_index);
	if (scope->flow_begin >= 0) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			result = link_struct_scope_flow (unit, scope_index, get_flow_index (unit, flow));
			flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
		} while (result && flow);
	} else {
		result = 1;
	}
	return (result);
}

int		link_decl_tag (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	if (check_scope_declarations_for_name_uniqueness (unit, decl->tag.scope)) {
		if (decl->is_in_process == 0) {
			decl->is_in_process = 1;
			if (decl->tag.type == TagType (struct) || decl->tag.type == TagType (union) || decl->tag.type == TagType (stroke)) {
				result = link_struct_scope (unit, decl->tag.scope);
			} else if (decl->tag.type == TagType (enum) || decl->tag.type == TagType (bitfield)) {
				result = link_enum_scope (unit, decl->tag.scope);
			} else {
				Error ("unknown tag type");
				result = 0;
			}
			decl->is_in_process = 0;
		} else {
			Error ("self referencing declaration of %s tag '%s'", g_tagname[decl->tag.type], decl->name);
			result = 0;
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_decl_block (struct unit *unit, int scope_index, int decl_index) {
	int			result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	result = link_struct_scope (unit, decl->block.scope);
	return (result);
}

int		link_decl_accessor (struct unit *unit, int scope_index, int decl_index) {
	int			result;
	struct decl	*decl;
	int			tagdecl;

	decl = get_decl (unit, decl_index);
	tagdecl = find_decl_tag (unit, scope_index, decl->accessor.name, decl->accessor.tagtype);
	if (tagdecl >= 0) {
		decl->accessor.decl = tagdecl;
		result = 1;
	} else {
		Error ("cannot link accessor");
		result = 0;
	}
	return (result);
}

int		link_decl (struct unit *unit, int scope_index, int decl_index) {
	int		result;
	struct decl	*decl;

	decl = get_decl (unit, decl_index);
	Debug ("linking: decl %s", decl->name);
	switch (decl->kind) {
		case DeclKind (var): result = link_decl_var (unit, scope_index, decl_index); break ;
		case DeclKind (const): result = link_decl_const (unit, scope_index, decl_index); break ;
		case DeclKind (func): result = link_decl_func (unit, scope_index, decl_index); break ;
		case DeclKind (tag): result = link_decl_tag (unit, scope_index, decl_index); break ;
		case DeclKind (block): result = link_decl_block (unit, scope_index, decl_index); break ;
		case DeclKind (accessor): result = link_decl_accessor (unit, scope_index, decl_index); break ;
		case DeclKind (enum):
		default: Unreachable ();
	}
	return (result);
}

int		link_unit_scope (struct unit *unit, int scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {
		struct scope	*scope;

		scope = get_scope (unit, scope_index);
		if (scope->flow_begin >= 0) {
			struct flow	*flow;

			flow = get_flow (unit, scope->flow_begin);
			do {
				if (flow->type == FlowType (decl)) {
					g_link_decl_index = flow->decl.index;
					result = link_decl (unit, scope_index, flow->decl.index);
				} else {
					Error ("unexpected flow type in unit scope");
					result = 0;
				}
				flow = flow->next >= 0 ? get_flow (unit, flow->next) : 0;
			} while (result && flow);
		}
	} else {
		result = 0;
	}
	return (result);
}

int		link_unit (struct unit *unit) {
	int		result;

	result = link_unit_scope (unit, unit->root_scope);
	return (result);
}








