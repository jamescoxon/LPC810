//Functions taken from minilib-c https://code.google.com/p/minilib-c/

typedef unsigned long size_t;
#define NULL ((void*)0)

#define ALIGNED(X) \
(((long)X & (sizeof (long) - 1)) == 0)

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
(((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)

char *strcpy(char *dst0, const char *src0)
{
#if defined(PREFER_SIZE_OVER_SPEED)
    char *s = dst0;
    
    while ((*dst0++ = *src0++))
        ;
    
    return s;
#else
    char *dst = dst0;
    const char *src = src0;
    long *aligned_dst;
    const long *aligned_src;
    
    /* If SRC or DEST is unaligned, then copy bytes.  */
    if (!UNALIGNED (src, dst))
    {
        aligned_dst = (long*)dst;
        aligned_src = (long*)src;
        
        /* SRC and DEST are both "long int" aligned, try to do "long int"
         sized copies.  */
        while (!DETECTNULL(*aligned_src))
        {
            *aligned_dst++ = *aligned_src++;
        }
        
        dst = (char*)aligned_dst;
        src = (char*)aligned_src;
    }
    
    while ((*dst++ = *src++))
        ;
    return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

char *strstr(const char *searchee, const char *lookfor)
{
    /* Less code size, but quadratic performance in the worst case.  */
    if (*searchee == 0)
    {
        if (*lookfor)
            return (char *) NULL;
        return (char *) searchee;
    }
    
    while (*searchee)
    {
        size_t i;
        i = 0;
        
        while (1)
        {
            if (lookfor[i] == 0)
            {
                return (char *) searchee;
            }
            
            if (lookfor[i] != searchee[i])
            {
                break;
            }
            i++;
        }
        searchee++;
    }
    
    return (char *) NULL;
}

char *strcat(char *s1, const char *s2)
{
#if defined(PREFER_SIZE_OVER_SPEED)
    char *s = s1;
    
    while (*s1)
        s1++;
    
    while ((*s1++ = *s2++))
        ;
    return s;
#else
    char *s = s1;
    
    
    /* Skip over the data in s1 as quickly as possible.  */
    if (ALIGNED (s1))
    {
        unsigned long *aligned_s1 = (unsigned long *)s1;
        while (!DETECTNULL (*aligned_s1))
            aligned_s1++;
        
        s1 = (char *)aligned_s1;
    }
    
    while (*s1)
        s1++;
    
    /* s1 now points to the its trailing null character, we can
     just use strcpy to do the work for us now.
     
     ?!? We might want to just include strcpy here.
     Also, this will cause many more unaligned string copies because
     s1 is much less likely to be aligned.  I don't know if its worth
     tweaking strcpy to handle this better.  */
    strcpy (s1, s2);
    
    return s;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

size_t strlen(const char *str)
{
    const char *start = str;
    
    while (*str)
        str++;
    return str - start;
}
