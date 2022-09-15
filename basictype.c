

int		is_basictype (const char *string, enum basictype *type) {
	int		index;

	index = 0;
	while (index < Array_Count (g_basictype) && 0 != strcmp (string, g_basictype[index])) {
		index += 1;
	}
	*type = index;
	return (index < Array_Count (g_basictype));
}

const char	*get_basictype_name (enum basictype type) {
	return (g_basictype[type]);
}

int		is_basictype_integral (enum basictype type) {
	return (type >= BasicType (char) && type <= BasicType (uint64));
}

int		is_basictype_signed (enum basictype type) {
	int		result;

	switch (type) {
		case BasicType (char):
		case BasicType (schar):
		case BasicType (short):
		case BasicType (wchar):
		case BasicType (int):
		case BasicType (long):
		case BasicType (longlong):
		case BasicType (size):
		case BasicType (int8):
		case BasicType (int16):
		case BasicType (int32):
		case BasicType (int64): {
			result = 1;
		} break ;
		default: {
			result = 0;
		} break ;
	}
	return (result);
}

int		is_basictype_unsigned (enum basictype type) {
	int		result;

	switch (type) {
		case BasicType (uchar):
		case BasicType (byte):
		case BasicType (ushort):
		case BasicType (uint):
		case BasicType (ulong):
		case BasicType (ulonglong):
		case BasicType (usize):
		case BasicType (uint8):
		case BasicType (uint16):
		case BasicType (uint32):
		case BasicType (uint64): {
			result = 1;
		} break ;
		default: {
			result = 0;
		} break ;
	}
	return (result);
}

int		get_basictype_rank (enum basictype type) {
	int		result;

	switch (type) {
		case BasicType (char):
		case BasicType (schar):
		case BasicType (uchar):
		case BasicType (byte):
		case BasicType (int8):
		case BasicType (uint8): {
			result = 1;
		} break ;
		case BasicType (short):
		case BasicType (ushort):
		case BasicType (int16):
		case BasicType (uint16): {
			result = 2;
		} break ;
		case BasicType (wchar):
		case BasicType (int):
		case BasicType (uint):
		case BasicType (int32):
		case BasicType (uint32): {
			result = 3;
		} break ;
		case BasicType (long):
		case BasicType (ulong): {
			result = 4;
		} break ;
		case BasicType (longlong):
		case BasicType (ulonglong): {
			result = 5;
		} break ;
		case BasicType (size):
		case BasicType (usize): {
			if (g_is_build32) {
				result = 3;
			} else {
				result = 5;
			}
		} break ;
		case BasicType (int64):
		case BasicType (uint64): {
			if (g_platform == Platform (windows)) {
				result = 5;
			} else {
				result = 4;
			}
		} break ;
		default: Unreachable ();
	}
	return (result);
}

enum basictype	get_promoted_basictype (enum basictype type) {
	switch (type) {
		case BasicType (char):
		case BasicType (schar):
		case BasicType (uchar):
		case BasicType (byte):
		case BasicType (int8):
		case BasicType (uint8):
		case BasicType (short):
		case BasicType (ushort):
		case BasicType (int16):
		case BasicType (uint16): {
			type = BasicType (int);
		} break ;
		case BasicType (wchar):
		case BasicType (int32): {
			type = BasicType (int);
		} break ;
		case BasicType (uint32): {
			type = BasicType (uint);
		} break ;
	}
	return (type);
}

int		is_basictype_float (enum basictype type) {
	return (type == BasicType (float) || type == BasicType (double) || type == BasicType (longdouble));
}

int		get_basictype_size (enum basictype type) {
	int		result;

	switch (type) {
		case BasicType (void): {
			result = 0;
		} break ;
		case BasicType (char):
		case BasicType (schar):
		case BasicType (uchar):
		case BasicType (byte):
		case BasicType (int8):
		case BasicType (uint8): {
			result = 1;
		} break ;
		case BasicType (short):
		case BasicType (ushort):
		case BasicType (int16):
		case BasicType (uint16): {
			result = 2;
		} break ;
		case BasicType (wchar):
		case BasicType (int):
		case BasicType (uint):
		case BasicType (int32):
		case BasicType (uint32):
		case BasicType (float): {
			result = 4;
		} break ;
		case BasicType (long):
		case BasicType (ulong): {
			if (g_platform == Platform (windows)) {
				result = 4;
			} else {
				result = 8;
			}
		} break ;
		case BasicType (longlong):
		case BasicType (ulonglong):
		case BasicType (int64):
		case BasicType (double):
		case BasicType (uint64): {
			result = 8;
		} break ;
		case BasicType (longdouble): {
			if (g_platform == Platform (windows)) {
				if (g_is_build32) {
					result = 8;
				} else {
					result = 12;
				}
			} else {
				result = 12;
			}
		} break ;
		case BasicType (size):
		case BasicType (usize): {
			if (g_is_build32) {
				result = 4;
			} else {
				result = 8;
			}
		} break ;
	}
	return (result);
}

