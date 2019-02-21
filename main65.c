#ifdef __CC65__
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OPORT *(char*)(0xC000)
#define UDATA *(char*)(0xC010)
#define USTAT *(char*)(0xC011)

#define IOINIT {}
#define MAXMEM _heapmemavail()
//#define NULL (0)
#define PACKED

#define RAWMEM(A) mem0[A]

static char * mem0 = NULL;

void putchar(char c) {
    while ((USTAT & 0x02) == 0);
    UDATA = c;
}

char kbhit() {
    return ((USTAT & 0x01) != 0);
}

char getkb() {
    while (kbhit() == 0);
    return UDATA;
}

#else
#include <stdio.h>
#include <stdlib.h>
//char* itoa(int n, char *s, int b) { sprintf(s,"%d",n); return s; }
#define itoa(N,S,B) (sprintf(((char *)(S)),"%d",(N)), (char *)(S))
#define MAXMEM (4000)
#define IOINIT (set_conio_terminal_mode())
#define RAWMEM(A) mem[A]
#define PACKED __attribute__((__packed__))

char OPORT;

#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>

struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
 
    //new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_termios.c_lflag |= (ISIG);
 
    new_termios.c_cc[VMIN] = 0;
    //new_termios.c_cc[VTIME] = 0;

 
 
    tcsetattr(0, TCSANOW, &new_termios);
}

char kbhit() {
    int c;
    fseek (stdin, 0, SEEK_SET);
    if ((c=getc(stdin)) != -1) {
        ungetc(c,stdin);
        return 1;
    }
    else
        return 0;
}

char getkb() {
    while (!kbhit()) {}
    return getc(stdin);
}

//{
//    struct timeval tv = { 0L, 0L };
//    fd_set fds;
//    FD_ZERO(&fds);
//    FD_SET(0, &fds);
//    return select(1, &fds, NULL, NULL, &tv);
//}

#endif


#include <stdint.h>


//#define uchar uint8_t

#define MAXLINE 79
#define MAXFOR 5
#define MAXSUB 10

#define BS  (8)
#define DEL (127)
#define CR  (13)
#define LF  (10)
#define ETX (3)
#define CAN (24)

#define MINTOKEN (128)
#define MAXTOKEN (MINTOKEN + sizeof(cmdarr) / sizeof(cmdfun))

enum errcodes {ERR_NONE = 0, ERR_STOP, ERR_SYN, ERR_LIN, ERR_MOD, ERR_ST, ERR_NEXT, ERR_RET};
char *errtxt[] = {"OK","STOP","SN","LN","MD","ST","NF","RG"};

enum modes {MODE_DIRECT = 0, MODE_RUN};

enum toktypes {TOK_CMD, TOK_FUN};

typedef int (*cmdfun)(void);

typedef struct PACKED {
    uint8_t len;
    uint16_t num;
} lnhead;

typedef struct PACKED {
    void * exptr;
    int line;
    uint8_t var;
    int step;
    int limit;
} forentry;

typedef struct PACKED {
    void * exptr;
    int line;
} subentry;


int cmd_let(void);
int cmd_err(void);
int cmd_rem(void);
int cmd_print(void);
int cmd_poke(void);
int cmd_list(void);
int cmd_new(void);
int cmd_run(void);
int cmd_goto(void);
int cmd_gosub(void);
int cmd_return(void);
int cmd_stop(void);
int cmd_if(void);
int cmd_for(void);
int cmd_next(void);
int cmd_peek(void);


static uint8_t line[MAXLINE+1];
static uint8_t buf[MAXLINE+1];

static uint8_t isgoto;
static int curln;
static int curmd;
static uint8_t err;
static uint8_t *exptr;
static uint8_t forsp;
static uint8_t subsp;

static unsigned int mtot;
static unsigned int basb;
static unsigned int base;
static unsigned int meme;
static char *mem;

static int basvars['Z'-'A'+1];
static forentry forstack[MAXFOR+1];
static subentry substack[MAXSUB+1];

#define TOK_LET     (0 + MINTOKEN)
#define TOK_TO      (1 + MINTOKEN)
#define TOK_STEP    (2 + MINTOKEN)
#define TOK_REM     (3 + MINTOKEN)

uint8_t tokentab[] = 
    "leT"
    "tO"
    "steP"
    "reM"
    "prinT"
    "pokE"
    "lisT"
    "neW"
    "ruN"
    "gotO"
    "gosuB"
    "returN"
    "stoP"
    "iF"
    "foR"
    "nexT"
    "peeK";
    
