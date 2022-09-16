


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <windows.h>

#define Implementation_All
#include "def.h"
#include "memutil.h"
#include "array.h"

#define Tokenizer__Is_Preprocessor 0
#define Tokenizer__Is_Trigraph 0
#define Tokenizer__skip_newline 1
#define Tokenizer__no_line_directives 1
#define Tokenizer__Is_Global_Pos 0
#include "tokenizer.c"

#include "text_preprocessor.c"

int		write_data_to_file (const char *path, const char *data, usize size) {
	int		result;
	FILE	*file;

	file = fopen (path, "w");
	if (file) {
		int		written;

		written = fwrite (data, 1, size, file);
		if (written == size) {
			result = 1;
		} else {
			Error ("not all bytes written");
			result = 0;
		}
		fclose (file);
	} else {
		Error ("can't open file");
		result = 0;
	}
	return (result);
}

char	*read_file_until_eof (FILE *file) {
	char	*buffer;
	char	local[8 * 1024];
	int		len;
	int		result;

	buffer = 0;
	result = 1;
	Prepare_Array (buffer, 0);
	Push_Array_N (buffer, 0);
	while (result && (len = fread (local, 1, sizeof local, file)) > 0) {
		if (Prepare_Array (buffer, len)) {
			char	*ptr;

			ptr = Push_Array_N (buffer, len);
			memcpy (ptr, local, len);
			result = 1;
		} else {
			Error ("cannot prepare array");
			result = 0;
		}
	}
	if (result && !feof (file)) {
		Error ("error occuried while reading from file");
		result = 0;
	}
	if (!result) {
		if (buffer) {
			Free_Array (buffer);
			buffer = 0;
		}
	}
	return (buffer);
}

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
	BasicType (schar),
	BasicType (uchar),
	BasicType (byte),
	BasicType (short),
	BasicType (ushort),
	BasicType (wchar),
	BasicType (int),
	BasicType (uint),
	BasicType (long),
	BasicType (ulong),
	BasicType (longlong),
	BasicType (ulonglong),
	BasicType (size),
	BasicType (usize),
	BasicType (int8),
	BasicType (uint8),
	BasicType (int16),
	BasicType (uint16),
	BasicType (int32),
	BasicType (uint32),
	BasicType (int64),
	BasicType (uint64),
	BasicType (float),
	BasicType (double),
	BasicType (longdouble),
};

const char	*g_basictype[] = {
	"void", "char", "schar", "uchar", "byte", "short", "ushort", "wchar", "int", "uint", "long", "ulong",
	"longlong", "ulonglong", "size", "usize", "int8", "uint8", "int16", "uint16", "int32", "uint32",
	"int64", "uint64", "float", "double", "longdouble"
};

const char	*g_basictype_c_name[] = {
	"void", "char", "signed char", "unsigned char", "unsigned char", "short", "unsigned short", "int",
	"int", "unsigned int", "long int", "unsinged long int", "long long int", "unsinged long long int",
	"ptrdiff_t", "size_t", "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t",
	"float", "double", "long double",
};

#define TypeKind(name) TypeKind_##name
enum typekind {
	TypeKind (basic),
	TypeKind (tag),
	TypeKind (mod),
	TypeKind (typeof),
	TypeKind (deftype),
	TypeKind (internal),
};

const char	*g_typekind[] = {
	"basic", "tag", "mod", "typeof", "deftype", "internal",
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
	TagType (enum),
};

const char	*g_tagname[] = {
	"invalid", "struct", "union", "enum",
};

int		is_tagtype (const char *string, enum tagtype *out) {
	int		index;

	index = 1;
	while (index < Array_Count (g_tagname) && 0 != strcmp (string, g_tagname[index])) {
		index += 1;
	}
	if (out) {
		*out = index;
	}
	return (index < Array_Count (g_tagname));
}

struct type_basic {
	enum basictype	type;
};

struct type_tag {
	enum tagtype	type;
	const char		*name;
	uint64			decl;
};

struct type_typeof {
	uint	expr;
};