enum basictype	get_basictype_counterpart (enum basictype type) {
	enum basictype	result;

	switch (type) {
		case BasicType (void): result = BasicType (void); break ;
		case BasicType (char): result = BasicType (uchar); break ;
		case BasicType (schar): result = BasicType (uchar); break ;
		case BasicType (uchar): result = BasicType (schar); break ;
		case BasicType (byte): result = BasicType (schar); break ;
		case BasicType (short): result = BasicType (ushort); break ;
		case BasicType (ushort): result = BasicType (short); break ;
		case BasicType (wchar): result = BasicType (uint); break ;
		case BasicType (int): result = BasicType (uint); break ;
		case BasicType (uint): result = BasicType (int); break ;
		case BasicType (long): result = BasicType (ulong); break ;
		case BasicType (ulong): result = BasicType (long); break ;
		case BasicType (longlong): result = BasicType (ulonglong); break ;
		case BasicType (ulonglong): result = BasicType (longlong); break ;
		case BasicType (size): result = BasicType (usize); break ;
		case BasicType (usize): result = BasicType (size); break ;
		case BasicType (int8): result = BasicType (uint8); break ;
		case BasicType (uint8): result = BasicType (int8); break ;
		case BasicType (int16): result = BasicType (uint16); break ;
		case BasicType (uint16): result = BasicType (int16); break ;
		case BasicType (int32): result = BasicType (uint32); break ;
		case BasicType (uint32): result = BasicType (int32); break ;
		case BasicType (int64): result = BasicType (uint64); break ;
		case BasicType (uint64): result = BasicType (int64); break ;
		case BasicType (float): result = BasicType (float); break ;
		case BasicType (double): result = BasicType (double); break ;
		case BasicType (longdouble): result = BasicType (longdouble); break ;
		default: Unreachable ();
	}
	return (type);
}

enum basictype	get_common_arithmetic_basictype (enum basictype left, enum basictype right) {
	enum basictype	common;

	Assert (left != BasicType (void) && right != BasicType (void));
	if (left == BasicType (longdouble) || right == BasicType (longdouble)) {
		common = BasicType (longdouble);
	} else if (left == BasicType (double) || right == BasicType (double)) {
		common = BasicType (double);
	} else if (left == BasicType (float) || right == BasicType (float)) {
		common = BasicType (float);
	} else {
		int		left_signed, right_signed;

		left = get_promoted_basictype (left);
		right = get_promoted_basictype (right);
		left_signed = is_basictype_signed (left);
		right_signed = is_basictype_signed (right);
		if (left == right) {
			common = left;
		} else if (left_signed == right_signed) {
			if (get_basictype_rank (left) > get_basictype_rank (right)) {
				common = left;
			} else {
				common = right;
			}
		} else {
			int		left_rank, right_rank;

			left_rank = get_basictype_rank (left);
			right_rank = get_basictype_rank (right);
			if (!left_signed && left_rank >= right_rank) {
				common = left;
			} else if (!right_signed && right_rank >= left_rank) {
				common = right;
			} else {
				int		left_size, right_size;

				left_size = get_basictype_size (left);
				right_size = get_basictype_size (right);
				if (left_signed && left_size > right_size) {
					common = left;
				} else if (right_signed && right_size > left_size) {
					common = right;
				} else if (left_signed) {
					common = get_basictype_counterpart (left);
				} else {
					common = get_basictype_counterpart (right);
				}
			}
		}
	}
	return (common);
}

int		is_basictype_with_l_suffix (enum basictype type) {
	int		result;

	if (type == BasicType (long) || type == BasicType (ulong)) {
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		is_basictype_with_ll_suffix (enum basictype type) {
	int		result;

	if (type == BasicType (longlong) || type == BasicType (ulonglong) || type == BasicType (usize) || type == BasicType (size)) {
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}
