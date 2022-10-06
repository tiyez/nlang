

struct wrbuffer {
	char	**buffer;
};

void	init_wrbuffer (struct wrbuffer *wr) {
	wr->buffer = 0;
}

void	free_wrbuffer (struct wrbuffer *wr) {
	Free_Bucket (wr->buffer);
}

int		write_string_n (struct wrbuffer *wr, const char *string, usize len) {
	int		result;

	if (Prepare_Bucket_Continuous (wr->buffer, len)) {
		memcpy (Push_Bucket_N_Continuous (wr->buffer, len), string, len);
		result = 1;
	} else {
		Error ("cannot prepare wrbuffer for string %zu", len);
		result = 0;
	}
	return (result);
}

int		write_string (struct wrbuffer *wr, const char *string) {
	return (write_string_n (wr, string, strlen (string)));
}

int		write_format (struct wrbuffer *wr, const char *format, ...) {
	int		result;
	int		len;
	char	buffer[4 * 1024];
	va_list	valist;

	va_start (valist, format);
	len = vsnprintf (buffer, sizeof buffer, format, valist);
	va_end (valist);
	if (len > 0) {
		result = write_string_n (wr, buffer, len);
	} else {
		result = (len == 0);
	}
	return (result);
}

int		write_new_line (struct wrbuffer *wr) {
	return (write_string (wr, "\n"));
}

struct cbuffer {
	struct wrbuffer	cwr, *wr;
	int		indent_level;
	int		tab_size;
	int		is_flow_stack;
};

void	init_cbuffer (struct cbuffer *buffer) {
	buffer->wr = &buffer->cwr;
	init_wrbuffer (buffer->wr);
	buffer->indent_level = 0;
	buffer->tab_size = 4;
	buffer->is_flow_stack = 0;
}

void	free_cbuffer (struct cbuffer *buffer) {
	free_wrbuffer (buffer->wr);
	memset (buffer, 0, sizeof *buffer);
}

int		write_c_new_line (struct cbuffer *buffer) {
	return (write_format (buffer->wr, "\n%*.s", buffer->indent_level * buffer->tab_size, ""));
}

void	write_c_indent_up (struct cbuffer *buffer) {
	buffer->indent_level += 1;
}

void	write_c_indent_down (struct cbuffer *buffer) {
	buffer->indent_level -= 1;
	Assert (buffer->indent_level >= 0);
}



