


usize		g_unit_index = 0;
usize		g_head_lib_index = 0;







int		parse_unit (struct unit *unit, char **ptokens, const char *filename) {
	int		result;

	unit->filename = filename;
	unit->function_name = "<none>";
	result = parse_scope (unit, unit->scope, ptokens);
	return (result);
}

int		parse_source (struct unit *unit, char *content, usize size, const char *filename) {
	int		result;
	char	*tokens;
	int		*nl_array;

	preprocess_text (content, content + size, &nl_array);
	tokens = tokenize (content, nl_array, 1, filename);
	if (tokens) {
		char	*begin;

		init_pos (&unit->pos, filename);
		unit->pos.filepath = unit->filepath;
		if (tokens[-1] == Token (newline)) {
			tokens = next_token (tokens, &unit->pos);
		}
		begin = g_tokenizer.current;
		push_string_token (&g_tokenizer, 0, filename, strlen (filename), 0);
		if (parse_unit (unit, &tokens, get_next_from_tokenizer (&g_tokenizer, begin))) {
			result = 1;
		} else {
			Error ("cannot parse file '%s'", filename);
			result = 0;
		}
	} else {
		Error ("cannot tokenize file '%s'", filename);
		result = 0;
	}
	if (nl_array) {
		release_array (nl_array);
		/* todo: tokens cleanup */
	}
	return (result);
}

int		add_decl_to_lookup_table (struct unit *unit, struct unit *decl_unit, uint decl_index) {
	int			result;
	struct decl	*decl;
	uint64		gindex;

	if (decl_unit != unit) {
		gindex = ((uint64) Get_Bucket_Element_Index (g_libs, decl_unit) << 32) + decl_index;
	} else {
		gindex = decl_index;
	}
	decl = Get_Bucket_Element (decl_unit->buckets->decls, decl_index);
	if (decl->kind == DeclKind (tag)) {
		if (Prepare_Array (unit->tags, 1)) {
			struct declref	*ref;

			ref = Push_Array (unit->tags);
			ref->index = gindex;
			ref->key[0] = g_tagname[decl->tag.type][0];
			strncpy (ref->key + 1, decl->name, sizeof ref->key - 2);
			ref->key[sizeof ref->key - 1] = 0;
			result = 1;
		} else {
			Error ("cannot prepare tags array");
			result = 0;
		}
	} else if (decl->kind == DeclKind (var) || decl->kind == DeclKind (const) || decl->kind == DeclKind (func)) {
		if (Prepare_Array (unit->ordinaries, 1)) {
			struct declref	*ref;

			ref = Push_Array (unit->ordinaries);
			ref->index = gindex;
			strncpy (ref->key, decl->name, sizeof ref->key - 1);
			ref->key[sizeof ref->key - 1] = 0;
			result = 1;
		} else {
			Error ("cannot prepare tags array");
			result = 0;
		}
	} else if (decl->kind == DeclKind (define)) {
		if (decl->define.kind == DefineKind (macro) || decl->define.kind == DefineKind (accessor) || decl->define.kind == DefineKind (external) || decl->define.kind == DefineKind (builtin)) {
			if (Prepare_Array (unit->ordinaries, 1)) {
				struct declref	*ref;

				ref = Push_Array (unit->ordinaries);
				ref->index = gindex;
				strncpy (ref->key, decl->name, sizeof ref->key - 1);
				ref->key[sizeof ref->key - 1] = 0;
				result = 1;
			} else {
				Error ("cannot prepare tags array");
				result = 0;
			}
		} else if (decl->define.kind == DefineKind (type) || decl->define.kind == DefineKind (opaque)) {
			if (Prepare_Array (unit->tags, 1)) {
				struct declref	*ref;

				ref = Push_Array (unit->tags);
				ref->index = gindex;
				ref->key[0] = 't';
				strncpy (ref->key + 1, decl->name, sizeof ref->key - 2);
				ref->key[sizeof ref->key - 1] = 0;
				result = 1;
			} else {
				Error ("cannot prepare tags array");
				result = 0;
			}
		} else {
			result = 1;
		}
	} else {
		result = 1;
	}
	return (result);
}

int		declref_cmp (const void *left, const void *right) {
	const struct declref	*leftref, *rightref;

	leftref = left;
	rightref = right;
	return (strcmp (leftref->key, rightref->key));
}