cmdfun cmdarr[] = {
    cmd_let, 
    cmd_err, 
    cmd_err, 
    cmd_rem, 
    cmd_print, 
    cmd_poke, 
    cmd_list,
    cmd_new,
    cmd_run,
    cmd_goto,
    cmd_gosub,
    cmd_return,
    cmd_stop,
    cmd_if,
    cmd_for,
    cmd_next,
    cmd_peek 
};
                   
enum toktypes toktyp[] = {
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_CMD, 
    TOK_FUN 
};

int expression(void);


// IO support

void putstr(char *p) {
    while (*p) putchar(*p++);
}


uint8_t readline(uint8_t *s) {
    int p = 0;
    char c;
 
    while(1) {

        c = getkb();

        if ((c >= ' ') && (c <= '~') && (p < MAXLINE-1)) {
            putchar(c);
            s[p++] = c;
        }
        else if ((c == CR) || (c == LF) || (c == ETX) || (c == CAN)) {
            s[p] = 0;
            return !((c == ETX) || (c == CAN));
        }
        else if (((c == BS) || (c == DEL)) && (p > 0)) {
            --p;
            putchar(BS);
        }
    }
}


// BASIC line handling routines

int fndln(int n) {
    int p = basb;
    
    while ( (((lnhead*)(mem+p))->len != 0) &&
            (((lnhead*)(mem+p))->num < n) ) {
        p += ((lnhead*)(mem+p))->len;
    }
    return p;
}


void delln(int n) {
    int s;
    int d;
    int m;
    
    d = fndln(n);
    if (((lnhead*)(mem+d))->num == n) {
        
        m = ((lnhead*)(mem+d))->len;        
        s = d + m;
        memmove(mem + d, mem + s, base - d + sizeof(lnhead) + 1);
        
        base -= m;
    }
}


void addln(int n, void * l) {
    int p;
    int s;
    int d;
    int m;
    
    p = fndln(n);
    
    m = strlen((char*)l) + 1 + sizeof(lnhead);
    s = p;
    d = p + m;
    memmove(mem + d, mem + s, base - p + sizeof(lnhead));

    ((lnhead*)(mem+p))->len = m;
    ((lnhead*)(mem+p))->num = n;
    p += sizeof(lnhead);
    
    strcpy(mem + p, l);

    base += m;
}


// BASIC support routines

uint8_t tokenize(uint8_t *b) {
    uint8_t *t;
    uint8_t *s;
    uint8_t *d;
    uint8_t *p;
    uint8_t tok;
    uint8_t quot;
    uint8_t rem;
 
    s = b;
    d = b;
    quot = 0;
    rem = 0;
  

    while (*s != 0) {
//        printf("*%c*",(char)(*s));
        t = tokentab;
        tok = MINTOKEN;
        p = s;
        if (*s == '"') quot = 1-quot;
        if (quot || rem) {
            *d++ = *s++;
        }
        else if (*s == ' ') {
            s++;
        }
        else {
            while (*t != '\0') {
                //putchar(*t);
                //putchar(*s);
                //putchar('%');
                while (*s == ' ') s++;
                if (toupper(*t) == toupper(*s++)) {
                    //putchar('!');
                    if (isupper(*t)) break;
                    else t++;
                }
                else {
                    s = p;
                    tok++;
                    while (!isupper(*t++)) {}
                }
            }
            if (*t != '\0') {
                *d++ = tok;
                if (tok == TOK_REM) rem = 1;
            }
            else {
                s = p;
                *d++ = toupper(*s++);
            }
        }
    }
    *d = '\0';

    //putchar('#');
 
    return 0;
}


void printtok(uint8_t tok) {
    uint8_t *t;
    uint8_t c;
    uint8_t s;
    
    
    if ((tok < MAXTOKEN) && (tok >= MINTOKEN)) {
        s = toktyp[tok-MINTOKEN];
        t = tokentab;
        while (tok > MINTOKEN) {
            while (islower(*t++));
            tok--;
        }
        //if (s == TOK_CMD) putchar(' ');
        do {
            putchar(toupper(c = *t++));
        } while (islower(c));
        if (s == TOK_CMD) putchar(' ');
    }
    else
        if (tok == ':') putstr(" : ");
        else putchar(tok);
}


int fnbase() {
    int p = basb;
    
    while  ( ((lnhead*)(mem+p))->len != 0 )
        p += ((lnhead*)(mem+p))->len;
    
    return p;
}


