


#include <stdio.h>
#include <stdlib.h>

#define Implementation_All
#include "def.h"
#include "memutil.h"
#include "array.h"

char	*own_stpcpy (char *restrict dest, const char *restrict source) {
	while (*source) {
		*dest++ = *source++;
	}
	*dest = 0;
	return (dest);
}

#define Tokenizer__Is_Preprocessor 0
#define Tokenizer__Is_Trigraph 0
#define Tokenizer__skip_newline 1
#define Tokenizer__no_line_directives 1
#define Tokenizer__stpcpy own_stpcpy
#include "tokenizer.c"

#include "text_preprocessor.c"

char	*read_entire_file (const char *filename, usize *psize) {
	FILE	*file;
	char	*result;
	usize	size;

	file = fopen (filename, "r");
	if (file) {
		fseek (file, 0, SEEK_END);
		size = ftell (file);
		fseek (file, 0, SEEK_SET);
		result = malloc (size + 1);
		if (result) {
			usize	readed;

			readed = fread (result, 1, size, file);
			if (!ferror (file)) {
				if (psize) {
					*psize = readed;
				}
				result[readed] = 0;
			} else {
				Error ("cannot read entire file. readed: %zu, size: %zu", readed, size);
				free (result);
				result = 0;
			}
		} else {
			Error ("cannot allocate memory");
		}
		fclose (file);
	} else {
		Error ("cannot open file");
		result = 0;
	}
	return (result);
}

#define Max_Type_Depth (16)

#define BasicType(name) BasicType_##name
enum basictype {
	BasicType (void),
	BasicType (char),
	BasicType (uchar),
	BasicType (wchar),
	BasicType (byte),
	BasicType (ubyte),
	BasicType (short),
	BasicType (ushort),
	BasicType (int),
	BasicType (uint),
	BasicType (size),
	BasicType (usize),
	BasicType (float),
	BasicType (double),
};

const char	*g_basictype_name[] = {
	"void", "char", "uchar", "wchar", "byte", "ubyte", "short",
	"ushort", "int", "uint", "size", "usize", "float", "double",
};

int		is_basictype (const char *string, enum basictype *type) {
	int		index;

	index = 0;
	while (index < Array_Count (g_basictype_name) && 0 != strcmp (string, g_basictype_name[index])) {
		index += 1;
	}
	*type = index;
	return (index < Array_Count (g_basictype_name));
}

const char	*get_basictype_name (enum basictype type) {
	return (g_basictype_name[type]);
}

#define TypeKind(name) TypeKind_##name
enum typekind {
	TypeKind (basic),
	TypeKind (tag),
	TypeKind (mod),
	TypeKind (typeof),
	TypeKind (decl),
};

#define TypeMod(name) TypeMod_##name
enum typemod {
	TypeMod (pointer),
	TypeMod (array),
	TypeMod (function),
};

#define TagType(name) TagType_##name
enum tagtype {
	TagType (invalid),
	TagType (struct),
	TagType (union),
	TagType (stroke),
	TagType (enum),
	TagType (bitfield),
};

const char	*g_tagname[] = {
	"invalid", "struct", "union", "stroke", "enum", "bitfield",
};

struct type_basic {
	enum basictype	type;
	uint			is_const : 1;
	uint			is_volatile : 1;
};

struct type_tag {
	enum tagtype	type;
	const char		*name;
	uint			is_const : 1;
	uint			is_volatile : 1;
};

struct type_typeof {
	int		expr;
	uint	is_const : 1;
	uint	is_volatile : 1;
};

struct type_decl {
	int		index;
	uint	is_const : 1;
	uint	is_volatile : 1;
};

struct typemod_ptr {
	uint	is_const : 1;
	uint	is_restrict : 1;
	uint	is_volatile : 1;
	int		type;
};

struct typemod_array {
	uint	is_const : 1;
	uint	is_restrict : 1;
	uint	is_volatile : 1;
	uint	is_static : 1;
	int		expr;
	int		type;
};

struct typemod_func {
	int		param_scope;
	int		type;
};

struct type_mod {
	enum typemod	kind;
	union {
		struct typemod_ptr		ptr;
		struct typemod_array	array;
		struct typemod_func		func;
	};
};

struct type {
	enum typekind	kind;
	union {
		struct type_basic	basic;
		struct type_tag		tag;
		struct type_mod		mod;
		struct type_typeof	typeof;
		struct type_decl	decl;
	};
};

