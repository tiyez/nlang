

/*
	array format:
	(usize) capacity
	(usize) count
	(capacity bytes) content (with nullterm element)
*/

#define Get_Array_Count_(accessor) (((usize *) (accessor))[-1])
#define Get_Array_Capacity_(accessor) (((usize *) (accessor))[-2])
#define Get_Array_End_(accessor) ((accessor) + Get_Array_Count_ (accessor))
#define Get_Array_Last_(accessor) ((accessor) + Get_Array_Count_ (accessor) - 1)
#define Get_Array_Count(accessor) ((accessor) ? Get_Array_Count_ (accessor) : 0)
#define Get_Array_Capacity(accessor) ((accessor) ? Get_Array_Capacity_ (accessor) : 0)
#define Get_Array_End(accessor) ((accessor) ? Get_Array_End_ (accessor) : 0)
#define Get_Array_Last(accessor) ((accessor) && Get_Array_Count_(accessor) > 0 ? Get_Array_End_ (accessor) - 1 : 0)
#define Set_Array_Count(accessor, count) (Get_Array_Count_ (accessor) = (count))
#define Terminate_Array(accessor) memset (&(accessor)[Get_Array_Count_ (accessor)], 0, sizeof *(accessor))

#define Prepare_Array(accessor, tofit) \
	(((accessor) && (Get_Array_Count_ (accessor) + (tofit) + 1/* for nulltem */) * sizeof *(accessor) <= Get_Array_Capacity_ (accessor)) || \
	_expand_array ((void **) &(accessor)))

#define Free_Array(accessor) \
	((accessor) ? (free ((usize *) (accessor) - 2), 1) : 1)

#define Push_Array(accessor) \
	(memset (Get_Array_End_ (accessor), 0, sizeof *(accessor)), \
	Get_Array_Count_ (accessor) += 1, \
	Terminate_Array (accessor), \
	Get_Array_End_ (accessor) - 1)

#define Push_Array_N(accessor, count) \
	(memset (Get_Array_End_ (accessor), 0, sizeof *(accessor) * (count)), \
	Get_Array_Count_ (accessor) += (count), \
	Terminate_Array (accessor), \
	Get_Array_End_ (accessor) - (count))

#define Pop_Array(accessor) \
	(Get_Array_Count_ (accessor) -= 1, Terminate_Array (accessor))

#define Clear_Array(accessor) \
	((accessor) ? (Get_Array_Count_ (accessor) = 0) : 0)

#define Top_From_Array(accessor) (Get_Array_End_ (accessor) - 1)

#define Get_Element_Index(accessor, element) ((element) - (accessor))

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



