BINARY	:= rapidlauncher

all:
	gcc `pkg-config --cflags gtk+-3.0` RapidLauncher.c `pkg-config --libs gtk+-3.0`  -I../Appman/build/ -L../Appman/build/ -lappman `pkg-config --cflags --libs glib-2.0` -o ../build/$(BINARY) -O3

clean:
	rm ../build/$(BINARY)
	
install:
	install -D "$(RCKTL_DIR)/$(BINARY)" "$(DESTDIR)$(RCKTL_INSTALL_LIB_DIR)/$(BINARY)"

uninstall:
	rm -f "$(DESTDIR)$(RCKTL_INSTALL_LIB_DIR)/$(BINARY)"
