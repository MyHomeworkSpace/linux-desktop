#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#define URL_SCHEME "https"
#define URL_HOST "app.myhomework.space"
#define URL URL_SCHEME "://" URL_HOST

#define CONFIG_FOLDER_NAME "myhomeworkspace"

static GtkWidget * create(WebKitWebView * web_view, WebKitNavigationAction * navigation_action, gpointer user_data)
{
	// we got a request to open a new window
	// decline

	// TODO: should we allow opening another window of MHS, in the app? don't think this comes up naturally

	WebKitURIRequest * request = webkit_navigation_action_get_request(navigation_action);
	const gchar * uri = webkit_uri_request_get_uri(request);
	gtk_show_uri_on_window(NULL, uri, GDK_CURRENT_TIME, NULL);

	return NULL;
}

static bool is_mhs_url(const gchar * uri)
{
	GUri * g_uri = g_uri_parse(uri, (GUriFlags) SOUP_HTTP_URI_FLAGS, NULL);

	// we should intercept if it's NOT a mhs url
	// a MHS url is defined as the scheme and host matching our constant
	bool result = (
		strcmp(g_uri_get_scheme(g_uri), URL_SCHEME) == 0 &&
		strcmp(g_uri_get_host(g_uri), URL_HOST) == 0
	);

	g_uri_unref(g_uri);

	return result;
}

static void decide_policy_with_iframe_result(GObject * object, GAsyncResult * result, gpointer user_data)
{
	WebKitWebView * web_view = WEBKIT_WEB_VIEW(object);

	WebKitNavigationPolicyDecision * navigation_decision = (WebKitNavigationPolicyDecision *) user_data;
	WebKitPolicyDecision * decision = WEBKIT_POLICY_DECISION(navigation_decision);

	WebKitNavigationAction * navigation_action = webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
	WebKitURIRequest * request = webkit_navigation_action_get_request(navigation_action);

	const gchar * uri = webkit_uri_request_get_uri(request);

	GError * error = NULL;
	WebKitJavascriptResult * js_result = webkit_web_view_run_javascript_finish(web_view, result, &error);
	if (!js_result)
	{
		g_warning("Error running iframe js: %s", error->message);
		g_error_free(error);

		webkit_policy_decision_use(decision);
		g_object_unref(decision);
		return;
	}

	JSCValue * value = webkit_javascript_result_get_js_value(js_result);

	// the result is either
	// * null (no iframes are navigating)
	// * the destination url of an iframe that's navigating

	bool is_iframe = false;

	JSCValue * length_value = jsc_value_object_get_property(value, "length");
	gint32 length = jsc_value_to_int32(length_value);
	for (gint32 i = 0; i < length; i++)
	{
		JSCValue * iframe_url_value = jsc_value_object_get_property_at_index(value, i);
		char * iframe_url = jsc_value_to_string(iframe_url_value);

		if (strcmp(uri, iframe_url) == 0)
		{
			is_iframe = true;
			break;
		}
	}

	// at this point, either:
	// * it's a NEW_WINDOW action
	// * it's a NAVIGATION action off of mhs
	// that means we always intercept it, unless it's an iframe

	if (is_iframe)
	{
		// we found out that this is (probably) an iframe request
		// let it go through
		webkit_policy_decision_use(decision);
		g_object_unref(decision);
		return;
	}

	// ok, we should intercept it

	gtk_show_uri_on_window(NULL, uri, GDK_CURRENT_TIME, NULL);

	// block the action
	webkit_policy_decision_ignore(decision);
	g_object_unref(decision);
}

static gboolean decide_policy(WebKitWebView * web_view, WebKitPolicyDecision * decision, WebKitPolicyDecisionType type, gpointer user_data)
{
	if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION || type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
	{
		WebKitNavigationPolicyDecision * navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);

		WebKitNavigationAction * navigation_action = webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
		WebKitURIRequest * request = webkit_navigation_action_get_request(navigation_action);
		const gchar * uri = webkit_uri_request_get_uri(request);

		// TODO: should we allow opening another window of MHS, in the app? don't think this comes up naturally
		if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && is_mhs_url(uri))
		{
			// fast path: let it through
			return FALSE;
		}

		// THIS IS A HACK TO WORK AROUND A WEBKITGTK BUG :(
		// we want to always allow requests in iframes
		// unfortunately, webkit_navigation_policy_decision_get_frame_name is broken (at least in webkitgtk 2.32.4)
		// so, we do this horrible, horrible hack
		// (run some js to find iframes and return their urls. then we can check if our navigation url matches an iframe)
		// (of course, this could break if you click on a link that's the same as an existing iframe, but shhhhh)
		webkit_web_view_run_javascript(
			web_view,
			"(function() {"
				"let r = [];"
				"let f = document.querySelectorAll('iframe');"
				"for (let i = 0; i < f.length; i++) {"
					"r.push(f[i].src);"
				"}"
				"return r;"
			"})();",
			NULL,
			decide_policy_with_iframe_result,
			navigation_decision
		);

		// keep the decision alive while the javascript runs
		g_object_ref(decision);
		return TRUE;
	}

	// default to not blocking the standard policy
	return FALSE;
}

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

	g_signal_connect(web_view, "create", G_CALLBACK(create), NULL);
	g_signal_connect(web_view, "decide-policy", G_CALLBACK(decide_policy), NULL);

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

	GtkApplication * app = gtk_application_new("space.myhomework.desktop",  G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
	g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return 0;
}