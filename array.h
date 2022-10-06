

/*
	array format:
	(usize) capacity
	(usize) count
	(capacity bytes) content (with nullterm element)
*/

#define Get_Array_Count_(accessor) (((usize *) (accessor))[-1])
#define Get_Array_Capacity_(accessor) (((usize *) (accessor))[-2])
#define Get_Array_End_(accessor) ((accessor) + Get_Array_Count_ (accessor))
#define Get_Array_End_Sized_(accessor, elsize) ((void *) ((unsigned char *) (accessor) + (elsize) * Get_Array_Count_ (accessor)))
#define Get_Array_Last_(accessor) ((accessor) + Get_Array_Count_ (accessor) - 1)
#define Get_Array_Last_Sized_(accessor, elsize) ((void *) ((unsigned char *) (accessor) + (elsize) * (Get_Array_Count_ (accessor) - 1)))
#define Get_Array_Count(accessor) ((accessor) ? Get_Array_Count_ (accessor) : 0)
#define Get_Array_Capacity(accessor) ((accessor) ? Get_Array_Capacity_ (accessor) : 0)
#define Get_Array_End(accessor) ((accessor) ? Get_Array_End_ (accessor) : 0)
#define Get_Array_End_Sized(accessor, elsize) ((accessor) ? Get_Array_End_Sized_ (accessor, elsize) : 0)
#define Get_Array_Last(accessor) ((accessor) && Get_Array_Count_(accessor) > 0 ? Get_Array_Last_ (accessor) : 0)
#define Get_Array_Last_Sized(accessor, elsize) ((accessor) && Get_Array_Count_(accessor) > 0 ? Get_Array_Last_Sized_ (accessor, elsize) : 0)
#define Set_Array_Count(accessor, count) (Get_Array_Count_ (accessor) = (count))
#define Terminate_Array_Sized(accessor, elsize) memset ((unsigned char *) (accessor) + (elsize) * Get_Array_Count_ (accessor), 0, elsize)
#define Terminate_Array(accessor) Terminate_Array_Sized (accessor, sizeof *(accessor))
#define Is_Fit_To_Array_Sized(accessor, elsize, tofit) ((Get_Array_Count_ (accessor) + (tofit) + 1/* for nulltem */) * (elsize) <= Get_Array_Capacity_ (accessor))
#define Is_Fit_To_Array(accessor, tofit) Is_Fit_To_Array_Sized (accessor, sizeof *(accessor), tofit)

#define Prepare_Array_Sized(accessor, elsize, tofit) \
	(((accessor) && Is_Fit_To_Array_Sized (accessor, elsize, tofit)) || _expand_array ((void **) &(accessor)))

#define Prepare_Array(accessor, tofit) Prepare_Array_Sized (accessor, sizeof *(accessor), tofit)

#define Free_Array(accessor) \
	((accessor) ? (free ((usize *) (accessor) - 2), (accessor) = 0, 1) : 1)

#define Push_Array_Sized(accessor, elsize) \
	(memset (Get_Array_End_Sized_ (accessor, elsize), 0, elsize), \
	Get_Array_Count_ (accessor) += 1, \
	Terminate_Array_Sized (accessor, elsize), \
	Get_Array_Last_Sized_ (accessor, elsize))

#define Push_Array(accessor) \
	(memset (Get_Array_End_ (accessor), 0, sizeof *(accessor)), \
	Get_Array_Count_ (accessor) += 1, \
	Terminate_Array (accessor), \
	Get_Array_Last_ (accessor))

#define Push_Array_N_Sized(accessor, elsize, count) \
	(memset (Get_Array_End_Sized_ (accessor, elsize), 0, (elsize) * (count)), \
	Get_Array_Count_ (accessor) += (count), \
	Terminate_Array_Sized (accessor, elsize), \
	(void *) ((unsigned char *) Get_Array_End_Sized_ (accessor, elsize) - (elsize) * (count)))

#define Push_Array_N(accessor, count) \
	(memset (Get_Array_End_ (accessor), 0, sizeof *(accessor) * (count)), \
	Get_Array_Count_ (accessor) += (count), \
	Terminate_Array (accessor), \
	Get_Array_End_ (accessor) - (count))

#define Pop_Array(accessor) \
	(Get_Array_Count_ (accessor) -= 1, Terminate_Array (accessor))

#define Clear_Array(accessor) \
	((accessor) ? (Get_Array_Count_ (accessor) = 0, Terminate_Array (accessor)) : 0)

#define Clear_Array_Sized(accessor, elsize) \
	((accessor) ? (Get_Array_Count_ (accessor) = 0, Terminate_Array_Sized (accessor, elsize)) : 0)

#define Top_From_Array(accessor) (Get_Array_End_ (accessor) - 1)

#define Get_Element_Index(accessor, element) ((element) - (accessor))
#define Get_Element_Index_Sized(accessor, elsize, element) (((unsigned char *) (element) - (unsigned char *) (accessor)) / (elsize))

