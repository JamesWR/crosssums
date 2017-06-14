#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <gtk/gtk.h>

/* DATA AND DEFINITION AREA (.h) */

int insert = 1;		/* generate INSERT's for database */

/* definitions for the crosssums information */
#define GRID(xx, yy)	grid[(xx) + (yy) * (rp->x + 1)]
#define MAINWINDOWMENUBARHEIGHTS	105
#define BOXMAX	40
#define MAXX	30
#define MAXY	30
#define DEFAULTX	13
#define DEFAULTY	13
#define PRINTBORDERSIZE	0.5	/* 0.5 inch */
#define PRINTBOXMAX	0.75	/* 0.75 inch */

gint screenwidth, screenheight;

/* possible single sum solution structure */
struct ptr
{
	struct ptr *p_ptr;	/* pointer to next possibility */
	int flag;		/* general purpose flag */
	char values[10];	/* this possibility */
};

/* single cell structure */
struct grid
{
	short flag;		/* -1 is no numbers here */
	short hsum;		/* horizontal sum */
	short vsum;		/* vertical sum */
	short hlng;		/* horizontal length */
	struct ptr *hptr;	/* pointer to possible horizontal solution */
	short vlng;		/* vertical length */
	struct ptr *vptr;	/* pointer to possible vertical solution */
};

#define COMMENTSLENGTH	64
struct rec {
	short x;
	short y;
	struct grid *grid;
	short sumx;
	short sumy;
	short topbottom;
	short mode;
	short newsum;
	char comments[COMMENTSLENGTH];
} rec;

#define NWINDOWS	500
struct windows {
	GtkWidget *widget;
	GtkWidget *drawing_area;
	GdkPixmap *pixmap;
	struct rec rec;
	int modified;
	int formatupdate;
	char *filename;
}  windows[NWINDOWS];

#define MENU_FILE_NEW		(1000 + 0)
#define MENU_FILE_OPEN		(1000 + 1)
#define MENU_FILE_CLOSE		(1000 + 2)
#define MENU_FILE_SAVE		(1000 + 3)
#define MENU_FILE_SAVEAS	(1000 + 4)
#define MENU_FILE_PRINT		(1000 + 5)
#define MENU_FILE_PRINTALL	(1000 + 6)

#define MENU_EDIT_UNDO		(2000 + 0)
#define MENU_EDIT_CUT		(2000 + 1)
#define MENU_EDIT_COPY		(2000 + 2)
#define MENU_EDIT_PASTE		(2000 + 3)
#define MENU_EDIT_CLEAR		(2000 + 4)

#define MENU_MODE_ASYMSETUP	(3000 + 0)
#define MENU_MODE_SETUP		(3000 + 1)
#define MENU_MODE_SUMS		(3000 + 2)
#define MENU_MODE_VALIDATE	(3000 + 3)
#define MENU_MODE_SOLVE		(3000 + 4)
#define MENU_MODE_SOLVEALL	(3000 + 5)

/* ROUTINE PROTOTYPES */
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void destroy(long windownumber, guint action, GtkWidget *widget);
static void menuitem_response(long windownumber, guint action, GtkWidget *widget);
GtkWidget *createmenus(long windownumber);
static long new_window(int width, int height, char *title);
static void file_open(long windownumber);
static void file_save(long windownumber);
static void file_saveas(long windownumber);
static int open_by_file(char *name);
static void draw_crosssum(long windownumber);
static void dumpwindow(long windownumber);
int file_new();
void savewindow(long windownumber);
void showerror(char *windowname, char *message);
int runlengthcheck(long windownumber);
int sumscheck(long windownumber);
int onesumcheck(int length, int value);
void solve(long windownumber);
int solven(struct rec *rp, long windownumber);
void gensoln(struct rec *rp);
struct ptr *gensoln1(int n, int sum, char *xx, int which, int sofar, struct ptr *p);
struct ptr *permute(int n, int which, char *xx, struct ptr *p);
struct ptr *ladd(struct ptr *p, char *xx, int n);
void savesolution(struct rec *rp);
void setsolution(struct rec *rp);
void file_print(long windownumber);
static void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, long windownumber);
gchar *getcomments(long windownumber);
gchar *getctimes(long windownumber);
char *ncvt(char *s, short *v);
void dbformat(char *str, char *ostr);

/* MAIN */

int main(int argc, char *argv[])
{
	long windownumber;
	int i;
	int flag1 = 0;
	int flag2 = 0;
	GtkWidget *widget;
	GtkWidget *box;
	GtkWidget *draw;
	GdkScreen *screen;

	/* init */
	gtk_init(&argc, &argv);

	/* get screen size */
	widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
box = gtk_vbox_new(FALSE, 0);
gtk_container_add(GTK_CONTAINER(widget), box);
gtk_widget_show(box);
draw = gtk_drawing_area_new();
gtk_box_pack_start(GTK_BOX(box), draw, TRUE, TRUE, 0);
gtk_widget_show(draw);
gtk_widget_show(widget);
gtk_window_maximize(GTK_WINDOW(widget));
gtk_window_get_size(GTK_WINDOW(widget), &screenwidth, &screenheight);
fprintf(stderr, "screenwidth=%d screenheight=%d\n", screenwidth, screenheight);
	screen = gtk_window_get_screen(GTK_WINDOW(widget));
	screenwidth = gdk_screen_get_width(GDK_SCREEN(screen));
	screenheight = gdk_screen_get_height(GDK_SCREEN(screen)) - MAINWINDOWMENUBARHEIGHTS;
fprintf(stderr, "screenwidth=%d screenheight=%d\n", screenwidth, screenheight);
	gtk_widget_destroy(widget);

	for (windownumber = 0; windownumber < NWINDOWS; windownumber++) {
		windows[windownumber].widget = NULL;
	}

	for (i = 1; i < argc; i++) {
		flag1 = 1;
		if (open_by_file(argv[i]) >= 0)
			flag2++;
	}
	if (flag1 == 0) {
		if (file_new() < 0)
			return(0);
	} else if (flag2 == 0)
		return(0);

	gtk_main();

	return(0);
}

/* ROUTINES AREA */

/* delete (close window) event */
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data )
{
	long windownumber = (long)data;
	int count = 0;

	if (windows[windownumber].modified || windows[windownumber].formatupdate)
		savewindow(windownumber);
	windows[windownumber].widget = NULL;

	count = 0;
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
		if (windows[windownumber].widget)
			count++;
	if (count == 0)
		gtk_main_quit();

	return(FALSE);
}

/* expose window event */

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	long windownumber = (long)data;

	gdk_draw_drawable(widget->window, widget->style->fg_gc[gtk_widget_get_state(widget)], windows[windownumber].pixmap, event->area.x, event->area.y, event->area.x, event->area.y, event->area.width, event->area.height);
	return(FALSE);
}

/* configure event (create pixmap for drawing window */

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	long windownumber = (long)data;

	/* create pixmap and clear it */
	if (windows[windownumber].pixmap)
		g_object_unref(windows[windownumber].pixmap);
	windows[windownumber].pixmap = gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
	gdk_draw_rectangle(windows[windownumber].pixmap, widget->style->white_gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);

	return(TRUE);
}

/* key press event in drawing window */

static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	long windownumber = (long)data;
	struct rec *rp;
	int x, y;
	int cx, cy;
	int tx;
	int flag;
	int tb;
	int kv;

	rp = &windows[windownumber].rec;
	if (rp->mode == MENU_MODE_SUMS) {
		/* GDK_MOD2_MASK is Numeric Lock */
		if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK | GDK_META_MASK))
			return(FALSE);
		cx = rp->sumx;
		cy = rp->sumy;
		tb = rp->topbottom;
		kv = event->keyval & 0x7F;
		if (kv >= (int)'0' && kv <= (int)'9') {
			if (tb)
				if (rp->newsum)
					rp->GRID(cx, cy).hsum = kv - (int)'0';
				else
					rp->GRID(cx, cy).hsum = (rp->GRID(cx, cy).hsum % 10) * 10 + kv - (int)'0';
			else
				if (rp->newsum)
					rp->GRID(cx, cy).vsum = kv - (int)'0';
				else
					rp->GRID(cx, cy).vsum = (rp->GRID(cx, cy).vsum % 10) * 10 + kv - (int)'0';
			rp->newsum = 0;
			windows[windownumber].modified = 1;
			draw_crosssum(windownumber);
			return(FALSE);
		} else if (kv == 0x09 || kv == 0x0A || kv == 0x0E) {
			/* step to next box */
			if (tb && cy < rp->y && rp->GRID(cx, cy+1).flag >= 0) {
				rp->topbottom = 0;
				rp->newsum = 1;
				draw_crosssum(windownumber);
				return(FALSE);
			}
			tx = cx + 1;
			for (y = cy; y <= rp->y; y++) {
				for (x = tx; x <= rp->x; x++) {
					if (rp->GRID(x, y).flag >= 0)
						continue;
					if (x < rp->x && rp->GRID(x+1, y).flag >= 0) {
						rp->topbottom = 1;
						rp->newsum = 1;
						rp->sumx = x;
						rp->sumy = y;
						draw_crosssum(windownumber);
						return(FALSE);
					}
					if (y < rp->y && rp->GRID(x, y+1).flag >= 0) {
						rp->topbottom = 0;
						rp->newsum = 1;
						rp->sumx = x;
						rp->sumy = y;
						draw_crosssum(windownumber);
						return(FALSE);
					}
				}
				tx = 0;
			}
		}
	}
	return(FALSE);
}