uint8_t execute() {

    while (1) {
    
        isgoto = 0;
        
        if ((*exptr >= MINTOKEN) && (*exptr < MAXTOKEN)) {
            if (toktyp[*exptr - MINTOKEN] == TOK_CMD)
                err = cmdarr[*exptr++ - MINTOKEN]();
            else {
                err = ERR_SYN;
            }
        }
        else {
            // putstr("#impl LET#");
            err = cmdarr[TOK_LET - MINTOKEN]();
        }
        if (err != ERR_NONE) return err;

        if (isgoto)
            continue;
            
        if (*exptr == ':') {
            exptr++;
            continue;
        }
       
        if (*exptr++ != '\0') {
            err = ERR_SYN;
            return err;
        }
        
        if (curmd == MODE_DIRECT)
            return err;
            
        if (((lnhead*)(exptr))->len != 0) {
            curln = ((lnhead*)(exptr))->num;
            exptr += sizeof(lnhead);
        }
        else
            return err;
       
    }
}


void basclear() {
    char i;
    
    for (i='A'; i<='Z'; basvars[(i++)-'A']=0);
    base = fnbase();
    
    forsp = 0;
    subsp = 0;
    
}


int dogoto(int n) {
    void *p;

    p = mem + fndln(n);
    
    if ( (((lnhead*)(p))->num != n ) &&
         ( n != 0 )) {
        err = ERR_LIN;
        return err;
    }

    if (((lnhead*)(p))->len == 0)
        return err;
    
    exptr = (uint8_t *)p + sizeof(lnhead);
    curln = ((lnhead*)(p))->num;
    curmd = MODE_RUN;
    isgoto = 1;
    return err;
}


// Expression evaluation

int getnum() {
    int e = 0;
    cmdfun fun;
  
    if (isdigit(*exptr)) {
        if (*exptr == '0') {
            exptr++;
            if (*exptr == 'B') {
                exptr++;
                while ((*exptr == '0') || (*exptr == '1')) {
                    e = 2 * e + (*exptr++ == '0' ? 0 : 1);
                }
                return e;
            }
            else if (*exptr == 'X') {
                exptr++;
                while (isdigit(*exptr) ||
                       ((*exptr >='A') && (*exptr <= 'F'))) {
                    e = 16 * e + (isdigit(*exptr) ?
                        *exptr - '0' :
                        *exptr - 'A' + 10);
                    exptr++;
                }
                return e;
            }
        }

        while (isdigit(*exptr)) {
            e = 10 * e + (*exptr++ - '0');
        }
        return e;
    }
   
    if (isalpha(*exptr)) {
        e = basvars[*exptr++ - 'A'];
        return e;
    }
   
    if ((*exptr >= MINTOKEN) && (*exptr < MAXTOKEN)) {
        if (toktyp[*exptr - MINTOKEN] != TOK_FUN) {
            err = ERR_SYN;
            return e;
        }
       
        fun = cmdarr[*exptr++ - MINTOKEN];
        if (*exptr != '(') {
            err = ERR_SYN;
            return e;
        }
       
        exptr++;
        e = fun();

        if (*exptr != ')') {
            err = ERR_SYN;
            return e;
        }
       
        exptr++;
       
    }

    return e;
}


int parexp() {
  
    int e;
  
    switch (*exptr) {
        case '-':
            exptr++;
            return -parexp();
        case '(':
            exptr++;
            e = expression();
            if (*exptr == ')') exptr++;
            else  err = ERR_SYN;
            return e;
        default:
            return getnum();
    }
}


int powexp() {
  
    int e = parexp();
    int p,r;
  
    while (1) {
        switch (*exptr) {
            case '^':
                exptr++;
                p = parexp();
                r = 1;
                while (p-- > 0)
                    r = r * e;
                e = r;
                break;
            default:
                return e;
        }
    }
}


int mulexp() {
  
    int e = powexp();
  
    while (1) {
        switch (*exptr) {
            case '*':
                exptr++;
                e *= powexp();
                break;
            case '/':
                exptr++;
                e /= powexp();
                break;
            case '%':
                exptr++;
                e %= powexp();
                break;
            default:
                return e;
        }
    }
}


int addexp() {
  
    int e = mulexp();
  
    while (1) {
        switch (*exptr) {
            case '+':
                exptr++;
                e += mulexp();
                break;
            case '-':
                exptr++;
                e -= mulexp();
                break;
            default:
                return e;
        }
    }
}


int relexp() {
  
    int e = addexp();
  
    while (1) {
        switch (*exptr) {
            case '<':
                exptr++;
                e = (e < addexp());
                break;
            case '>':
                exptr++;
                e = (e > addexp());
                break;
            case '=':
                exptr++;
                e = (e == addexp());
                break;
            case '#':
                exptr++;
                e = (e != addexp());
                break;
            default:
                return e;
        }
    }
}