#define Is_Array_Element_Sized(accessor, elsize, elem) ((isize) elem >= (isize) (accessor) && (((usize) elem - (usize) (accessor)) % (elsize) == 0) && ((usize) elem - (usize) (accessor)) / (elsize) < Get_Array_Count_ (accessor))

static inline int	_expand_array(void **pptr) {
	int		success;
	void	*memory;
	usize	cap = *pptr ? Get_Array_Capacity_ (*pptr) + sizeof (usize) * 2 : 0;
	usize	count = *pptr ? Get_Array_Count_ (*pptr) : 0;

	memory = expand_array (*pptr ? (usize *) *pptr - 2 : 0, &cap);
	if (memory) {
		*pptr = (usize *) memory + 2;
		Get_Array_Capacity_ (*pptr) = cap - sizeof (usize) * 2;
		Get_Array_Count_ (*pptr) = count;
		success = 1;
	} else {
		Error ("cannot prepare an array");
		success = 0;
	}
	return (success);
}


// buffer

#define Init_Buffer(accessor) ((accessor) = 0)

#define Append_Buffer(accessor, data, count) \
	(Prepare_Array (accessor, count) && \
		(memcpy (Push_Array_N (accessor, count), data, (count) * sizeof *(accessor)), 1) \
	)

#define Insert_Buffer(accessor, offset, data, count) \
	(Prepare_Array (accessor, count) && \
		Push_Array_N (accessor, count) && \
		(memmove ((accessor) + ((offset) + (count)), \
			(accessor) + (offset), \
			(Get_Array_Count_ (accessor) - (offset) - (count) + 1) * sizeof *(accessor)), 1) && \
		(memmove ((accessor) + (offset), data, (count) * sizeof *(accessor))) \
	)

#define Erase_Buffer(accessor, offset, count) \
	((memmove ((accessor) + (offset), (accessor) + (offset) + (count), (Get_Array_Count_ (accessor) - (offset) - (count) + 1) * sizeof *(accessor)), 1) && \
	(Get_Array_Count_ (accessor) -= (count), 1))



int		_expand_array(void **pptr);












/*
	Bucket it's a array of pointers to fixed array;
*/

