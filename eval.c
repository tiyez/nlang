

int		eval_expr (struct unit *unit, uint expr_index, struct evalvalue *out);

int		eval_const_expr (struct unit *unit, uint expr_index, struct evalvalue *out) {
	int		result;

	result = eval_expr (unit, expr_index, out);
	return (result);
}

int		eval_enum_value (struct unit *unit, uint decl_index, uint enum_index, struct evalvalue *out) {
	int				result;
	struct decl		*decl;
	struct scope	*scope;
	int				overindex = 0;
	uint			overexpr = 0;

	decl = get_decl (unit, decl_index);
	Assert (decl->kind == DeclKind (tag));
	Assert (decl->tag.type == TagType (enum));
	scope = get_scope (unit, decl->tag.scope);
	decl_index = scope->decl_begin;
	while (decl_index && decl_index != enum_index) {
		struct decl	*decl;

		decl = get_decl (unit, decl_index);
		if (decl->enumt.expr) {
			overindex = 0;
			overexpr = decl->enumt.expr;
		} else {
			overindex += 1;
		}
		decl_index = decl->next;
	}
	if (overexpr) {
		if (eval_const_expr (unit, overexpr, out)) {
			if (out->type == EvalType (basic) && is_basictype_integral (out->basic)) {
				if (is_basictype_signed (out->basic)) {
					out->value += overindex;
				} else {
					out->uvalue += overindex;
				}
				result = 1;
			} else {
				Error ("non-integral enum value");
				result = 0;
			}
		} else {
			result = 0;
		}
	} else {
		out->type = EvalType (basic);
		out->basic = BasicType (int);
		out->value = overindex;
		result = 1;
	}
	return (result);
}

