#include <gtk/gtk.h>
#include "../Appman/build/appman.h"

#define SPACE 40
#define ICON_SIZE 80
#define ICON_PADDING 10
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 30

static gint screen_width;
static gint screen_height;
static AppmanApp** apps = NULL;
static int apps_count = 0;
static GtkApplication *application;
static GtkWidget *main_window;
static int keep_in_memory = FALSE;
static GtkWidget *search_entry;

static void activate (GApplication *app, gpointer user_data);

void set_widget_color(GtkWidget *widget, float red, float green, float blue, float opacity) {
	GdkRGBA bgcolor = {.0, .0, .1, 1.0}; //Default color
	if (red || green || blue || opacity) {
		bgcolor.red = red;
		bgcolor.green = green;
		bgcolor.blue = blue;
		bgcolor.alpha = opacity;
	}
	gchar *css;
	GtkCssProvider *provider;
	provider = gtk_css_provider_new ();
	css = g_strdup_printf ("* { background-color: %s; }", gdk_rgba_to_string (&bgcolor));
	gtk_css_provider_load_from_data (provider, css, -1, NULL);
	g_free (css);
	gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref (provider);
}

gboolean resize_image(GtkWidget *widget, int width, int height) {
	GdkPixbuf *pixbuf =	gtk_image_get_pixbuf(GTK_IMAGE(widget));
	if (pixbuf == NULL)
	{
		g_printerr("Failed to resize image\n");
		return FALSE;
	}
	GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled_pixbuf);
	
	return TRUE;
}

void resize_text(gchar* str, int limit) {
	if (strlen(str) > limit) {
		str[limit-2] = '.';
		str[limit-1] = '.';
		str[limit] = '\0';
	}
}

/* Signal Handlers */

static gboolean on_icon_mouse_enter_callback (GtkWidget *widget, GdkEvent *event, gpointer data) {
    set_widget_color((GtkWidget*)widget, 0.1, 0.5, .8, 1.0); //sky-blue
    return FALSE;
}


static gboolean on_icon_mouse_leave_callback (GtkWidget *widget, GdkEvent *event, gpointer data) {
    set_widget_color((GtkWidget*)widget, 0, 0, 0, 0); //set default color
    return FALSE;
}

static gboolean on_icon_button_press_callback (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	appman_app_start((AppmanApp*)data);
	keep_in_memory == TRUE ? gtk_widget_hide (main_window) : g_application_quit (application);
	return TRUE;
}

static gboolean on_window_key_press_callback (GtkSearchEntry *searchentry, GdkEvent *event, gpointer data)
{
	if (((GdkEventKey*)event)->keyval == GDK_KEY_Escape)
		keep_in_memory == TRUE ? gtk_widget_hide (main_window) : g_application_quit (application);
	return FALSE;
}

static gboolean on_icon_key_press_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (((GdkEventKey*)event)->keyval == GDK_KEY_Return) {
		appman_app_start((AppmanApp*)data);
		keep_in_memory == TRUE ? gtk_widget_hide (main_window) : g_application_quit (application);
	}
	return FALSE;
}

static gboolean on_search_entry_key_release_callback (GtkSearchEntry *searchentry, GdkEvent *event, gpointer data)
{
	gint keyval = ((GdkEventKey*)event)->keyval;
	if (((keyval >= GDK_KEY_A) && (keyval <= GDK_KEY_Z)) || ((keyval >= GDK_KEY_0) && (keyval <= GDK_KEY_9)) || ((keyval >= GDK_KEY_a) && (keyval <= GDK_KEY_z)) || (keyval == GDK_KEY_BackSpace) || (keyval == GDK_KEY_Cancel)) {
		gint i, row;
		gint columns = screen_width/(SPACE+ICON_SIZE) - 2;
		/* I'm not using TRIE or binary search because in general there is a small number of installed application,
		 * so the efficiency is good enought also with this simple code.
		 */
		const gchar *query = gtk_entry_get_text((GtkEntry*)searchentry);
		AppmanApp *app;
		for (i = 0; i < apps_count; i++) {
			row = i / columns;
			GtkWidget *child = gtk_grid_get_child_at((GtkGrid*)data, i - (row * columns), row);
			gchar *name = appman_app_get_name (apps[i]);
			if (!(g_str_has_prefix ( g_utf8_strdown ( name, strlen(name) ), g_utf8_strdown (query, strlen(query))))) {
				gtk_widget_hide (child);
			} else {
				gtk_widget_show (child);
				if (strlen(name) == strlen(query))
					gtk_widget_grab_focus (child);
			}
		}
	}
	return FALSE;
}

