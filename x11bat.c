#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/axp20x-battery/capacity"
#define BATTERY_STATUS_PATH "/sys/class/power_supply/axp20x-battery/status"
#define UPDATE_INTERVAL 5
#define WINDOW_WIDTH 200
#define WINDOW_HEIGHT 50

int is_battery_charging() {
    FILE *file = fopen(BATTERY_STATUS_PATH, "r");
    if (!file) {
        perror("Failed to open battery status file");
        return -1;
    }
    char status[16];
    fscanf(file, "%15s", status);
    fclose(file);
    return strcmp(status, "Charging") == 0;
}

int get_battery_capacity() {
    FILE *file = fopen(BATTERY_CAPACITY_PATH, "r");
    if (!file) {
        perror("Failed to open battery capacity file");
        return -1;
    }
    int capacity;
    fscanf(file, "%d", &capacity);
    fclose(file);
    return capacity;
}

void draw_text(Display *display, Window window, GC gc, int screen, const char *text) {
    XClearWindow(display, window);
    XDrawString(display, window, gc, 10, 20, text, strlen(text));
}


void draw_battery(Display *display, Window window, GC gc, int capacity, int charging) {
    // Clear the window
    XClearWindow(display, window);

    // Determine the color based on the battery status
    unsigned long color;
    if (capacity < 15) {
        color = 0xFF0000; // Red color
    } else if (charging) {
        color = 0xFFFF00; // Yellow color
    } else {
        color = 0x00FF00; // Green color
    }

    // Draw the battery level as a rectangle
    int bar_width = (WINDOW_WIDTH - 20) * capacity / 100;
    XSetForeground(display, gc, color);
    XFillRectangle(display, window, gc, 10, 10, bar_width, 30);

    // Draw the battery percentage as text
    char text[20];
    snprintf(text, sizeof(text), "%d%%", capacity);
    XSetForeground(display, gc, 0x112233); // 
    XDrawString(display, window, gc, WINDOW_WIDTH / 2 - 10, WINDOW_HEIGHT / 2 + 5, text, strlen(text));
}

int main() {
    Display *display;
    Window window;
    XEvent event;
    int screen;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    // Set black background and white foreground
    XSetWindowAttributes attributes;
    attributes.background_pixel = BlackPixel(display, screen);
    attributes.border_pixel = BlackPixel(display, screen);

    window = XCreateWindow(display, RootWindow(display, screen), 10, 10,WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                           DefaultDepth(display, screen), InputOutput, DefaultVisual(display, screen),
                           CWBackPixel | CWBorderPixel, &attributes);


    /* set the titlebar name */
    XStoreName(display, window, "x11bat");

    // Set WM_CLASS property
    XClassHint *class_hint = XAllocClassHint();
    if (class_hint) {
        class_hint->res_name = "x11bat";
        class_hint->res_class = "x11bat";
        XSetClassHint(display, window, class_hint);
        XFree(class_hint);
    }

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));
    XSetBackground(display, gc, BlackPixel(display, screen));

    char battery_status[256];

    while (1) {
        while (XPending(display) > 0) {
            XNextEvent(display, &event);
            if (event.type == Expose) {
                int capacity = get_battery_capacity();
                int charging = is_battery_charging();
                if (capacity >= 0) {
                    draw_battery(display, window, gc, capacity, charging);
                }
            }
            if (event.type == KeyPress) {
                XCloseDisplay(display);
                return 0;
            }
        }

        int capacity = get_battery_capacity();
        int charging = is_battery_charging();
        if (capacity >= 0) {
            draw_battery(display, window, gc, capacity, charging);
        }

        sleep(UPDATE_INTERVAL);
    }

    XCloseDisplay(display);
    return 0;
}
