/* 
 *  2d_gfx.h: 2-D Graphics primitives, i.e. pixel, line, rectangle, triangle, etc.
 */
#pragma once

// 32 bit ARGB colors
#define BLACK      0x00000000 
#define WHITE      0x00FFFFFF 
#define DARK_GRAY  0x00222222
#define LIGHT_GRAY 0x00DDDDDD
#define RED        0x00FF0000 
#define GREEN      0x0000FF00 
#define BLUE       0x000000FF 
#define YELLOW     0x00FFFF00 
#define PURPLE     0x00FF00FF

#define abs(a) ((a > 0) ? a : -a)
#define ROUND(a) ((int)(a + 0.5))

typedef struct point {
    uint16_t X, Y;
} Point;

// Draw a single pixel
void draw_pixel(uint16_t X, uint16_t Y, uint32_t color)
{
    uint32_t *framebuffer = (uint32_t *)*(uint32_t *)0x9028; 
    framebuffer += (Y * 1920 + X);
    *framebuffer = color;
}

// Draw a line
//   Adapted from Wikipedia page on Bresenham's line algorithm
void draw_line(Point start, Point end, uint32_t color)
{
    int16_t deltaX = abs((end.X - start.X));    // Delta X, change in X values, positive absolute value
    int16_t deltaY = -abs((end.Y - start.Y));   // Delta Y, change in Y values, negative absolute value
    int16_t signX = (start.X < end.X) ? 1 : -1; // Sign of X direction, moving right (positive) or left (negative)
    int16_t signY = (start.Y < end.Y) ? 1 : -1; // Sign of Y direction, moving down (positive) or up (negative)
    int16_t error = deltaX + deltaY;
    int16_t errorX2;

    while (1) {
        draw_pixel(start.X, start.Y, color);
        if (start.X == end.X && start.Y == end.Y) break;

        errorX2 = error * 2;
        if (errorX2 >= deltaY) {
            error   += deltaY;
            start.X += signX;
        }
        if (errorX2 <= deltaX) {
            error   += deltaX;
            start.Y += signY;
        }
    }
}

// Draw a triangle
void draw_triangle(Point vertex0, Point vertex1, Point vertex2, uint32_t color)
{
    // Draw lines between all 3 points
    draw_line(vertex0, vertex1, color);
    draw_line(vertex1, vertex2, color);
    draw_line(vertex2, vertex0, color);
}

// Draw a rectangle
void draw_rect(Point top_left, Point bottom_right, uint32_t color)
{
    Point temp = bottom_right;
    
    // Draw 4 lines, 2 horizontal parallel sides, 2 vertical parallel sides
    // Top of rectangle
    temp.Y = top_left.Y;
    draw_line(top_left, temp, color);

    // Right side
    draw_line(temp, bottom_right, color);

    // Bottom
    temp.X = top_left.X;
    temp.Y = bottom_right.Y;
    draw_line(bottom_right, temp, color);

    // Left side
    draw_line(temp, top_left, color);
}

// Draw a polygon
void draw_polygon(Point vertex_array[], uint8_t num_vertices, uint32_t color)
{
    // Draw lines up to last line
    for (uint8_t i = 0; i < num_vertices - 1; i++)
        draw_line(vertex_array[i], vertex_array[i+1], color);

    // Draw last line
    draw_line(vertex_array[num_vertices - 1], vertex_array[0], color);
}

// Draw a circle
//  Adapted from "Computer Graphics Principles and Practice in C 2nd Edition"
void draw_circle(Point center, uint16_t radius, uint32_t color)
{
    int16_t x = 0;
    int16_t y = radius;
    int16_t p = 1 - radius;

    // Draw initial 8 octant points
    draw_pixel(center.X + x, center.Y + y, color);
    draw_pixel(center.X - x, center.Y + y, color);
    draw_pixel(center.X + x, center.Y - y, color);
    draw_pixel(center.X - x, center.Y - y, color);
    draw_pixel(center.X + y, center.Y + x, color);
    draw_pixel(center.X - y, center.Y + x, color);
    draw_pixel(center.X + y, center.Y - x, color);
    draw_pixel(center.X - y, center.Y - x, color);

    while (x < y) {
        x++;
        if (p < 0) p += 2*x + 1;
        else {
            y--;
            p += 2*(x-y) + 1;
        }

        // Draw next set of 8 octant points
        draw_pixel(center.X + x, center.Y + y, color);
        draw_pixel(center.X - x, center.Y + y, color);
        draw_pixel(center.X + x, center.Y - y, color);
        draw_pixel(center.X - x, center.Y - y, color);
        draw_pixel(center.X + y, center.Y + x, color);
        draw_pixel(center.X - y, center.Y + x, color);
        draw_pixel(center.X + y, center.Y - x, color);
        draw_pixel(center.X - y, center.Y - x, color);
    }
}

