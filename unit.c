








int		parse_unit (struct unit *unit, char **ptokens) {
	static const char	*typeinfo_text = "\n"
	"enum typekind {\n"
	"	basic;\n"
	"	struct;\n"
	"	enum;\n"
	"	union;\n"
	"	pointer;\n"
	"	array;\n"
	"	function;\n"
	"}\n"
	"accessor TypeKind enum typekind;\n"
	"\n"
	"enum typebasic {\n"
	"	void;\n"
	"	char;\n"
	"	uchar;\n"
	"	wchar;\n"
	"	byte;\n"
	"	ubyte;\n"
	"	short;\n"
	"	ushort;\n"
	"	int;\n"
	"	uint;\n"
	"	size;\n"
	"	usize;\n"
	"	float;\n"
	"	double;\n"
	"}\n"
	"accessor TypeBasic enum typebasic;\n"
	"\n"
	"struct typeinfo {\n"
	"	size	usize;\n"
	"	count	int;\n"
	"	kind	enum typekind;\n"
	"	basic	enum typebasic;\n"
	"	tagname	*const char;\n"
	"	type	*const struct typeinfo;\n"
	"	members	*const struct typemember;\n"
	"}\n"
	"\n"
	"struct typemember {\n"
	"	name	*const char;\n"
	"	type	*const struct typeinfo;\n"
	"	value	int;\n"
	"	offset	int;\n"
	"}\n"
	"\n"
	"external expand_array (*void) (array *void, size *usize);\n"
	"\n";
	static struct tokenizer	tokenizer = {0};
	char	*tokens;
	int		result;

	tokens = get_first_token (&tokenizer);
	if (tokens == 0) {
		tokens = tokenize_with (&tokenizer, typeinfo_text, (int []) {0}, 1, "<typeinfo?>");
	}
	if (tokens) {
		print_tokens (tokens, 1, "typeinfo", stderr);
		if (tokens[-1] == Token (newline)) {
			tokens = next_token (tokens, 0);
		}
		if (parse_scope (unit, unit->root_scope, &tokens)) {
			unit->typeinfo_struct_decl = find_decl_tag (unit, unit->root_scope, "typeinfo", TagType (struct));
			Assert (unit->typeinfo_struct_decl >= 0);
			result = parse_scope (unit, unit->root_scope, ptokens);
		} else {
			result = 0;
		}
	} else {
		Error ("cannot parse typeinfo types");
		result = 0;
	}
	return (result);
}