void add_application(GtkGrid *grid, AppmanApp *app, int i, int j) {
	GtkWidget *event_box = gtk_event_box_new ();
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *image = gtk_image_new_from_file (appman_app_get_icon_path (app));
	resize_image(image, ICON_SIZE, ICON_SIZE);
	gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, ICON_PADDING);
	gchar *str = appman_app_get_name (app);
	resize_text(str, 15);
	GtkWidget *label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), g_strconcat("<span color='#FFFFFF'>",str,"</span>", NULL));
	g_free(str);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (event_box), box);
	gtk_widget_set_can_focus (event_box, TRUE);
	gtk_grid_attach(GTK_GRID(grid), event_box, j, i, 1, 1);
	g_signal_connect (G_OBJECT (event_box), "button-press-event", G_CALLBACK (on_icon_button_press_callback), (gpointer) app);
	g_signal_connect (G_OBJECT (event_box), "enter-notify-event", G_CALLBACK (on_icon_mouse_enter_callback), NULL);
	g_signal_connect (G_OBJECT (event_box), "leave-notify-event", G_CALLBACK (on_icon_mouse_leave_callback), NULL);
	g_signal_connect (G_OBJECT (event_box), "key-press-event", G_CALLBACK (on_icon_key_press_callback), (gpointer) app);
	g_signal_connect (G_OBJECT (event_box), "focus-in-event", G_CALLBACK (on_icon_mouse_enter_callback), NULL); //Same as mouse events
	g_signal_connect (G_OBJECT (event_box), "focus-out-event", G_CALLBACK (on_icon_mouse_leave_callback), NULL);
}

void add_applications(GtkGrid *grid) {
	int columns = screen_width/(SPACE+ICON_SIZE) - 2;
	AppmanApplicationHandler *apphandl = appman_application_handler_new ();
	apps = appman_application_handler_get_apps (apphandl, &apps_count);
	int j = 0;
	int t = apps_count/columns;
	int i = 0;
	int x = 0;
	for (j = 0; j < columns; j++) gtk_grid_insert_column (grid, j);
	for (i=0; i < t; i++) {
		gtk_grid_insert_row (grid, i);
		for (j = 0; j < columns; j++) {
			add_application(grid, apps[x], i, j);
			x++;
		}
	}
	//reminder of division
	j = apps_count % columns;
	gtk_grid_insert_row (grid, i);
	for (x = 0; x < j; x++) {
		add_application(grid, apps[x+i*columns], i, x);
	}
	
	g_object_unref (apphandl);
}


/** Maybe they aren't so useful
void add_filter_buttons (GtkWidget *box, GtkWidget *grid) {
	GtkWidget *btn_utility = gtk_button_new_with_label ("Utility");
	gtk_widget_set_size_request (btn_utility, BUTTON_WIDTH, BUTTON_HEIGHT);
	g_signal_connect (G_OBJECT (btn_utility), "clicked", G_CALLBACK (on_button_clicked_callback), (gpointer) grid);
	GtkWidget *btn_audiovideo = gtk_button_new_with_label ("AudioVideo");
	gtk_widget_set_size_request (btn_audiovideo, BUTTON_WIDTH, BUTTON_HEIGHT);
	GtkWidget *btn_game = gtk_button_new_with_label ("Game");
	gtk_widget_set_size_request (btn_game, BUTTON_WIDTH, BUTTON_HEIGHT);
	GtkWidget *btn_network = gtk_button_new_with_label ("Network");
	gtk_widget_set_size_request (btn_network, BUTTON_WIDTH, BUTTON_HEIGHT);
	GtkWidget *btn_office = gtk_button_new_with_label ("Office");
	gtk_widget_set_size_request (btn_office, BUTTON_WIDTH, BUTTON_HEIGHT);
	GtkWidget *btn_development = gtk_button_new_with_label ("Development");
	gtk_widget_set_size_request (btn_development, BUTTON_WIDTH, BUTTON_HEIGHT);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
	gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (hbox), btn_utility, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), btn_audiovideo, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), btn_game, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), btn_network, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), btn_office, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), btn_development, FALSE, FALSE, 0);
	gtk_widget_set_margin_top(hbox, 20);
	gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
}
*/