/* mouse button press event in drawing window */

static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	long windownumber = (long)data;
	int x, y;
	int tx, ty;
	int boxsize;
	struct rec *rp;

	rp = &windows[windownumber].rec;

	/* calculate box size */
	boxsize = screenwidth / (rp->x + 1);
	if ( (screenheight / (rp->y + 1)) < boxsize)
		boxsize = screenheight / (rp->y + 1);
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* get coords of box */
	x = event->x / boxsize;
	y = event->y / boxsize;
	if (event->button == 1 && windows[windownumber].pixmap != NULL)
		if (rp->mode == MENU_MODE_ASYMSETUP ) {
			if ( (x > 0 && x <= rp->x) && (y > 0 && y <= rp->y)) {
				if (rp->GRID(x, y).flag < 0)
					rp->GRID(x, y).flag = 0;
				else
					rp->GRID(x, y).flag = -1;
				rp->GRID(x, y).hsum = 0;
				rp->GRID(x, y).vsum = 0;
				tx = rp->x + 1 - x;
				ty = rp->y + 1 - y;
				if (x != tx || y != ty) {
					if (rp->GRID(tx, ty).flag < 0)
						rp->GRID(tx, ty).flag = 0;
					else
						rp->GRID(tx, ty).flag = -1;
					rp->GRID(tx, ty).hsum = 0;
					rp->GRID(tx, ty).vsum = 0;
				}
				windows[windownumber].modified = 1;
				draw_crosssum(windownumber);
			}
		} else if (rp->mode == MENU_MODE_SETUP) {
			if ( (x > 0 && x <= rp->x) && (y > 0 && y <= rp->y)) {
				if (rp->GRID(x, y).flag < 0)
					rp->GRID(x, y).flag = 0;
				else
					rp->GRID(x, y).flag = -1;
				rp->GRID(x, y).hsum = 0;
				rp->GRID(x, y).vsum = 0;
				windows[windownumber].modified = 1;
				draw_crosssum(windownumber);
			}
		} else if (rp->mode == MENU_MODE_SUMS) {
			if (x <= rp->x && y <= rp->y) {
				tx = event->x - x * boxsize;
				ty = event->y - y * boxsize;
				if (tx >= ty && rp->GRID(x, y).flag < 0 && (x < rp->x && rp->GRID(x+1, y).flag >= 0)) {
					rp->sumx = x;
					rp->sumy = y;
					rp->topbottom = 1;
					rp->newsum = 1;
					draw_crosssum(windownumber);
				} else if (tx < ty && rp->GRID(x, y).flag < 0 && (y < rp->y && rp->GRID(x, y+1).flag >= 0)) {
					rp->sumx = x;
					rp->sumy = y;
					rp->topbottom = 0;
					rp->newsum = 1;
					draw_crosssum(windownumber);
				}
			}
		}
	return(TRUE);
}

/* destroy (save altered windows and exit) */

static void destroy(long windownumber, guint action, GtkWidget *widget)
{
	/* check for save windows */
	for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
		if (windows[windownumber].widget && windows[windownumber].modified || windows[windownumber].formatupdate)
			savewindow(windownumber);

	gtk_main_quit();
}

/* menu events */

static void menuitem_response(long windownumber, guint action, GtkWidget *widget)
{
	int count;
	GtkWidget *topwidget = windows[windownumber].widget;
	int x, y;
	struct rec *rp;

	rp = &windows[windownumber].rec;
	switch(action) {
		case MENU_FILE_NEW:
			(void)file_new();
			break;
		case MENU_FILE_OPEN:
			file_open(windownumber);
			break;
		case MENU_FILE_CLOSE:
			if (windows[windownumber].modified || windows[windownumber].formatupdate)
				savewindow(windownumber);
			gtk_widget_destroy(topwidget);
			windows[windownumber].widget = NULL;
			count = 0;
			for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
				if (windows[windownumber].widget)
					count++;
			if (count == 0)
				gtk_main_quit();
			break;
		case MENU_FILE_SAVE:
			file_save(windownumber);
			break;
		case MENU_FILE_SAVEAS:
			file_saveas(windownumber);
			break;
		case MENU_FILE_PRINT:
			file_print(windownumber);
			break;
		case MENU_FILE_PRINTALL:
			file_print(-1);
			break;

		case MENU_EDIT_UNDO:
			fprintf(stderr, "UNDO NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_CUT:
			fprintf(stderr, "CUT NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_COPY:
			fprintf(stderr, "COPY NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_PASTE:
			fprintf(stderr, "PASTE NOT IMPLEMENTED\n");
			break;
		case MENU_EDIT_CLEAR:
			fprintf(stderr, "CLEAR NOT IMPLEMENTED\n");
			break;

		case MENU_MODE_ASYMSETUP:
			rp->mode = MENU_MODE_ASYMSETUP;
			draw_crosssum(windownumber);
			break;
		case MENU_MODE_SETUP:
			rp->mode = MENU_MODE_SETUP;
			draw_crosssum(windownumber);
			break;
		case MENU_MODE_SUMS:
			if (runlengthcheck(windownumber))
				break;
			rp->mode = MENU_MODE_SUMS;
			for (y = 0; y <= rp->y; y++)
			for (x = 0; x <= rp->x; x++) {
				if (rp->GRID(x, y).flag < 0) {
					if (x < rp->x && rp->GRID(x+1, y).flag >= 0) {
						rp->topbottom = 1;
						rp->newsum = 1;
						rp->sumx = x;
						rp->sumy = y;
						goto DONE;
					}
					if (y < rp->y && rp->GRID(x, y+1).flag >= 0) {
						rp->topbottom = 0;
						rp->newsum = 1;
						rp->sumx = x;
						rp->sumy = y;
						goto DONE;
					}
				}
			}
		DONE:	draw_crosssum(windownumber);
			break;
		case MENU_MODE_VALIDATE:
			if (runlengthcheck(windownumber))
				break;
			if (sumscheck(windownumber))
				break;
			rp->mode = MENU_MODE_VALIDATE;
			draw_crosssum(windownumber);
			break;
		case MENU_MODE_SOLVE:
			solve(windownumber);
			draw_crosssum(windownumber);
			break;
		case MENU_MODE_SOLVEALL:
			for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
				if (windows[windownumber].widget) {
					solve(windownumber);
					draw_crosssum(windownumber);
				}
			break;
	}
}

/* menu data */

static GtkItemFactoryEntry menu_items[] = {
	{"/_File", NULL, NULL, 0, "<Branch>"},
	{"/_File/_New", "<control>N", menuitem_response, MENU_FILE_NEW, "<StockItem>", GTK_STOCK_NEW},
	{"/_File/_Open", "<control>O", menuitem_response, MENU_FILE_OPEN, "<StockItem>", GTK_STOCK_OPEN},
	{"/_File/_Close", "<control>W", menuitem_response, MENU_FILE_CLOSE, "<Item>"},
	{"/_File/_Save", "<control>S", menuitem_response, MENU_FILE_SAVE, "<StockItem>", GTK_STOCK_SAVE},
	{"/_File/Save_As", NULL, menuitem_response, MENU_FILE_SAVEAS, "<Item>"},
	{"/_File/sep1", NULL, NULL, 0, "<Separator>"},
	{"/_File/_Print", "<control>P", menuitem_response, MENU_FILE_PRINT, "<Item>"},
	{"/_File/P_rintAll", "<shift><control>P", menuitem_response, MENU_FILE_PRINTALL, "<Item>"},
	{"/_File/sep2", NULL, NULL, 0, "<Separator>"},
	{"/_File/_Quit", "<CTRL>Q", destroy, 0, "<StockItem>", GTK_STOCK_QUIT},

	{"/_Edit", NULL, NULL, 0, "<Branch>"},
	{"/_Edit/_Undo", "<control>Z", menuitem_response, MENU_EDIT_UNDO, "<StockItem>", GTK_STOCK_UNDO},
	{"/_Edit/_Cut", "<control>X", menuitem_response, MENU_EDIT_CUT, "<StockItem>", GTK_STOCK_CUT},
	{"/_Edit/_Copy", "<control>C", menuitem_response, MENU_EDIT_COPY, "<StockItem>", GTK_STOCK_COPY},
	{"/_Edit/_Paste", "<control>V", menuitem_response, MENU_EDIT_PASTE, "<StockItem>", GTK_STOCK_PASTE},
	{"/_Edit/_Clear", NULL, menuitem_response, MENU_EDIT_CLEAR, "<StockItem>", GTK_STOCK_CLEAR},

	{"/_Mode", NULL, NULL, 0, "<Branch>"},
	{"/_Mode/_AsymSetup", "<control>1", menuitem_response, MENU_MODE_ASYMSETUP, "<Item>"},
	{"/_Mode/_Setup", "<control>2", menuitem_response, MENU_MODE_SETUP, "<Item>"},
	{"/_Mode/Sums", "<control>3", menuitem_response, MENU_MODE_SUMS, "<Item>"},
	{"/_Mode/Validate", "<control>4", menuitem_response, MENU_MODE_VALIDATE, "<Item>"},
	{"/_Mode/Solve", "<control>5", menuitem_response, MENU_MODE_SOLVE, "<Item>"},
	{"/_Mode/SolveAll", "<control>6", menuitem_response, MENU_MODE_SOLVEALL, "<Item>"},

	{"/_Help", NULL, NULL, 0, "<Branch>"},
};

/* create menus */

GtkWidget *createmenus(long windownumber)
{
	GtkWidget *widget = windows[windownumber].widget;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make accelerator group */
	accel_group = gtk_accel_group_new();

	/* Make an ItemFactory */
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	/* Generate menu items */
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)windownumber);

	/* Attach the new accelerator group to the widget */
	gtk_window_add_accel_group(GTK_WINDOW(widget), accel_group);

	/* return the menu bar */
	return(gtk_item_factory_get_widget(item_factory, "<main>"));
}

/* menu File New response - get dimensions for new window and create it */

int file_new()
{
	int x = DEFAULTX, y = DEFAULTY;
	int tx, ty;
	int boxsize;
	gchar *str;
	gchar *xstr, *ystr, *p;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *stock;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *xentry;
	GtkWidget *yentry;
	long windownumber;
	struct rec *rp;
	gint dialogresponse;

	/* get dimensions */
	dialog = gtk_dialog_new_with_buttons("Dimensions", NULL, GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, "Cancel", GTK_RESPONSE_CANCEL, NULL);

	/* set default response */
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);

	label = gtk_label_new("X");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	xentry = gtk_entry_new();
	str = g_strdup_printf("%d", DEFAULTX);
	gtk_entry_set_text(GTK_ENTRY(xentry), str);
	g_free(str);
	gtk_table_attach_defaults(GTK_TABLE(table), xentry, 1, 2, 0, 1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), xentry);

	label = gtk_label_new("Y");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	yentry = gtk_entry_new();
	str = g_strdup_printf("%d", DEFAULTY);
	gtk_entry_set_text(GTK_ENTRY(yentry), str);
	g_free(str);
	gtk_table_attach_defaults(GTK_TABLE(table), yentry, 1, 2, 1, 2);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), yentry);

	gtk_widget_show_all(dialog);

	dialogresponse = gtk_dialog_run(GTK_DIALOG(dialog));
	if (dialogresponse == GTK_RESPONSE_OK) {
		xstr = gtk_entry_get_text(GTK_ENTRY(xentry));	/* ERROR */
		for (p = xstr, tx = 0; *p; p++) {
			if (*p >= '0' && *p <= '9')
				tx = tx * 10 + *p - '0';
			else {
				tx = 0;
				break;
			}
		}
		ystr = gtk_entry_get_text(GTK_ENTRY(yentry));	/* ERROR */
		for (p = ystr, ty = 0; *p; p++) {
			if (*p >= '0' && *p <= '9')
				ty = ty * 10 + *p - '0';
			else {
				ty = 0;
				break;
			}
		}
		if (tx >= 2 && tx <= MAXX && ty >= 2 && ty <= MAXY) {
			x = tx;
			y = ty;
		} else {
			showerror(NULL, "Dimensions too large");
			dialogresponse = GTK_RESPONSE_CANCEL;
		}
	}

	gtk_widget_destroy(dialog);

	if (dialogresponse != GTK_RESPONSE_OK)
		return(-1);

	/* calculate box size */
	boxsize = screenwidth / (x + 1);
	if ( (screenheight / (y + 1)) < boxsize)
		boxsize = screenheight / (y + 1);
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* create new window */
	windownumber = new_window((x + 1) * boxsize, (y + 1) * boxsize + 2, NULL);
	/* set up initial grid */
	rp = &windows[windownumber].rec;
	rp->x = x;
	rp->y = y;
	if ( (rp->grid = (struct grid *)malloc(sizeof(struct grid) * (rp->x + 1) * (rp->y + 1))) == NULL) {
		showerror(windows[windownumber].filename, "malloc failure");
		return(-1);
	}
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		if (x == 0 || y == 0)
			rp->GRID(x, y).flag = -1;
		else
			rp->GRID(x, y).flag = 0;
		rp->GRID(x, y).hsum = 0;
		rp->GRID(x, y).vsum = 0;
	}
	rp->sumx = 0;
	rp->sumy = 0;
	rp->mode = MENU_MODE_ASYMSETUP;
	rp->comments[0] = 0;

	/* draw content */
	draw_crosssum(windownumber);
	windows[windownumber].filename = NULL;
	windows[windownumber].modified = 0;
	windows[windownumber].formatupdate = 0;
	return(windownumber);
}

