#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define USAGE "USAGE: %s [-x#] [inputfiles]\n"

int xflag;

void exit(int status);

char *ncvt(char *s, short *v);
void fhsum(FILE *ofp, int x, int y, int xscale, int yscale, int hsum, int fontsize);
void fvsum(FILE *ofp, int x, int y, int xscale, int yscale, int vsum, int fontsize);

main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int c;
	FILE *fp;
	char lprbuffer[4096];

	while ((c = getopt(argc, argv, "x:")) != EOF)
		switch(c) {
		case 'x':
			xflag = atoi(optarg);
			break;
		default:
			fprintf(stderr, USAGE, argv[0]);
			exit(1);
		}

	strncpy(lprbuffer, "lpr", sizeof(lprbuffer));
	if (optind == argc) {
		convert(stdin, "STDIN");
		strncat(lprbuffer, " ", sizeof(lprbuffer));
		strncat(lprbuffer, "STDIN.ps", sizeof(lprbuffer));
	} else while (optind < argc) {
		if ( (fp = fopen(argv[optind], "r")) == NULL)
			fprintf(stderr, "Cannot open %s\n", argv[optind]);
		else {
			convert(fp, argv[optind]);
			strncat(lprbuffer, " ", sizeof(lprbuffer));
			strncat(lprbuffer, argv[optind], sizeof(lprbuffer));
			strncat(lprbuffer, ".ps", sizeof(lprbuffer));
			fclose(fp);
		}
		optind++;
	}

	system(lprbuffer);
	lprbuffer[0] = 'r'; lprbuffer[1] = 'm'; lprbuffer[2] = ' ';
	system(lprbuffer);

	exit(0);
}

#define GRID(xx, yy)	grid[(xx) + (yy) * (rp->x + 1)]

struct grid {
	short flag;
	short hsum;
	short vsum;
};

#define COMMENTSLENGTH	64
struct rec {
	short x;
	short y;
	struct grid *grid;
	short z1, z2, z3;
	char comments[COMMENTSLENGTH];
} rec;

#define BORDER	36	/* 36 point (.5 inch) border all around */
#define XMIN	BORDER			/* left margin */
#define XMAX	((int)(8.5 * 72 - BORDER))	/* right margin */
#define YMIN	BORDER			/* top margin */
#define YMAX	(11 * 72 - BORDER)	/* bottom margin */

#define GRAY	.8	/* gray level for shading */

