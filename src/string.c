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
    char *s = dst0;
    
    while ((*dst0++ = *src0++))
        ;
    
    return s;
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
    char *s = s1;
    
    while (*s1)
        s1++;
    
    while ((*s1++ = *s2++))
        ;
    return s;
}

size_t strlen(const char *str)
{
    const char *start = str;
    
    while (*str)
        str++;
    return str - start;
}