/* basic new window routine */

static long new_window(int width, int height, char *title)
{
	GtkWidget *menuwindow;
	GtkWidget *button;
	GtkWidget *windowbox;
	GtkWidget *windowboxlabel;
	GtkWidget *mainwindow;
	struct rec *rp;
	long windownumber;
	char ltitle[16];

	for (windownumber = 0; windownumber < NWINDOWS; windownumber++) {
		if (windows[windownumber].widget == (GtkWidget *)0) {
			break;
		}
	}
	if (windownumber >= NWINDOWS) {
		showerror(NULL, "Too many windows");
		return;
	}

	/* top widget */
	windows[windownumber].widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(windows[windownumber].widget), FALSE);
	rp = &windows[windownumber].rec;
	rp->mode = MENU_MODE_ASYMSETUP;
	rp->sumx = 1;
	rp->sumy = 1;
	windows[windownumber].modified = 0;
	windows[windownumber].formatupdate = 0;
	/* title */
	if (title == NULL) {
		sprintf(ltitle, "Window %ld", windownumber + 1);
		title = ltitle;
	}
	gtk_window_set_title(GTK_WINDOW(windows[windownumber].widget), title);
	/* callback on delete-event */
	g_signal_connect(windows[windownumber].widget, "delete_event", G_CALLBACK(delete_event), (gpointer)windownumber);

	/* window box */
	windowbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(windows[windownumber].widget), windowbox);
	gtk_widget_show(windowbox);

	/* create menus */
	menuwindow = createmenus(windownumber);
	/* pack in box */
	gtk_box_pack_start(GTK_BOX(windowbox), menuwindow, FALSE, FALSE, 0);
	gtk_widget_show(menuwindow);

	/* create drawing area */
	windows[windownumber].drawing_area = gtk_drawing_area_new();
	/* set size */
	gtk_widget_set_size_request(windows[windownumber].drawing_area, width, height);
	/* pack in box */
	gtk_box_pack_start(GTK_BOX(windowbox), windows[windownumber].drawing_area, TRUE, TRUE, 0);
	/* display */
	gtk_widget_show(windows[windownumber].drawing_area);

	/* signals to handle backing pixmap */
	g_signal_connect(windows[windownumber].drawing_area, "expose_event", G_CALLBACK(expose_event), (gpointer)windownumber);
	g_signal_connect(windows[windownumber].drawing_area, "configure_event", G_CALLBACK(configure_event), (gpointer)windownumber);

	/* signals to handle events */
	g_signal_connect(windows[windownumber].drawing_area, "button_press_event", G_CALLBACK(button_press_event), (gpointer)windownumber);
	g_signal_connect(windows[windownumber].widget, "key_press_event", G_CALLBACK(key_press_event), (gpointer)windownumber);
	gtk_widget_set_events(windows[windownumber].drawing_area, GDK_EXPOSURE_MASK
		| GDK_LEAVE_NOTIFY_MASK
		| GDK_KEY_PRESS_MASK
		| GDK_BUTTON_PRESS_MASK);
	/* display */
	gtk_widget_show(windows[windownumber].widget);

	/* clear filename */
	windows[windownumber].filename = NULL;
	windows[windownumber].modified = 0;
	windows[windownumber].formatupdate = 0;

	return(windownumber);
}

/* File Open handler */

static void file_open(long windownumber)
{
	GtkWidget *dialog;
	char *path;
	char *p;
	char *file = 0;
	long newwindownumber;

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(windows[windownumber].widget), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (windows[windownumber].filename) {
		path = strdup(windows[windownumber].filename);
		for (p = path; *p; p++)
			if (*p == '/')
				file = p;
		if (file) {
			*file = 0;
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
		}
		free(path);
	}
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		windows[windownumber].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		newwindownumber =  open_by_file(windows[windownumber].filename);
		gtk_window_set_transient_for(GTK_WINDOW(windows[newwindownumber].widget), GTK_WINDOW(windows[windownumber].widget));
	}
	gtk_widget_destroy(dialog);
}

/* File Save handler */

