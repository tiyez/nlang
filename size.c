

int		size_decl (struct unit *unit, uint decl_index, int is_global);

int		size_type (struct unit *unit, uint type_index, struct sizevalue *value) {
	int			result;
	struct type	*type;

	type = get_type (unit, type_index);
	//Debug ("size type start %s", g_typekind[type->kind]);
	if (type->kind == TypeKind (basic)) {
		if (value) {
			value->size = get_basictype_size (type->basic.type);
			value->alignment = value->size;
			result = 1;
		} else {
			result = 1;
		}
	} else if (type->kind == TypeKind (tag)) {
		if (value) {
			struct unit	*unit_decl;
			struct decl	*decl;

			Assert (type->tag.decl);
			if (is_lib_index (type->tag.decl)) {
				unit_decl = get_lib (get_lib_index (type->tag.decl));
			} else {
				unit_decl = unit;
			}
			decl = get_decl (unit_decl, unlib_index (type->tag.decl));
			Assert (decl->kind == DeclKind (tag));
			if (!decl->is_sized) {
				if (size_decl (unit_decl, unlib_index (type->tag.decl), 0)) {
					value->size = decl->size;
					value->alignment = decl->alignment;
					result = 1;
				} else {
					result = 0;
				}
			} else {
				value->size = decl->size;
				value->alignment = decl->alignment;
				result = 1;
			}
		} else {
			result = 1;
		}
	} else if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (pointer)) {
			if (size_type (unit, type->mod.forward, 0)) {
				if (value) {
					if (unit->flags[Flag (build32)]) {
						value->size = 4;
					} else {
						value->size = 8;
					}
					value->alignment = value->size;
				}
				result = 1;
			} else {
				result = 0;
			}
		} else if (type->mod.kind == TypeMod (array)) {
			struct sizevalue	elementsize = {0};

			if (size_type (unit, type->mod.forward, &elementsize)) {
				if (type->mod.expr) {
					struct unit			*decl_unit;
					struct evalvalue	arraysize = {0};

					if (is_lib_index (type->mod.expr)) {
						decl_unit = get_lib (get_lib_index (type->mod.expr));
					} else {
						decl_unit = unit;
					}
					if (eval_const_expr (decl_unit, unlib_index (type->mod.expr), &arraysize)) {
						if (arraysize.type == EvalType (basic) && is_basictype_integral (arraysize.basic)) {
							if ((is_basictype_signed (arraysize.basic) && arraysize.value > 0) || (is_basictype_unsigned (arraysize.basic) && arraysize.uvalue > 0)) {
								type->mod.count = arraysize.uvalue;
								if (value) {
									value->size = elementsize.size * arraysize.uvalue;
									value->alignment = elementsize.alignment;
								}
								result = 1;
							} else {
								Error ("negative or zero size of array");
								result = 0;
							}
						} else {
							Error ("non-integral size of array");
							result = 0;
						}
					} else {
						result = 0;
					}
				} else {
					Error ("array must have a size");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (type->mod.kind == TypeMod (function)) {
			if (size_type (unit, type->mod.forward, 0)) {
				result = size_param_scope (unit, type->mod.param_scope);
				if (value) {
					value->size = 1;
					value->alignment = 1;
				}
			} else {
				result = 0;
			}
		} else {
			Unreachable ();
		}
	} else if (type->kind == TypeKind (internal)) {
		if (value) {
			struct unit	*decl_unit;
			struct decl	*decl;

			if (is_lib_index (type->internal.decl)) {
				decl_unit = get_lib (get_lib_index (type->internal.decl));
			} else {
				decl_unit = unit;
			}
			decl = get_decl (decl_unit, unlib_index (type->internal.decl));
			Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (external));
			if (!decl->is_sized) {
				if (size_decl (decl_unit, unlib_index (type->internal.decl), 0)) {
					value->size = decl->size;
					value->alignment = decl->alignment;
					result = 1;
				} else {
					result = 0;
				}
			} else {
				value->size = decl->size;
				value->alignment = decl->alignment;
				result = 1;
			}
		} else {
			result = 1;
		}
	} else if (type->kind == TypeKind (opaque)) {
		if (value) {
			value->size = 1;
			value->alignment = 1;
		} else {
			result = 1;
		}
	} else {
		Unreachable ();
	}
	//Debug ("size type end %d", result);
	return (result);
}