/* Function called only the first time that the application is open if keep_in_memory is set to yes */
static void startup (GApplication *app, gpointer user_data) {
	main_window = gtk_application_window_new (app);
	/* Get the size of the screen */
	GdkScreen *screen = gdk_screen_get_default ();
	screen_width = gdk_screen_get_width (screen);
	screen_height = gdk_screen_get_height (screen);
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_min_content_height ((GtkScrolledWindow*)scrolled_window, screen_height-80 );
	search_entry = gtk_search_entry_new ();
	gtk_widget_set_margin_top(search_entry, 20);
	gtk_widget_set_margin_start(search_entry, screen_width-800);
	gtk_widget_set_margin_end(search_entry, screen_width-800);
	GtkWidget *grid = gtk_grid_new ();
	gtk_grid_set_row_spacing((GtkGrid*)grid, SPACE);
	gtk_grid_set_column_spacing((GtkGrid*)grid, SPACE);
	add_applications((GtkGrid*)grid);
	set_widget_color(main_window, 0, 0, 0, 0);
	gtk_window_set_decorated(GTK_WINDOW(main_window), FALSE);
	gtk_window_fullscreen (GTK_WINDOW(main_window));
	gtk_window_set_modal(GTK_WINDOW(main_window), TRUE);
	gtk_window_set_title(GTK_WINDOW(main_window), "RapidLauncher");
	
	//add_filter_buttons (box, grid); DISABLED
	gtk_box_pack_start (GTK_BOX (box), search_entry, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER(scrolled_window), grid);
	gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (box), scrolled_window, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (main_window), box);
	
	gtk_widget_set_events(main_window, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK);

	g_signal_connect (G_OBJECT (main_window), "key-press-event", G_CALLBACK (on_window_key_press_callback), NULL);
	g_signal_connect (G_OBJECT (search_entry), "key-release-event", G_CALLBACK (on_search_entry_key_release_callback), grid);
	g_signal_connect (application, "activate", G_CALLBACK (activate), grid);
	
	gtk_widget_grab_focus (search_entry);
	gtk_widget_show_all (main_window);
	
}

static void activate (GApplication *app, gpointer user_data) {
	if (keep_in_memory == TRUE) {
		gtk_widget_show (main_window);
		gtk_window_get_focus (main_window);
		gtk_widget_grab_focus (search_entry);
		gtk_entry_set_text((GtkEntry*)search_entry, "");
		gint i, row;
		gint columns = screen_width/(SPACE+ICON_SIZE) - 2;
		for (i = 0; i < apps_count; i++) {
			row = i / columns;
			GtkWidget *child = gtk_grid_get_child_at((GtkGrid*)user_data, i - (row * columns), row);
			gtk_widget_show (child);
		}
	} else
		startup (app, user_data);
}

int main (int argc, char **argv) {
	int status;
	/* Check arguments */
	if ((argc > 1) && (g_strcmp0(argv[1], "inmem") == 0)) {
		keep_in_memory = TRUE;
	}
	application = gtk_application_new ("com.rapidlauncher", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (application, "startup", G_CALLBACK (startup), NULL);
	status = g_application_run (G_APPLICATION (application), argc, argv);
	g_object_unref (application);
	return status;
}