int expression() {
  
    int e = relexp();
  
    while (1) {
        switch (*exptr) {
            case '&':
                exptr++;
                e &= relexp();
                break;
            case '|':
                exptr++;
                e |= relexp();
            default:
                return e;
        }
    }
}


// BASIC commands

int cmd_err(void) {
    err = ERR_SYN;
    return err;
}


int cmd_rem(void) {
    while (*exptr != '\0') exptr++;
    return err;
}


int cmd_let(void) {
    int v;
  
    if (isalpha(*exptr)) {
        v = *exptr++ - 'A';
        if (*exptr == '=') {
            exptr++;
            basvars[v] = expression();
            return err;
        }
    }
    err = ERR_SYN;
    return err;
}


int cmd_print(void) {
  
    uint8_t n = 1;
    uint8_t c;
  
    while (1) {
      
        while ((*exptr == ',') || (*exptr == ';')) {
            n = 0;
            if (*exptr == ',') putchar('\t');
            exptr++;
        }
      
        if ((*exptr == ':') || (*exptr == '\0')) {
            if (n == 1) putstr("\r\n");
            return ERR_NONE;
        }
      
        if (*exptr == '"') {
            n = 1;
            exptr++;
            while ((c = *exptr++) != '"') {
                if (c == '\0') return ERR_SYN;
                else putchar(c);
            }
        }
        else {
            n = 1;
            putstr(itoa(expression(),buf,10));
            if (err != ERR_NONE) return err;
        }
    }
}


int cmd_poke(void) {
//    uint8_t *a;
    int a;
    uint8_t d;
  
//    a = (uint8_t *)(expression());
    a = expression();
    if (err != ERR_NONE) return err;
    if (*exptr == ',') {
        exptr++;
        d = (uint8_t)(expression());
        if (err != ERR_NONE) return err;
    }
    else
        return ERR_SYN;
   
    RAWMEM(a) = d;  
//    *a = d;
    return ERR_NONE;

}


int cmd_list(void) {
    int p;
    int n;
    uint8_t c;

    n = expression();
    if (err != ERR_NONE) return err;
    
    p = fndln(n);
    
    while ( ((lnhead*)(mem+p))->len != 0 ) {
        putstr(itoa(((lnhead*)(mem+p))->num,buf,10));
        putchar(' ');
        p += sizeof(lnhead);
        while ( (c = *((uint8_t*)(mem+(p++)))) != 0)
            printtok(c);
        putstr("\r\n");
    }
    return err;
}


int cmd_new(void) {
    ((lnhead*)(mem+basb))->len=0;
    ((lnhead*)(mem+basb))->num=0;
    basclear();
    return err;
}


int cmd_run(void) {
    basclear();
    return cmd_goto();
}


int cmd_goto(void) {
    int n;

    n = expression();
    if (err != ERR_NONE) return err;
    
    return dogoto(n);
}


int cmd_gosub(void) {
    int n;
    subentry ste;
    
    if (curmd != MODE_RUN) {
        err = ERR_MOD;
        return err;
    }

    n = expression();
    if (err != ERR_NONE)
        return err;
    
    ste.exptr = exptr;
    ste.line = curln;
    
    if (subsp < MAXSUB)
        substack[subsp++] = ste;
    else
        err = ERR_ST;
        
    return dogoto(n);
}


int cmd_return(void) {
    if (subsp == 0) {
        err = ERR_RET;
        return err;
    }
    
    subsp--;
    exptr = substack[subsp].exptr;
    curln = substack[subsp].line;
    curmd = MODE_RUN;
    return err;

}


int cmd_stop(void) {
    err = ERR_STOP;
    return err;
}


int cmd_if(void) {
    int e;

    e = expression();
    if (err != ERR_NONE) return err;
    
    if (e != 0) {
        isgoto = 1;
        return err;
    }
    
    while (*exptr != '\0') exptr++;
    
    return err;
}


int cmd_for(void) {
    int p;
    uint8_t v;
    int s;
    int l;
    forentry ste;
    
    if (curmd != MODE_RUN) {
        err = ERR_MOD;
        return err;
    }
    
    if (!isalpha(v = *exptr)) {
        err = ERR_SYN;
        return err;
    }
    
    if (cmd_let() != ERR_NONE)
        return err;
    
    if (*exptr != TOK_TO) {
        err = ERR_SYN;
        return err;
    }
    
    exptr++;
    l = expression();
    if (err != ERR_NONE)
        return err;
    
    if (*exptr == TOK_STEP) {
        exptr++;
        s = expression();
        if (err != ERR_NONE)
            return err;
    }
    else {
        s = (basvars[v - 'A'] > l) ? -1 : 1;
    }

    for (p = 0; p < forsp; p++) {
        if (forstack[p].var == v) {
            forsp = p;
            break;
        }
    }
    
    ste.exptr = exptr;
    ste.line = curln;
    ste.var = v;
    ste.step = s;
    ste.limit = l;
    
    if (forsp < MAXFOR)
        forstack[forsp++] = ste;
    else
        err = ERR_ST;
    
    return err;
}