int		size_code_scope (struct unit *unit, uint scope_index, struct typestack *typestack);

int		size_macro_instance (struct unit *unit, uint macro_decl_index, struct typestack *typestack) {
	int			result;
	struct decl	*decl;

	//Debug ("size macro instance start");
	decl = get_decl (unit, macro_decl_index);
	Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (macro));
	result = size_code_scope (unit, decl->define.macro.scope, typestack);
	//Debug ("size macro instance end");
	return (result);
}

int		size_typeinfo (struct unit *unit, uint typeinfo_index);

int		size_tag_typemembers (struct unit *unit, uint typemember_index, enum tagtype tagtype, int *pcount, usize *psize) {
	int		result;

	//Debug ("size tag typemembers start");
	if (tagtype == TagType (enum)) {
		result = 1;
		*psize = 4;
		*pcount = 0;
		while (result && typemember_index) {
			struct typemember	*member;

			member = get_typemember (unit, typemember_index);
			if (member->name) {
				*pcount += 1;
				typemember_index = Get_Next_Bucket_Index (unit->buckets->typemembers, typemember_index);
			} else {
				typemember_index = 0;
			}
		}
	} else if (tagtype == TagType (struct) || tagtype == TagType (union)) {
		int		offset;

		offset = 0;
		*pcount = 0;
		result = 1;
		if (get_typemember (unit, typemember_index)->offset < 0) while (result && typemember_index) {
			struct typemember	*member;

			member = get_typemember (unit, typemember_index);
			if (member->name) {
				*pcount += 1;
				if (member->offset < 0) {
					member->offset = 0;
					Assert (member->typeinfo);
					if (size_typeinfo (unit, member->typeinfo)) {
						member = get_typemember (unit, typemember_index);
						if (tagtype == TagType (struct)) {
							usize	size;

							size = get_typeinfo (unit, member->typeinfo)->size;
							Assert (Is_Power_Of_Two (size));
							offset = (offset + size - 1) & ~(size - 1);
							member->offset = offset;
							member->value = 0;
							offset += size;
							result = 1;
						} else {
							member->offset = 0;
							member->value = 0;
							result = 1;
						}
					} else {
						result = 0;
					}
				} else {
					result = 1;
				}
				typemember_index = Get_Next_Bucket_Index (unit->buckets->typemembers, typemember_index);
			} else {
				typemember_index = 0;
			}
		}
		*psize = (offset + 4 - 1) & ~(4 - 1);
	} else {
		Unreachable ();
	}
	//Debug ("size tag typemembers end");
	return (result);
}

