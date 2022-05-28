


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

int		is_basictype_integral (enum basictype type) {
	return (type >= BasicType (char) && type <= BasicType (usize));
}

int		is_basictype_signed (enum basictype type) {
	int		result;

	switch (type) {
		BasicType (char):
		BasicType (wchar):
		BasicType (byte):
		BasicType (short):
		BasicType (int):
		BasicType (size): {
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
		case BasicType (ubyte):
		case BasicType (ushort):
		case BasicType (uint):
		case BasicType (usize): {
			result = 1;
		} break ;
		default: {
			result = 0;
		} break ;
	}
	return (result);
}

int		is_basictype_float (enum basictype type) {
	return (type == BasicType (float) || type == BasicType (double));
}

int		get_basictype_size (enum basictype type) {
	int		result;

	switch (type) {
		case BasicType (void): {
			result = 0;
		} break ;
		case BasicType (char):
		case BasicType (uchar):
		case BasicType (wchar):
		case BasicType (byte):
		case BasicType (ubyte): {
			result = 1;
		} break ;
		case BasicType (short):
		case BasicType (ushort): {
			result = 2;
		} break ;
		case BasicType (float):
		case BasicType (int):
		case BasicType (uint): {
			result = 4;
		} break ;
		case BasicType (double):
		case BasicType (size):
		case BasicType (usize): {
			result = 8;
		} break ;
	}
	return (result);
}

#define TypeKind(name) TypeKind_##name
enum typekind {
	TypeKind (basic),
	TypeKind (tag),
	TypeKind (mod),
	TypeKind (group),
	TypeKind (typeof),
	TypeKind (decl),
	TypeKind (accessor),
};

const char	*g_typekind[] = {
	"basic", "tag", "mod", "group", "typeof", "decl", "accessor",
};

#define TypeMod(name) TypeMod_##name
enum typemod {
	TypeMod (pointer),
	TypeMod (array),
	TypeMod (function),
};

const char	*g_typemod[] = {
	"pointer", "array", "function",
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

int		is_tagtype (const char *string, enum tagtype *out) {
	int		index;

	index = 1;
	while (index < Array_Count (g_tagname) && 0 != strcmp (string, g_tagname[index])) {
		index += 1;
	}
	*out = index;
	return (index < Array_Count (g_tagname));
}

struct type_basic {
	enum basictype	type;
	uint			is_const : 1;
	uint			is_volatile : 1;
};

struct type_group {
	int			type;
};

struct type_tag {
	enum tagtype	type;
	const char		*name;
	int				decl;
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

struct type_accessor {
	enum tagtype	tagtype;
	const char		*tagname;
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
		struct type_group	group;
		struct type_tag		tag;
		struct type_mod		mod;
		struct type_typeof	typeof;
		struct type_decl	decl;
		struct type_accessor	accessor;
	};
};

#define DeclKind(name) DeclKind_##name
enum declkind {
	DeclKind (var),
	DeclKind (const),
	DeclKind (tag),
	DeclKind (func),
	DeclKind (macro),
	DeclKind (param),
	DeclKind (block),
	DeclKind (enum),
	DeclKind (accessor),
};

struct decl_const {
	int		expr;
};

struct decl_func {
	int		param_scope;
	int		scope;
};

struct decl_macro {
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

struct decl_accessor {
	enum tagtype	tagtype;
	const char		*name;
	int				decl;
};

struct decl {
	int				next;
	const char		*name;
	int				type;
	uint			is_in_process : 1;
	enum declkind	kind;
	union {
		struct decl_const	dconst;
		struct decl_tag		tag;
		struct decl_func	func;
		struct decl_macro	macro;
		struct decl_block	block;
		struct decl_enum	enumt;
		struct decl_accessor	accessor;
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
	ScopeKind (macro),
};

struct scope {
	enum scopekind	kind;
	enum tagtype	tagtype;
	int				parent_scope;
	int				param_scope;
	int				type_index;
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

const char	*g_exprtype[] = {
	"invalid", "op", "constant", "identifier", "decl", "compound", "funcparam", "typeinfo",
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

struct typestack {
	struct type	types[Max_Type_Depth];
	int			types_count;
	int			head;
	int			is_lvalue;
};


int		make_scope (struct unit *unit, enum scopekind kind, int parent);
struct scope	*get_scope (struct unit *unit, int scope_index);
struct decl	*get_decl (struct unit *unit, int index);
void	print_type (struct unit *unit, int head, FILE *file);


#include "expr.c"
#include "type.c"
#include "typestack.c"
#include "scope.c"
#include "link.c"
#include "macro.c"
#include "writer.c"
#include "cbackend.c"

void	init_unit (struct unit *unit) {
	unit->root_scope = make_scope (unit, ScopeKind (unit), -1);
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


	// int		head = -1;
	// if (parse_scope (unit, unit->root_scope, &tokens)) {
	// 	print_scope (unit, unit->root_scope, 0, stdout);
	// 	if (link_unit (unit)) {
	// 		Debug ("-------------------");
	// 		print_scope (unit, unit->root_scope, 0, stdout);
	// 	} else {
	// 		Error ("cannot link unit");
	// 	}
	// } else {
	// 	Error ("cannot parse code scope");
	// }


	// int		head = -1;
	// if (parse_type (unit, &tokens, &head)) {
	// 	if (head >= 0) {
	// 		struct cbuffer	cbuffer;

	// 		printf ("\ntype: ");
	// 		print_type (unit, head, stdout);
	// 		printf ("\ntree: ");
	// 		print_type_tree (unit, head, stdout);
	// 		printf ("\nc: ");
	// 		init_cbuffer (&cbuffer);
	// 		c_backend_translate_type (unit, head, "a", &cbuffer);
	// 		printf ("%s", cbuffer.wr->buffer);
	// 		printf ("\n");
	// 	} else {
	// 		Debug ("empty type");
	// 	}
	// } else {
	// 	Error ("cannot parse type");
	// }


	int		head = -1;
	if (parse_scope (unit, unit->root_scope, &tokens)) {
		print_scope (unit, unit->root_scope, 0, stdout);
		if (link_unit (unit)) {
			struct cbuffer	cbuffer;

			Debug ("-------------------");
			print_scope (unit, unit->root_scope, 0, stdout);
			Debug ("===================");
			init_cbuffer (&cbuffer);
			c_backend_translate_unit (unit, &cbuffer);
			printf ("%s", cbuffer.wr->buffer);
			printf ("\n");
		} else {
			Error ("cannot link unit");
		}
	} else {
		Error ("cannot parse code scope");
	}


	// parse_unit (unit, tokens);

}