uint64	find_decl_in_table (struct unit *unit, const char *name, enum tagtype tagtype, int is_typedef) {
	uint64			result;
	struct declref	*arrayref;
	struct declref	*elem;
	struct declref	ref;

	if (tagtype == TagType (invalid) && !is_typedef) {
		Assert (unit->ordinaries);
		strncpy (ref.key, name, sizeof ref.key - 1);
		ref.key[sizeof ref.key - 1] = 0;
		arrayref = unit->ordinaries;
	} else if (tagtype == TagType (invalid) && is_typedef) {
		Assert (unit->tags);
		ref.key[0] = 't';
		strncpy (ref.key + 1, name, sizeof ref.key - 2);
		ref.key[sizeof ref.key - 1] = 0;
		arrayref = unit->tags;
	} else {
		Assert (unit->tags);
		ref.key[0] = g_tagname[tagtype][0];
		strncpy (ref.key + 1, name, sizeof ref.key - 2);
		ref.key[sizeof ref.key - 1] = 0;
		arrayref = unit->tags;
	}
	elem = bsearch (&ref, arrayref, Get_Array_Count (arrayref), sizeof *arrayref, declref_cmp);
	if (elem) {
		int		elem_index;
		int		next;

		elem_index = Get_Element_Index (arrayref, elem);
		do {
			struct decl	*decl;
			uint		libindex, declindex;

			elem = arrayref + elem_index;
			libindex = elem->index >> 32;
			declindex = elem->index & 0xFFFFFFFFu;
			if (libindex) {
				struct unit	*libunit;

				libunit = Get_Bucket_Element (g_libs, libindex);
				decl = Get_Bucket_Element (libunit->buckets->decls, declindex);
			} else {
				decl = Get_Bucket_Element (unit->buckets->decls, declindex);
			}
			Assert (decl);
			if (0 == strcmp (decl->name, name)) {
				result = elem->index;
				next = 0;
			} else {
				if (elem_index + 1 < Get_Array_Count (arrayref)) {
					elem_index += 1;
					if (0 == strcmp (ref.key, arrayref[elem_index].key)) {
						next = 1;
					} else {
						next = 0;
						result = 0;
					}
				} else {
					result = 0;
					next = 0;
				}
			}
		} while (next);
	} else {
		result = 0;
	}
	return (result);
}

int		check_declref_array_for_duplicates (struct unit *unit, struct declref *array) {
	int		result;

	if (Get_Array_Count (array) > 1) {
		struct declref	*elem;

		elem = array;
		result = 1;
		do {
			if (0 == strcmp (elem[0].key, elem[1].key)) {
				struct decl	*left, *right;

				if (elem[0].index >> 32) {
					struct unit *otherlib;

					otherlib = Get_Bucket_Element (g_libs, elem[0].index >> 32);
					left = Get_Bucket_Element (otherlib->buckets->decls, elem[0].index & 0xFFFFFFFFu);
				} else {
					left = Get_Bucket_Element (unit->buckets->decls, elem[0].index);
				}
				if (elem[1].index >> 32) {
					struct unit *otherlib;

					otherlib = Get_Bucket_Element (g_libs, elem[1].index >> 32);
					right = Get_Bucket_Element (otherlib->buckets->decls, elem[1].index & 0xFFFFFFFFu);
				} else {
					right = Get_Bucket_Element (unit->buckets->decls, elem[1].index);
				}
				Assert (left && right);
				if (0 == strcmp (left->name, right->name)) {
					if (left->kind == DeclKind (tag)) {
						Error ("redefinition of %s %s", g_tagname[left->tag.type], left->name);
					} else {
						Error ("redefinition of %s", left->name);
					}
					result = 0;
				}
			}
			elem += 1;
		} while ((elem - array) + 1 < Get_Array_Count (array));
	} else {
		result = 1;
	}
	return (result);
}

