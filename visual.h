#ifndef EMARALD_VISUAL_H
#define EMARALD_VISUAL_H

#include "interpreter.h"

/* ==================== Visualization Module ==================== */

/* Plot types */
typedef enum {
    PLOT_LINE,
    PLOT_SCATTER,
    PLOT_BAR,
    PLOT_HISTOGRAM,
    PLOT_HEATMAP,
} PlotType;

/* Color definitions */
typedef enum {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_GRAY,
} PlotColor;

/* Figure/Plot context */
typedef struct {
    PlotType type;
    char title[256];
    char xlabel[256];
    char ylabel[256];
    PlotColor color;
    int width;
    int height;
    bool grid_enabled;
    bool has_legend;
    char* output_file;
} PlotContext;

/* ==================== Public API ==================== */

/* Figure management */
PlotContext* visual_create_figure(int width, int height);
void visual_free_figure(PlotContext* fig);

/* Plot functions */
void visual_plot(PlotContext* fig, Value x_data, Value y_data);
void visual_scatter(PlotContext* fig, Value x_data, Value y_data);
void visual_bar(PlotContext* fig, Value categories, Value values);
void visual_histogram(PlotContext* fig, Value data, int bins);
void visual_heatmap(PlotContext* fig, Value data);

/* Plot configuration */
void visual_set_title(PlotContext* fig, const char* title);
void visual_set_xlabel(PlotContext* fig, const char* label);
void visual_set_ylabel(PlotContext* fig, const char* label);
void visual_set_color(PlotContext* fig, PlotColor color);
void visual_enable_grid(PlotContext* fig, bool enable);
void visual_enable_legend(PlotContext* fig, bool enable);

/* Output */
void visual_show(PlotContext* fig);
void visual_save(PlotContext* fig, const char* filename, int dpi);

/* Utility functions */
PlotColor visual_color_from_string(const char* color_name);
const char* visual_color_to_string(PlotColor color);

#endif /* EMARALD_VISUAL_H */
