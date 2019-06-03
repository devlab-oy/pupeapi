#include <stdio.h>
#include <ctype.h>
#include <string.h>
int color=7;
static void setcolor(int c)
{
    if(color != c)
    {
        if((color&8) && !(c&8)){printf("\33[m");color=c^7;}
        else if((c&8) && (color<8))printf("\33[1m");
        printf("\33[3%cm", "04261537"[c&7]);
        color = c;
    }
}
static int kelpo(int c) { return isalnum(c) || c=='_'; }
int main(void)
{
    char Buf[2048];
    unsigned a;
    int rivialku=1;
    for(a=0;;)
    {
        int ch = getchar();
    next:
        if(ch == EOF)break;
        if(rivialku)
        {
            if(ch=='(' || ch==':')
            {
                const char *p;
                char *n;
                Buf[a] = 0;
                setcolor(5);
                
                for(;;)
                {
                    const char *q = strstr(Buf, "/../");
                    if(!q)break;
                    for(p=q+3, n=q; n>Buf; )
                        if(*--n=='/')break;
                    while(*p)*n++ = *p++;
                    *n = 0;
                }
                
                p = Buf;                
                if(!strncmp(Buf, "/usr/include", 4+8))p = Buf+4+8;
                else if(!strncmp(Buf, "/usr/local/include", 4+6+8))p = Buf+4+6+8;
                if(p != Buf)
                {
                    strcpy(Buf, "/inc");
                    for(n=Buf+4; *p; *n++=*p++);
                    *n=0;
                }
                printf("%s", Buf);
                
                rivialku = 0;
            }
            else
            {
                Buf[a++] = ch;
                continue;
            }
        }
        if(ch == '\n')
        {
            a = 0;
            rivialku = 1;
            setcolor(7);
            putchar(ch);
            continue;
        }
        if(strchr("<>,",ch))
        {
            setcolor(2);
            putchar(ch);
            continue;
        }
        if(strchr("()",ch))
        {
            setcolor(10);
            putchar(ch);
            continue;
        }
        if(isdigit(ch))
        {
            setcolor(9);
            for(;;)
            {
                putchar(ch);
                ch = getchar();
                if(!isdigit(ch))goto next;
            }
        }
        if(!kelpo(ch))
        {
            setcolor(6);
            putchar(ch);
            continue;
        }
        for(a=0;;)
        {
            Buf[a++] = ch;
            ch = getchar();
            if(!kelpo(ch))break;
        }
        Buf[a] = 0;

        if(!strcmp(Buf, "basic_string"))
        {
            int n=0;
            setcolor(7);
            for(;;)
            {
                // printf("n=%d,ch=%c\n", n, ch);
                if(ch=='>')--n;
                else if(ch=='<')++n;
                if(ch==EOF || (!n && ch=='>'))break;
                ch=getchar();
            }
            strcpy(Buf, "string");
            ch=getchar();
        }
        
        if(!strcmp(Buf, "void") || !strcmp(Buf, "bool")
        || !strcmp(Buf, "char") || !strcmp(Buf, "int")
        || !strcmp(Buf, "long") || !strcmp(Buf, "double"))
            setcolor(15);
        else
            setcolor(7);
        printf("%s", Buf);
        fflush(stdout);
        goto next;
    }
    setcolor(7);
    return 0;
}
