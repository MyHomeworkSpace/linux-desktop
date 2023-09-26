#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#define URL "https://app.myhomework.space"

static void activate(GtkApplication * app)
{
	GtkApplicationWindow * app_window = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
	gtk_window_set_title(GTK_WINDOW(app_window), "MyHomeworkSpace");

	WebKitSettings * settings = webkit_settings_new();

	webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
	webkit_settings_set_enable_developer_extras(settings, true);

	WebKitWebView * web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_settings(settings));

	webkit_web_view_load_uri(web_view, URL);

	gtk_container_add(GTK_CONTAINER(app_window), GTK_WIDGET(web_view));

	gtk_application_add_window(app, GTK_WINDOW(app_window));
	gtk_widget_show_all(GTK_WIDGET(app_window));
}

int main(int argc, char * argv[])
{
	gtk_init(&argc, &argv);

	GtkApplication * app = gtk_application_new("space.myhomework.desktop", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
	g_application_run(G_APPLICATION(app), 0, NULL);
	g_object_unref(app);

	return 0;
}