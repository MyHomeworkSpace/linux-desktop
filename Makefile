FLAGS := -g -Wall

WEBKITGTK_CFLAGS := `pkg-config --cflags webkit2gtk-4.1`
WEBKITGTK_LIBS := `pkg-config --libs webkit2gtk-4.1`
GTK3_CFLAGS := `pkg-config --cflags gtk+-3.0`
GTK3_LIBS := `pkg-config --libs gtk+-3.0`

myhomeworkspace: main.cpp
	$(CXX) $(FLAGS) \
		$(WEBKITGTK_CFLAGS) \
		$(GTK3_CFLAGS) \
		-o myhomeworkspace \
		main.cpp \
		$(WEBKITGTK_LIBS) \
		$(GTK3_LIBS)

.PHONY: install

install: myhomeworkspace
	cp myhomeworkspace /usr/local/bin/myhomeworkspace

	mkdir -p /usr/share/myhomeworkspace
	cp icon128.png /usr/share/myhomeworkspace

	desktop-file-install myhomeworkspace.desktop
	update-desktop-database