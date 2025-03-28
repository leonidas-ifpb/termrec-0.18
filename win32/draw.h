typedef struct { unsigned char r,g,b,a; } color;

extern void draw_vt(HDC dc, int px, int py, tty vt);
extern void draw_init(LOGFONT *df);
extern void draw_free(void);
extern void draw_border(HDC dc, tty vt);

extern int chx,chy;

#define clWoodenBurn 0x000F1728
#define clWoodenDown 0x004B5E6B
#define clWoodenMid  0x008ba9c3
#define clWoodenUp   0x009ECCE2