static void file_save(long windownumber)
{
	FILE *fp;
	struct rec *rp;
	int x, y;

	if (windows[windownumber].filename == NULL) {
		file_saveas(windownumber);
		return;
	}

	if ((fp = fopen(windows[windownumber].filename, "w")) == NULL) {
		showerror(windows[windownumber].filename, "Cannot create");
		return;
	}

	rp = &windows[windownumber].rec;

	/* header */
	if (fprintf(fp, "CS\n") < 0) {
		showerror(windows[windownumber].filename, "Write failure");
		fclose(fp);
		return;
	}

	/* x and y */
	if (fprintf(fp, "%d %d\n", rp->x, rp->y) < 0) {
		showerror(windows[windownumber].filename, "Write failure");
		fclose(fp);
		return;
	}

	/* mode */
	if (fprintf(fp, "%d\n", rp->mode) < 0) {
		showerror(windows[windownumber].filename, "Write failure");
		fclose(fp);
		return;
	}

	/* flagged squares */
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		/* skip empty squares */
		if (rp->GRID(x, y).flag >= 0)
			continue;
		if (fprintf(fp, "%d %d %d %d\n", x, y, rp->GRID(x, y).hsum, rp->GRID(x, y).vsum) < 0) {
			showerror(windows[windownumber].filename, "Write failure");
			fclose(fp);
			return;
		}
	}

	windows[windownumber].modified = 0;
	windows[windownumber].formatupdate = 0;
	fclose(fp);
}

/* File SaveAs handler (also invoked if new not yet saved) */

static void file_saveas(long windownumber)
{
	GtkWidget *dialog;
	char *str;
	char ltitle[16];

	dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(windows[windownumber].widget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	if (windows[windownumber].filename == NULL) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), ".");
		sprintf(ltitle, "Window %ld", windownumber + 1);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), ltitle);
	} else
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), windows[windownumber].filename);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		windows[windownumber].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		file_save(windownumber);
	}

	gtk_widget_destroy(dialog);
}

/* byte reverse handler (from Motorola 68K created files) */

#define flip(v)	((v >> 8) | ((v & 0xFF) << 8))

/* open existing file and extract data */

static int open_by_file(char *name)
{
	FILE *fp;
	short x, y;
	short hsum, vsum;
	int boxsize;
	int windownumber = 0;
	struct rec *rp;
	char *p, *sname;
	char buffer[2048];

	/* open file */
	if ((fp = fopen(name, "r")) == NULL) {
		showerror(name, "Cannot open");
		return(-1);
	}

	/* check file type */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) || strcmp(buffer, "CS\n")) {
		showerror(name, "Not Checksums file");
		fclose(fp);
		return(-1);
	}

	/* read x and y */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		showerror(name, "Read failure");
		fclose(fp);
		return(-1);
	}
	p = ncvt(p, &x);
	p = ncvt(p, &y);
	if (*p != '\n' || x > MAXX || y > MAXY) {
		showerror(name, "Bad file");
		fclose(fp);
		return(-1);
	}

	/* calculate box size */
	boxsize = screenwidth / (x + 1);
	if ( (screenheight / (y + 1)) < boxsize)
		boxsize = screenheight / (y + 1);
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	/* get window */
	sname = name;
	for (p = name; *p; p++) {
		if (*p == '/')
			sname = p+1;
	}
	windownumber = new_window((x + 1) * boxsize, (y + 1) * boxsize + 2, sname);

	/* and enter x & y sizes */
	rp = &windows[windownumber].rec;
	rp->x = x;
	rp->y = y;

	/* read mode */
	if ( !(p = fgets(buffer, sizeof(buffer), fp)) ) {
		showerror(name, "Bad file");
		fclose(fp);
		return(-1);
	}
	p = ncvt(p, &rp->mode);

	/* allocate grid */
	if ( (rp->grid = (struct grid *)malloc(sizeof(struct grid) * (rp->x + 1) * (rp->y + 1))) == NULL) {
		showerror(name, "malloc failure");
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

	/* bring to top */
	gtk_window_present(GTK_WINDOW(windows[windownumber].widget));

	/* draw content */
	draw_crosssum(windownumber);

	/* set filename */
	windows[windownumber].filename = strdup(name);

	return(windownumber);
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

/* draw it */

static void draw_crosssum(long windownumber)
{
	int x, y;
	int boxsize;
	struct rec *rp;
	gchar *str;
	PangoLayout *layout;
	PangoContext *context;
	PangoFontDescription *desc;

	rp = &windows[windownumber].rec;

	/* calculate box size */
	boxsize = screenwidth / (rp->x + 1);
	if ( (screenheight / (rp->y + 1)) < boxsize)
		boxsize = screenheight / (rp->y + 1);
	if (boxsize > BOXMAX)
		boxsize = BOXMAX;

	context = gdk_pango_context_get();
	layout = pango_layout_new(context);
	desc = pango_font_description_from_string("sans 10");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

	/* gray cells */
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		if (rp->GRID(x, y).flag < 0) {
			/* gray box */
			gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->dark_gc[0], TRUE, x * boxsize, y * boxsize, boxsize, boxsize);

			/* diagonal line */
			if ( (x < rp->x && rp->GRID(x + 1, y).flag >= 0) || (y < rp->y && rp->GRID(x, y + 1).flag >= 0) )
				gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize, y * boxsize, (x + 1) * boxsize, (y + 1) * boxsize);

			/* in sums enter mode, highlight entry cell */
			if (rp->mode == MENU_MODE_SUMS && rp->sumx == x && rp->sumy == y) {
				if (rp->topbottom) {
					gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->white_gc, TRUE, x * boxsize + boxsize/2, y * boxsize, boxsize/2, boxsize/2);
				} else {
					gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->white_gc, TRUE, x * boxsize, y * boxsize + boxsize/2, boxsize/2, boxsize/2);
				}
			}

			/* xsum */
			if ( (x < rp->x && rp->GRID(x, y).hsum && rp->GRID(x + 1, y).flag >= 0) ) {
				if (rp->GRID(x, y).hsum < 10)
					str = g_strdup_printf(" %d", rp->GRID(x, y).hsum);
				else
					str = g_strdup_printf("%d", rp->GRID(x, y).hsum);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				gdk_draw_layout(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize + boxsize/2, y * boxsize + boxsize*0/16, layout);
			}

			/* ysum */
			if ( (y < rp->y && rp->GRID(x, y).vsum && rp->GRID(x, y + 1).flag >= 0) ) {
				if (rp->GRID(x, y).vsum < 10)
					str = g_strdup_printf(" %d", rp->GRID(x, y).vsum);
				else
					str = g_strdup_printf("%d", rp->GRID(x, y).vsum);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				gdk_draw_layout(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize + boxsize*0/16, y * boxsize + boxsize*9/16, layout);
			}
		} else {
			gdk_draw_rectangle(windows[windownumber].pixmap, windows[windownumber].widget->style->white_gc, TRUE, x * boxsize, y * boxsize, boxsize, boxsize);
			if (rp->GRID(x, y).flag != 0) {
				if (rp->GRID(x, y).flag == 0x7F)
					str = g_strdup_printf("--");
				else
					str = g_strdup_printf("%d", rp->GRID(x, y).flag);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				gdk_draw_layout(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize + boxsize/4, y * boxsize + boxsize/4, layout);
			}
		}
	}

	/* vertical lines */
	for (x = 0; x <= rp->x + 1; x++) {
		gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, x * boxsize, 0, x * boxsize, boxsize * (rp->y + 1));
	}

	/* horizontal lines */
	for (y = 0; y <= rp->y + 1; y++) {
		gdk_draw_line(windows[windownumber].pixmap, windows[windownumber].widget->style->black_gc, 0, y * boxsize, boxsize * (rp->x + 1), y * boxsize);
	}

	/* flag entire area for redraw */
	gtk_widget_queue_draw_area(windows[windownumber].widget, 0, 0, windows[windownumber].widget->allocation.width, windows[windownumber].widget->allocation.height);
}

/* ask if you want to save an altered window */

void savewindow(long windownumber)
{
	GtkWidget *dialog;
	GtkResponseType result;
	char lmessage[2048];

	if (windows[windownumber].modified == 0 && windows[windownumber].formatupdate) {
		file_save(windownumber);
		return;
	}

	if (windows[windownumber].filename)
		sprintf(lmessage, "%s has changed, save?", windows[windownumber].filename);
	else
		sprintf(lmessage, "Window %ld has changed, save?", windownumber + 1);
	dialog = gtk_message_dialog_new(GTK_WINDOW(windows[windownumber].widget), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, lmessage);	/* ERROR */
	gtk_window_set_title(GTK_WINDOW(dialog), "Save?");
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (result == GTK_RESPONSE_YES || result == GTK_RESPONSE_ACCEPT)
		if (windows[windownumber].filename)
			file_save(windownumber);
		else
			file_saveas(windownumber);
}

/* error dialog handler */

void showerror(char *windowname, char *message)
{
	GtkWidget *dialog;
	char *lmessage;

	if (windowname != NULL) {
		lmessage = (char *)malloc(strlen(windowname) + strlen(message) + 4);
		sprintf(lmessage, "%s: %s", windowname, message);
	} else {
		lmessage = (char *)malloc(strlen(message) + 4);
		sprintf(lmessage, "%s", message);
	}
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, lmessage);	/* ERROR */
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	free(lmessage);
}

