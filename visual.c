#include "visual.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ==================== Visualization Module Implementation ==================== */

PlotContext* visual_create_figure(int width, int height) {
    PlotContext* fig = malloc(sizeof(PlotContext));
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Memory allocation failed for figure");
        return NULL;
    }
    
    memset(fig, 0, sizeof(PlotContext));
    fig->width = width > 0 ? width : 800;
    fig->height = height > 0 ? height : 600;
    fig->color = COLOR_BLUE;
    fig->grid_enabled = true;
    fig->has_legend = false;
    fig->type = PLOT_LINE;
    
    return fig;
}

void visual_free_figure(PlotContext* fig) {
    if (!fig) return;
    
    if (fig->output_file) {
        free(fig->output_file);
        fig->output_file = NULL;
    }
    
    free(fig);
}

/* Plot functions */
void visual_plot(PlotContext* fig, Value x_data, Value y_data) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    if (x_data.type != VAL_ARRAY || y_data.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "Plot data must be arrays");
        return;
    }
    
    fig->type = PLOT_LINE;
}

void visual_scatter(PlotContext* fig, Value x_data, Value y_data) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    if (x_data.type != VAL_ARRAY || y_data.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "Scatter data must be arrays");
        return;
    }
    
    fig->type = PLOT_SCATTER;
}

void visual_bar(PlotContext* fig, Value categories, Value values) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    if (categories.type != VAL_ARRAY || values.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "Bar plot data must be arrays");
        return;
    }
    
    fig->type = PLOT_BAR;
}

void visual_histogram(PlotContext* fig, Value data, int bins) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    if (data.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "Histogram data must be an array");
        return;
    }
    
    if (bins <= 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid bin count: %d", bins);
        return;
    }
    
    fig->type = PLOT_HISTOGRAM;
}

void visual_heatmap(PlotContext* fig, Value data) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    if (data.type != VAL_ARRAY) {
        error_report(ERR_TYPE, 0, "Heatmap data must be an array");
        return;
    }
    
    fig->type = PLOT_HEATMAP;
}

/* Plot configuration */
void visual_set_title(PlotContext* fig, const char* title) {
    if (!fig || !title) return;
    strncpy(fig->title, title, sizeof(fig->title) - 1);
    fig->title[sizeof(fig->title) - 1] = '\0';
}

void visual_set_xlabel(PlotContext* fig, const char* label) {
    if (!fig || !label) return;
    strncpy(fig->xlabel, label, sizeof(fig->xlabel) - 1);
    fig->xlabel[sizeof(fig->xlabel) - 1] = '\0';
}

void visual_set_ylabel(PlotContext* fig, const char* label) {
    if (!fig || !label) return;
    strncpy(fig->ylabel, label, sizeof(fig->ylabel) - 1);
    fig->ylabel[sizeof(fig->ylabel) - 1] = '\0';
}

void visual_set_color(PlotContext* fig, PlotColor color) {
    if (!fig) return;
    fig->color = color;
}

void visual_enable_grid(PlotContext* fig, bool enable) {
    if (!fig) return;
    fig->grid_enabled = enable;
}

void visual_enable_legend(PlotContext* fig, bool enable) {
    if (!fig) return;
    fig->has_legend = enable;
}

/* Output functions */
void visual_show(PlotContext* fig) {
    if (!fig) {
        error_report(ERR_RUNTIME, 0, "Invalid figure context");
        return;
    }
    
    /* Generate SVG or write to terminal */
    fprintf(stdout, "[Plot: %s (%d x %d px)]\n", fig->title, fig->width, fig->height);
}

void visual_save(PlotContext* fig, const char* filename, int dpi) {
    if (!fig || !filename) {
        error_report(ERR_IO, 0, "Invalid figure or filename");
        return;
    }
    
    if (dpi <= 0) {
        error_report_formatted(ERR_RUNTIME, 0, "Invalid DPI: %d", dpi);
        return;
    }
    
    /* Save to file (PNG, SVG, PDF, etc.) */
    FILE* f = fopen(filename, "wb");
    if (!f) {
        error_report_formatted(ERR_IO, 0, "Cannot create file: %s", filename);
        return;
    }
    
    /* Write file header */
    fprintf(f, "# Emarald Plot\n");
    fprintf(f, "# Title: %s\n", fig->title);
    fprintf(f, "# Size: %dx%d px\n", fig->width, fig->height);
    fprintf(f, "# DPI: %d\n", dpi);
    
    fclose(f);
}

/* Color utilities */
PlotColor visual_color_from_string(const char* color_name) {
    if (!color_name) return COLOR_BLUE;
    
    if (strcmp(color_name, "red") == 0) return COLOR_RED;
    if (strcmp(color_name, "green") == 0) return COLOR_GREEN;
    if (strcmp(color_name, "blue") == 0) return COLOR_BLUE;
    if (strcmp(color_name, "yellow") == 0) return COLOR_YELLOW;
    if (strcmp(color_name, "cyan") == 0) return COLOR_CYAN;
    if (strcmp(color_name, "magenta") == 0) return COLOR_MAGENTA;
    if (strcmp(color_name, "black") == 0) return COLOR_BLACK;
    if (strcmp(color_name, "white") == 0) return COLOR_WHITE;
    if (strcmp(color_name, "gray") == 0) return COLOR_GRAY;
    
    return COLOR_BLUE;
}

const char* visual_color_to_string(PlotColor color) {
    switch (color) {
        case COLOR_RED:     return "red";
        case COLOR_GREEN:   return "green";
        case COLOR_BLUE:    return "blue";
        case COLOR_YELLOW:  return "yellow";
        case COLOR_CYAN:    return "cyan";
        case COLOR_MAGENTA: return "magenta";
        case COLOR_BLACK:   return "black";
        case COLOR_WHITE:   return "white";
        case COLOR_GRAY:    return "gray";
        default:            return "blue";
    }
}
