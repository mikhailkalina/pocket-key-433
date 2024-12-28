#pragma once

// Menu function prototype
typedef void (*MenuFunction)(int param);

/**
 * @brief Menu item structure
 */
struct MenuItem
{
    const MenuItem *parent;
    const MenuItem *prev;
    const MenuItem *next;
    const MenuItem *child;
    MenuFunction func;
    int param;
    const char *text;
};