static GtkPrintSettings *printsettings = NULL;

void file_print(long windownumber)
{
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	int npages;

	if (windownumber < 0) {
		npages = 0;
		for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
			if (windows[windownumber].widget)
				npages++;
		windownumber = -1;
	} else {
		npages = 1;
	}
	print = gtk_print_operation_new();
	if (printsettings != NULL)
		gtk_print_operation_set_print_settings(print, printsettings);
	/* next two should probably be in a begin_print callback */
	gtk_print_operation_set_n_pages(print, npages);
	gtk_print_operation_set_unit(print, GTK_UNIT_MM);
	g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), (gpointer)windownumber);
	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (printsettings != NULL)
			g_object_unref(printsettings);
		printsettings = g_object_ref(gtk_print_operation_get_print_settings(print));
	}
	g_object_unref(print);
}

/* print it */

static void draw_page(GtkPrintOperation *operation, GtkPrintContext *printcontext, int page_nr, long windownumber)
{
	int x, y;
	struct rec *rp;
	cairo_t *cr;
	gchar *str;
	PangoLayout *layout;
	PangoContext *pangocontext;
	PangoFontDescription *desc;
	gdouble printboxsize;
	gdouble cwidth, cheight;
	gdouble printborder;
	gdouble printboxmax;
	gdouble printtitleheight;
	gdouble printfontscale = 0.25;
	char *t, *ts;

	if (windownumber < 0) {
		x = 0;
		for (windownumber = 0; windownumber < NWINDOWS; windownumber++)
			if (windows[windownumber].widget)
				if (x++ == page_nr)
					break;
		if (windownumber >= NWINDOWS)
			return;
	}

	rp = &windows[windownumber].rec;

	cr = gtk_print_context_get_cairo_context(printcontext);
	cwidth = gtk_print_context_get_width(printcontext);
	cheight = gtk_print_context_get_height(printcontext);
	printborder = cwidth / 8.5 * PRINTBORDERSIZE;
	printboxmax = cwidth / 8.5 * PRINTBOXMAX;
	printtitleheight = printborder;

	cairo_set_line_width(cr, 0.2);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	/* calculate box size */
	printboxsize = (cwidth - printborder * 2) / (rp->x + 1);
	if ( ((cheight - printtitleheight - printborder * 2) / (rp->y + 1)) < printboxsize)
		printboxsize = (cheight - printtitleheight - printborder * 2) / (rp->y + 1);
	if (printboxsize > printboxmax)
		printboxsize = printboxmax;

	pangocontext = gdk_pango_context_get();
	layout = pango_layout_new(pangocontext);
	str = g_strdup_printf("sans %.1f", printboxsize * printfontscale);
	desc = pango_font_description_from_string(str);
	g_free(str);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	/* title */
	pango_layout_set_width(layout, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	ts = windows[windownumber].filename;
	for (t = windows[windownumber].filename; *t; t++)
		if (*t == '/')
			ts = t;
	/* START DATABASE */
	{
		FILE *fp;
		char str[2048];
		char ostr[2048];

		snprintf(str, sizeof(str), "echo 'SELECT * FROM t_crosssums WHERE xs_name=\"%s\";' | mysql --user=root --password=bar.mysql home", ts);
		if (fp = popen(str, "r")) {
			fgets(str, sizeof(str), fp);
			if (fgets(str, sizeof(str), fp)) {
				dbformat(str, ostr);
				pango_layout_set_text(layout, ostr, -1);
				cairo_move_to(cr, printborder + (cwidth - 2 * printborder) * 2 / 3, printborder);
				pango_cairo_show_layout(cr, layout);
			}
			pclose(fp);
		}
	}
	/* END DATABASE */
	pango_layout_set_text(layout, ts, -1);
	cairo_move_to(cr, printborder, printborder);
	pango_cairo_show_layout(cr, layout);

	pango_layout_set_width(layout, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

	/* boxes */
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		if (rp->GRID(x, y).flag < 0) {
			/* gray box */
			cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
			cairo_rectangle(cr, printborder + x * printboxsize, printborder + printtitleheight + y * printboxsize, printboxsize, printboxsize);
			cairo_fill(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

			/* diagonal line */
			if ( (x < rp->x && rp->GRID(x + 1, y).flag >= 0) || (y < rp->y && rp->GRID(x, y + 1).flag >= 0) ) {
				cairo_move_to(cr, printborder + x * printboxsize, printborder + printtitleheight + y * printboxsize);
				cairo_line_to(cr, printborder + (x + 1) * printboxsize, printborder + printtitleheight + (y + 1) * printboxsize);
				cairo_stroke(cr);
			}

			/* xsum */
			if ( (x < rp->x && rp->GRID(x, y).hsum && rp->GRID(x + 1, y).flag >= 0) ) {
				pango_layout_set_width(layout, printboxsize/2);
				str = g_strdup_printf("%d", rp->GRID(x, y).hsum);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				cairo_move_to(cr, printborder + x * printboxsize + printboxsize*3/4, printborder + printtitleheight + y * printboxsize + printboxsize*0/16);
				pango_cairo_show_layout(cr, layout);
			}

			/* ysum */
			if ( (y < rp->y && rp->GRID(x, y).vsum && rp->GRID(x, y + 1).flag >= 0) ) {
				pango_layout_set_width(layout, printboxsize/2);
				str = g_strdup_printf("%d", rp->GRID(x, y).vsum);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				cairo_move_to(cr, printborder + x * printboxsize + printboxsize*1/4, printborder + printtitleheight + y * printboxsize + printboxsize*9/16);
				pango_cairo_show_layout(cr, layout);
			}

		} else {
			if (rp->GRID(x, y).flag > 0) {
				pango_layout_set_width(layout, printboxsize);
				if (rp->GRID(x, y).flag == 0x7F)
					str = g_strdup_printf("--");
				else
					str = g_strdup_printf("%d", rp->GRID(x, y).flag);
				pango_layout_set_text(layout, str, -1);
				g_free(str);
				cairo_move_to(cr, printborder + x * printboxsize + printboxsize/2, printborder + printtitleheight + y * printboxsize + printboxsize/2);
				pango_cairo_show_layout(cr, layout);
			}
		}
	}

	/* vertical lines */
	for (x = 0; x <= rp->x + 1; x++) {
		cairo_move_to(cr, printborder + x * printboxsize, printborder + printtitleheight);
		cairo_line_to(cr, printborder + x * printboxsize, printborder + printtitleheight + printboxsize * (rp->y + 1));
		cairo_stroke(cr);
	}

	/* horizontal lines */
	for (y = 0; y <= rp->y + 1; y++) {
		cairo_move_to(cr, printborder, printborder + printtitleheight + y * printboxsize);
		cairo_line_to(cr, printborder + printboxsize * (rp->x + 1), printborder + printtitleheight + y * printboxsize);
		cairo_stroke(cr);
	}
}

void dbformat(char *str, char *ostr)
{
	char *xs_id;
	char *xs_name;
	char *xs_size;
	char *xs_ctime;
	char *xs_4pc;
	char *xs_pc;
	char *xs_mtime;
	char *xs_iterations;
	char *xs_notes;
	char *xs_pending;
	char *istr = str;

	/* remove newline */
	while (*str && *str != '\n') str++;
	if (*str) *str++ = 0;
	str = istr;

	xs_id = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_name = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_size = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_ctime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_4pc = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_pc = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_mtime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_iterations = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_notes = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	xs_pending = str;
	sprintf(ostr, "%s %s:%s:%s %s %s", xs_size, xs_ctime, xs_4pc, xs_pc, xs_mtime, xs_notes);
}

char *commentsfile = "/home/asa/play/crosssums/Ranking";
int commentsinit = 0;
char **comments;
int ncomments;
char *ntimesfile = "/home/asa/play/crosssums/Ntimes";
int ntimesinit = 0;
char **ntimes;
int nntimes;
char *ctimesfile = "/home/asa/play/crosssums/Ctimes";
int ctimesinit = 0;
char **ctimes;
int nctimes;

gchar *getcomments(long windownumber)
{
	FILE *fp;
	char buffer[1024];
	int i;
	char *p;
	char *cptr;
	char *nptr;
	char *filename = NULL;

	if (windows[windownumber].filename == NULL)
		return("");

	/* find local file name */
	for (p = windows[windownumber].filename; *p; p++)
		if (*p == '/')
			filename = p+1;
	if (filename == NULL)
		filename = windows[windownumber].filename;

	if (ntimesinit == 0) {
		/* initialize ntimes from file */
		if ((fp = fopen(ntimesfile, "r")) == NULL)
			return("");
		nntimes = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			nntimes++;
		}
		fclose(fp);
		if ((ntimes = (char **)malloc(nntimes * sizeof(ntimes[0]))) == NULL)
			return("");
		if ((fp = fopen(ntimesfile, "r")) == NULL)
			return("");
		i = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (i > nntimes)
				break;
			for (p = buffer; *p && *p != '\n'; p++) ;
			*p = 0;
			ntimes[i] = strdup(buffer);
			i++;
		}
		fclose(fp);
		ntimesinit++;
	}

	if (commentsinit == 0) {
		/* initialize comments from file */
		if ((fp = fopen(commentsfile, "r")) == NULL)
			return("");
		ncomments = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			ncomments++;
		}
		fclose(fp);
		if ((comments = (char **)malloc(ncomments * sizeof(comments[0]))) == NULL)
			return("");
		if ((fp = fopen(commentsfile, "r")) == NULL)
			return("");
		i = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (i > ncomments)
				break;
			for (p = buffer; *p && *p != '\n'; p++) ;
			*p = 0;
			comments[i] = strdup(buffer);
			i++;
		}
		fclose(fp);
		commentsinit++;
	}

	buffer[0] = 0;

	for (i = 0; i < nntimes; i++) {
		for (p = filename, nptr = ntimes[i]; *p && *nptr && *p == *nptr; p++, nptr++) ;
		if (*p == '-' && *nptr == '\t') {
			sprintf(buffer, "[%s] ", nptr + 1);
			break;
		}
	}

	for (i = 0; i < ncomments; i++) {
		for (p = filename, cptr = comments[i]; *p && *cptr && *p == *cptr; p++, cptr++) ;
		if (*p == '\0' && *cptr == '\t') {
			strcat(buffer, cptr + 1);
			break;
		}
	}
	return(strdup(buffer));
}