struct type_deftype {
	const char	*name;
};

struct type_internal {
	uint64	decl;
};

struct type_mod {
	uint			forward;
	enum typemod	kind;
	union {
		uint64	expr;
		uint64	count;
		uint64	param_scope;
	};
};

struct type_flags {
	uint	is_group : 1;
	uint	is_const : 1;
	uint	is_volatile : 1;
	uint	is_restrict : 1;
};

struct type {
	struct type_flags	flags;
	enum typekind		kind;
	union {
		struct type_basic		basic;
		struct type_tag			tag;
		struct type_mod			mod;
		struct type_typeof		typeof;
		struct type_deftype		deftype;
		struct type_internal	internal;
	};
};

#define DeclKind(name) DeclKind_##name
enum declkind {
	DeclKind (var),
	DeclKind (const),
	DeclKind (alias),
	DeclKind (tag),
	DeclKind (func),
	DeclKind (param),
	DeclKind (block),
	DeclKind (enum),
	DeclKind (define),
};

const char	*g_declkind[] = {
	"var", "const", "alias", "tag", "func", "param", "block", "enum", "define",
};

#define DefineKind(name) DefineKind_##name
enum definekind {
	DefineKind (macro),
	DefineKind (accessor),
	DefineKind (external),
	DefineKind (type),
	DefineKind (visability),
	DefineKind (funcprefix),
	DefineKind (builtin),
};
static const int g_definekind_ordinal[] = {
	1, 1, 1, 0, 0, 0, 1,
};
const char *g_definekind[] = {
	"macro", "accessor", "external", "type", "visability", "funcprefix", "builtin",
};

#define Visability(name) Visability_##name
enum visability {
	Visability (private),
	Visability (public),
};
static const char	*const g_visability[] = {
	"private", "public",
};

#define Builtin(name) Builtin_##name
enum builtin {
	Builtin (flag),
	Builtin (platform),
	Builtin (option),
};
static const char	*const g_builtin[] = {
	"__Flag", "__Platform", "__Option",
};

struct decl_const {
	uint	expr;
};

struct decl_var {
	enum visability	visability;
	uint			init_scope;
};

struct decl_func {
	enum visability	visability;
	const char		*prefix;
	uint			param_scope;
	uint			scope;
};

struct decl_tag {
	enum tagtype	type;
	int				is_external;
	uint			scope;
	uint			param_scope;
	uint			member_table;
};

struct decl_block {
	uint	scope;
};

struct decl_enum {
	uint	expr;
	uint	params;
};

struct decl_param {
	uint	expr;
};

struct decl_alias {
	uint	expr;
};

struct decl_define_macro {
	uint	param_scope;
	uint	scope;
};

struct decl_define_accessor {
	enum tagtype	tagtype;
	const char		*name;
	uint64			decl;
};

struct decl_define_visability {
	enum visability	type;
	const char		*target;
};

struct decl_define_funcprefix {
	const char	*prefix;
	const char	*target;
};

struct decl_define_builtin {
	enum builtin	type;
};

struct decl_define {
	enum definekind	kind;
	union {
		struct decl_define_macro		macro;
		struct decl_define_accessor		accessor;
		struct decl_define_visability	visability;
		struct decl_define_funcprefix	funcprefix;
		struct decl_define_builtin		builtin;
	};
};

struct decl {
	uint			next;
	const char		*name;
	const char		*filename;
	int				line;
	uint			type;
	uint			is_in_process : 1;
	uint			is_linked : 1;
	uint			is_sized : 1;
	uint			is_global : 1;
	unsigned		size;
	unsigned		offset;
	unsigned		alignment;
	enum declkind	kind;
	union {
		struct decl_var		var;
		struct decl_const	dconst;
		struct decl_tag		tag;
		struct decl_func	func;
		struct decl_param	param;
		struct decl_block	block;
		struct decl_enum	enumt;
		struct decl_alias	alias;
		struct decl_define	define;
	};
};

