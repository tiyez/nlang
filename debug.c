




void	print_included_libs (struct unit *unit) {
	if (unit->libptrs) {
		struct unit	**libs;

		Debug ("libs for %s:", unit->filename);
		libs = unit->libptrs;
		while (*libs) {
			Debug ("-- %s", (*libs)->filename);
			libs += 1;
		}
	} else {
		Debug ("no libs for %s", unit->filename);
	}
}

void	print_symbols_included_from (struct unit *unit, uint lib_index) {
	if (unit->ordinaries) {
		struct declref	*refs;
		struct unit		*lib;

		lib = get_lib (lib_index);
		refs = unit->ordinaries;
		while (refs->index) {
			if (get_lib_index (refs->index) == lib_index) {
				Debug ("-- %s [%s]", get_decl (lib, unlib_index (refs->index))->name, refs->key);
			}
			refs += 1;
		}
	}
}