convert(FILE *fp, char *name)
{
	short x, y;
	short hsum, vsum;
	char oname[512];
	FILE *ofp;
	struct rec *rp = &rec;
	int xscale, yscale;
	int fontsize;
	char buffer[2048];
	char *p;

	/* check file type */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) || strcmp(buffer, "CS\n")) {
		fprintf(stderr, "%s Not Checksums file\n", name);
		return;
	}

	/* read x and y */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		fprintf(stderr, "%s Read failure\n", name);
		fclose(fp);
		return(-1);
	}
	p = ncvt(p, &x);
	p = ncvt(p, &y);
	if (*p != '\n' || x > 30 || y > 30) {
		fprintf(stderr, "%s Bad file\n", name);
		fclose(fp);
		return(-1);
	}
	rp->x = x;
	rp->y = y;

	/* read mode */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		fprintf(stderr, "%s Bad file\n", name);
		fclose(fp);
		return(-1);
	}

	/* allocate grid */
	if ( (rp->grid = (struct grid *)malloc(sizeof(struct grid) * (rp->x + 1) * (rp->y + 1))) == NULL) {
		fprintf(stderr, "%s malloc failure\n", name);
		fclose(fp);
		return(-1);
	}
	/* clear grid */
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		rp->GRID(x, y).flag = 0;
		rp->GRID(x, y).hsum = 0;
		rp->GRID(x, y).vsum = 0;
	}

	/* read in flagged squares */
	while (p = fgets(buffer, sizeof(buffer), fp)) {
		p = ncvt(p, &x);
		p = ncvt(p, &y);
		p = ncvt(p, &hsum);
		p = ncvt(p, &vsum);
		rp->GRID(x, y).flag = -1;
		rp->GRID(x, y).hsum = hsum;
		rp->GRID(x, y).vsum = vsum;
	}

	/* creat output file */
	strncpy(oname, name, sizeof(oname));
	strncat(oname, ".ps", sizeof(oname));
	if ( (ofp = fopen(oname, "w")) == NULL) {
		fprintf(stderr, "%s.ps: output file create error\n", name);
		return;
	}
	
	/* generate output */
	fontsize = 10;
	header(ofp, fontsize);

	title(ofp, name, fontsize);
	comments(ofp, rp->comments, fontsize);

	xscale = (XMAX - XMIN) / (rp->x + 1);
	yscale = (YMAX - YMIN) / (rp->y + 1);
	if (xscale > 36)
		xscale = 36;
	if (yscale > 36)
		yscale = 36;
	if (xscale < yscale)
		yscale = xscale;
	else
		xscale = yscale;
	for (y = 0; y <= rp->y; y++)
	for (x = 0; x <= rp->x; x++) {
		if (rp->GRID(x, y).flag < 0) {
			fill(ofp, x, y, xscale, yscale);
			if (checkx(rp, x, y)) {
				diag(ofp, x, y, xscale, yscale);
				rdiag(ofp, x, y, xscale, yscale);
			}
		}
		box(ofp, x, y, xscale, yscale);
		if (rp->GRID(x, y).hsum || rp->GRID(x, y).vsum)
			diag(ofp, x, y, xscale, yscale);
		if (rp->GRID(x, y).hsum)
			fhsum(ofp, x, y, xscale, yscale, rp->GRID(x, y).hsum, fontsize);
		if (rp->GRID(x, y).vsum)
			fvsum(ofp, x, y, xscale, yscale, rp->GRID(x, y).vsum, fontsize);
	}

	showpage(ofp);

	trailer(ofp, 1);

	free(rp->grid);

	fclose(ofp);
}

char *ncvt(char *s, short *v)
{
	int rv = 0;

	while (*s >= '0' && *s <= '9') {
		rv = rv * 10 + *s - '0';
		s++;
	}
	while (*s == ' ') s++;
	*v = rv;
	return(s);
}

/*
	xflag
		0 = X in all unused boxes
		1 = X in all unused boxes except top and left
		2 = X in "edge" unused boxes
		else = no X
*/
checkx(rp, x, y)
struct rec *rp;
int x, y;
{

	if ((rp->GRID(x, y).hsum == 0) && (rp->GRID(x, y).vsum == 0)) {
		switch(xflag) {
			case 0:
				return(1);
			case 1:
				if (x > 0 && y > 0)
					return(1);
				else
					return(0);
			case 2:
				if (x > 0 && rp->GRID(x-1, y).flag >= 0)
					return(1);
				if (x < rp->x && rp->GRID(x+1, y).flag >= 0)
					return(1);
				if (y > 0 && rp->GRID(x, y-1).flag >= 0)
					return(1);
				if (y < rp->y && rp->GRID(x, y+1).flag >= 0)
					return(1);
			default:
				return(0);
		}
	}
	return(0);
}

header(FILE *ofp, int fontsize)
{
	fprintf(ofp, "%s\n", "%!PS-Adobe-3.0");
	fprintf(ofp, "%s\n", "%%BoundingBox: 0 0 612 792");
	fprintf(ofp, "%s\n", "%%Pages: atend");
	fprintf(ofp, "%s\n", "%%EndComments");
	fprintf(ofp, "%s\n", "%%Page: 0 0");
	fprintf(ofp, "%s\n", "gsave");
	fprintf(ofp, "%s\n", "1 setlinewidth");
	fprintf(ofp, "%s %d %d %s\n", "/Times-Roman findfont", fontsize, fontsize, "matrix scale makefont setfont");
}