int		initialize_lookup_tables (struct unit *unit) {
	int		result;
	int		count;
	struct unit	**libunitptr;
	uint	index;

	count = 0;
	libunitptr = unit->libptrs;
	if (libunitptr) while (*libunitptr) {
		struct unit	*libunit;

		libunit = *libunitptr;
		if (libunit->manifest.is_expose_all) {
			index = get_scope (libunit, libunit->scope)->decl_begin;
			result = 1;
			while (result && index) {
				struct decl	*decl;

				decl = get_decl (libunit, index);
				if (decl->name && decl->name[0] != '_') {
					result = add_decl_to_lookup_table (unit, libunit, index);
				} else if (decl->name && decl->kind == DeclKind (define) && decl->define.kind == DefineKind (builtin)) {
					result = add_decl_to_lookup_table (unit, libunit, index);
				}
				index = decl->next;
			}
		} else if (libunit->manifest.expose) {
			const char	*token;

			token = libunit->manifest.expose;
			do {
				enum tagtype	tagtype;
				uint64			index;

				tagtype = TagType (invalid);
				if (is_tagtype (token, &tagtype)) {
					token = next_const_token (token, 0);
					index = find_decl_in_table (libunit, token, tagtype, 0);
				} else {
					index = find_decl_in_table (libunit, token, TagType (invalid), 0);
					if (!index) {
						index = find_decl_in_table (libunit, token, TagType (invalid), 1);
					}
				}
				if (index) {
					if (index >> 32) {
						struct unit	*otherunit;

						otherunit = Get_Bucket_Element (g_libs, index >> 32);
						result = add_decl_to_lookup_table (unit, otherunit, index & 0xFFFFFFFFu);
					} else {
						result = add_decl_to_lookup_table (unit, libunit, index);
					}
				} else {
					if (tagtype == TagType (invalid)) {
						Error ("undefined symbol in manifest expose scope '%s'", token);
					} else {
						Error ("undefined symbol in manifest expose scope '%s %s'", g_tagname[tagtype], token);
					}
					result = 0;
				}
				token = next_const_token (token, 0);
			} while (result && token[-1]);
		} else {
			result = 1;
		}
		libunitptr += 1;
	} else {
		result = 1;
	}
	if (result) {
		uint	decl_index;

		decl_index = get_scope (unit, unit->scope)->decl_begin;
		while (result && decl_index) {
			result = add_decl_to_lookup_table (unit, unit, decl_index);
			decl_index = get_decl (unit, decl_index)->next;
		}
	}
	if (result) {
		if (unit->ordinaries) {
			qsort (unit->ordinaries, Get_Array_Count (unit->ordinaries), sizeof *unit->ordinaries, declref_cmp);
		}
		if (unit->tags) {
			qsort (unit->tags, Get_Array_Count (unit->tags), sizeof *unit->tags, declref_cmp);
		}
		if (check_declref_array_for_duplicates (unit, unit->ordinaries)) {
			if (check_declref_array_for_duplicates (unit, unit->tags)) {
				result = 1;
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	return (result);
}

int		is_option_end (const char *token) {
	return (token[-1] == Token (eof) || is_token (token, Token (punctuator), ";"));
}

const char	*next_option (const char *opt) {
	opt = next_const_token (opt, 0);
	Assert (opt[-1]);
	opt = next_const_token (opt, 0);
	if (is_option_end (opt)) {
		opt = 0;
	}
	return (opt);
}

const char	*get_option_value (const char *option) {
	option = next_const_token (option, 0);
	return (option);
}

const char	*get_value_from_options (const char *options, const char *option_name) {
	const char	*result;

	result = 0;
	while (!result && options) {
		if (0 == strcmp (options, option_name)) {
			result = get_option_value (options);
		}
		options = next_option (options);
	}
	return (result);
}

int		are_option_values_the_same (const char *left, const char *right) {
	int		result;

	if (left && right && !is_option_end (left) && !is_option_end (right)) {
		const char	*opts, *value;

		opts = left;
		do {
			value = get_value_from_options (right, opts);
			if (value) {
				const char	*lvalue;

				lvalue = get_option_value (opts);
				if (lvalue[-1] == value[-1] && 0 == strcmp (lvalue, value)) {
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 1;
			}
			opts = next_option (opts);
		} while (result && opts);
	} else {
		result = 1;
	}
	return (result);
}

int		are_options_valid (const char *options) {
	int		result;

	if (options && options[-1]) {
		do {
			if (options[-1] == Token (identifier)) {
				options = next_const_token (options, 0);
				if (options[-1] == Token (preprocessing_number)) {
					options = next_const_token (options, 0);
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} while (result && options[-1]);
	} else {
		result = 1;
	}
	return (result);
}

int		translate_cc_includes (struct unit *unit, struct cbuffer *buffer) {
	int		result;

	if (unit->manifest.cc_includes) {
		const char	*token;

		token = unit->manifest.cc_includes;
		do {
			result = cbackend_include (unit, token, buffer);
			token = next_const_token (token, 0);
		} while (result && token[-1]);
	} else {
		result = 1;
	}
	return (result);
}

int		translate_unit_to_c (struct unit *unit, const char *filename) {
	int				result;
	struct cbuffer	cbuffer = {0};
	uint			index;

	init_cbuffer (&cbuffer);
	index = Get_Bucket_First_Index (g_libs);
	result = 1;
	while (result && index) {
		struct unit	*libunit;

		libunit = Get_Bucket_Element (g_libs, index);
		result = translate_cc_includes (libunit, &cbuffer);
		index = Get_Next_Bucket_Index (g_libs, index);
	}
	if (result) {
		result = translate_cc_includes (unit, &cbuffer);
	}
	if (result) {
		index = Get_Bucket_First_Index (g_libs);
		while (result && index) {
			struct unit	*libunit;

			libunit = Get_Bucket_Element (g_libs, index);
			result = cbackend_tags (libunit, libunit->scope, &cbuffer);
			index = Get_Next_Bucket_Index (g_libs, index);
		}
	}
	if (result) {
		result = cbackend_tags (unit, unit->scope, &cbuffer);
	}
	if (result) {
		index = Get_Bucket_First_Index (g_libs);
		while (result && index) {
			struct unit	*libunit;

			libunit = Get_Bucket_Element (g_libs, index);
			result = cbackend_prototypes (libunit, libunit->scope, &cbuffer);
			index = Get_Next_Bucket_Index (g_libs, index);
		}
	}
	if (result) {
		result = cbackend_prototypes (unit, unit->scope, &cbuffer);
	}
	if (result) {
		index = Get_Bucket_First_Index (g_libs);
		while (result && index) {
			struct unit	*libunit;

			libunit = Get_Bucket_Element (g_libs, index);
			result = cbackend_typeinfo (libunit, &cbuffer);
			index = Get_Next_Bucket_Index (g_libs, index);
		}
	}
	if (result) {
		result = cbackend_typeinfo (unit, &cbuffer);
	}
	if (result) {
		index = Get_Bucket_First_Index (g_libs);
		while (result && index) {
			struct unit	*libunit;

			libunit = Get_Bucket_Element (g_libs, index);
			result = cbackend_definitions (libunit, libunit->scope, &cbuffer);
			index = Get_Next_Bucket_Index (g_libs, index);
		}
	}
	if (result) {
		result = cbackend_definitions (unit, unit->scope, &cbuffer);
	}
	if (result) {
		FILE	*file;

		file = fopen (filename, "w");
		if (file) {
			char	**ptr;

			ptr = cbuffer.wr->buffer;
			while (result && *ptr) {
				if (Get_Array_Count (*ptr) == fwrite (*ptr, 1, Get_Array_Count (*ptr), file)) {
					result = 1;
					ptr += 1;
				} else {
					Error ("not all bytes are written to file");
					result = 0;
				}
			}
			fclose (file);
		} else {
			Error ("cannot open '%s' for write", filename);
			result = 0;
		}
	}
	free_cbuffer (&cbuffer);
	return (result);
}

void	print_cl_cc_sources (struct unit *unit, char **pptr, const char *filepath) {
	if (unit->manifest.cc_sources) {
		char	path[256], workdir[256], *ptr2;
		const char	*token;

		ptr2 = strrchr (filepath, '\\');
		Assert (ptr2);
		strncpy (workdir, filepath, ptr2 - filepath + 1);
		workdir[ptr2 - filepath + 1] = 0;
		token = unit->manifest.cc_sources;
		do {
			if (make_path_from_relative (path, workdir, token)) {
				*pptr += sprintf (*pptr, "\"%s\" ", path);
			} else {
				Error ("cannot get full path of '%s'", token);
			}
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

void	print_cl_cc_include_paths (struct unit *unit, char **pptr) {
	if (unit->manifest.cc_include_paths) {
		const char	*token;

		token = unit->manifest.cc_include_paths;
		do {
			char	include_path_[256];

			strcpy (include_path_, token);
			if (include_path_[strlen (include_path_) - 1] == '\\') {
				include_path_[strlen (include_path_) - 1] = 0;
			}
			*pptr += sprintf (*pptr, "/I\"%s\" ", include_path_);
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

void	print_cl_cc_flags (struct unit *unit, char **pptr) {
	if (unit->manifest.cc_flags) {
		const char	*token;

		token = unit->manifest.cc_flags;
		do {
			*pptr += sprintf (*pptr, "%s ", token);
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

void	print_cl_cc_linker_flags (struct unit *unit, char **pptr) {
	if (unit->manifest.cc_linker_flags) {
		const char	*token;

		token = unit->manifest.cc_linker_flags;
		do {
			*pptr += sprintf (*pptr, "%s ", token);
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

void	print_cl_cc_libpaths (struct unit *unit, char **pptr) {
	if (unit->manifest.cc_libpaths) {
		const char	*token;

		token = unit->manifest.cc_libpaths;
		do {
			*pptr += sprintf (*pptr, "/LIBPATH:\"%s\" ", token);
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

void	print_cl_cc_libs (struct unit *unit, char **pptr) {
	if (unit->manifest.cc_libs) {
		const char	*token;

		token = unit->manifest.cc_libs;
		do {
			*pptr += sprintf (*pptr, "%s ", token);
			token = next_const_token (token, 0);
		} while (token[-1]);
	}
}

int		compile_unit_c (struct unit *unit, const char *filename, const char *include_path) {
	int		result;
	char	*ptr;
	char	command[32 * 1024];
	char	include_path_[256];
	struct unit	*lib;
	uint		lib_index;

	strcpy (include_path_, include_path);
	if (include_path_[strlen (include_path_) - 1] == '\\') {
		include_path_[strlen (include_path_) - 1] = 0;
	}
	command[0] = 0;
	ptr = command;
	ptr += sprintf (ptr, "cl.exe /nologo \"%s\" /I\"%s\" ", filename, include_path_);
	print_cl_cc_sources (unit, &ptr, unit->entry_filepath);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_sources (Get_Bucket_Element (g_libs, lib_index), &ptr, Get_Bucket_Element (g_libs, lib_index)->entry_filepath);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	print_cl_cc_include_paths (unit, &ptr);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_include_paths (Get_Bucket_Element (g_libs, lib_index), &ptr);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	print_cl_cc_flags (unit, &ptr);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_flags (Get_Bucket_Element (g_libs, lib_index), &ptr);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	{
		char	output[256], *ptr2;

		strcpy (output, filename);
		ptr2 = strrchr (output, '\\');
		Assert (ptr2);
		ptr2 += 1;
		*ptr2 = 0;
		ptr += sprintf (ptr, "/Fo\"%s\\\" ", output);
	}
	ptr += sprintf (ptr, "/link ");
	print_cl_cc_linker_flags (unit, &ptr);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_linker_flags (Get_Bucket_Element (g_libs, lib_index), &ptr);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	print_cl_cc_libpaths (unit, &ptr);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_libpaths (Get_Bucket_Element (g_libs, lib_index), &ptr);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	print_cl_cc_libs (unit, &ptr);
	lib_index = Get_Bucket_First_Index (g_libs);
	while (lib_index) {
		print_cl_cc_libs (Get_Bucket_Element (g_libs, lib_index), &ptr);
		lib_index = Get_Next_Bucket_Index (g_libs, lib_index);
	}
	{
		char	output[256], *ptr2;

		strcpy (output, filename);
		ptr2 = strrchr (output, '\\');
		if (ptr2) {
			ptr2 += 1;
		} else {
			ptr2 = output;
		}
		if ((ptr2 = strchr (ptr2, '.'))) {
			strcpy (ptr2, ".exe");
		} else {
			strcat (output, ".exe");
		}
		ptr += sprintf (ptr, "/OUT:\"%s\" ", output);
	}
	if (g_show_build_command) {
		printf ("\nbuild command: %s\n", command);
	}
	result = 0 == system (command);
	return (result);
}

void	init_unit (struct unit *unit) {
	unit->buckets = &unit->active_buckets;
	unit->scope = make_scope (unit, ScopeKind (unit), 0);
}

void	clear_buckets (struct buckets *buckets) {
	Clear_Bucket (buckets->decls);
	Clear_Bucket (buckets->types);
	Clear_Bucket (buckets->exprs);
	Clear_Bucket (buckets->flows);
	Clear_Bucket (buckets->scopes);
	Clear_Bucket (buckets->typeinfos);
	Clear_Bucket (buckets->typemembers);
}

void	free_buckets (struct buckets *buckets) {
	Free_Bucket (buckets->decls);
	Free_Bucket (buckets->types);
	Free_Bucket (buckets->exprs);
	Free_Bucket (buckets->flows);
	Free_Bucket (buckets->scopes);
	Free_Bucket (buckets->typeinfos);
	Free_Bucket (buckets->typemembers);
}

int		is_active_buckets (struct unit *unit) {
	return (unit->buckets == &unit->active_buckets);
}

int		run_test (struct unit *unit, const char *exe_path, const char *refpath) {
	int		result;
	char	*reffile;
	usize	size;

	Debug ("refpath: %s", refpath);
	if (PathFileExistsA (refpath)) {
		reffile = read_entire_file (refpath, &size);
		if (reffile) {
			result = 1;
		} else {
			Error ("cannot read reference file '%s'", refpath);
			result = 0;
		}
	} else {
		reffile = 0;
		result = 1;
	}
	if (result) {
		FILE	*pipe;

		pipe = _popen (exe_path, "rt");
		if (pipe) {
			char	*data;

			data = read_file_until_eof (pipe);
			if (data) {
				if (reffile) {
					if (0 == strcmp (data, reffile)) {
						result = 1;
					} else {
						fprintf (stderr, "\nerror: output mismatch\n");
						fprintf (stderr, "reference:\n%s\n----------------\n", reffile);
						fprintf (stderr, "output:\n%s\n----------------\n", data);
						result = 0;
					}
				} else {
					if (write_data_to_file (refpath, data, Get_Array_Count (data))) {
						result = 1;
					} else {
						Error ("cannot write data to reference file");
						result = 0;
					}
				}
				Free_Array (data);
			} else {
				Error ("cannot read from pipe");
				result = 0;
			}
			_pclose (pipe);
		} else {
			Error ("cannot launch process '%s'", exe_path);
			result = 0;
		}
		if (reffile) {
			free (reffile);
		}
	}
	return (result);
}

int		add_lib_to_unit (struct unit *unit, struct unit *lib) {
	int		result;

	if (Prepare_Array (unit->libptrs, 1)) {
		*Push_Array (unit->libptrs) = lib;
		result = 1;
	} else {
		Error ("cannot prepare libptrs array");
		result = 0;
	}
	return (result);
}

int		build_unit (struct unit *unit, const char *entry_filename, const char *include_path, const char *working_path) {
	int		result;
	char	*content;
	usize	size;
	char	path[256];

	if (Prepare_Array (g_unit_stack, 1)) {
		*Push_Array (g_unit_stack) = unit;
	}
	unit->flags[Flag (entry)] = 1;
	if (!unit->flags[Flag (lib)]) {
		path[0] = 0;
		strcat (path, include_path);
		strcat (path, "_head");
		if (Prepare_Bucket (g_libs, 1)) {
			struct unit	*libunit;

			libunit = Push_Bucket (g_libs);
			init_unit (libunit);
			memcpy (libunit->flags, unit->flags, sizeof libunit->flags);
			libunit->flags[Flag (lib)] = 1;
			libunit->flags[Flag (unit_index)] = g_unit_index;
			g_unit_index += 1;
			g_head_lib_index = Get_Bucket_Element_Index (g_libs, libunit);
			if (build_unit (libunit, "_head", include_path, include_path)) {
				if (Prepare_Array (unit->libptrs, 1)) {
					*Push_Array (unit->libptrs) = Get_Bucket_Element (g_libs, g_head_lib_index);
					g_typeinfo_struct_decl = make_lib_index (g_head_lib_index, find_decl_in_table (libunit, "typeinfo", TagType (struct), 0));
					Assert (g_typeinfo_struct_decl);
					result = 1;
				} else {
					Error ("cannot prepare libptrs array");
					result = 0;
				}
			} else {
				Error ("cannot build lib '@_head'");
				result = 0;
			}
		} else {
			Error ("cannot prepare array for libunit");
			result = 0;
		}
	} else if (g_head_lib_index) {
		if (g_head_lib_index != Get_Bucket_Element_Index (g_libs, unit)) {
			if (add_lib_to_unit (unit, Get_Bucket_Element (g_libs, g_head_lib_index))) {
				result = 1;
			} else {
				Error ("cannot add lib to unit");
				result = 0;
			}
		} else {
			result = 1;
		}
	} else {
		Error ("_head lib not found");
		result = 0;
	}
	if (result) {
		path[0] = 0;
		strcat (path, working_path);
		strcat (path, entry_filename);
		Assert (0 == strrchr (entry_filename, '\\'));
		content = read_entire_file (path, &size);
		if (content) {
			unit->filepath = g_tokenizer.current;
			push_string_token (&g_tokenizer, 0, path, strlen (path), 0);
			unit->filepath = get_next_from_tokenizer (&g_tokenizer, (char *) unit->filepath);
			unit->entry_filepath = unit->filepath;
			if (parse_source (unit, content, size, entry_filename)) {
				free (content);
				content = 0;
				unit->flags[Flag (entry)] = 0;
				if (unit->manifest.libs) {
					const char	*lib;

					lib = unit->manifest.libs;
					do {
						char	lib_working_path[256];
						char	lib_filename[64];

						path[0] = 0;
						if (*lib == '@') {
							if (lib[1] == '_') {
								Error ("private global libs are not allowed in manifest");
								result = 0;
							} else {
								char	*ptr;

								if (make_path_from_relative (path, include_path, lib + 1)) {
									ptr = strrchr (path, '\\');
									Assert (ptr);
									ptr += 1;
									strncpy (lib_working_path, path, ptr - path);
									lib_working_path[ptr - path] = 0;
									strcpy (lib_filename, ptr);
									result = 1;
								} else {
									Error ("cannot get path to lib '%s'", lib);
									result = 0;
								}
							}
						} else if (isalpha (lib[0]) && lib[1] == ':') {
							char	*ptr;

							ptr = strrchr (lib, '\\');
							if (ptr) {
								ptr += 1;
								strncpy (lib_working_path, lib, ptr - lib);
								lib_working_path[ptr - lib] = 0;
								strcpy (lib_filename, ptr);
								result = 1;
							} else {
								Error ("invalid fullpath '%s'", lib);
								result = 0;
							}
						} else {
							char	*ptr;

							if (make_path_from_relative (path, working_path, lib)) {
								ptr = strrchr (path, '\\');
								Assert (ptr);
								ptr += 1;
								strncpy (lib_working_path, path, ptr - path);
								lib_working_path[ptr - lib] = 0;
								strcpy (lib_filename, ptr);
								result = 1;
							} else {
								Error ("cannot get path to lib '%s'", lib);
								result = 0;
							}
						}
						if (result) {
							struct tokenizer	tokenizer = {0};
							uint				index;
							int					found;
							const char			*man_options = 0;
							const char			*libname;

							libname = lib;
							lib = next_const_token (lib, 0);
							if (!is_token (lib, Token (punctuator), ";")) {
								char	*begin;

								begin = tokenizer.current;
								do {
									result = copy_token (&tokenizer, lib);
									lib = next_const_token (lib, 0);
								} while (result && lib[-1] && !is_token (lib, Token (punctuator), ";"));
								if (result) {
									end_tokenizer (&tokenizer, 0);
									man_options = get_next_from_tokenizer (&tokenizer, begin);
									if (are_options_valid (man_options)) {
										result = 1;
									} else {
										Error ("invalid options in %s for lib %s", entry_filename, lib_filename);
										result = 0;
									}
								}
							}
							if (result) {
								struct unit	*libunit;

								libunit = 0;
								index = Get_Bucket_First_Index (g_libs);
								while (!libunit && index) {
									struct unit	*libptr;

									libptr = Get_Bucket_Element (g_libs, index);
									if (0 == strcmp (path, libptr->entry_filepath)) {
										if (are_option_values_the_same (libptr->manifest.options, man_options)) {
											libunit = libptr;
										}
									}
									index = Get_Next_Bucket_Index (g_libs, index);
								}
								if (!libunit) {
									if (Prepare_Bucket (g_libs, 1)) {
										libunit = Push_Bucket (g_libs);
										init_unit (libunit);
										memcpy (libunit->flags, unit->flags, sizeof libunit->flags);
										libunit->flags[Flag (lib)] = 1;
										libunit->flags[Flag (unit_index)] = g_unit_index;
										g_unit_index += 1;
										libunit->manifest.options = man_options;
										if (build_unit (libunit, lib_filename, include_path, lib_working_path)) {
											result = 1;
										} else {
											Error ("cannot build lib '%s'", libname);
											result = 0;
										}
									} else {
										Error ("cannot prepare array for libunit");
										result = 0;
									}
								} else {
									result = 1;
								}
								if (result) {
									if (add_lib_to_unit (unit, libunit)) {
										result = 1;
									} else {
										Error ("cannot add lib to unit");
										result = 0;
									}
								}
							}
						}
						if (result) {
							Assert (is_token (lib, Token (punctuator), ";"));
							lib = next_const_token (lib, 0);
						}
					} while (result && lib[-1]);
				}
				if (result && unit->manifest.sources) {
					const char	*source;

					source = unit->manifest.sources;
					do {
						if (source[0] == '@') {
							Error ("global sources are not allowed in manifest");
							result = 0;
						} else {
							char	path[256];

							if (make_path_from_relative (path, working_path, source)) {
								content = read_entire_file (path, &size);
								if (content) {
									unit->filepath = g_tokenizer.current;
									push_string_token (&g_tokenizer, 0, path, strlen (path), 0);
									unit->filepath = get_next_from_tokenizer (&g_tokenizer, (char *) unit->filepath);
									if (parse_source (unit, content, size, source)) {
										result = 1;
									} else {
										Error ("cannot parse source file '%s'", source);
										result = 0;
									}
									free (content);
									content = 0;
								} else {
									Error ("cannot open source file '%s'", source);
									result = 0;
								}
							} else {
								Error ("cannot get path of source file '%s'", source);
								result = 0;
							}
						}
					} while (result && source[-1]);
				}
				if (result) {
					if (initialize_lookup_tables (unit)) {
						if (link_unit (unit)) {
							// Debug ("size start");
							if (size_unit (unit)) {
								// Debug ("size end");
								if (!unit->flags[Flag (lib)]) {
									path[0] = 0;
									strcat (path, working_path);
									strcat (path, "build");
									if (!PathFileExistsA (path)) {
										CreateDirectoryA (path, 0);
									}
									strcat (path, "\\");
									strcat (path, entry_filename);
									strcat (path, ".c");
									if (translate_unit_to_c (unit, path)) {
										if (compile_unit_c (unit, path, include_path)) {
											if (g_is_test) {
												char	*ptr;
												char	refpath[256];

												path[0] = 0;
												strcat (path, working_path);
												strcat (path, "build\\");
												strcat (path, entry_filename);
												ptr = strrchr (path, '\\');
												Assert (ptr);
												ptr = strchr (ptr, '.');
												strcpy (ptr, ".exe");
												refpath[0] = 0;
												strcat (refpath, working_path);
												strcat (refpath, entry_filename);
												ptr = strrchr (refpath, '\\');
												Assert (ptr);
												ptr = strchr (ptr, '.');
												strcpy (ptr, ".refout");
												if (run_test (unit, path, refpath)) {
													result = 1;
												} else {
													result = 0;
												}
											} else {
												result = 1;
											}
										} else {
											result = 0;
										}
									} else {
										Error ("cannot translate to c");
										result = 0;
									}
								} else {
									result = 1;
								}
							} else {
								Error ("cannot size");
								result = 0;
							}
						} else {
							Error ("cannot link");
							result = 0;
						}
					} else {
						Error ("cannot initialize lookup tables");
						result = 0;
					}
				}
			} else {
				free (content);
				content = 0;
				Error ("cannot parse entry source");
				result = 0;
			}
		} else {
			Error ("cannot open file of unit entry '%s' '%s'", entry_filename, path);
			result = 0;
		}
	}
	Pop_Array (g_unit_stack);
	return (result);
}

int		run_compiler (const char *entry_filename, const char *include_path, const char *working_path, int flags[Flag (_count)]) {
	int			result;
	char		head_path[256];
	struct unit	unit = {0};

	init_unit (&unit);
	memcpy (unit.flags, flags, sizeof unit.flags);
	unit.flags[Flag (lib)] = 0;
	unit.flags[Flag (unit_index)] = 0;
	g_unit_index = 1;
	g_unit = &unit;
	result = build_unit (&unit, entry_filename, include_path, working_path);
	g_unit = 0;
	return (result);
}