int		size_typeinfo (struct unit *unit, uint typeinfo_index) {
	int				result;
	struct typeinfo	*type;

	//Debug ("size typeinfo start");
	type = get_typeinfo (unit, typeinfo_index);
	if (type->kind == TypeInfo_Kind (basic)) {
		type->size = get_basictype_size (type->basic);
		type->count = 1;
		result = 1;
	} else if (type->kind == TypeInfo_Kind (struct)) {
		result = size_tag_typemembers (unit, type->members, TagType (struct), &type->count, &type->size);
	} else if (type->kind == TypeInfo_Kind (enum)) {
		result = size_tag_typemembers (unit, type->members, TagType (enum), &type->count, &type->size);
	} else if (type->kind == TypeInfo_Kind (union)) {
		result = size_tag_typemembers (unit, type->members, TagType (union), &type->count, &type->size);
	} else if (type->kind == TypeInfo_Kind (pointer)) {
		if (unit->flags[Flag (build32)]) {
			type->size = 4;
		} else {
			type->size = 8;
		}
		type->count = 1;
		result = size_typeinfo (unit, type->typeinfo);
	} else if (type->kind == TypeInfo_Kind (array)) {
		if (size_typeinfo (unit, type->typeinfo)) {
			if (type->size) {
				int		expr_index;
				struct evalvalue	value = {0};

				expr_index = type->size;
				if (size_expr (unit, expr_index)) {
					if (eval_const_expr (unit, expr_index, &value)) {
						if (value.type == EvalType (basic) && is_basictype_integral (value.basic)) {
							if (value.value > 0) {
								type->count = value.value;
								type->size = type->count * get_typeinfo (unit, type->typeinfo)->size;
								result = 1;
							} else {
								Error ("value of array size is less or equal to zero");
								result = 0;
							}
						} else {
							Error ("non-integral value for array size");
							result = 0;
						}
					} else {
						Error ("cannot evaluate expression of array size");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				Error ("incomplete array type");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (type->kind == TypeInfo_Kind (function)) {
		type->size = 1;
		type->count = 0;
		if (size_typeinfo (unit, type->typeinfo)) {
			if (type->members) {
				uint	typemember_index;

				typemember_index = type->members;
				result = 1;
				while (result && typemember_index) {
					struct typemember	*member;

					member = get_typemember (unit, typemember_index);
					if (member->name) {
						type->count += 1;
						if (0 != strcmp (member->name, "...")) {
							struct typeinfo	*type;

							Assert (member->typeinfo);
							type = get_typeinfo (unit, member->typeinfo);
							if (type->kind == TypeInfo_Kind (array)) {
								if (unit->flags[Flag (build32)]) {
									type->size = 4;
								} else {
									type->size = 8;
								}
								type->count = 1;
								result = size_typeinfo (unit, type->typeinfo);
							} else {
								result = size_typeinfo (unit, member->typeinfo);
							}
						} else {
							result = 1;
						}
						if (result) {
							member->offset = 0;
							member->value = 0;
						}
						typemember_index = Get_Next_Bucket_Index (unit->buckets->typemembers, typemember_index);
					} else {
						typemember_index = 0;
					}
				}
			} else {
				result = 1;
			}
		} else {
			result = 0;
		}
	} else {
		Unreachable ();
	}
	//Debug ("size typeinfo start");
	return (result);
}

int		size_typestack (struct unit *unit, struct typestack *typestack, struct sizevalue *value) {
	int			result;
	struct type	*type;

	type = get_typestack_head (typestack);
	Assert (type);
	Assert (value);
	if (type->kind == TypeKind (basic)) {
		value->size = get_basictype_size (type->basic.type);
		value->alignment = value->size;
		result = 1;
	} else if (type->kind == TypeKind (tag)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		Assert (type->tag.decl);
		if (is_lib_index (type->tag.decl)) {
			decl_unit = get_lib (get_lib_index (type->tag.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (type->tag.decl));
		Assert (decl->is_sized);
		value->size = decl->size;
		value->alignment = decl->alignment;
		result = 1;
	} else if (type->kind == TypeKind (mod)) {
		if (type->mod.kind == TypeMod (pointer)) {
			if (unit->flags[Flag (build32)]) {
				value->size = 4;
			} else {
				value->size = 8;
			}
			value->alignment = value->size;
			result = 1;
		} else if (type->mod.kind == TypeMod (array)) {
			struct type			head = {0};
			struct sizevalue	elementsize = {0};

			head = *type;
			pop_typestack (typestack);
			if (size_typestack (unit, typestack, &elementsize)) {
				push_typestack (typestack, &head);
				value->size = head.mod.count * elementsize.size;
				value->alignment = elementsize.alignment;
				result = 1;
			} else {
				result = 0;
			}
		} else if (type->mod.kind == TypeMod (function)) {
			value->size = 1;
			value->alignment = 1;
			result = 1;
		}
	} else if (type->kind == TypeKind (internal)) {
		struct unit	*decl_unit;
		struct decl	*decl;

		if (is_lib_index (type->internal.decl)) {
			decl_unit = get_lib (get_lib_index (type->internal.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (type->internal.decl));
		Assert (decl->kind == DeclKind (define) && decl->define.kind == DefineKind (external));
		Assert (decl->is_sized);
		value->size = decl->size;
		value->alignment = decl->alignment;
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}

int		size_expr (struct unit *unit, int expr_index, struct typestack *typestack) {
	int			result;
	struct expr	*expr;
	int			old_sizeof_context;

	//Debug ("size expr start");
	expr = get_expr (unit, expr_index);
	old_sizeof_context = typestack->is_sizeof_context;
	typestack->is_sizeof_context = is_typestack_sizeof_context (typestack, expr);
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (function_call)) {
			if (size_expr (unit, expr->op.forward, typestack)) {
				update_typestack (unit, expr, typestack, 0);
				if (expr->op.backward) {
					struct typestack	rightstack = {0};

					result = size_expr (unit, expr->op.backward, &rightstack);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (array_subscript)) {
			if (size_expr (unit, expr->op.forward, typestack)) {
				struct typestack	rightstack = {0};

				init_typestack (&rightstack, 0);
				if (size_expr (unit, expr->op.backward, &rightstack)) {
					update_typestack (unit, expr, typestack, &rightstack);
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (cast)) {
			if (size_expr (unit, expr->op.forward, typestack)) {
				struct sizevalue	value = {0};

				if (size_type (unit, expr->op.backward, &value)) {
					update_typestack (unit, expr, typestack, 0);
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (typesizeof) || expr->op.type == OpType (typealignof)) {
			struct sizevalue	value = {0};

			Assert (expr->op.backward);
			if (size_type (unit, expr->op.backward, &value)) {
				enum optype	optype;

				optype = expr->op.type;
				memset (expr, 0, sizeof *expr);
				expr->type = ExprType (constant);
				expr->constant.type = BasicType (usize);
				if (optype == OpType (typesizeof)) {
					expr->constant.uvalue = value.size;
				} else if (optype == OpType (typealignof)) {
					expr->constant.uvalue = value.alignment;
				} else {
					Unreachable ();
				}
				update_typestack (unit, expr, typestack, 0);
				result = 1;
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (sizeof) || expr->op.type == OpType (alignof)) {
			if (size_expr (unit, expr->op.forward, typestack)) {
				struct sizevalue	value = {0};

				if (size_typestack (unit, typestack, &value)) {
					enum optype	optype;

					optype = expr->op.type;
					expr->type = ExprType (constant);
					expr->constant.type = BasicType (usize);
					if (optype == OpType (sizeof)) {
						expr->constant.uvalue = value.size;
					} else {
						expr->constant.uvalue = value.alignment;
					}
					update_typestack (unit, expr, typestack, 0);
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else {
			if (is_expr_unary (expr)) {
				if (size_expr (unit, expr->op.forward, typestack)) {
					update_typestack (unit, expr, typestack, 0);
					result = 1;
				} else {
					result = 0;
				}
			} else {
				if (size_expr (unit, expr->op.backward, typestack)) {
					struct typestack	rightstack = {0};

					init_typestack (&rightstack, 0);
					if (size_expr (unit, expr->op.forward, &rightstack)) {
						update_typestack (unit, expr, typestack, &rightstack);
						result = 1;
					} else {
						result = 0;
					}
				} else {
					result = 0;
				}
			}
		}
	} else if (expr->type == ExprType (funcparam)) {
		if (expr->funcparam.expr) {
			init_typestack (typestack, typestack->is_sizeof_context);
			result = size_expr (unit, expr->funcparam.expr, typestack);
		} else {
			result = 1;
		}
		if (result) {
			if (expr->funcparam.next) {
				result = size_expr (unit, expr->funcparam.next, typestack);
			} else {
				result = 1;
			}
		}
	} else if (expr->type == ExprType (typeinfo)) {
		Assert (expr->typeinfo.index);
		if (size_typeinfo (unit, expr->typeinfo.index)) {
			update_typestack (unit, expr, typestack, 0);
			result = 1;
		}
	} else if (expr->type == ExprType (macrocall)) {
		Assert (expr->macrocall.instance);
		result = size_macro_instance (unit, expr->macrocall.instance, typestack);
	} else if (expr->type == ExprType (identifier)) {
		struct decl	*decl;
		struct unit	*decl_unit;

		Assert (expr->iden.decl);
		if (is_lib_index (expr->iden.decl)) {
			decl_unit = get_lib (get_lib_index (expr->iden.decl));
		} else {
			decl_unit = unit;
		}
		decl = get_decl (decl_unit, unlib_index (expr->iden.decl));
		if (!decl->is_sized) {
			if (size_decl (decl_unit, unlib_index (expr->iden.decl), 0)) {
				push_typestack_recursive (unit, decl_unit, typestack, decl->type);
				typestack->value = ValueCategory (lvalue);
				result = 1;
			} else {
				result = 0;
			}
		} else {
			init_typestack (typestack, typestack->is_sizeof_context);
			push_typestack_recursive (unit, decl_unit, typestack, decl->type);
			result = 1;
		}
		if (result) {
			struct type	*left;

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
					init_typestack (typestack, typestack->is_sizeof_context);
					push_basictype_to_typestack (typestack, BasicType (int), 0);
				}
				typestack->value = ValueCategory (lvalue);
			}
		}
	} else {
		update_typestack (unit, expr, typestack, 0);
		result = 1;
	}
	typestack->is_sizeof_context = old_sizeof_context;
	//Debug ("size expr end");
	return (result);
}

int		size_tag_scope (struct unit *unit, uint scope_index, struct sizevalue *value) {
	int				result;
	struct scope	*scope;
	unsigned		offset;

	//Debug ("size tag scope start");
	offset = 0;
	scope = get_scope (unit, scope_index);
	if (scope->tagtype == TagType (enum)) {
		value->size = 4;
		value->alignment = 4;
	} else {
		value->size = 0;
		value->alignment = 0;
	}
	Assert (scope->kind == ScopeKind (tag));
	if (scope->decl_begin) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			if (scope->tagtype == TagType (enum)) {
				if (decl->enumt.params) {
					struct typestack	typestack = {0};

					init_typestack (&typestack, 0);
					result = size_expr (unit, decl->enumt.params, &typestack);
				} else {
					result = 1;
				}
			} else if (scope->tagtype == TagType (struct) || scope->tagtype == TagType (union)) {
				if (decl->kind == DeclKind (block)) {
					struct sizevalue	blocksize = {0};

					if (!decl->is_in_process) {
						decl->is_in_process = 1;
						if (size_tag_scope (unit, decl->block.scope, &blocksize)) {
							decl->is_in_process = 0;
							decl->is_sized = 1;
							decl->size = blocksize.size;
							decl->alignment = blocksize.alignment;
							Assert (blocksize.alignment > 0);
							if (offset % blocksize.alignment) {
								offset += get_alignment_diff (offset, blocksize.alignment);
							}
							decl->offset = offset;
							offset += blocksize.size;
							if (blocksize.alignment > value->alignment) {
								value->alignment = blocksize.alignment;
							}
							result = 1;
						} else {
							decl->is_in_process = 0;
							result = 0;
						}
					} else {
						result = 1;
					}
				} else {
					struct sizevalue	membervalue = {0};

					Assert (decl->type);
					if (size_type (unit, decl->type, &membervalue)) {
						decl->size = membervalue.size;
						Assert (membervalue.alignment > 0);
						if (offset % membervalue.alignment) {
							offset += get_alignment_diff (offset, membervalue.alignment);
						}
						decl->offset = offset;
						offset += membervalue.size;
						if (membervalue.alignment > value->alignment) {
							value->alignment = membervalue.alignment;
						}
						result = 1;
					} else {
						result = 0;
					}
				}
			} else {
				Unreachable ();
			}
			if (decl->next) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
		if (result && scope->tagtype != TagType (enum)) {
			if (offset % value->alignment) {
				offset += get_alignment_diff (offset, value->alignment);
			}
			value->size = offset;
		}
	} else {
		Unreachable ();
	}
	//Debug ("size tag scope end %d", result);
	return (result);
}

int		size_flow (struct unit *unit, uint flow_index, struct typestack *typestack) {
	int			result;
	struct flow	*flow;

	//Debug ("size flow start");
	flow = get_flow (unit, flow_index);
	if (flow->type == FlowType (decl)) {
		result = size_decl (unit, flow->decl.index, 0);
	} else if (flow->type == FlowType (expr)) {
		if (flow->expr.index) {
			result = size_expr (unit, flow->expr.index, typestack);
		} else {
			result = 1;
		}
	} else if (flow->type == FlowType (block)) {
		result = size_code_scope (unit, flow->block.scope, typestack);
	} else if (flow->type == FlowType (if)) {
		if (size_expr (unit, flow->fif.expr, typestack)) {
			init_typestack (typestack, typestack->is_sizeof_context);
			if (size_flow (unit, flow->fif.flow_body, typestack)) {
				if (flow->fif.else_body) {
					init_typestack (typestack, typestack->is_sizeof_context);
					result = size_flow (unit, flow->fif.else_body, typestack);
				} else {
					result = 1;
				}
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (while)) {
		if (size_expr (unit, flow->fwhile.expr, typestack)) {
			init_typestack (typestack, typestack->is_sizeof_context);
			result = size_flow (unit, flow->fwhile.flow_body, typestack);
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (dowhile)) {
		if (size_flow (unit, flow->dowhile.flow_body, typestack)) {
			init_typestack (typestack, typestack->is_sizeof_context);
			result = size_expr (unit, flow->dowhile.expr, typestack);
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (assert)) {
		if (size_expr (unit, flow->assert.expr, typestack)) {
			result = 1;
		} else {
			result = 0;
		}
	} else if (flow->type == FlowType (assert) || flow->type == FlowType (static_assert) || flow->type == FlowType (unreachable)) {
		result = 1;
	} else {
		Unreachable ();
	}
	//Debug ("size flow end");
	return (result);
}

int		size_code_scope (struct unit *unit, uint scope_index, struct typestack *typestack) {
	int				result;
	struct scope	*scope;

	//Debug ("size code scope start");
	scope = get_scope (unit, scope_index);
	if (scope->flow_begin) {
		struct flow	*flow;

		flow = get_flow (unit, scope->flow_begin);
		do {
			init_typestack (typestack, typestack->is_sizeof_context);
			result = size_flow (unit, get_flow_index (unit, flow), typestack);
			if (flow->next) {
				flow = get_flow (unit, flow->next);
			} else {
				flow = 0;
			}
		} while (result && flow);
	} else {
		result = 1;
	}
	//Debug ("size code scope end");
	return (result);
}

int		size_param_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;

	//Debug ("size param scope start");
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		uint	decl_index;

		decl_index = scope->decl_begin;
		do {
			result = size_decl (unit, decl_index, 0);
			decl_index = get_decl (unit, decl_index)->next;
		} while (result && decl_index);
	} else {
		result = 1;
	}
	//Debug ("size param scope end");
	return (result);
}

int		size_decl (struct unit *unit, uint decl_index, int is_global) {
	int			result;
	struct decl	*decl;

	//Debug ("size decl start");
	decl = get_decl (unit, decl_index);
	if (!decl->is_in_process) {
		if (decl->kind == DeclKind (var)) {
			if (!decl->is_sized) {
				struct sizevalue	value = {0};
				struct type			*type;

				decl->is_in_process = 1;
				type = get_type (unit, decl->type);
				if (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (array) && !type->mod.expr) {
					if (decl->var.init_scope) {
						type->mod.count = count_flows_in_scope (unit, decl->var.init_scope);
						Assert (type->mod.count);
						if (size_type (unit, type->mod.forward, &value)) {
							value.size *= type->mod.count;
							result = 1;
						} else {
							result = 0;
						}
					} else {
						Error ("array must have a size");
						result = 0;
					}
				} else {
					result = size_type (unit, decl->type, &value);
				}
				decl->size = value.size;
				decl->alignment = value.alignment;
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
		} else if (decl->kind == DeclKind (const)) {
			decl->is_sized = 1;
			result = 1;
		} else if (decl->kind == DeclKind (tag)) {
			if (!decl->is_sized) {
				struct sizevalue	value = {0};

				decl->is_in_process = 1;
				if (is_opaque_tag_decl (decl)) {
					decl->size = 1;
					decl->alignment = 1;
					decl->offset = 0;
					result = 1;
				} else {
					if (size_tag_scope (unit, decl->tag.scope, &value)) {
						decl->size = value.size;
						decl->alignment = value.alignment;
						decl->offset = 0;
						result = 1;
					} else {
						result = 0;
					}
				}
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
		} else if (decl->kind == DeclKind (func)) {
			struct sizevalue	value = {0};
			struct typestack	typestack = {0};

			if (!decl->is_sized) {
				decl->is_in_process = 1;
				if (size_type (unit, decl->type, &value)) {
					decl->size = value.size;
					decl->alignment = value.alignment;
					result = 1;
				} else {
					result = 0;
				}
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
			if (result && is_global) {
				if (size_code_scope (unit, decl->func.scope, &typestack)) {
					result = 1;
				} else {
					result = 0;
				}
			}
		} else if (decl->kind == DeclKind (define)) {
			if (!decl->is_sized) {
				decl->is_in_process = 1;
				if (decl->define.kind == DefineKind (external)) {
					struct sizevalue	value = {0};

					if (size_type (unit, decl->type, &value)) {
						decl->size = value.size;
						decl->alignment = value.alignment;
						result = 1;
					} else {
						result = 0;
					}
				} else if (decl->define.kind == DefineKind (macro) || decl->define.kind == DefineKind (type) || decl->define.kind == DefineKind (visability) ||
					decl->define.kind == DefineKind (funcprefix) || decl->define.kind == DefineKind (builtin) || decl->define.kind == DefineKind (accessor) ||
					decl->define.kind == DefineKind (assert) || decl->define.kind == DefineKind (opaque)) {
					result = 1;
				} else {
					Unreachable ();
				}
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
		} else if (decl->kind == DeclKind (alias)) {
			decl->is_sized = 1;
			result = 1;
		} else if (decl->kind == DeclKind (block)) {
			if (!decl->is_sized) {
				struct sizevalue	value = {0};

				decl->is_in_process = 1;
				if (is_opaque_tag_decl (decl)) {
					decl->size = 1;
					decl->alignment = 1;
					decl->offset = 0;
					result = 1;
				} else {
					if (size_tag_scope (unit, decl->tag.scope, &value)) {
						decl->size = value.size;
						decl->alignment = value.alignment;
						result = 1;
					} else {
						result = 0;
					}
				}
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
		} else if (decl->kind == DeclKind (param)) {
			if (!decl->is_sized) {
				decl->is_in_process = 1;
				if (decl->type) {
					struct sizevalue	value = {0};
					struct type			*type;

					type = get_type (unit, decl->type);
					if (type->kind == TypeKind (mod) && type->mod.kind == TypeMod (array)) {
						if (size_type (unit, type->mod.forward, &value)) {
							if (g_is_build32) {
								value.size = 4;
							} else {
								value.size = 8;
							}
							value.alignment = value.size;
							result = 1;
						} else {
							result = 0;
						}
					} else {
						if (size_type (unit, decl->type, &value)) {
							result = 1;
						} else {
							result = 0;
						}
					}
					decl->size = value.size;
					decl->alignment = value.alignment;
				} else {
					result = 1;
				}
				decl->is_in_process = 0;
				decl->is_sized = 1;
			} else {
				result = 1;
			}
		} else {
			Unreachable ();
		}
	} else {
		Error ("self referencing");
		result = 0;
	}
	//Debug ("size decl end");
	return (result);
}

int		size_unit_scope (struct unit *unit, uint scope_index) {
	int				result;
	struct scope	*scope;

	//Debug ("size unit scope start");
	scope = get_scope (unit, scope_index);
	if (scope->decl_begin) {
		struct decl	*decl;

		decl = get_decl (unit, scope->decl_begin);
		do {
			result = size_decl (unit, get_decl_index (unit, decl), 1);
			if (decl->next) {
				decl = get_decl (unit, decl->next);
			} else {
				decl = 0;
			}
		} while (result && decl);
	} else {
		result = 1;
	}
	//Debug ("size unit scope end");
	return (result);
}

int		size_unit (struct unit *unit) {
	int		result;

	result = size_unit_scope (unit, unit->scope);
	return (result);
}








