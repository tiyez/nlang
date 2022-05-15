

struct wrbuffer {
	char	*buffer;
};

void	init_wrbuffer (struct wrbuffer *wr) {
	wr->buffer = 0;
}

void	free_wrbuffer (struct wrbuffer *wr) {
	Free_Array (wr->buffer);
}

int		write_string_n (struct wrbuffer *wr, const char *string, usize len) {
	int		result;

	if (Prepare_Array (wr->buffer, len)) {
		memcpy (Push_Array_N (wr->buffer, len), string, len);
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

struct cbuffer {
	struct wrbuffer	cwr, *wr;
};

void	init_cbuffer (struct cbuffer *buffer) {
	buffer->wr = &buffer->cwr;
	init_wrbuffer (buffer->wr);
}