gchar *getctimes(long windownumber)
{
	FILE *fp;
	char buffer[1024];
	int i;
	char *p;
	char *cptr;
	char *filename = NULL;

	if (windows[windownumber].filename == NULL)
		return("");

	/* find local file name */
	for (p = windows[windownumber].filename; *p; p++)
		if (*p == '/')
			filename = p+1;
	if (filename == NULL)
		filename = windows[windownumber].filename;

	if (ctimesinit == 0) {
		/* initialize ctimes from file */
		if ((fp = fopen(ctimesfile, "r")) == NULL)
			return("");
		nctimes = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			nctimes++;
		}
		fclose(fp);
		if ((ctimes = (char **)malloc(nctimes * sizeof(ctimes[0]))) == NULL)
			return("");
		if ((fp = fopen(ctimesfile, "r")) == NULL)
			return("");
		i = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (i > nctimes)
				break;
			for (p = buffer; *p && *p != '\n'; p++) ;
			*p = 0;
			for (p = buffer; *p && *p != '\t'; p++) ;
			for (p = p + 1; *p; p++)
				if (*p == '\t')
					*p = ',';
			ctimes[i] = strdup(buffer);
			i++;
		}
		fclose(fp);
		ctimesinit++;
	}

	buffer[0] = 0;
	for (i = 0; i < nctimes; i++) {
		for (p = filename, cptr = ctimes[i]; *p && *cptr && *p == *cptr; p++, cptr++) ;
		if (*p == '\0' && *cptr == '\t') {
			sprintf(buffer, "%s", cptr + 1);
			break;
		}
	}

	return(strdup(buffer));
}

/* in shifting from design to sums entry mode, check to make sure all run lengths are 2-9 boxes long */

int runlengthcheck(long windownumber)
{
	int x, y;
	int t;
	struct rec *rp;
	char m[64];

	rp = &windows[windownumber].rec;

	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		if (rp->GRID(x, y).flag < 0) {
			for (t = x+1; t <= rp->x; t++)
				if (rp->GRID(t, y).flag < 0)
					break;
			t -= x+1;
			if (t == 1 || t > 9) {
				sprintf(m, "Bad horizontal length at %dx%d", x, y);
				showerror(windows[windownumber].filename, m);
				return(-1);
			}
			for (t = y+1; t <= rp->y; t++)
				if (rp->GRID(x, t).flag < 0)
					break;
			t -= y+1;
			if (t == 1 || t > 9) {
				sprintf(m, "Bad vertical length at %dx%d", x, y);
				showerror(windows[windownumber].filename, m);
				return(-1);
			}
		}
	}
	return(0);
}

/* in going from sums entry mode to validate mode, check that sums are valid for each run length and that horizontal and vertical sums total match */

int sumscheck(long windownumber)
{
	int x, y;
	int t;
	struct rec *rp;
	char m[64];
	int hsum, vsum;

	rp = &windows[windownumber].rec;

	hsum = 0;
	vsum = 0;
	for (x = 0; x <= rp->x; x++)
	for (y = 0; y <= rp->y; y++) {
		if (rp->GRID(x, y).flag < 0) {
			for (t = x+1; t <= rp->x; t++)
				if (rp->GRID(t, y).flag < 0)
					break;
			t -= x+1;
			if (t <= 0)
				rp->GRID(x, y).hsum = 0;
			else if (onesumcheck(t, rp->GRID(x, y).hsum)) {
				sprintf(m, "Bad horizontal sum at %dx%d", x, y);
				showerror(windows[windownumber].filename, m);
				rp->sumx = x;
				rp->sumy = y;
				rp->topbottom = 1;
				rp->newsum = 1;
				draw_crosssum(windownumber);
				return(-1);
			}
			hsum += rp->GRID(x, y).hsum;
			for (t = y+1; t <= rp->y; t++)
				if (rp->GRID(x, t).flag < 0)
					break;
			t -= y+1;
			if (t <= 0)
				rp->GRID(x, y).vsum = 0;
			else if (onesumcheck(t, rp->GRID(x, y).vsum)) {
				sprintf(m, "Bad vertical sum at %dx%d", x, y);
				showerror(windows[windownumber].filename, m);
				rp->sumx = x;
				rp->sumy = y;
				rp->topbottom = 0;
				rp->newsum = 1;
				draw_crosssum(windownumber);
				return(-1);
			}
			vsum += rp->GRID(x, y).vsum;
		}
	}
	if (hsum != vsum) {
		showerror(windows[windownumber].filename, "horizontal and vertical sum mismatch");
		return(-1);
	}
	return(0);
}

int onesumcheck(int length, int value)
{
	if (length == 1) {
		return(-1);
	} else if (length == 2) {
		if (value < 3 || value > 17)
			return(-1);
	} else if (length == 3) {
		if (value < 6 || value > 24)
			return(-1);
	} else if (length == 4) {
		if (value < 10 || value > 30)
			return(-1);
	} else if (length == 5) {
		if (value < 15 || value > 35)
			return(-1);
	} else if (length == 6) {
		if (value < 21 || value > 39)
			return(-1);
	} else if (length == 7) {
		if (value < 28 || value > 42)
			return(-1);
	} else if (length == 8) {
		if (value < 35 || value > 44)
			return(-1);
	} else if (length == 9) {
		if (value != 45)
			return(-1);
	}
	return(0);
}

int pointcount;
int fourpointcount;
char *solution = NULL;

/* solve */
void solve(long windownumber)
{
	struct rec *rp = &windows[windownumber].rec;
	int x, y;
	struct tms tms;
	double clocktick = (double)sysconf(_SC_CLK_TCK);
	long long baset, deltat;
	int n;

	times(&tms);
	baset = tms.tms_utime;

	if ((solution = (char *)malloc((rp->x + 1) * (rp->y + 1) * sizeof(char))) == NULL)
	{
		showerror(windows[windownumber].filename, "malloc failure");
		return;
	}
	for (x = 0; x < rp->x + 1; x++)
	for (y = 0; y < rp->y + 1; y++)
		solution[x + y*(rp->x+1)] = (char)0;
	fourpointcount = 0;
	pointcount = 0;
	n = solven(rp, windownumber);
	setsolution(rp);
	free(solution);
	solution = NULL;

	times(&tms);
	deltat = tms.tms_utime - baset;
	if (n > 1) {
		if (insert) {
			/* INSERT */
			char *t, *tt;

			t = windows[windownumber].filename;
			for (tt = t; *tt; tt++)
				if (*tt == '/')
					t = tt + 1;
			fprintf(stderr, "INSERT INTO t_crosssums VALUES (0, '%s', '%dx%d', %.2f, %d, %d, '', '', '%d solutions', 'pending');\n", t, rp->x, rp->y, deltat / clocktick, fourpointcount, pointcount, n);
		} else {
			/* simple printout */
			fprintf(stderr, "%s: TIME: %.2f 4PC: %d PC: %d, %d solutions\n", windows[windownumber].filename, deltat / clocktick, fourpointcount, pointcount, n);
		}
	} else if (n <= 0) {
		if (insert) {
			/* INSERT */
			char *t, *tt;

			t = windows[windownumber].filename;
			for (tt = t; *tt; tt++)
				if (*tt == '/')
					t = tt + 1;
			fprintf(stderr, "INSERT INTO t_crosssums VALUES (0, '%s', '%dx%d', %.2f, %d, %d, '', '', 'no solutions', 'pending');\n", t, rp->x, rp->y, deltat / clocktick, fourpointcount, pointcount);
		} else {
			/* simple printout */
			fprintf(stderr, "%s: TIME: %.2f 4PC: %d PC: %d, no solutions\n", windows[windownumber].filename, deltat / clocktick, fourpointcount, pointcount);
		}
	} else {
		if (insert) {
			/* INSERT */
			char *t, *tt;

			t = windows[windownumber].filename;
			for (tt = t; *tt; tt++)
				if (*tt == '/')
					t = tt + 1;
			fprintf(stderr, "INSERT INTO t_crosssums VALUES (0, '%s', '%dx%d', %.2f, %d, %d, '', '', '', 'pending');\n", t, rp->x, rp->y, deltat / clocktick, fourpointcount, pointcount);
		} else {
			/* simple printout */
			fprintf(stderr, "%s: TIME: %.2f 4PC: %d PC: %d\n", windows[windownumber].filename, deltat / clocktick, fourpointcount, pointcount);
		}
	}
}