#define DeclKind(name) DeclKind_##name
enum declkind {
	DeclKind (var),
	DeclKind (const),
	DeclKind (func),
	DeclKind (tag),
	DeclKind (block),
	DeclKind (enum),
};

struct decl_const {
	int		expr;
};

struct decl_func {
	int		param_scope;
	int		scope;
};

struct decl_tag {
	enum tagtype	type;
	int				scope;
};

struct decl_block {
	int		scope;
};

struct decl_enum {
	int		expr;
};

struct decl {
	int				next;
	const char		*name;
	int				type;
	uint			is_in_process : 1;
	enum declkind	kind;
	union {
		struct decl_const	dconst;
		struct decl_func	func;
		struct decl_tag		tag;
		struct decl_block	block;
		struct decl_enum	enumt;
	};
};

#define FlowType(name) FlowType_##name
enum flowtype {
	FlowType (decl),
	FlowType (expr),
	FlowType (block),
	FlowType (if),
	FlowType (while),
	FlowType (dowhile),
};

struct flow_decl {
	int		index;
};

struct flow_expr {
	int		index;
};

struct flow_block {
	int		scope;
};

struct flow_if {
	int		expr;
	int		flow_body;
	int		else_body;
};

struct flow_while {
	int		expr;
	int		flow_body;
};

struct flow_dowhile {
	int		expr;
	int		flow_body;
};

struct flow {
	int				next;
	enum flowtype	type;
	union {
		struct flow_decl		decl;
		struct flow_expr		expr;
		struct flow_block		block;
		struct flow_if			fif;
		struct flow_while		fwhile;
		struct flow_dowhile		dowhile;
	};
};

#define ScopeKind(name) ScopeKind_##name
enum scopekind {
	ScopeKind (unit),
	ScopeKind (func),
	ScopeKind (code),
	ScopeKind (tag),
	ScopeKind (param),
};

struct scope {
	enum scopekind	kind;
	enum tagtype	tagtype;
	int				parent_scope;
	int				param_scope;
	int				decl_begin;
	int				decl_last;
	int				flow_begin;
	int				flow_last;
};

#define ExprType(name) ExprType_##name
enum exprtype {
	ExprType (invalid),
	ExprType (op),
	ExprType (constant),
	ExprType (identifier),
	ExprType (decl),
	ExprType (compound),
	ExprType (funcparam),
	ExprType (typeinfo),
};

struct expr_constant {
	enum basictype	type;
	union {
		isize	value;
		usize	uvalue;
		double	fvalue;
	};
};

#define OpType(name) OpType_##name
enum optype {
	OpType (group),

	OpType (function_call),
	OpType (array_subscript),
	OpType (member_access),
	OpType (indirect_access),

	OpType (unary_plus),
	OpType (unary_minus),
	OpType (logical_not),
	OpType (bitwise_not),
	OpType (indirect),
	OpType (cast),
	OpType (address_of),
	/* sizeof */
	/* _Alignof */

	OpType (multiply),
	OpType (divide),
	OpType (modulo),

	OpType (add),
	OpType (subtract),

	OpType (left_shift),
	OpType (right_shift),

	OpType (less),
	OpType (greater),
	OpType (less_equal),
	OpType (greater_equal),

	OpType (equal),
	OpType (not_equal),

	OpType (bitwise_and),

	OpType (bitwise_xor),

	OpType (bitwise_or),

	OpType (logical_and),

	OpType (logical_or),

	/* ternary condition */

	OpType (assign),
	OpType (add_assign),
	OpType (subtract_assign),
	OpType (multiply_assign),
	OpType (divide_assign),
	OpType (modulo_assign),
	OpType (left_shift_assign),
	OpType (right_shift_assign),
	OpType (bitwise_and_assign),
	OpType (bitwise_xor_assign),
	OpType (bitwise_or_assign),

	/* comma */
};

struct exprop {
	enum optype		type;
	int				forward, backward;
};

struct exprfuncparam {
	int		expr;
	int		next;
};

struct exprtypeinfo {
	int		type;
};

struct exprdecl {
	int		index;
};

struct expr {
	enum exprtype	type;
	union {
		struct exprop			op;
		struct expr_constant	constant;
		const char				*identifier;
		struct exprdecl			decl;
		void					*compound;
		struct exprfuncparam	funcparam;
		struct exprtypeinfo		typeinfo;
	};
};

