

/* Includes ------------------------------------------------------------------*/
#include "fifo.h"


#define minimum(x, y)  (((x) < (y))? (x) : (y))


/************************************************************************
    Function : 
    Description : Init the fifo structure.
            _fifo : reference to _fifo struct.
            size : size of the fifo.
            retval ERROR if memory allocation fails or SUCCESS.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
ErrorStatus fifo_Init(fifo_TypeDef * _fifo, u32 size)
{
    u8 *p = NULL;
    
    /* check for a valid pointer */
    if (_fifo == NULL || size == 0)
    {
        return ERROR;
    }
    
    p = (u8 *)malloc(size);

    if(p)
    {
        _fifo->baseAddr = p;
        /* allocate fifo space. */
        memset((u8 *)_fifo->baseAddr, 0x00, sizeof(u8) * size);
        /* tail = head --> empty fifo */
        _fifo->head = 0;
        _fifo->tail = 0;
        _fifo->size = size;
        _fifo->len = 0;
        return SUCCESS;
    }
    
    return ERROR;
}

/************************************************************************
    Function : 
    Description : Reset the fifo structure.
            _fifo : reference to _fifo struct.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
ErrorStatus fifo_Reset(fifo_TypeDef * _fifo)
{
    if (_fifo == NULL)
    {
        return ERROR;
    }
    else
    {
        if (_fifo->baseAddr != NULL)
        {
            /* optional (clear memory region) */
            memset((u8 *)_fifo->baseAddr, 0x00, _fifo->size);
            
            _fifo->head = 0;
            _fifo->tail = 0;
            _fifo->len = 0;
            return SUCCESS;
        }
        else
        {
            /* FiFo invalid base address. */
            return ERROR;
        }
    }
}

GM_ERRCODE fifo_delete(fifo_TypeDef * fifo)
{
    if (fifo == NULL)
    {
        return GM_PARAM_ERROR;
    }
    
	/* delete fifo space. */
	if (fifo->baseAddr == NULL)
	{
		return GM_SUCCESS;
	}
	
    free(fifo->baseAddr);
    fifo->baseAddr = 0;
	fifo->head = 0;
	fifo->tail = 0;
	fifo->size = 0;
	return GM_SUCCESS;
}

/************************************************************************
    Function : 
    Description : returns how much data is stored in the fifo.
            _fifo : reference to _fifo struct.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
u32 fifo_GetLen(fifo_TypeDef * _fifo)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;

    if (_fifo == NULL)
    {
        return ERROR;
    }
    
    read_point = _fifo->head;
    write_point = _fifo->tail;

    left_data_size = write_point - read_point;
    if(left_data_size < 0)
    {
        left_data_size += _fifo->size;
    }

    return left_data_size;
}


/************************************************************************
    Function : 
    Description : returns how much free sapce in the fifo.
            _fifo : reference to _fifo struct.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
u32 fifo_GetFreeSpace(fifo_TypeDef * _fifo)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;

    if (_fifo == NULL)
    {
        return 0;
    }
    
    read_point = _fifo->head;
    write_point = _fifo->tail;

    left_data_size = write_point - read_point;
    if(left_data_size < 0)
    {
        left_data_size += _fifo->size;
    }

    return (_fifo->size - left_data_size - 1);
}


/************************************************************************
    Function : 
    Description : Insert data to the fifo.
            _fifo : reference to _fifo struct.
            data : reference to data buffer.
            len : data buffer length.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
u32 fifo_insert(fifo_TypeDef * _fifo, u8 *data, u32 len)
{
    int read_point;
    int write_point;
    int left_size;
    int to_end;

    if(len > 1500)
    {
        return 0;
    }

    read_point = _fifo->head;
    write_point = _fifo->tail;

    left_size = read_point - write_point;
    if(left_size <= 0)
    {
        left_size += _fifo->size;
    }

    if((u32)left_size <= (len))
    {
        return 0;
    }

    to_end = _fifo->size - write_point;
    if((u32)to_end >= len)
    {
        memcpy(_fifo->baseAddr + write_point,data,len);
        write_point += len;
        write_point %= _fifo->size;
    }
    else
    {
        memcpy(_fifo->baseAddr + write_point,data,to_end);
        memcpy(_fifo->baseAddr,data + to_end,len -to_end);
        write_point = len -to_end;
    }

    _fifo->tail = write_point;
    
    return len;
}


/************************************************************************
    Function : 
    Description : Retrieve one msg from the fifo.
            fifo : reference to fifo struct.
            data : reference to msg data buffer.
            len : data buffer length.
    Change history:
    Note:
    Author:     
************************************************************************/
ErrorStatus fifo_peek(fifo_TypeDef * fifo, u8 *data, u32 len)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;
    int to_end = 0;

    read_point = fifo->head;
    write_point = fifo->tail;

    left_data_size = write_point - read_point;
    if(left_data_size < 0)
    {
        left_data_size += fifo->size;
    }

    if((u32)left_data_size < len)
    {
        return ERROR;
    }

    to_end = fifo->size - read_point;
    if((u32)to_end >= len)
    {
        memcpy(data,fifo->baseAddr + read_point,len);
        read_point += len;
        if(read_point >=fifo->size)
        {
            read_point = 0;
        }
    }
    else
    {
        memcpy(data,fifo->baseAddr + read_point,to_end);
        memcpy(data+to_end,fifo->baseAddr,len -to_end);
        read_point = len - to_end;
    }

    return SUCCESS;
}