int		eval_expr (struct unit *unit, uint expr_index, struct evalvalue *out) {
	int			result;
	struct expr	*expr;
	int			is_build32;

	is_build32 = unit->flags[Flag (build32)];
	expr = get_expr (unit, expr_index);
	if (expr->type == ExprType (op)) {
		if (expr->op.type == OpType (function_call)) {
			Error ("function_call operator in constant expression");
			result = 0;
		} else if (expr->op.type == OpType (array_subscript)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				struct evalvalue right = {0}, *ptrvalue = 0, *intvalue = 0;

				if (eval_expr (unit, expr->op.backward, &right)) {
					if (out->type == EvalType (basic) && is_basictype_integral (out->basic)) {
						if (right.type == EvalType (string) || right.type == EvalType (typemember_pointer) || right.type == EvalType (typeinfo_pointer)) {
							ptrvalue = &right;
							intvalue = out;
						} else {
							Error ("one of the operands of _[_] operator must be pointer");
							result = 0;
						}
					} else if (out->type == EvalType (string) || out->type == EvalType (typemember_pointer) || out->type == EvalType (typeinfo_pointer)) {
						if (right.type == EvalType (basic) && is_basictype_integral (right.basic)) {
							ptrvalue = out;
							intvalue = &right;
						} else {
							Error ("one of the operands of _[_] operator must be integral");
							result = 0;
						}
					} else {
						Error ("one of the operands of _[_] operator must be integral");
						result = 0;
					}
					if (result) {
						if (ptrvalue->type == EvalType (string)) {
							int			index;
							const char	*token;

							index = intvalue->value;
							token = ptrvalue->string;
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							if (index >= 0 && index < get_token_length (token)) {
								out->value = (unsigned char) token[index];
								result = 1;
							} else {
								Error ("index is passed out of string array");
								result = 0;
							}
						} else if (ptrvalue->type == EvalType (typemember_pointer)) {
							int		index;
							int		typemember_index;

							typemember_index = ptrvalue->typemember_index;
							index = intvalue->value;
							typemember_index += index;
							if (Is_Bucket_Index_Valid (unit->typemembers, typemember_index)) {
								out->type = EvalType (typemember);
								out->typemember_index = typemember_index;
								result = 1;
							} else {
								Error ("index is outbound of typemember array");
								result = 0;
							}
						} else if (ptrvalue->type == EvalType (typeinfo_pointer)) {
							int		index;
							int		typeinfo_index;

							typeinfo_index = ptrvalue->typeinfo_index;
							index = intvalue->value;
							typeinfo_index += index;
							if (Is_Bucket_Index_Valid (unit->typeinfos, typeinfo_index)) {
								out->type = EvalType (typeinfo);
								out->typeinfo_index = typeinfo_index;
								result = 1;
							} else {
								Error ("index is outbound of typemember array");
								result = 0;
							}
						} else {
							Unreachable ();
						}
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (indirect_access) || expr->op.type == OpType (member_access)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				int		is_valid;

				if (expr->op.type == OpType (indirect_access)) {
					is_valid = out->type == EvalType (typeinfo_pointer) || out->type == EvalType (typemember_pointer);
				} else if (expr->op.type == OpType (member_access)) {
					is_valid = out->type == EvalType (typeinfo) || out->type == EvalType (typemember);
				} else {
					Unreachable ();
				}
				if (is_valid) {
					struct expr	*iden_expr;

					iden_expr = get_expr (unit, expr->op.backward);
					Assert (iden_expr->type == ExprType (identifier));
					if (out->type == EvalType (typeinfo)) {
						uint	typeinfo_index;

						typeinfo_index = out->typeinfo_index;
						if (0 == strcmp (iden_expr->iden.name, "size")) {
							out->type = EvalType (basic);
							out->basic = BasicType (usize);
							out->uvalue = get_typeinfo (unit, typeinfo_index)->size;
						} else if (0 == strcmp (iden_expr->iden.name, "count")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typeinfo (unit, typeinfo_index)->count;
						} else if (0 == strcmp (iden_expr->iden.name, "qualifiers")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typeinfo (unit, typeinfo_index)->qualifiers;
						} else if (0 == strcmp (iden_expr->iden.name, "kind")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typeinfo (unit, typeinfo_index)->kind;
						} else if (0 == strcmp (iden_expr->iden.name, "basic")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typeinfo (unit, typeinfo_index)->basic;
						} else if (0 == strcmp (iden_expr->iden.name, "tagname")) {
							out->type = EvalType (string);
							out->string = get_typeinfo (unit, typeinfo_index)->tagname;
						} else if (0 == strcmp (iden_expr->iden.name, "type")) {
							out->type = EvalType (typeinfo_pointer);
							out->typeinfo_index = get_typeinfo (unit, typeinfo_index)->typeinfo;
						} else if (0 == strcmp (iden_expr->iden.name, "members")) {
							out->type = EvalType (typemember_pointer);
							out->typemember_index = get_typeinfo (unit, typeinfo_index)->members;
						} else {
							Error ("struct typeinfo doesn't contain '%s' member", iden_expr->iden.name);
							result = 0;
						}
					} else if (out->type == EvalType (typemember)) {
						uint	typemember_index;

						typemember_index = out->typemember_index;
						if (0 == strcmp (iden_expr->iden.name, "name")) {
							out->type = EvalType (string);
							out->string = get_typemember (unit, typemember_index)->name;
						} else if (0 == strcmp (iden_expr->iden.name, "type")) {
							if (get_typemember (unit, typemember_index)->name) {
								out->type = EvalType (typeinfo_pointer);
								out->typeinfo_index = get_typemember (unit, typemember_index)->typeinfo;
							} else {
								out->type = EvalType (typeinfo_pointer);
								out->typeinfo_index = 0;
							}
						} else if (0 == strcmp (iden_expr->iden.name, "members")) {
							if (get_typemember (unit, typemember_index)->name) {
								out->type = EvalType (typemember_pointer);
								out->typemember_index = 0;
							} else {
								out->type = EvalType (typemember_pointer);
								out->typemember_index = get_typemember (unit, typemember_index)->typeinfo;
							}
						} else if (0 == strcmp (iden_expr->iden.name, "value")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typemember (unit, typemember_index)->value;
						} else if (0 == strcmp (iden_expr->iden.name, "offset")) {
							out->type = EvalType (basic);
							out->basic = BasicType (int);
							out->value = get_typemember (unit, typemember_index)->offset;
						} else {
							Error ("struct typemember doesn't contain '%s' member", iden_expr->iden.name);
							result = 0;
						}
					} else {
						Unreachable ();
					}
				} else {
					Error ("_._ operator on non-struct value");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (unary_plus)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				if (out->type == EvalType (basic)) {
					result = 1;
				} else {
					Error ("invalid operand for !_ operator");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (unary_minus)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				if (out->type == EvalType (basic)) {
					if (is_basictype_integral (out->basic)) {
						if (is_basictype_signed (out->basic)) {
							out->value = -out->value;
						} else {
							out->type = get_basictype_counterpart (out->type);
							out->value = -out->uvalue;
						}
					} else {
						out->fvalue = -out->fvalue;
					}
					result = 1;
				} else {
					Error ("invalid operand for !_ operator");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (logical_not)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				if (out->type == EvalType (basic)) {
					int		value;

					if (is_basictype_integral (out->basic)) {
						if (is_basictype_signed (out->basic)) {
							value = !out->value;
						} else {
							value = !out->uvalue;
						}
					} else {
						value = !out->fvalue;
					}
					out->type = EvalType (basic);
					out->basic = BasicType (int);
					out->value = value;
				} else if (is_evaltype_pointer (out->type)) {
					out->type = EvalType (basic);
					out->basic = BasicType (int);
					out->value = 0;
				} else {
					Error ("invalid operand for !_ operator");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (bitwise_not)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				if (out->type == EvalType (basic)) {
					if (is_basictype_integral (out->basic)) {
						if (is_basictype_signed (out->basic)) {
							out->type = get_basictype_counterpart (out->type);
							out->uvalue = ~out->uvalue;
						} else {
							out->uvalue = ~out->uvalue;
						}
						result = 1;
					} else {
						Error ("bitwise_not operator is undefined for floating point values");
						result = 0;
					}
				} else {
					Error ("invalid operand for !_ operator");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (indirect)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				if (is_evaltype_pointer (out->type)) {
					if (out->type == EvalType (string)) {
						const char	*string;

						string = out->string;
						out->type = EvalType (basic);
						out->basic = BasicType (int);
						out->value = (unsigned char) string[0];
					} else if (out->type == EvalType (typeinfo_pointer)) {
						out->type = EvalType (typeinfo);
					} else if (out->type == EvalType (typemember_pointer)) {
						out->type = EvalType (typemember);
					} else {
						Unreachable ();
					}
					result = 1;
				} else {
					Error ("non-pointer operand for indirection operator");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (cast)) {
			if (eval_expr (unit, expr->op.forward, out)) {
				struct type	*type;

				type = get_type (unit, expr->op.backward);
				if (type->kind == TypeKind (basic)) {
					enum basictype	cast_to;

					cast_to = type->basic.type;
					if (cast_to < BasicType (int) && is_basictype_integral (cast_to)) {
						if (is_basictype_signed (cast_to)) {
							cast_to = BasicType (int);
						} else {
							cast_to = BasicType (uint);
						}
					}
					if (out->type == EvalType (basic)) {
						if (is_basictype_integral (out->basic)) {
							if (is_basictype_integral (cast_to)) {
								out->basic = cast_to;
							} else if (is_basictype_float (cast_to)) {
								out->basic = cast_to;
								out->fvalue = out->value;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							if (is_basictype_integral (cast_to)) {
								out->basic = cast_to;
								out->value = out->fvalue;
							} else if (is_basictype_float (cast_to)) {
								out->basic = cast_to;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("only basic type value is allowed for type cast operator");
						result = 0;
					}
				} else {
					Error ("only basic type cast is allowed in constant expressions");
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (address_of)) {
			Error ("&_ operator is forbidden in constant expressions");
			result = 0;
		} else if (expr->op.type == OpType (multiply)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value *= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value *= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->value * evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value *= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue *= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->uvalue * evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->fvalue *= evalright.value;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->fvalue *= evalright.uvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->fvalue * evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : out->basic;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ * _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (divide)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value /= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value /= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->value / evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value /= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue /= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->uvalue / evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->fvalue /= evalright.value;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->fvalue /= evalright.uvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->fvalue / evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : out->basic;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ / _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (modulo)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value %= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value %= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ % _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value *= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue *= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ % _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ % _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ % _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (add)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value += evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value += evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->value + evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value += evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue += evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->uvalue + evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->fvalue += evalright.value;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->fvalue += evalright.uvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->fvalue + evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : out->basic;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else {
							Error ("invalid basic type '%s'", g_basictype [out->basic]);
							result = 0;
						}
					} else {
						Error ("_ + _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (subtract)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value -= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value -= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->value - evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value -= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue -= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->uvalue - evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : BasicType (double);
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->fvalue -= evalright.value;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->fvalue -= evalright.uvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? BasicType (double) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								out->fvalue = out->fvalue - evalright.fvalue;
								out->basic = get_basictype_size (out->basic) < get_basictype_size (evalright.basic) ? evalright.basic : out->basic;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ - _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (left_shift)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value <<= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value <<= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ << _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value <<= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue <<= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ << _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ << _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ << _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (right_shift)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value >>= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value >>= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ >> _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value >>= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue >>= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ >> _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ >> _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ >> _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (less)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value < evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value < evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value < evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue < evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue < evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue < evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue < evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue < evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue < evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ < _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (greater)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value > evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value > evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value > evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue > evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue > evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue > evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue > evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue > evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue > evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ > _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (less_equal)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value <= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value <= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value <= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue <= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue <= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue <= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue <= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue <= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue <= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ < _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (greater_equal)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value >= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value >= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value >= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue >= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue >= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue >= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue >= evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue >= evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue >= evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ < _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (equal)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value == evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value == evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value == evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue == evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue == evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue == evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue == evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue == evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue == evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ < _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (not_equal)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->value != evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->value != evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->value != evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_unsigned (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->uvalue != evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->uvalue != evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->uvalue != evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else if (is_basictype_float (out->basic)) {
							int		value;

							if (is_basictype_signed (evalright.basic)) {
								value = out->fvalue != evalright.value;
							} else if (is_basictype_unsigned (evalright.basic)) {
								value = out->fvalue != evalright.uvalue;
							} else if (is_basictype_float (evalright.basic)) {
								value = out->fvalue != evalright.fvalue;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
							out->basic = BasicType (int);
							out->value = value;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ < _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (bitwise_and)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value &= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value &= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ & _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value &= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue &= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ & _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ & _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ & _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (bitwise_xor)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value ^= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value ^= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ ^ _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value ^= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue ^= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ ^ _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ ^ _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ ^ _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (bitwise_or)) {
			if (eval_expr (unit, expr->op.backward, out)) {
				struct evalvalue	evalright = {0};

				if (eval_expr (unit, expr->op.forward, &evalright)) {
					if (out->type == EvalType (basic) && evalright.type == EvalType (basic)) {
						if (is_basictype_signed (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value |= evalright.value;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->value |= evalright.uvalue;
								out->basic = evalright.basic > out->basic ? get_basictype_counterpart (evalright.basic) : out->basic;
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ | _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_unsigned (out->basic)) {
							if (is_basictype_signed (evalright.basic)) {
								out->value |= evalright.value;
								out->basic = evalright.basic > out->basic ? evalright.basic : get_basictype_counterpart (out->basic);
							} else if (is_basictype_unsigned (evalright.basic)) {
								out->uvalue |= evalright.uvalue;
								out->basic = Max (out->basic, evalright.basic);
							} else if (is_basictype_float (evalright.basic)) {
								Error ("_ | _ operator is not defined for floating point values");
								result = 0;
							} else {
								Error ("invalid basic type");
								result = 0;
							}
						} else if (is_basictype_float (out->basic)) {
							Error ("_ | _ operator is not defined for floating point values");
							result = 0;
						} else {
							Error ("invalid basic type");
							result = 0;
						}
					} else {
						Error ("_ | _ operator allows only basic types");
						result = 0;
					}
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else if (expr->op.type == OpType (logical_and) || expr->op.type == OpType (logical_or)) {
			Todo ();
		} else {
			Error ("assignment is forbidden in constant expressions");
			result = 0;
		}
	} else if (expr->type == ExprType (constant)) {
		out->type = EvalType (basic);
		out->basic = expr->constant.type;
		out->uvalue = expr->constant.uvalue;
		result = 1;
	} else if (expr->type == ExprType (string)) {
		out->type = EvalType (string);
		out->string = expr->string.token;
		result = 1;
	} else if (expr->type == ExprType (identifier)) {
		struct decl	*decl;

		Assert (expr->iden.decl >= 0);
		decl = get_decl (unit, expr->iden.decl);
		if (decl->kind == DeclKind (const)) {
			result = eval_const_expr (unit, decl->dconst.expr, out);
		} else {
			Error ("non-constant symbol '%s'", decl->name);
			result = 0;
		}
	} else if (expr->type == ExprType (funcparam)) {
		Unreachable ();
	} else if (expr->type == ExprType (macroparam)) {
		Assert (expr->macroparam.expr);
		result = eval_expr (unit, expr->macroparam.expr, out);
	} else if (expr->type == ExprType (typeinfo)) {
		out->type = EvalType (typeinfo);
		Assert (expr->typeinfo.index);
		out->typeinfo_index = expr->typeinfo.index;
		result = 1;
	} else {
		Unreachable ();
	}
	return (result);
}






