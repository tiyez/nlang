
enum e_typekind {
    e_typekind_basic,
    e_typekind_struct,
    e_typekind_enum,
    e_typekind_union,
    e_typekind_pointer,
    e_typekind_array,
    e_typekind_function,
};
enum e_typebasic {
    e_typebasic_void,
    e_typebasic_char,
    e_typebasic_uchar,
    e_typebasic_wchar,
    e_typebasic_byte,
    e_typebasic_ubyte,
    e_typebasic_short,
    e_typebasic_ushort,
    e_typebasic_int,
    e_typebasic_uint,
    e_typebasic_size,
    e_typebasic_usize,
    e_typebasic_float,
    e_typebasic_double,
};
enum e_typequalifier {
    e_typequalifier_const = 1,
    e_typequalifier_volatile = 2,
    e_typequalifier_restrict = 4,
};
struct s_typeinfo {
    unsigned long long size;
    int count;
    enum e_typequalifier qualifiers;
    enum e_typekind kind;
    enum e_typebasic basic;
    const char *tagname;
    const struct s_typeinfo *type;
    const struct s_typemember *members;
};
struct s_typemember {
    const char *name;
    const struct s_typeinfo *type;
    const struct s_typemember *members;
    int value;
    int offset;
};
enum e_hello {
    e_hello_hello,
    e_hello_world,
};
enum e_scopekind {
    e_scopekind_unit,
    e_scopekind_func,
    e_scopekind_code,
    e_scopekind_tag,
    e_scopekind_param,
    e_scopekind_macro,
};
enum e_tagtype {
    e_tagtype_invalid,
    e_tagtype_struct,
    e_tagtype_union,
    e_tagtype_enum,
};
struct s_scope {
    enum e_scopekind kind;
    enum e_tagtype tagtype;
    int parent_scope;
    int param_scope;
    int type_index;
    int decl_begin;
    int decl_last;
    int flow_begin;
    int flow_last;
};
enum e_hello array[3];
const char *get_enum_name (int index, struct s_typeinfo typeinfo);
struct s_scope array_struct[3];
void print_struct (void *ptr, const struct s_typeinfo *typeinfo);
int main (int argc, char *argv[]);
struct s_typeinfo const g_typeinfos[];
struct s_typemember const g_typemembers[] = {
    { "hello", 0, 0, 0, 0 },
    { "world", 0, 0, 1, 0 },
    { 0, 0, 0, 0, 0 },
};
struct s_typeinfo const g_typeinfos[] = {
    { 4, 2, 0, 2, 0, "hello", 0, g_typemembers + 0 },
    { 4, 2, 0, 2, 0, "hello", 0, g_typemembers + 0 },
    { 4, 2, 0, 2, 0, "hello", 0, g_typemembers + 0 },
};
enum e_hello array[3] = {
    e_hello_hello,
    e_hello_world,
    e_hello_world,
};
const char *get_enum_name (int index, struct s_typeinfo typeinfo) {
    return typeinfo.members[index].name;
}
struct s_scope array_struct[3] = {
    {
        1,
    },
};
void print_struct (void *ptr, const struct s_typeinfo *typeinfo) {
    ((typeinfo->kind == e_typekind_struct)) ? ((1)) : ((*((int *) 0) = 0));
}
int main (int argc, char *argv[]) {
    int index;
    const struct s_typemember *member;
    float pi;;
    printf ("%s:%s: ", "test.n", "main"), printf ("Hello %s", "World", "Hello %s", "World"), printf ("\n");
    pi = (3.140000);
    ((1 != 1)) ? ((1)) : ((*((int *) 0) = 0));
    ((1 == 1)) ? ((1)) : ((*((int *) 0) = 0));
    index = 0;
    member = (g_typeinfos[0]).members;
    while (member->name) {
        printf ("member: %s\n", member->name);
        member += 1;
    }
    printf ("Hello (hello): %s\n", get_enum_name (e_hello_hello, (g_typeinfos[1])));
    printf ("Hello (world): %s\n", get_enum_name (e_hello_world, (g_typeinfos[2])));
    return 1;
}