// Draw an ellipse
//  Adapted from "Computer Graphics Principles and Practice in C 2nd Edition"
void draw_ellipse(Point center, uint16_t radiusX, uint16_t radiusY, uint32_t color)
{
    int32_t rx2 = radiusX * radiusX;
    int32_t ry2 = radiusY * radiusY;
    int32_t twoRx2 = 2*rx2;
    int32_t twoRy2 = 2*ry2;
    int32_t p;
    int32_t x = 0;
    int32_t y = radiusY;
    int32_t px = 0;
    int32_t py = twoRx2 * y;

    // Draw initial 4 quadrant points
    draw_pixel(center.X + x, center.Y + y, color);
    draw_pixel(center.X - x, center.Y + y, color);
    draw_pixel(center.X + x, center.Y - y, color);
    draw_pixel(center.X - x, center.Y - y, color);

    // Region 1
    p = ROUND(ry2 - (rx2 * radiusY) + (0.25 * rx2));
    while (px < py) {
        x++;
        px += twoRy2;
        if (p < 0) p += ry2 + px;
        else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }

        // Draw 4 quadrant points
        draw_pixel(center.X + x, center.Y + y, color);
        draw_pixel(center.X - x, center.Y + y, color);
        draw_pixel(center.X + x, center.Y - y, color);
        draw_pixel(center.X - x, center.Y - y, color);
    }

    // Region 2
    p = ROUND(ry2 * (x + 0.5)*(x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2*ry2);
    while (y > 0) {
        y--;
        py -= twoRx2; if (p > 0) p += rx2 - py;
        else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }

        // Draw 4 quadrant points
        draw_pixel(center.X + x, center.Y + y, color);
        draw_pixel(center.X - x, center.Y + y, color);
        draw_pixel(center.X + x, center.Y - y, color);
        draw_pixel(center.X - x, center.Y - y, color);
    }
}

// Fill an area with a solid color
void boundary_fill(uint16_t X, uint16_t Y, uint32_t fill_color, uint32_t boundary_color)
{
    // Recursive - may use a lot of stack space
    uint32_t *framebuffer = (uint32_t *)*(uint32_t *)0x9028; 
    framebuffer += (Y * 1920 + X);

    if (*framebuffer != fill_color && *framebuffer != boundary_color) {
        *framebuffer = fill_color;

        // Check 4 pixels around current pixel
        boundary_fill(X + 1, Y, fill_color, boundary_color);
        boundary_fill(X - 1, Y, fill_color, boundary_color);
        boundary_fill(X, Y + 1, fill_color, boundary_color);
        boundary_fill(X, Y - 1, fill_color, boundary_color);
    }
} 


// Fill triangle with a solid color
void fill_triangle_solid(Point p0, Point p1, Point p2, uint32_t color)
{
    Point temp;

    // Get center (Centroid) of a triangle
    temp.X = (p0.X + p1.X + p2.X) / 3;
    temp.Y = (p0.Y + p1.Y + p2.Y) / 3;

    // First draw triangles sides (boundaries)
    draw_triangle(p0, p1, p2, color - 1);

    // Then fill in boundaries 
    boundary_fill(temp.X, temp.Y, color, color - 1);

    // Then redraw boundaries to correct color
    draw_triangle(p0, p1, p2, color);
}

// Fill rectangle with a solid color
void fill_rect_solid(Point top_left, Point bottom_right, uint32_t color)
{
   // Brute force method 
   for (uint16_t y = top_left.Y; y < bottom_right.Y; y++)
       for (uint16_t x = top_left.X; x < bottom_right.X; x++)
           draw_pixel(x, y, color);
}

// Fill a polygon with a solid color
void fill_polygon_solid(Point vertex_array[], uint8_t num_vertices, uint32_t color)
{
    Point temp;

    // Assuming this works in general, get center (Centroid) of a triangle in the polygon
    temp.X = (vertex_array[0].X + vertex_array[1].X + vertex_array[2].X) / 3;
    temp.Y = (vertex_array[0].Y + vertex_array[1].Y + vertex_array[2].Y) / 3;

    // First draw polygon sides (boundaries)
    draw_polygon(vertex_array, num_vertices, color - 1);

    // Then fill in boundaries 
    boundary_fill(temp.X, temp.Y, color, color - 1);

    // Then redraw boundaries to correct color
    draw_polygon(vertex_array, num_vertices, color);
}

// Fill a circle with a solid color
void fill_circle_solid(Point center, uint16_t radius, uint32_t color)
{
    // Brute force method
    for (int16_t y = -radius; y <= radius; y++)
        for (int16_t x = -radius; x <= radius; x++)
            if (x*x + y*y < radius*radius - radius)
                draw_pixel(center.X + x, center.Y + y, color);
}

// Fill an ellipse with a solid color
void fill_ellipse_solid(Point center, uint16_t radiusX, uint16_t radiusY, uint32_t color)
{
    // First draw boundaries a different color
    draw_ellipse(center, radiusX, radiusY, color - 1);

    // Then fill in the boundaries
    boundary_fill(center.X, center.Y, color, color - 1);

    // Then redraw boundaries as the correct color
    draw_ellipse(center, radiusX, radiusY, color);
}








