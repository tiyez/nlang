




int		link_unit_scope (struct unit *unit, int scope_index) {
	int		result;

	if (check_scope_declarations_for_name_uniqueness (unit, scope_index)) {

	} else {
		result = 0;
	}
}

int		link_unit (struct unit *unit) {
	int		result;

	result = link_unit_scope (unit, unit->root_scope);
	return (result);
}