int cmd_next(void) {
//    forentry ste;
    uint8_t v;
    int p;
    int f;

    //putstr("SP:"); putstr(itoa(forsp,buf,10)); putstr("\r\n");
    //for (p=0; p<forsp; p++) {
        //putstr("\r\n#"); putstr(itoa(p,buf,10)); putstr("\r\n");
        //ste = forstack[p];
        //putstr("exptr:"); putstr(itoa(*(uint8_t*)(ste.exptr),buf,10)); putstr("\r\n");
        //putstr("line:"); putstr(itoa(ste.line,buf,10)); putstr("\r\n");
        //putstr("var:"); putstr(itoa(ste.var,buf,10)); putstr("\r\n");
        //putstr("step:"); putstr(itoa(ste.step,buf,10)); putstr("\r\n");
        //putstr("limit:"); putstr(itoa(ste.limit,buf,10)); putstr("\r\n");
    //}

    if (isalpha(v = *exptr)) {
        exptr++;
        f = 0;
        for (p=0; p<forsp; p++) {
            if (forstack[p].var == v) {
                f = p + 1;
                break;
            }
        }
        forsp = f;
    }
    
    if (forsp == 0) {
        err = ERR_NEXT;
        return err;
    }
    
    basvars[forstack[forsp-1].var - 'A'] += forstack[forsp-1].step;
    if (forstack[forsp-1].step < 0 ?
        basvars[forstack[forsp-1].var - 'A'] < forstack[forsp-1].limit :
        basvars[forstack[forsp-1].var - 'A'] > forstack[forsp-1].limit) {
        forsp--;
        return err;
    }
    else {
        exptr = forstack[forsp-1].exptr;
        curln = forstack[forsp-1].line;
        curmd = MODE_RUN;
        return err;
    }
    
    return err;
}


// BASIC functions

int cmd_peek(void) {
    int a;

    a = expression();
    if (err != ERR_NONE)
        return 0;
    return RAWMEM(a);
}





int main() {
 
    char c='A';
    int l = 7;
    int n;
    unsigned int f;

    IOINIT;

    mtot = MAXMEM-256;
    mem = malloc(mtot);
    putstr(mem == NULL ? "malloc failed\r\n" : "malloc succeeded\r\n");
    meme = mtot;
    basb = 0;
    
    
    ((lnhead*)(mem+basb))->len=5;
    ((lnhead*)(mem+basb))->num=1000;
    *((uint8_t*)(mem+basb+3)) = 131;
    *((uint8_t*)(mem+basb+4)) = 0;
    ((lnhead*)(mem+basb+5))->len=7;
    ((lnhead*)(mem+basb+5))->num=1010;
    *((uint8_t*)(mem+basb+8)) = 129;
    *((uint8_t*)(mem+basb+9)) = 49;
    *((uint8_t*)(mem+basb+10)) = 44;
    *((uint8_t*)(mem+basb+11)) = 0;
    ((lnhead*)(mem+basb+12))->len=0;
    ((lnhead*)(mem+basb+12))->num=0;
    
    basclear();
    
    f = meme - base;

    putstr(itoa(f,line,10));
    putstr(" bytes free\r\n");
 
    putstr(itoa(basb,line,10));
    putstr(" bas begin\r\n");
 
    putstr(itoa(base,line,10));
    putstr(" bas end\r\n");
 
    while (1) {
    
        OPORT = l;

        c = readline(line);
        putstr("\r\n");
        tokenize(line);

        exptr = line;
        curln = 0;
        curmd = MODE_DIRECT;
        err = ERR_NONE;

        if (c && (*exptr != '\0')) {
            if (isdigit(*exptr)) {

                n = 0;
                while (isdigit(*exptr))
                    n = 10 * n + (*exptr++ - '0');

                delln(n);

                if (*exptr != '\0')
                    addln(n,exptr);
               
            }
            else {

                execute();

                putstr(errtxt[err]);

                if ((err != ERR_NONE) && (err != ERR_STOP))
                    putstr(" err");
                if (curmd != MODE_DIRECT) {
                    putstr(" in ");
                    putstr(itoa(curln,buf,10));
                }
                putstr("\r\n");

            }
        }
        
        l--;
    
    }

    return 0;

}