/************************************************************************
    Function : 
    Description : Retrieve one msg from the fifo.
            fifo : reference to fifo struct.
            data : reference to msg data buffer.
            len : data buffer length.  it is input and output varible
    Change history:
    Note:
    Author:     
************************************************************************/
ErrorStatus fifo_peek_and_get_len(fifo_TypeDef * fifo, u8 *data, u32 *len_p)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;
    int to_end = 0;

    read_point = fifo->head;
    write_point = fifo->tail;

    left_data_size = write_point - read_point;
    if(left_data_size < 0)
    {
        left_data_size += fifo->size;
    }

    if((u32)left_data_size < (*len_p))
    {
        (*len_p) = (u32)left_data_size;
    }

    to_end = fifo->size - read_point;
    if((u32)to_end >= (*len_p))
    {
        memcpy(data,fifo->baseAddr + read_point,(*len_p));
        read_point += (*len_p);
        if(read_point >=fifo->size)
        {
            read_point = 0;
        }
    }
    else
    {
        memcpy(data,fifo->baseAddr + read_point,to_end);
        memcpy(data+to_end,fifo->baseAddr,(*len_p) -to_end);
        read_point = (*len_p) - to_end;
    }

    return SUCCESS;
}



ErrorStatus fifo_pop_len(fifo_TypeDef * fifo, u32 len)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;

    read_point = fifo->head;
    write_point = fifo->tail;
	left_data_size = write_point - read_point;
	
    if(left_data_size < 0)
    {
        left_data_size += fifo->size;
    }

    if((u32)left_data_size < len)
    {
        return ERROR;
    }
    read_point += len;
    if(read_point >= fifo->size)
    {
        read_point -= fifo->size;
    }

    fifo->head = read_point;
    fifo->len -= len;

    return SUCCESS;
}


/************************************************************************
    Function : 
    Description : returns SET if the FIFO is full.
            _fifo : reference to _fifo struct.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
FlagStatus fifo_NotFull(fifo_TypeDef * _fifo)
{
    if (_fifo == NULL)
    {
        return RESET;
    }
    else
    {
        return (_fifo->len != _fifo->size) ? SET : RESET;
    }
}



/************************************************************************
    Function : 
    Description : Retrieve data from the fifo.
            _fifo : reference to _fifo struct.
            data : reference to data buffer.
            len : data buffer length.
    Change history:
    Note:
    Author:     
    Date:     Jan-10-2013
************************************************************************/
u32 fifo_retrieve(fifo_TypeDef * _fifo, u8 *data, u32 len)
{
    int read_point = 0;
    int write_point = 0;
    int left_data_size = 0;
    int to_end = 0;

    read_point = _fifo->head;
    write_point = _fifo->tail;

    left_data_size = write_point - read_point;
    if(left_data_size < 0)
    {
        left_data_size += _fifo->size;
    }

    if((u32)left_data_size < len)
    {
        return ERROR;
    }

    to_end = _fifo->size - read_point;
    if((u32)to_end >= len)
    {
        memcpy(data,_fifo->baseAddr + read_point,len);
    }
    else
    {
        memcpy(data,_fifo->baseAddr + read_point,to_end);
        memcpy(data+to_end,_fifo->baseAddr,len -to_end);
        //read_point = len - to_end;
    }

    read_point += len;
    if(read_point >=_fifo->size)
    {
        read_point -= _fifo->size;
    }

    return SUCCESS;
}


ErrorStatus fifo_peek_until(fifo_TypeDef* fifo, u8* data, u16* len_p, const u8 until_char)
{
	u32 read_point = fifo->head;
    u32 write_point = fifo->tail;
	s32 data_size = write_point - read_point;
	u32 index = 0;
	bool has_until_char = false;

	if (NULL == fifo || NULL == data || NULL == len_p)
	{
		return ERROR;
	}
	
    if(data_size < 0)
    {
        data_size += fifo->size;
    }

	for (index = 0; index < data_size && (fifo->baseAddr[read_point] != until_char) && index < *len_p - 1; index++)
	{
		data[index] = fifo->baseAddr[read_point];
		read_point++;
		read_point %= fifo->size;
	}


	for (; index < data_size && index < *len_p - 1 && ((fifo->baseAddr[read_point] == until_char) || (fifo->baseAddr[read_point] == '\r')); index++)
	{
		data[index] = fifo->baseAddr[read_point];
		read_point++;
		read_point %= fifo->size;
		has_until_char = true;
	}

	*len_p = index;
	data[index] = '\0';
	if (has_until_char)
	{
    	return SUCCESS;
	}
	else
	{
		return ERROR;
	}	    
}


