

int		push_unit_path (struct unit *unit) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (unit);
		if (unit->flags[Flag (lib)]) {
			path->unit.index = Get_Bucket_Element_Index (g_libs, unit);
		} else {
			path->unit.index = 0;
		}
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_function_path (struct unit *unit, uint decl) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (function);
		path->function.decl = decl;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_macro_path (struct unit *unit, uint decl) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (macro);
		path->macro.decl = decl;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_tag_path (struct unit *unit, uint decl) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (tag);
		path->tag.decl = decl;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_decl_path (struct unit *unit, uint64 decl) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (decl);
		path->decl.index = decl;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_flow_path (struct unit *unit, uint scope, uint index) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (flow);
		path->flow.scope = scope;
		path->flow.index = index;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_type_path (struct unit *unit, uint head) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (type);
		path->type.head = head;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		push_expr_path (struct unit *unit, uint head) {
	int		result;

	if (Prepare_Array (unit->paths, 1)) {
		struct path	*path;

		path = Push_Array (unit->paths);
		path->kind = Path_Kind (expr);
		path->expr.head = head;
		result = 1;
	} else {
		result = 0;
	}
	return (result);
}

int		pop_path (struct unit *unit) {
	int		result;

	if (Get_Array_Count (unit->paths) > 0) {
		Pop_Array (unit->paths);
		result = 1;
	} else {
		result = 1;
	}
	return (result);
}

void	print_path (struct unit *unit, struct path *paths, FILE *file) {
	int		index;

	fprintf (file, "\n== path stack:\n");
	index = 0;
	while (index < Get_Array_Count (paths)) {
		struct path	*path;

		path = paths + index;
		if (path->kind == Path_Kind (unit)) {
			struct unit	*pathunit;

			if (path->unit.index) {
				pathunit = Get_Bucket_Element (g_libs, path->unit.index);
			} else {
				pathunit = g_unit;
			}
			fprintf (file, "-- unit: %s (%s)\n", pathunit->filename, pathunit->filepath);
		} else if (path->kind == Path_Kind (function)) {
			struct decl	*decl;

			decl = get_decl (unit, path->function.decl);
			fprintf (file, "-- function: %s ", decl->name);
			print_type (unit, decl->type, file);
			fprintf (file, "\n");
		} else if (path->kind == Path_Kind (macro)) {
			struct decl	*decl;

			decl = get_decl (unit, path->macro.decl);
			fprintf (file, "-- macro: %s\n", decl->name);
		} else if (path->kind == Path_Kind (tag)) {
			struct decl	*decl;

			decl = get_decl (unit, path->tag.decl);
			fprintf (file, "-- tag: %s %s\n", g_tagname[decl->tag.type], decl->name);
		} else if (path->kind == Path_Kind (decl)) {
			struct decl	*decl;

			if (is_lib_index (path->decl.index)) {
				decl = get_decl (get_lib (get_lib_index (path->decl.index)), unlib_index (path->decl.index));
			} else {
				decl = get_decl (unit, unlib_index (path->decl.index));
			}
			fprintf (file, "-- decl: %s ", decl->name);
			if (decl->type) {
				print_type (unit, decl->type, file);
			}
			fprintf (file, "\n");
		} else if (path->kind == Path_Kind (flow)) {
			struct flow	*flow;

			flow = get_flow (unit, path->flow.index);
			if (flow->type == FlowType (decl)) {
				fprintf (file, "-- flow: decl\n");
			} else if (flow->type == FlowType (expr)) {
				fprintf (file, "-- flow: expr\n");
			} else if (flow->type == FlowType (block)) {
				fprintf (file, "-- flow: block\n");
			} else if (flow->type == FlowType (if)) {
				fprintf (file, "-- flow: if\n");
			} else if (flow->type == FlowType (while)) {
				fprintf (file, "-- flow: while\n");
			} else if (flow->type == FlowType (dowhile)) {
				fprintf (file, "-- flow: dowhile\n");
			}
		} else if (path->kind == Path_Kind (type)) {
			fprintf (file, "-- type: ");
			print_type (unit, path->type.head, file);
			fprintf (file, "\n");
		} else if (path->kind == Path_Kind (expr)) {
			fprintf (file, "-- expr: ");
			print_expr (unit, path->expr.head, file);
			fprintf (file, "\n");
		}
		index += 1;
	}
	fprintf (file, "\n");
}





