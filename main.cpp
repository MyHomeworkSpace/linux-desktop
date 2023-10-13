#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#define URL "https://app.myhomework.space"
#define CONFIG_FOLDER_NAME "myhomeworkspace"

static void activate(GtkApplication * app)
{
	char * base_directory = g_build_filename(g_get_user_data_dir(), CONFIG_FOLDER_NAME, NULL);

	g_mkdir_with_parents(base_directory, 0700);

	char * webkit_data_directory = g_build_filename(base_directory, "wkdata", NULL);
	char * webkit_cache_directory = g_build_filename(g_get_user_cache_dir(), CONFIG_FOLDER_NAME, "wkcache", NULL);
	WebKitWebsiteDataManager * website_data_manager = webkit_website_data_manager_new(
		"base-data-directory", webkit_data_directory,
		"base-cache-directory", webkit_cache_directory,
		NULL
	);
	g_free(webkit_data_directory);
	g_free(webkit_cache_directory);

	char * cookie_db_file = g_build_filename(base_directory, "cookies.db", NULL);
	WebKitCookieManager * cookie_manager = webkit_website_data_manager_get_cookie_manager(website_data_manager);
	webkit_cookie_manager_set_persistent_storage(cookie_manager, cookie_db_file, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
	free(cookie_db_file);

	WebKitWebContext * web_context = webkit_web_context_new_with_website_data_manager(website_data_manager);
	g_free(base_directory);

	WebKitSettings * settings = webkit_settings_new();

	webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
	webkit_settings_set_enable_smooth_scrolling(settings, true);
	webkit_settings_set_enable_developer_extras(settings, true);

	WebKitWebView * web_view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
		"web-context", web_context,
		"settings", settings,
	NULL));

	webkit_web_view_load_uri(web_view, URL);

	GtkApplicationWindow * app_window = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
	gtk_window_set_title(GTK_WINDOW(app_window), "MyHomeworkSpace");
	gtk_window_set_default_size(GTK_WINDOW(app_window), 1100, 600);

	gtk_container_add(GTK_CONTAINER(app_window), GTK_WIDGET(web_view));

	gtk_application_add_window(app, GTK_WINDOW(app_window));
	gtk_widget_show_all(GTK_WIDGET(app_window));
}

int main(int argc, char * argv[])
{
	gtk_init(&argc, &argv);

	GtkApplication * app = gtk_application_new("space.myhomework.desktop", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
	g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return 0;
}