#define FlowType(name) FlowType_##name
enum flowtype {
	FlowType (decl),
	FlowType (expr),
	FlowType (block),
	FlowType (if),
	FlowType (constif),
	FlowType (while),
	FlowType (dowhile),
	FlowType (init),
};

#define InitType(name) InitType_##name
enum inittype {
	InitType (expr),
	InitType (list),
	InitType (array),
	InitType (struct),
};

struct flow_decl {
	uint	index;
};

struct flow_expr {
	uint	index;
};

struct flow_block {
	uint	scope;
};

struct flow_if {
	uint	expr;
	uint	flow_body;
	uint	else_body;
};

struct flow_constif {
	uint	expr;
	uint	flow_body;
	uint	else_body;
};

struct flow_while {
	uint	expr;
	uint	flow_body;
};

struct flow_dowhile {
	uint	expr;
	uint	flow_body;
};

struct flow_init {
	enum inittype	type;
	uint			body;
};

struct flow {
	uint			next;
	enum flowtype	type;
	int				line;
	union {
		struct flow_decl		decl;
		struct flow_expr		expr;
		struct flow_block		block;
		struct flow_if			fif;
		struct flow_constif		constif;
		struct flow_while		fwhile;
		struct flow_dowhile		dowhile;
		struct flow_init		init;
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
	ScopeKind (init),
};
const char	*g_scopekind[] = {
	"unit", "func", "code", "tag", "param", "macro", "init_array", "init_struct",
};

struct scope {
	enum scopekind	kind;
	enum tagtype	tagtype;
	uint			parent_scope;
	uint			param_scope;
	uint			type_index;
	uint			decl_begin;
	uint			decl_last;
	uint			flow_begin;
	uint			flow_last;
};

#define ExprType(name) ExprType_##name
enum exprtype {
	ExprType (invalid),
	ExprType (op),
	ExprType (constant),
	ExprType (string),
	ExprType (identifier),
	ExprType (enum),
	ExprType (table),
	ExprType (funcparam),
	ExprType (macrocall),
	ExprType (macroparam),
	ExprType (typeinfo),
};

const char	*g_exprtype[] = {
	"invalid", "op", "constant", "string", "identifier", "enum", "table", "funcparam", "macrocall", "macroparam", "typeinfo",
};

#define OpType(name) OpType_##name
enum optype {
	OpType (group),
	OpType (typesizeof),
	OpType (typealignof),

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
	OpType (sizeof),
	OpType (alignof),

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

const char	*g_opname[] = {
	"group", "typesizeof", "typealignof", "function_call", "array_subscript", "member_access", "indirect_access",
	"unary_plus", "unary_minus", "logical_not", "bitwise_not", "indirect", "cast", "address_of", "sizeof", "alignof",
	"multiply", "divide", "modulo", "add", "subtract", "left_shift", "right_shift", "less", "greater", "less_equal",
	"greater_equal", "equal", "not_equal", "bitwise_and", "bitwise_xor", "bitwise_or", "logical_and", "logical_or",
	"assign", "add_assign", "subtract_assign", "multiply_assign", "divide_assign", "modulo_assign", "left_shift_assign",
	"right_shift_assign", "bitwise_and_assign", "bitwise_xor_assign", "bitwise_or_assign",
};

int		is_arithmetic_optype (enum optype optype) {
	return ((optype >= OpType (multiply) && optype <= OpType (subtract)) || (optype >= OpType (less) && optype <= OpType (not_equal)) || (optype >= OpType (bitwise_and) && optype <= OpType (bitwise_or)));
}

int		is_arithmethic_assignment_optype (enum optype optype) {
	return ((optype >= OpType (add_assign) && optype <= OpType (modulo_assign)) || (optype >= OpType (bitwise_and_assign) && optype <= OpType (bitwise_or_assign)));
}

int		is_bitwise_optype (enum optype optype) {
	return ((optype >= OpType (bitwise_and) && optype <= OpType (bitwise_or)) || optype == OpType (bitwise_not));
}

int		is_bitwise_assignment_optype (enum optype optype) {
	return (optype >= OpType (bitwise_and_assign) && optype <= OpType (bitwise_or_assign));
}

int		is_assignment_optype (enum optype optype) {
	return (optype >= OpType (assign) && optype <= OpType (bitwise_or_assign));
}

int		is_comparison_optype (enum optype optype) {
	return (optype >= OpType (less) && optype <= OpType (not_equal));
}

struct exprvalue {
	enum basictype	type;
	union {
		isize		value;
		usize		uvalue;
		long double	fvalue;
	};
};

struct exprop {
	enum optype	type;
	uint		forward;
	uint		backward;
};

struct expriden {
	const char	*name;
	uint64		decl;
};

struct exprenum {
	uint	lib_index;
	uint	decl;
	uint	enum_decl;
};

struct exprtable {
	uint	lib_index;
	uint	decl;
	uint	tag_decl;
};

struct exprfuncparam {
	uint	expr;
	uint	next;
};

struct exprmacrocall {
	uint64	decl;
	uint	instance;
};

struct exprmacroparam {
	const char	*name;
	uint		decl;
	uint		expr;
};

struct exprtypeinfo {
	uint	lib_index;
	uint	type;
	uint	index;
};

struct exprstr {
	const char	*token;
};

struct expr {
	enum exprtype	type;
	union {
		struct exprop			op;
		struct exprvalue		constant;
		struct expriden			iden;
		struct exprstr			string;
		struct exprenum			enumt;
		struct exprtable		table;
		struct exprfuncparam	funcparam;
		struct exprmacrocall	macrocall;
		struct exprmacroparam	macroparam;
		struct exprtypeinfo		typeinfo;
	};
};

#define Path_Kind(name) Path_Kind_##name
enum path_kind {
	Path_Kind (unit),
	Path_Kind (function),
	Path_Kind (macro),
	Path_Kind (tag),
	Path_Kind (decl),
	Path_Kind (flow),
	Path_Kind (type),
	Path_Kind (expr),
};

struct path_unit {
	uint64	index;
};

struct path_function {
	uint	decl;
};

struct path_macro {
	uint	decl;
};

struct path_tag {
	uint	decl;
};

struct path_decl {
	uint64	index;
};

struct path_flow {
	uint	scope;
	uint	index;
};

struct path_type {
	uint	head;
};

struct path_expr {
	uint	head;
};

struct path {
	enum path_kind	kind;
	union {
		struct path_unit		unit;
		struct path_function	function;
		struct path_macro		macro;
		struct path_tag			tag;
		struct path_decl		decl;
		struct path_flow		flow;
		struct path_type		type;
		struct path_expr		expr;
	};
};

struct typemember {
	const char	*name;
	uint	typeinfo;
	int		offset;
	int		value;
	uint	decl_index;
};

#define TypeInfo_Kind(name) TypeInfo_Kind_##name
enum typeinfo_kind {
	TypeInfo_Kind (basic),
	TypeInfo_Kind (struct),
	TypeInfo_Kind (enum),
	TypeInfo_Kind (union),
	TypeInfo_Kind (pointer),
	TypeInfo_Kind (array),
	TypeInfo_Kind (function),
};

#define TypeQualifier(name) TypeQualifier_##name
enum typequalifier {
	TypeQualifier (const) = 0x1,
	TypeQualifier (volatile) = 0x2,
	TypeQualifier (restrict) = 0x4,
};

struct typeinfo {
	usize	size;
	int		count;
	enum typequalifier	qualifiers;
	enum typeinfo_kind	kind;
	enum basictype		basic;
	const char	*tagname;
	uint	typeinfo;
	uint	members;
	uint	type_decl_index;
};

#define Flag(name) Flag_##name
enum flag {
	Flag (build32),
	Flag (test),
	Flag (lib),
	Flag (source),
	Flag (entry),
	Flag (unit_index),
	Flag (shortname),
	Flag (_count),
};
static const char	*const g_flag[Flag (_count)] = {
	"build32", "test", "lib", "source", "entry", "unit_index", "shortname"
};

struct declref {
	uint64		index;
	char		key[32];
};

struct manifest {
	const char			*libs;
	const char			*sources;
	const char			*options;
	const char			*expose;
	int					is_expose_all;
	const char			*cc_include_paths;
	const char			*cc_includes;
	const char			*cc_libpaths;
	const char			*cc_libs;
	const char			*cc_flags;
	const char			*cc_linker_flags;
};

struct unit {
	uint				scope;
	uint				link_decl_index;
	const char			*filename;
	const char			*function_name;
	char				*filepath;
	int					flags[Flag (_count)];
	struct position		pos;
	struct manifest		manifest;
	struct path			*paths;
	struct unit			**libptrs;
	struct decl			**decls;
	struct type			**types;
	struct expr			**exprs;
	struct flow			**flows;
	struct scope		**scopes;
	struct typeinfo		**typeinfos;
	struct typemember	**typemembers;
	struct declref		*ordinaries;
	struct declref		*tags;
};

#define Platform(name) Platform_##name
enum platform {
	Platform (windows),
	Platform (linux),
	Platform (macos),
};

struct tokenizer	g_tokenizer;
struct path		**g_path = 0;
struct unit		**g_libs = 0;
struct unit		*g_unit = 0;
uint64			g_typeinfo_struct_decl = 0;
int				g_is_build32 = 0;
enum platform	g_platform = Platform (windows);
int				g_shortname = 0;
int				g_is_test = 0;

#define ValueCategory(name) ValueCategory_##name
enum valuecategory {
	ValueCategory (lvalue),
	ValueCategory (rvalue),
};

struct typestack {
	struct type			types[Max_Type_Depth];
	int					types_count;
	int					head;
	enum valuecategory	value;
	int					is_sizeof_context;
};

#define EvalType(name) EvalType_##name
enum evaltype {
	EvalType (basic),
	EvalType (string),
	EvalType (typeinfo),
	EvalType (typeinfo_pointer),
	EvalType (typemember),
	EvalType (typemember_pointer),
};

int		is_evaltype_pointer (enum evaltype type) {
	return (type == EvalType (string) || type == EvalType (typeinfo_pointer) || type == EvalType (typemember_pointer));
}

struct evalvalue {
	enum evaltype	type;
	enum basictype	basic;
	union {
		usize	uvalue;
		isize	value;
		double	fvalue;
		const char	*string;
		uint	typeinfo_index;
		uint	typemember_index;
	};
};

struct sizevalue {
	unsigned	size;
	unsigned	alignment;
};

uint		make_scope (struct unit *unit, enum scopekind kind, uint parent);
struct scope	*get_scope (struct unit *unit, uint scope_index);
struct decl	*get_decl (struct unit *unit, uint index);
struct flow	*get_flow (struct unit *unit, uint index);
uint	make_param_decl (struct unit *unit, uint scope_index, const char *name, uint type_index, int line);
uint	get_flow_index (struct unit *unit, struct flow *flow);
void	print_expr (struct unit *unit, uint head_index, FILE *file);
void	print_type (struct unit *unit, uint head, FILE *file);
uint	make_type_copy (struct unit *unit, struct unit *decl_unit, uint type_index);
uint	make_expr_copy (struct unit *unit, struct unit *decl_unit, uint expr_index);
uint	make_scope_copy (struct unit *unit, struct unit *decl_unit, uint scope_index, uint parent_scope, uint param_scope);
uint64	find_decl_in_table (struct unit *unit, const char *name, enum tagtype tagtype, int is_typedef);
void	push_const_char_pointer_to_typestack (struct typestack *typestack);

int		is_lib_index (uint64 index) {
	return ((index >> 32) != 0);
}

uint	get_lib_index (uint64 index) {
	return (index >> 32);
}

uint	unlib_index (uint64 index) {
	return (index & 0xFFFFFFFFu);
}

uint64	make_lib_index (uint lib_index, uint bucket_index) {
	return (((uint64) lib_index << 32) + bucket_index);
}

struct unit	*get_lib (uint lib_index) {
	return (Get_Bucket_Element (g_libs, lib_index));
}

int		make_path_from_relative (char *path, const char *relto, const char *rel) {
	int			result;
	const char	*ptr;

	// Debug ("trying from [%s] [%s]", relto, rel);
	Assert (relto[strlen (relto) - 1] == '\\');
	ptr = strrchr (relto, '\\');
	while (ptr && 0 == strncmp (rel, "..\\", 3)) {
		rel += 3;
		ptr = strrchr (ptr - 1, '\\');
	}
	if (ptr) {
		strncpy (path, relto, ptr - relto + 1);
		path[ptr - relto + 1] = 0;
		strcat (path, rel);
		// Debug ("path: %s", path);
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

#include "basictype.c"
#include "path.c"
#include "expr.c"
#include "type.c"
#include "typestack.c"
#include "scope.c"
#include "copy.c"
#include "link.c"
#include "macro.c"
#include "typeinfo.c"
#include "eval.c"
#include "size.c"
#include "writer.c"
#include "unit.c"
#include "cbackend.c"

int		main (int argc, char *argv[]) {
	char	*input_file = 0;
	int		flags[Flag (_count)];
	char	include_path[256];
	char	working_path[256];

	if (GetModuleFileNameA (0, include_path, sizeof include_path)) {
		char	*ptr, *ptr2;

		ptr = strrchr (include_path, '\\');
		if (ptr) {
			*(ptr + 1) = 0;
			ptr += 1;
		} else {
			include_path[0] = 0;
			ptr = include_path;
		}
		ptr2 = strrchr (argv[0], '\\');
		if (ptr2) {
			ptr2 += 1;
		} else {
			ptr2 = argv[0];
		}
		if (0 == strcmp (ptr2, "main") || 0 == strcmp (ptr2, "main.exe")) {
			strcpy (ptr, "release\\includes\\");
		} else {
			strcpy (ptr, "includes\\");
		}
		// Debug ("include_path: %s", include_path);
	} else {
		Error ("cannot get executable path");
		exit (1);
	}
	if (GetCurrentDirectory (sizeof working_path, working_path)) {
		if (working_path[strlen (working_path) - 1] != '\\') {
			strcat (working_path, "\\");
		}
		// Debug ("working_path: %s", working_path);
	} else {
		Error ("cannot get working path");
		exit (1);
	}
	argv += 1;
	while (*argv) {
		if (**argv == '-') {
			if (0 == strcmp (*argv + 1, "build32")) {
				flags[Flag (build32)] = 1;
			} else if (0 == strcmp (*argv + 1, "test")) {
				flags[Flag (test)] = 1;
				g_is_test = 1;
			} else if (0 == strcmp (*argv + 1, "shortname")) {
				flags[Flag (shortname)] = 1;
				g_shortname = 1;
			} else {
				fprintf (stderr, "error: unknown flag '%s'\n", *argv);
				exit (1);
			}
		} else if (0 == input_file) {
			input_file = *argv;
		} else {
			fprintf (stderr, "error: only one input file is allowed\n");
			exit (1);
		}
		argv += 1;
	}
	if (0 == input_file) {
		fprintf (stderr, "error: no input file\n");
		exit (1);
	}

#if DebugMode
#else
	freopen ("NUL", "w", stderr);
	// freopen ("NUL", "w", stdout);
#endif

	g_is_build32 = flags[Flag (build32)];

	char	path[256];
	char	filename[64];
	int		result;

	if (make_path_from_relative (path, working_path, input_file)) {
		char	*ptr;

		ptr = strrchr (path, '\\');
		Assert (ptr);
		strncpy (working_path, path, (ptr - path) + 1);
		working_path[(ptr - path) + 1] = 0;
		strcpy (filename, ptr + 1);
		fprintf (stdout, "%s\n", filename);
		result = run_compiler (filename, include_path, working_path, flags);
	} else {
		Error ("cannot create relative path");
		result = 0;
	}

	if (result) {
		fprintf (stdout, "\ndone\n");
	} else {
		fprintf (stdout, "\nfailed\n");
	}
	return (result == 0);
}

#pragma comment (lib, "Shlwapi.lib")