/* top level solver */
int solven(struct rec *rp, long windownumber)
{
	int n;
	char message[32];

	/* generate possible single sum solutions */
	gensoln(rp);

	/* eliminate possible solutions on a 4 point basis */
	fourpointeliminate(rp);

	n = solutions(rp, 0, 0);

	if (n == 0)
		showerror(windows[windownumber].filename, "No solution");
	else if (n > 1) {
		sprintf(message, "%d solutions", n);
		showerror(windows[windownumber].filename, message);
	}
	return(n);
}

/* generate all possible single sum solutions */
void gensoln(struct rec *rp)
{
	int x, y;
	int tx, ty;
	int n;
	char xx[10];

	xx[0] = 0;

	/* step through all cells */
	for (y = 0; y <= rp->y; y++)
	for (x = 0; x <= rp->x; x++)
	{
		/* horizontal sum */
		if (rp->GRID(x, y).hsum)
		{
			/* count and enter length */
			for (tx = x + 1; tx <= rp->x; tx++)
				if (rp->GRID(tx, y).flag < 0)
					break;
			if (n = tx - x - 1)
			{
				rp->GRID(x, y).hlng = n;
				/* generate possible solutions */
				rp->GRID(x, y).hptr = gensoln1(n, rp->GRID(x, y).hsum, xx, 1, 0, (struct ptr *)0);
			} else
			{
				/* this is a glitch */
				rp->GRID(x, y).hsum = 0;
			}
		}
		/* vertical sum */
		if (rp->GRID(x, y).vsum)
		{
			/* count and enter length */
			for (ty = y + 1; ty <= rp->y; ty++)
				if (rp->GRID(x, ty).flag < 0)
					break;
			if (n = ty - y - 1)
			{
				rp->GRID(x, y).vlng = n;
				/* generate possible solutions */
				rp->GRID(x, y).vptr = gensoln1(n, rp->GRID(x, y).vsum, xx, 1, 0, (struct ptr *)0);
			} else
			{
				/* this is a glitch */
				rp->GRID(x, y).vsum = 0;
			}
		}
	}
}

/* generate possible solutions for one sum */
struct ptr *gensoln1(int n, int sum, char *xx, int which, int sofar, struct ptr *p)
{
	int i;
	int min, max;

	if (which == n)
	{
		/* last one - must be exact */
		i = sum - sofar;
		if (i >= 1 && i <= 9 && i > xx[which - 1])
		{
			xx[which] = i;
			p = permute(n, 1, xx, p);
			return(p);
		}
		return((struct ptr *)0);
	}
	min = (sum - sofar) - (19 - (n - which)) * (n - which)/2;
	if (min <= xx[which-1])
		min = xx[which-1] + 1;
	max = (sum - sofar - (n - which) * (n - which + 1) / 2) / (n - which + 1);
	if (max > 9 - (n - which))
		max = 9 - (n - which);
	for (i = min; i <= max; i++)
	{
		xx[which] = i;
		p = gensoln1(n, sum, xx, which + 1, sofar + i, p);
	}
	return(p);
}

#define FLIP(a, b)	{int t; t = xx[a]; xx[a] = xx[b]; xx[b] = t; }

struct ptr *permute(int n, int which, char *xx, struct ptr *p)
{
	int i;

	if (n - which == 1)
	{
		p = ladd(p, xx, n);
		FLIP(which, which+1);
		p = ladd(p, xx, n);
		FLIP(which, which+1);
		return(p);
	}

	for (i = n - which + 1; i > 0; --i)
	{
		FLIP(which, which+i-1);
		p = permute(n, which+1, xx, p);
		FLIP(which, which+i-1);
	}
	return(p);
}

struct ptr *ladd(struct ptr *p, char *xx, int n)
{
	struct ptr *tp;
	int i;

	tp = (struct ptr *)malloc(sizeof(struct ptr));
	if (tp == NULL) {
		fprintf(stderr, "malloc failure on size %d\n", n);
		exit(1);
	}
	tp->p_ptr = p;
	for (i = 0; i <= n; i++)
		tp->values[i] = xx[i];
	return(tp);
}

/* eliminate possible solutions on a point basis */
/*
 * The algorithm here is that each point in the grid (x, y)
 * has a corresponding horizontal (tx, y) and vertical (x, ty) sum
 * that includes that point.
 * Step through the possible horizontal solutions and note what values
 *	can occur (xmask)
 * Step through the possible vertical solutions and note what values
 *	can occur (ymask)
 * The intersection (mask) of those values are the only ones that can occur
 * Step back through the horizontal and vertical solutions and eliminate
 *	solutions with (x, y) values that cannot occur
 * Set flag if you delete any solutions
 * If any deletions have occurred, you have to make another pass
 *	because that will affect other cells
 */
pointeliminate(struct rec *rp)
{
	int flag;
	int x, y;
	int tx, ty;
	struct ptr *p, *lp;
	int xmask, ymask, mask;
	int i;

	do
	{
		/* make one pass through the grid */
		pointcount++;
		flag = 0;
		/* step through each "empty" square */
		for (y = 1; y <= rp->y; y++)
		for (x = 1; x <= rp->x; x++)
		{
			if (rp->GRID(x, y).flag < 0)
				continue;
			/* find the x solutions for this point */
			for (tx = x - 1; rp->GRID(tx, y).flag >= 0; --tx)
				;
			/* find the y solutions for this point */
			for (ty = y - 1; rp->GRID(x, ty).flag >= 0; --ty)
				;
			/* step through x solutions looking for possible values */
			xmask = 0;
			i = x - tx;
			for (p = rp->GRID(tx, y).hptr; p; p = p->p_ptr)
			{
				xmask |= 1 << p->values[i];
			}
			/* step through y solutions looking for possible values */
			ymask = 0;
			i = y - ty;
			for (p = rp->GRID(x, ty).vptr; p; p = p->p_ptr)
			{
				ymask |= 1 << p->values[i];
			}
			/* common values */
			mask = xmask & ymask;
			/* eliminate any x solutions that don't work */
			i = x - tx;
			lp = (struct ptr *)&rp->GRID(tx, y).hptr;
			for (p = lp->p_ptr; p; p = lp->p_ptr)
			{
				if ((1 << p->values[i]) & mask)
				{
					lp = p;
				} else
				{
					flag++;
					lp->p_ptr = p->p_ptr;
					free(p);
				}
			}
			/* eliminate any y solutions that don't work */
			i = y - ty;
			lp = (struct ptr *)&rp->GRID(x, ty).vptr;
			for (p = lp->p_ptr; p; p = lp->p_ptr)
			{
				if ((1 << p->values[i]) & mask)
				{
					lp = p;
				} else
				{
					flag++;
					lp->p_ptr = p->p_ptr;
					free(p);
				}
			}
		}
	} while (flag);
}

/* eliminate possible solutions on a four point basis */
/*
 * The algorithm here is more complex and involved 4 points.
 * For each point in the grid (x, y) certain additional points
 *	(dx, dy) may exist such that four sums exist such that:
 *	dx > x, dy > y
 *	sum1 includes points (x, y) and (dx, y) (horizontal)
 *	sum2 includes points (x, y) and (x, dy) (vertical)
 *	sum3 includes points (x, dy) and (dx, dy) (horizontal)
 *	sum4 includes points (dx, y) and (dx, dy) (vertical)
 *	That is, for sums exist that include the 4 points
 * For each such case, look for all possible solutions of the four sums that
 *	will work and delete any solutions that are not included.
 * Set flag if you delete any solutions
 * If any deletions have occurred, you have to make another pass
 *	because that will affect other four points
 *
 * Note that this elimination includes the pointelimination, but the
 *	pointelimination is done first because it is more efficient.
 *
 * Notes:
 *	(x, y) is current point being processed
 *	(dx, dy) is the secondary point
 *	(sx, y) is sum1
 *	(x, sy) is sum2
 *	(rx, dy) is sum3
 *	(dx, ry) is sum4
 */
