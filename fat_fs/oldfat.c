static bool 
szWildMatch8 (const char *pat, const char *str) 
{
    const char *s, *p;
    uint8_t star = 0;

loopStart:
    for (s = str, p = pat; *s; ++s, ++p) 
    {
        switch (*p) 
        {
        case '?':
            if (*s == '.') goto starCheck;
            break;
        case '*':
            star = 1;
            str = s, pat = p;
            if (!* (++pat)) return 1;
            goto loopStart;
        default:
            if (toupper (*s) != toupper (*p))
                goto starCheck;
            break;
        }
    }
    if (*p == '*') ++p;
    return (!*p);

starCheck:
    if (!star)
        return 0;
    str++;
    goto loopStart;
}