static inline int		_prepare_bucket (void ***pptr, usize elem_size, usize tofit, int is_continuous) {
	int		result;
	void	**ptr = *pptr;

	if (ptr) {
		void	**inptr;

		if (is_continuous) {
			inptr = Get_Array_Last_ (ptr);
			Assert (*inptr);
			if (!Is_Fit_To_Array_Sized (*inptr, elem_size, tofit)) {
				inptr += 1;
				Assert (!*inptr);
			}
		} else {
			inptr = ptr;
			while (*inptr && !Is_Fit_To_Array_Sized (*inptr, elem_size, tofit)) {
				inptr += 1;
			}
		}
		if (!*inptr) {
			if (Prepare_Array (ptr, 1)) {
				inptr = Push_Array (ptr);
				if (Prepare_Array_Sized (*inptr, elem_size, tofit)) {
					result = 1;
				} else {
					result = 0;
				}
			} else {
				result = 0;
			}
		} else {
			result = 1;
		}
	} else {
		if (Prepare_Array (ptr, 1)) {
			void	**inptr;

			inptr = Push_Array (ptr);
			if (Prepare_Array_Sized (*inptr, elem_size, tofit)) {
				result = 1;
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	}
	*pptr = ptr;
	return (result);
}

#define Prepare_Bucket(acc, tofit) (_prepare_bucket ((void ***) &(acc), sizeof (**(acc)), tofit, 0))
#define Prepare_Bucket_Continuous(acc, tofit) (_prepare_bucket ((void ***) &(acc), sizeof (**(acc)), tofit, 1))

static inline void	*_push_bucket_n (void **ptr, usize elem_size, usize num, int is_continuous) {
	void	*result;
	void	**inptr;

	Assert (ptr);
	if (is_continuous) {
		inptr = Get_Array_Last_ (ptr);
		Assert (Is_Fit_To_Array_Sized (*inptr, elem_size, num));
	} else {
		inptr = ptr;
		while (*inptr && !Is_Fit_To_Array_Sized (*inptr, elem_size, num)) {
			inptr += 1;
		}
	}
	Assert (*inptr);
	result = Push_Array_N_Sized (*inptr, elem_size, num);
	return (result);
}

#define Push_Bucket(acc) (_push_bucket_n ((void **) (acc), sizeof (**(acc)), 1, 0))
#define Push_Bucket_N(acc, num) (_push_bucket_n ((void **) (acc), sizeof (**(acc)), num, 0))
#define Push_Bucket_N_Continuous(acc, num) (_push_bucket_n ((void **) (acc), sizeof (**(acc)), num, 1))

static inline uint	_get_bucket_elem_index (void **ptr, usize elem_size, void *elem) {
	void	**inptr;
	int		result;

	Assert (ptr);
	inptr = ptr;
	while (*inptr && !Is_Array_Element_Sized (*inptr, elem_size, elem)) {
		inptr += 1;
	}
	Assert (*inptr);
	result = (Get_Element_Index (ptr, inptr) << 16) + Get_Element_Index_Sized (*inptr, elem_size, elem) + 1;
	return (result);
}

#define Get_Bucket_Element_Index(acc, elem) (_get_bucket_elem_index ((void **) (acc), sizeof (**(acc)), (void *) elem))

#define Get_Bucket_Element(acc, index) ((acc)[((index) - 1) >> 16] + (((index) - 1) & 0xFFFFu))

static inline int	_is_bucket_index_valid (void **ptr, uint index) {
	int		result;

	if (ptr && index) {
		uint	array_index;

		index -= 1;
		array_index = index >> 16;
		if (array_index >= 0 && array_index < Get_Array_Count_ (ptr)) {
			void	**inptr;
			uint	elem_index;

			inptr = ptr + array_index;
			elem_index = index & 0xFFFFu;
			if (*inptr && elem_index >= 0 && elem_index < Get_Array_Count_ (*inptr)) {
				result = 1;
			} else {
				result = 0;
			}
		} else {
			result = 0;
		}
	} else {
		result = 0;
	}
	return (result);
}

#define Is_Bucket_Index_Valid(acc, index) (_is_bucket_index_valid ((void **) (acc), index))

static inline uint	_get_next_bucket_index (void **ptr, uint index) {
	uint	result;
	uint	array_index, elem_index;

	Assert (index);
	index -= 1;
	array_index = index >> 16;
	elem_index = index & 0xFFFFu;
	Assert (ptr);
	Assert (array_index >= 0 && array_index < Get_Array_Count_ (ptr));
	Assert (ptr[array_index]);
	Assert (elem_index < Get_Array_Count_ (ptr[array_index]));
	if (elem_index + 1 < Get_Array_Count_ (ptr[array_index])) {
		result = (array_index << 16) + elem_index + 1 + 1;
	} else {
		array_index += 1;
		while (array_index < Get_Array_Count_ (ptr) && !(ptr[array_index] && Get_Array_Count_ (ptr[array_index]) > 0)) {
			array_index += 1;
		}
		if (array_index < Get_Array_Count_ (ptr)) {
			result = (array_index << 16) + 1;
		} else {
			result = 0;
		}
	}
	return (result);
}

#define Get_Next_Bucket_Index(acc, index) (_get_next_bucket_index ((void **) (acc), index))

static inline usize _get_bucket_count (void **ptr) {
	usize	count;

	count = 0;
	if (ptr) {
		void	**inptr;

		inptr = ptr;
		while (*inptr) {
			count += Get_Array_Count (*inptr);
			inptr += 1;
		}
	}
	return (count);
}

#define Get_Bucket_Count(acc) (_get_bucket_count ((void **) (acc)))

static inline uint	_get_bucket_first_index (void **ptr) {
	uint	index;

	if (ptr) {
		void	**inptr;

		inptr = ptr;
		while (*inptr && Get_Array_Count_ (*inptr) <= 0) {
			inptr += 1;
		}
		if (*inptr) {
			index = (Get_Element_Index (ptr, inptr) << 16) + 1;
		} else {
			index = 0;
		}
	} else {
		index = 0;
	}
	return (index);
}

#define Get_Bucket_First_Index(acc) (_get_bucket_first_index ((void **) (acc)))

static inline int	_get_bucket_ordindex (void **ptr, uint index) {
	int		ordindex;

	if (ptr && index) {
		uint	array_index, elem_index;
		int		arrindex;

		index -= 1;
		array_index = index >> 16;
		elem_index = index & 0xFFFFu;
		Assert (array_index < Get_Array_Count (ptr));
		Assert (elem_index < Get_Array_Count (ptr[array_index]));
		ordindex = 0;
		arrindex = 0;
		while (arrindex < array_index) {
			ordindex += Get_Array_Count (ptr[arrindex]);
			arrindex += 1;
		}
		ordindex += elem_index;
	} else {
		ordindex = -1;
	}
	return (ordindex);
}

#define Get_Bucket_OrdIndex(acc, index) (_get_bucket_ordindex ((void **) (acc), index))

static inline void	_free_bucket (void ***pptr) {
	void	**ptr = *pptr;

	if (ptr) {
		void	**inptr;

		inptr = ptr;
		while (*inptr) {
			Free_Array (*inptr);
			inptr += 1;
		}
		Free_Array (ptr);
		*pptr = 0;
	}
}

#define Free_Bucket(acc) (_free_bucket ((void ***) &(acc)))

static inline void	_clear_bucket (void **ptr, usize elem_size) {
	if (ptr) {
		void	**inptr;

		inptr = ptr;
		while (*inptr) {
			Clear_Array_Sized (*inptr, elem_size);
			inptr += 1;
		}
	}
}

#define Clear_Bucket(acc) (_clear_bucket ((void **) (acc), sizeof **(acc)))