fourpointeliminate(struct rec *rp)
{
	int flag;
	int x, y;
	int sx, sy;
	int dx, dy;
	int rx, ry;
	struct ptr *p, *lp;
	struct ptr *p1, *p2, *p3, *p4;
	int s1s2x, s1s2y;
	int s1s4x, s1s4y;
	int s3s2x, s3s2y;
	int s3s4x, s3s4y;
	int n1, n2, n3, n4;
	
	do
	{
		/* make one pass through the grid */
		fourpointcount++;

		/* eliminate solutions on a point basis first */
		pointeliminate(rp);

		/* now process on a four point basis */

		flag = 0;
		/* step through each "empty" square */
		for (y = 1; y <= rp->y; y++)
		for (x = 1; x <= rp->x; x++)
		{
			if (rp->GRID(x, y).flag < 0)
				continue;
			/* find sum2 */
			for (sx = x - 1; rp->GRID(sx, y).flag >= 0; --sx)
				;
			/* find sum1 */
			for (sy = y - 1; rp->GRID(x, sy).flag >= 0; --sy)
				;

			/* step through all possible dx, dy */
			for (dy = y+1; dy <= rp->GRID(x, sy).vlng + sy; dy++)
			for (dx = x+1; dx <= rp->GRID(sx, y).hlng + sx; dx++)
			{
				/* find sum4 */
				for (ry = y - 1; rp->GRID(dx, ry).flag >= 0; --ry)
					;
				if (dy > rp->GRID(dx, ry).vlng + ry)
					continue;

				/* find sum3 */
				for (rx = x - 1; rp->GRID(rx, dy).flag >= 0; --rx)
					;
				if (dx > rp->GRID(rx, dy).hlng + rx)
					continue;

				/* calculate all offsets */
				s1s2x = x - sx;
				s1s2y = y - sy;
				s1s4x = dx - sx;
				s1s4y = y - ry;
				s3s2x = x - rx;
				s3s2y = dy - sy;
				s3s4x = dx - rx;
				s3s4y = dy - ry;

				/* clear all of the flags */
				n1 = 0;
				for (p = rp->GRID(sx, y).hptr; p; p = p->p_ptr)
				{
					p->flag = 0;
					n1++;
				}
				n2 = 0;
				for (p = rp->GRID(x, sy).vptr; p; p = p->p_ptr)
				{
					p->flag = 0;
					n2++;
				}
				n3 = 0;
				for (p = rp->GRID(rx, dy).hptr; p; p = p->p_ptr)
				{
					p->flag = 0;
					n3++;
				}
				n4 = 0;
				for (p = rp->GRID(dx, ry).vptr; p; p = p->p_ptr)
				{
					p->flag = 0;
					n4++;
				}

				/* find all solutions and flag the components */
				/* step through all sum1 possibilities */
				for (p1 = rp->GRID(sx, y).hptr; p1; p1 = p1->p_ptr)
				{
					/* for each sum1 possibility, step through sum2
					 * possibilities with matching s1s2 values
					 */
					for (p2 = rp->GRID(x, sy).vptr; p2; p2 = p2->p_ptr)
					{
						if (p1->values[s1s2x] != p2->values[s1s2y])
							continue;
						/* s1s2 matches, step through sum4
						 * possibilities with matching s1s4 values
						 */
						for (p4 = rp->GRID(dx, ry).vptr; p4; p4 = p4->p_ptr)
						{
							if (p1->values[s1s4x] != p4->values[s1s4y])
								continue;
							/* s1s4 matches, step through sum3 possibilities
							 * s3s2 and s3s4 must match
							 */
							for (p3 = rp->GRID(rx, dy).hptr; p3; p3 = p3->p_ptr)
							{
								if (p3->values[s3s2x] != p2->values[s3s2y])
									continue;
								if (p3->values[s3s4x] != p4->values[s3s4y])
									continue;
								/* found a solution, mark all flags */
								p1->flag = 1;
								p2->flag = 1;
								p3->flag = 1;
								p4->flag = 1;
							}
						}
					}
				}

				/* step through all of the solutions and eliminate any without flags */
				/* sum1 */
				lp = (struct ptr *)&rp->GRID(sx, y).hptr;
				for (p = lp->p_ptr; p; p = lp->p_ptr)
				{
					if (p->flag)
					{
						lp = p;
					} else
					{
						flag++;
						lp->p_ptr = p->p_ptr;
						free(p);
					}
				}
				/* sum2 */
				lp = (struct ptr *)&rp->GRID(x, sy).vptr;
				for (p = lp->p_ptr; p; p = lp->p_ptr)
				{
					if (p->flag)
					{
						lp = p;
					} else
					{
						flag++;
						lp->p_ptr = p->p_ptr;
						free(p);
					}
				}
				/* sum3 */
				lp = (struct ptr *)&rp->GRID(rx, dy).hptr;
				for (p = lp->p_ptr; p; p = lp->p_ptr)
				{
					if (p->flag)
					{
						lp = p;
					} else
					{
						flag++;
						lp->p_ptr = p->p_ptr;
						free(p);
					}
				}
				/* sum4 */
				lp = (struct ptr *)&rp->GRID(dx, ry).vptr;
				for (p = lp->p_ptr; p; p = lp->p_ptr)
				{
					if (p->flag)
					{
						lp = p;
					} else
					{
						flag++;
						lp->p_ptr = p->p_ptr;
						free(p);
					}
				}
			}
		}
	} while (flag);
}

/* find all solutions */
/*
 * steps through the remaining possibilites trying all of them completely
 * and prints out all of them that work as well as number of solutions if
 * more than one or NO SOLUTIONS if there are none
 */
solutions(struct rec *rp, int x, int y)
{
	x++;
	if (x > rp->x)
	{
		x = 0;
		y++;
		if (y > rp->y)
		{
			savesolution(rp);
			return(1);
		}
	}
	for (;;)
	{
		if (rp->GRID(x, y).hsum)
		{
			return(xsolutions(rp, x, y));
		} else if (rp->GRID(x, y).vsum)
		{
			return(ysolutions(rp, x, y));
		}
		x++;
		if (x > rp->x)
		{
			x = 0;
			y++;
			if (y > rp->y)
			{
				savesolution(rp);
				return(1);
			}
		}
	}
}

/* find this x solution */
/* (x, y) points to a hsum */
xsolutions(struct rec *rp, int x, int y)
{
	struct ptr *p;
	int i;
	int n = 0;

	/* step through all possible x solutions for this point */
	for (p = rp->GRID(x, y).hptr; p; p = p->p_ptr)
	{
		/* step through each point in the sum */
		for (i = 1; i <= rp->GRID(x, y).hlng; i++)
		{
			if (rp->GRID(x+i, y).flag)
			{
				/* if point is already set, it must match */
				if (rp->GRID(x+i, y).flag != p->values[i])
					break;
			} else
			{
				/* otherwise, set and flag it */
				rp->GRID(x+i, y).flag = p->values[i];
				p->values[i] = -p->values[i];
			}
		}
		if (i > rp->GRID(x, y).hlng)
		{
			/* this solution is a possibility */
			if (rp->GRID(x, y).vsum)
				n += ysolutions(rp, x, y);
			else
				n += solutions(rp, x, y);
		}
		/* reset grid points */
		for (i = 1; i <= rp->GRID(x, y).hlng; i++)
		{
			if (p->values[i] < 0)
			{
				p->values[i] = -p->values[i];
				rp->GRID(x+i, y).flag = 0;
			}
		}
	}
	return(n);
}

/* find this y solution */
/* (x, y) points to a vsum */
ysolutions(struct rec *rp, int x, int y)
{
	struct ptr *p;
	int i;
	int n = 0;

	/* step through all possible y solutions for this point */
	for (p = rp->GRID(x, y).vptr; p; p = p->p_ptr)
	{
		/* step through each point in the sum */
		for (i = 1; i <= rp->GRID(x, y).vlng; i++)
		{
			if (rp->GRID(x, y+i).flag)
			{
				/* if point is already set, it must match */
				if (rp->GRID(x, y+i).flag != p->values[i])
					break;
			} else
			{
				/* otherwise, set and flag it */
				rp->GRID(x, y+i).flag = p->values[i];
				p->values[i] = -p->values[i];
			}
		}
		if (i > rp->GRID(x, y).vlng)
		{
			/* this solution is a possibility */
			n += solutions(rp, x, y);
		}
		/* reset grid points */
		for (i = 1; i <= rp->GRID(x, y).vlng; i++)
		{
			if (p->values[i] < 0)
			{
				p->values[i] = -p->values[i];
				rp->GRID(x, y+i).flag = 0;
			}
		}
	}
	return(n);
}

void savesolution(struct rec *rp)
{
	int x, y;

	for (y = 1; y <= rp->y; y++)
	for (x = 1; x <= rp->x; x++)
	{
		if (rp->GRID(x, y).flag >= 0) {
			if (solution[x + y * (rp->x + 1)] == 0)
				solution[x + y*(rp->x+1)] = (char)rp->GRID(x, y).flag;
			else if (solution[x + y * (rp->x + 1)] != rp->GRID(x, y).flag)
				solution[x + y*(rp->x+1)] = (char)0x7F;
		}
	}
}

void setsolution(struct rec *rp)
{
	int x, y;

	for (y = 1; y <= rp->y; y++)
	for (x = 1; x <= rp->x; x++)
	{
		if (rp->GRID(x, y).flag >= 0)
			rp->GRID(x, y).flag = solution[x + y*(rp->x+1)];
	}
}

short swap(short v)
{
	return((short)((((unsigned short)v >> 8) & 0xFF) | (((unsigned short)v &0xFF) << 8)));
}