trailer(FILE *ofp, int pages)
{
	fprintf(ofp, "%s\n", "grestore");
	fprintf(ofp, "%s\n", "%%Trailer");
	fprintf(ofp, "%s%d\n", "%%Pages: ", pages);
	fprintf(ofp, "%s\n", "%%EOF");
}

title(FILE *ofp, char *name, int fontsize)
{
	fprintf(ofp, "%ld %d moveto\n",
		(XMIN + XMAX) / 2 - strlen(name) * (int)(fontsize/2),
		YMAX + BORDER/2 - (int)(fontsize/2));
	fprintf(ofp, "(%s) show\n", name);
}

comments(FILE *ofp, char *comments, int fontsize)
{
	fprintf(ofp, "%d %d moveto\n",
		XMIN,
		YMAX + BORDER/2 - (int)(fontsize/2));
	fprintf(ofp, "(%s) show\n", comments);
}

fill(FILE *ofp, int x, int y, int xscale, int yscale)
{
	outline(ofp, x, y, xscale, yscale);
	fprintf(ofp, "%.3f %.3f %.3f setrgbcolor\n", GRAY, GRAY, GRAY);
	fprintf(ofp, "fill\n");
}

box(FILE *ofp, int x, int y, int xscale, int yscale)
{
	outline(ofp, x, y, xscale, yscale);
	fprintf(ofp, "0 0 0 setrgbcolor\n");
	fprintf(ofp, "stroke\n");
}

outline(FILE *ofp, int x, int y, int xscale, int yscale)
{
	fprintf(ofp, "%d %d moveto\n",
		x * xscale + XMIN,
		YMAX - (y * yscale));
	fprintf(ofp, "%d %d lineto\n",
		(x + 1) * xscale + XMIN,
		YMAX - (y * yscale));
	fprintf(ofp, "%d %d lineto\n",
		(x + 1) * xscale + XMIN,
		YMAX - ((y + 1) * yscale));
	fprintf(ofp, "%d %d lineto\n",
		x * xscale + XMIN,
		YMAX - ((y + 1) * yscale));
	fprintf(ofp, "closepath\n");
}

diag(FILE *ofp, int x, int y, int xscale, int yscale)
{
	fprintf(ofp, "%d %d moveto\n",
		x * xscale + XMIN,
		YMAX - (y * yscale));
	fprintf(ofp, "%d %d lineto\n",
		(x + 1) * xscale + XMIN,
		YMAX - ((y + 1) * yscale));
	fprintf(ofp, "0 0 0 setrgbcolor\n");
	fprintf(ofp, "stroke\n");
}

rdiag(FILE *ofp, int x, int y, int xscale, int yscale)
{
	fprintf(ofp, "%d %d moveto\n",
		(x + 1) * xscale + XMIN,
		YMAX - (y * yscale));
	fprintf(ofp, "%d %d lineto\n",
		x * xscale + XMIN,
		YMAX - ((y + 1) * yscale));
	fprintf(ofp, "0 0 0 setrgbcolor\n");
	fprintf(ofp, "stroke\n");
}

void fhsum(FILE *ofp, int x, int y, int xscale, int yscale, int hsum, int fontsize)
{
	fprintf(ofp, "%d %d moveto\n",
		(x + 1) * xscale - fontsize - 1 + XMIN,
		YMAX - (y * yscale + fontsize));
	fprintf(ofp, "(%2d) show\n", hsum);
}

void fvsum(FILE *ofp, int x, int y, int xscale, int yscale, int vsum, int fontsize)
{
	fprintf(ofp, "%d %d moveto\n",
		x * xscale + 2 + XMIN,
		YMAX - ((y + 1) * yscale - 1));
	fprintf(ofp, "(%2d) show\n", vsum);
}

showpage(FILE *ofp)
{
	fprintf(ofp, "showpage\n");
}