struct unit {
	int				root_scope;
	struct decl		*decls;
	struct type		*types;
	struct expr		*exprs;
	struct flow		*flows;
	struct scope	*scopes;
};

int		make_scope (struct unit *unit, enum scopekind kind, int parent);
struct scope	*get_scope (struct unit *unit, int scope_index);
struct decl	*get_decl (struct unit *unit, int index);
void	print_type (struct unit *unit, int head, FILE *file);


#include "expr.c"
#include "type.c"
#include "scope.c"
// #include "semantic.c"
#include "link.c"

void	init_unit (struct unit *unit) {
	unit->root_scope = make_scope (unit, ScopeKind (unit), -1);
}

int		parse_global_var (struct unit *unit, char **ptokens, const char *name, int type) {
	return (0);
}

int		parse_function (struct unit *unit, char **ptokens, const char *name, int type) {
	return (0);
}

int		parse_global_var_or_function (struct unit *unit, char **ptokens) {
	int			result;
	const char	*name;
	int			type_index;

	Assert ((*ptokens)[-1] == Token (identifier));
	name = *ptokens;
	*ptokens = next_token (*ptokens, 0);
	if (parse_type (unit, ptokens, &type_index)) {
		if ((*ptokens)[-1] == Token (punctuator)) {
			if (0 == strcmp (*ptokens, "{")) {
				result = parse_function (unit, ptokens, name, type_index);
			} else if (0 == strcmp (*ptokens, ";") || 0 == strcmp (*ptokens, "=")) {
				result = parse_global_var (unit, ptokens, name, type_index);
			} else {
				Error ("unexpected token");
				result = 0;
			}
		} else {
			Error ("unexpected token");
			result = 0;
		}
	} else {
		Error ("cannot make type");
		result = 0;
	}
	return (result);
}

int		parse_unit (struct unit *unit, char **ptokens) {
	int		result;

	if ((*ptokens)[-1] == Token (identifier)) {
		if (0 == strcmp (*ptokens, "struct")) {
			*ptokens = next_token (*ptokens, 0);
			// result = parse_decl_struct (unit, ptokens);
		} else if (0 == strcmp (*ptokens, "enum")) {
			*ptokens = next_token (*ptokens, 0);
			// result = parse_decl_enum (unit, ptokens);
		} else if (0 == strcmp (*ptokens, "const")) {
			*ptokens = next_token (*ptokens, 0);
			// result = parse_global_const (unit, ptokens);
		} else {
			result = parse_global_var_or_function (unit, ptokens);
		}
	} else {
		Error ("unexpected token");
		result = 0;
	}
	return (result);
}

int		main () {
	printf ("Hello World!\n");

	char	*text;
	usize	size;
	text = read_entire_file ("test.n", &size);
	printf ("test.n:\n%s\n", text);
	char	*tokens;
	int		*nl_array;

	size = preprocess_text (text, text + size, &nl_array);

	tokens = tokenize (text, nl_array, 1, "test.n");
	print_tokens (tokens, 1, "", stdout);

	if (tokens[-1] == Token (newline)) {
		tokens = next_token (tokens, 0);
	}

	struct unit	cunit = {0}, *unit = &cunit;
	struct type	*type = 0;

	init_unit (unit);


	// int		head = -1;
	// if (parse_expr (unit, &tokens, &head)) {
	// 	if (head >= 0) {
	// 		print_expr (unit, head, stdout);
	// 		fprintf (stdout, "\n\n");
	// 		print_expr_table (unit, head, stdout);
	// 	} else {
	// 		Debug ("empty expr");
	// 	}
	// } else {
	// 	Error ("cannot parse expr");
	// }

	int		head = -1;
	if (parse_scope (unit, unit->root_scope, &tokens)) {
		print_scope (unit, unit->root_scope, 0, stdout);
		if (link_unit (unit)) {
			Debug ("-------------------");
			print_scope (unit, unit->root_scope, 0, stdout);
		} else {
			Error ("cannot link unit");
		}
	} else {
		Error ("cannot parse code scope");
	}

	// int		head = -1;
	// if (parse_type (unit, &tokens, &head)) {
	// 	if (head >= 0) {
	// 		print_type (unit, head, stdout);
	// 	} else {
	// 		Debug ("empty type");
	// 	}
	// } else {
	// 	Error ("cannot parse type");
	// }

	// parse_unit (unit, tokens);

}





