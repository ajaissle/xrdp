/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * painter, gc
 */

#include "xrdp.h"

/*****************************************************************************/
struct xrdp_painter *APP_CC
xrdp_painter_create(struct xrdp_wm *wm, struct xrdp_session *session)
{
    struct xrdp_painter *self;

    self = (struct xrdp_painter *)g_malloc(sizeof(struct xrdp_painter), 1);
    self->wm = wm;
    self->session = session;
    self->rop = 0xcc; /* copy gota use 0xcc*/
    self->clip_children = 1;
    return self;
}

/*****************************************************************************/
void APP_CC
xrdp_painter_delete(struct xrdp_painter *self)
{
    if (self == 0)
    {
        return;
    }

    g_free(self);
}

/*****************************************************************************/
int APP_CC
wm_painter_set_target(struct xrdp_painter *self)
{
    int surface_index;
    int index;
    struct list *del_list;

    if (self->wm->target_surface->type == WND_TYPE_SCREEN)
    {
        if (self->wm->current_surface_index != 0xffff)
        {
            libxrdp_orders_send_switch_os_surface(self->session, 0xffff);
            self->wm->current_surface_index = 0xffff;
        }
    }
    else if (self->wm->target_surface->type == WND_TYPE_OFFSCREEN)
    {
        surface_index = self->wm->target_surface->item_index;

        if (surface_index != self->wm->current_surface_index)
        {
            if (self->wm->target_surface->tab_stop == 0) /* tab_stop is hack */
            {
                del_list = self->wm->cache->xrdp_os_del_list;
                index = list_index_of(del_list, surface_index);
                list_remove_item(del_list, index);
                libxrdp_orders_send_create_os_surface(self->session, surface_index,
                                                      self->wm->target_surface->width,
                                                      self->wm->target_surface->height,
                                                      del_list);
                self->wm->target_surface->tab_stop = 1;
                list_clear(del_list);
            }

            libxrdp_orders_send_switch_os_surface(self->session, surface_index);
            self->wm->current_surface_index = surface_index;
        }
    }
    else
    {
        g_writeln("xrdp_painter_begin_update: bad target_surface");
    }

    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_begin_update(struct xrdp_painter *self)
{
    if (self == 0)
    {
        return 0;
    }

    libxrdp_orders_init(self->session);
    wm_painter_set_target(self);
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_end_update(struct xrdp_painter *self)
{
    if (self == 0)
    {
        return 0;
    }

    libxrdp_orders_send(self->session);
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_font_needed(struct xrdp_painter *self)
{
    if (self->font == 0)
    {
        self->font = self->wm->default_font;
    }

    return 0;
}

#if 0
/*****************************************************************************/
/* returns boolean, true if there is something to draw */
static int APP_CC
xrdp_painter_clip_adj(struct xrdp_painter *self, int *x, int *y,
                      int *cx, int *cy)
{
    int dx;
    int dy;

    if (!self->use_clip)
    {
        return 1;
    }

    if (self->clip.left > *x)
    {
        dx = self->clip.left - *x;
    }
    else
    {
        dx = 0;
    }

    if (self->clip.top > *y)
    {
        dy = self->clip.top - *y;
    }
    else
    {
        dy = 0;
    }

    if (*x + *cx > self->clip.right)
    {
        *cx = *cx - ((*x + *cx) - self->clip.right);
    }

    if (*y + *cy > self->clip.bottom)
    {
        *cy = *cy - ((*y + *cy) - self->clip.bottom);
    }

    *cx = *cx - dx;
    *cy = *cy - dy;

    if (*cx <= 0)
    {
        return 0;
    }

    if (*cy <= 0)
    {
        return 0;
    }

    *x = *x + dx;
    *y = *y + dy;
    return 1;
}
#endif

/*****************************************************************************/
int APP_CC
xrdp_painter_set_clip(struct xrdp_painter *self,
                      int x, int y, int cx, int cy)
{
    self->use_clip = &self->clip;
    self->clip.left = x;
    self->clip.top = y;
    self->clip.right = x + cx;
    self->clip.bottom = y + cy;
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_clr_clip(struct xrdp_painter *self)
{
    self->use_clip = 0;
    return 0;
}

#if 0
/*****************************************************************************/
static int APP_CC
xrdp_painter_rop(int rop, int src, int dst)
{
    switch (rop & 0x0f)
    {
        case 0x0:
            return 0;
        case 0x1:
            return ~(src | dst);
        case 0x2:
            return (~src) & dst;
        case 0x3:
            return ~src;
        case 0x4:
            return src & (~dst);
        case 0x5:
            return ~(dst);
        case 0x6:
            return src ^ dst;
        case 0x7:
            return ~(src & dst);
        case 0x8:
            return src & dst;
        case 0x9:
            return ~(src) ^ dst;
        case 0xa:
            return dst;
        case 0xb:
            return (~src) | dst;
        case 0xc:
            return src;
        case 0xd:
            return src | (~dst);
        case 0xe:
            return src | dst;
        case 0xf:
            return ~0;
    }

    return dst;
}
#endif

/*****************************************************************************/
int APP_CC
xrdp_painter_text_width(struct xrdp_painter *self, char *text)
{
    int index;
    int rv;
    int len;
    struct xrdp_font_char *font_item;
    twchar *wstr;

    xrdp_painter_font_needed(self);

    if (self->font == 0)
    {
        return 0;
    }

    if (text == 0)
    {
        return 0;
    }

    rv = 0;
    len = g_mbstowcs(0, text, 0);
    wstr = (twchar *)g_malloc((len + 2) * sizeof(twchar), 0);
    g_mbstowcs(wstr, text, len + 1);

    for (index = 0; index < len; index++)
    {
        font_item = self->font->font_items + wstr[index];
        rv = rv + font_item->incby;
    }

    g_free(wstr);
    return rv;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_text_height(struct xrdp_painter *self, char *text)
{
    int index;
    int rv;
    int len;
    struct xrdp_font_char *font_item;
    twchar *wstr;

    xrdp_painter_font_needed(self);

    if (self->font == 0)
    {
        return 0;
    }

    if (text == 0)
    {
        return 0;
    }

    rv = 0;
    len = g_mbstowcs(0, text, 0);
    wstr = (twchar *)g_malloc((len + 2) * sizeof(twchar), 0);
    g_mbstowcs(wstr, text, len + 1);

    for (index = 0; index < len; index++)
    {
        font_item = self->font->font_items + wstr[index];
        rv = MAX(rv, font_item->height);
    }

    g_free(wstr);
    return rv;
}

/*****************************************************************************/
static int APP_CC
xrdp_painter_setup_brush(struct xrdp_painter *self,
                         struct xrdp_brush *out_brush,
                         struct xrdp_brush *in_brush)
{
    int cache_id;

    g_memcpy(out_brush, in_brush, sizeof(struct xrdp_brush));

    if (in_brush->style == 3)
    {
        if (self->session->client_info->brush_cache_code == 1)
        {
            cache_id = xrdp_cache_add_brush(self->wm->cache, in_brush->pattern);
            g_memset(out_brush->pattern, 0, 8);
            out_brush->pattern[0] = cache_id;
            out_brush->style = 0x81;
        }
    }

    return 0;
}

/*****************************************************************************/
/* fill in an area of the screen with one color */
int APP_CC
xrdp_painter_fill_rect(struct xrdp_painter *self,
                       struct xrdp_bitmap *dst,
                       int x, int y, int cx, int cy)
{
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_rect rect;
    struct xrdp_region *region;
    struct xrdp_brush brush;
    int k;
    int dx;
    int dy;
    int rop;

    if (self == 0)
    {
        return 0;
    }

    /* todo data */

    if (dst->type == WND_TYPE_BITMAP) /* 0 */
    {
        return 0;
    }

    xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
    region = xrdp_region_create(self->wm);

    if (dst->type != WND_TYPE_OFFSCREEN)
    {
        xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy, region,
                               self->clip_children);
    }
    else
    {
        xrdp_region_add_rect(region, &clip_rect);
    }

    x += dx;
    y += dy;

    if (self->mix_mode == 0 && self->rop == 0xcc)
    {
        k = 0;

        while (xrdp_region_get_rect(region, k, &rect) == 0)
        {
            if (rect_intersect(&rect, &clip_rect, &draw_rect))
            {
                libxrdp_orders_rect(self->session, x, y, cx, cy,
                                    self->fg_color, &draw_rect);
            }

            k++;
        }
    }
    else if (self->mix_mode == 0 &&
             ((self->rop & 0xf) == 0x0 || /* black */
              (self->rop & 0xf) == 0xf || /* white */
              (self->rop & 0xf) == 0x5))  /* DSTINVERT */
    {
        k = 0;

        while (xrdp_region_get_rect(region, k, &rect) == 0)
        {
            if (rect_intersect(&rect, &clip_rect, &draw_rect))
            {
                libxrdp_orders_dest_blt(self->session, x, y, cx, cy,
                                        self->rop, &draw_rect);
            }

            k++;
        }
    }
    else
    {
        k = 0;
        rop = self->rop;

        /* if opcode is in the form 0x00, 0x11, 0x22, ... convert it */
        if (((rop & 0xf0) >> 4) == (rop & 0xf))
        {
            switch (rop)
            {
                case 0x66: /* xor */
                    rop = 0x5a;
                    break;
                case 0xaa: /* noop */
                    rop = 0xfb;
                    break;
                case 0xcc: /* copy */
                    rop = 0xf0;
                    break;
                case 0x88: /* and */
                    rop = 0xc0;
                    break;
            }
        }

        xrdp_painter_setup_brush(self, &brush, &self->brush);

        while (xrdp_region_get_rect(region, k, &rect) == 0)
        {
            if (rect_intersect(&rect, &clip_rect, &draw_rect))
            {
                libxrdp_orders_pat_blt(self->session, x, y, cx, cy,
                                       rop, self->bg_color, self->fg_color,
                                       &brush, &draw_rect);
            }

            k++;
        }
    }

    xrdp_region_delete(region);
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_draw_text(struct xrdp_painter *self,
                       struct xrdp_bitmap *dst,
                       int x, int y, const char *text)
{
    int i;
    int f;
    int c;
    int k;
    int x1;
    int y1;
    int flags;
    int len;
    int index;
    int total_width;
    int total_height;
    int dx;
    int dy;
    char *data;
    struct xrdp_region *region;
    struct xrdp_rect rect;
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_font *font;
    struct xrdp_font_char *font_item;
    twchar *wstr;

    if (self == 0)
    {
        return 0;
    }

    len = g_mbstowcs(0, text, 0);

    if (len < 1)
    {
        return 0;
    }

    /* todo data */

    if (dst->type == 0)
    {
        return 0;
    }

    xrdp_painter_font_needed(self);

    if (self->font == 0)
    {
        return 0;
    }

    /* convert to wide char */
    wstr = (twchar *)g_malloc((len + 2) * sizeof(twchar), 0);
    g_mbstowcs(wstr, text, len + 1);
    font = self->font;
    f = 0;
    k = 0;
    total_width = 0;
    total_height = 0;
    data = (char *)g_malloc(len * 4, 1);

    for (index = 0; index < len; index++)
    {
        font_item = font->font_items + wstr[index];
        i = xrdp_cache_add_char(self->wm->cache, font_item);
        f = HIWORD(i);
        c = LOWORD(i);
        data[index * 2] = c;
        data[index * 2 + 1] = k;
        k = font_item->incby;
        total_width += k;
        total_height = MAX(total_height, font_item->height);
    }

    xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
    region = xrdp_region_create(self->wm);

    if (dst->type != WND_TYPE_OFFSCREEN)
    {
        xrdp_wm_get_vis_region(self->wm, dst, x, y, total_width, total_height,
                               region, self->clip_children);
    }
    else
    {
        xrdp_region_add_rect(region, &clip_rect);
    }

    x += dx;
    y += dy;
    k = 0;

    while (xrdp_region_get_rect(region, k, &rect) == 0)
    {
        if (rect_intersect(&rect, &clip_rect, &draw_rect))
        {
            x1 = x;
            y1 = y + total_height;
            flags = 0x03; /* 0x03 0x73; TEXT2_IMPLICIT_X and something else */
            libxrdp_orders_text(self->session, f, flags, 0,
                                self->fg_color, 0,
                                x - 1, y - 1, x + total_width, y + total_height,
                                0, 0, 0, 0,
                                x1, y1, data, len * 2, &draw_rect);
        }

        k++;
    }

    xrdp_region_delete(region);
    g_free(data);
    g_free(wstr);
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_draw_text2(struct xrdp_painter *self,
                        struct xrdp_bitmap *dst,
                        int font, int flags, int mixmode,
                        int clip_left, int clip_top,
                        int clip_right, int clip_bottom,
                        int box_left, int box_top,
                        int box_right, int box_bottom,
                        int x, int y, char *data, int data_len)
{
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_rect rect;
    struct xrdp_region *region;
    int k;
    int dx;
    int dy;

    if (self == 0)
    {
        return 0;
    }

    /* todo data */

    if (dst->type == WND_TYPE_BITMAP)
    {
        return 0;
    }

    xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
    region = xrdp_region_create(self->wm);

    if (dst->type != WND_TYPE_OFFSCREEN)
    {
        if (box_right - box_left > 1)
        {
            xrdp_wm_get_vis_region(self->wm, dst, box_left, box_top,
                                   box_right - box_left, box_bottom - box_top,
                                   region, self->clip_children);
        }
        else
        {
            xrdp_wm_get_vis_region(self->wm, dst, clip_left, clip_top,
                                   clip_right - clip_left, clip_bottom - clip_top,
                                   region, self->clip_children);
        }
    }
    else
    {
        xrdp_region_add_rect(region, &clip_rect);
    }

    clip_left += dx;
    clip_top += dy;
    clip_right += dx;
    clip_bottom += dy;
    box_left += dx;
    box_top += dy;
    box_right += dx;
    box_bottom += dy;
    x += dx;
    y += dy;
    k = 0;

    while (xrdp_region_get_rect(region, k, &rect) == 0)
    {
        if (rect_intersect(&rect, &clip_rect, &draw_rect))
        {
            libxrdp_orders_text(self->session, font, flags, mixmode,
                                self->fg_color, self->bg_color,
                                clip_left, clip_top, clip_right, clip_bottom,
                                box_left, box_top, box_right, box_bottom,
                                x, y, data, data_len, &draw_rect);
        }

        k++;
    }

    xrdp_region_delete(region);
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_copy(struct xrdp_painter *self,
                  struct xrdp_bitmap *src,
                  struct xrdp_bitmap *dst,
                  int x, int y, int cx, int cy,
                  int srcx, int srcy)
{
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_rect rect1;
    struct xrdp_rect rect2;
    struct xrdp_region *region;
    struct xrdp_bitmap *b;
    int i;
    int j;
    int k;
    int dx;
    int dy;
    int palette_id;
    int bitmap_id;
    int cache_id;
    int cache_idx;
    int dstx;
    int dsty;
    int w;
    int h;
    int index;
    struct list *del_list;

    if (self == 0 || src == 0 || dst == 0)
    {
        return 0;
    }

    /* todo data */

    if (dst->type == WND_TYPE_BITMAP)
    {
        return 0;
    }

    if (src->type == WND_TYPE_SCREEN)
    {
        xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
        region = xrdp_region_create(self->wm);

        if (dst->type != WND_TYPE_OFFSCREEN)
        {
            xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy,
                                   region, self->clip_children);
        }
        else
        {
            xrdp_region_add_rect(region, &clip_rect);
        }

        x += dx;
        y += dy;
        srcx += dx;
        srcy += dy;
        k = 0;

        while (xrdp_region_get_rect(region, k, &rect1) == 0)
        {
            if (rect_intersect(&rect1, &clip_rect, &draw_rect))
            {
                libxrdp_orders_screen_blt(self->session, x, y, cx, cy,
                                          srcx, srcy, self->rop, &draw_rect);
            }

            k++;
        }

        xrdp_region_delete(region);
    }
    else if (src->type == WND_TYPE_OFFSCREEN)
    {
        //g_writeln("xrdp_painter_copy: todo");

        xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
        region = xrdp_region_create(self->wm);

        if (dst->type != WND_TYPE_OFFSCREEN)
        {
            //g_writeln("off screen to screen");
            xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy,
                                   region, self->clip_children);
        }
        else
        {
            //g_writeln("off screen to off screen");
            xrdp_region_add_rect(region, &clip_rect);
        }

        x += dx;
        y += dy;

        palette_id = 0;
        cache_id = 255; // todo
        cache_idx = src->item_index; // todo

        if (src->tab_stop == 0)
        {
            g_writeln("xrdp_painter_copy: warning src not created");
            del_list = self->wm->cache->xrdp_os_del_list;
            index = list_index_of(del_list, cache_idx);
            list_remove_item(del_list, index);
            libxrdp_orders_send_create_os_surface(self->session,
                                                  cache_idx,
                                                  src->width,
                                                  src->height,
                                                  del_list);
            src->tab_stop = 1;
            list_clear(del_list);
        }


        k = 0;

        while (xrdp_region_get_rect(region, k, &rect1) == 0)
        {
            if (rect_intersect(&rect1, &clip_rect, &rect2))
            {
                MAKERECT(rect1, x, y, cx, cy);

                if (rect_intersect(&rect2, &rect1, &draw_rect))
                {
                    libxrdp_orders_mem_blt(self->session, cache_id, palette_id,
                                           x, y, cx, cy, self->rop, srcx, srcy,
                                           cache_idx, &draw_rect);
                }
            }

            k++;
        }

        xrdp_region_delete(region);
    }
    else if (src->data != 0)
        /* todo, the non bitmap cache part is gone, it should be put back */
    {
        xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
        region = xrdp_region_create(self->wm);

        if (dst->type != WND_TYPE_OFFSCREEN)
        {
            xrdp_wm_get_vis_region(self->wm, dst, x, y, cx, cy,
                                   region, self->clip_children);
        }
        else
        {
            xrdp_region_add_rect(region, &clip_rect);
        }

        x += dx;
        y += dy;
        palette_id = 0;
        j = srcy;

        while (j < (srcy + cy))
        {
            i = srcx;

            while (i < (srcx + cx))
            {
                w = MIN(64, ((srcx + cx) - i));
                h = MIN(64, ((srcy + cy) - j));
                b = xrdp_bitmap_create(w, h, src->bpp, 0, self->wm);
                xrdp_bitmap_copy_box_with_crc(src, b, i, j, w, h);
                bitmap_id = xrdp_cache_add_bitmap(self->wm->cache, b, self->wm->hints);
                cache_id = HIWORD(bitmap_id);
                cache_idx = LOWORD(bitmap_id);
                dstx = (x + i) - srcx;
                dsty = (y + j) - srcy;
                k = 0;

                while (xrdp_region_get_rect(region, k, &rect1) == 0)
                {
                    if (rect_intersect(&rect1, &clip_rect, &rect2))
                    {
                        MAKERECT(rect1, dstx, dsty, w, h);

                        if (rect_intersect(&rect2, &rect1, &draw_rect))
                        {
                            libxrdp_orders_mem_blt(self->session, cache_id, palette_id,
                                                   dstx, dsty, w, h, self->rop, 0, 0,
                                                   cache_idx, &draw_rect);
                        }
                    }

                    k++;
                }

                i += 64;
            }

            j += 64;
        }

        xrdp_region_delete(region);
    }

    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_composite(struct xrdp_painter* self,
                       struct xrdp_bitmap* src,
                       int srcformat,
                       int srcwidth,
                       int srcrepeat,
                       struct xrdp_bitmap* dst,
                       int* srctransform,
                       int mskflags,
                       struct xrdp_bitmap* msk,
                       int mskformat, int mskwidth, int mskrepeat, int op,
                       int srcx, int srcy, int mskx, int msky,
                       int dstx, int dsty, int width, int height, int dstformat)
{
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_rect rect1;
    struct xrdp_rect rect2;
    struct xrdp_region* region;
    int k;
    int dx;
    int dy;
    int palette_id;
    int cache_srcidx;
    int cache_mskidx;
    
    if (self == 0 || src == 0 || dst == 0)
    {
        return 0;
    }
    
    /* todo data */
    
    if (dst->type == WND_TYPE_BITMAP)
    {
        return 0;
    }
    
    if (src->type == WND_TYPE_OFFSCREEN)
    {
        xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
        region = xrdp_region_create(self->wm);
        xrdp_region_add_rect(region, &clip_rect);
        dstx += dx;
        dsty += dy;
        
        palette_id = 0;
        cache_srcidx = src->item_index;
        cache_mskidx = -1;
        if (mskflags & 1)
        {
            if (msk != 0)
            {
                cache_mskidx = msk->item_index; // todo
            }
        }
        
        k = 0;
        while (xrdp_region_get_rect(region, k, &rect1) == 0)
        {
            if (rect_intersect(&rect1, &clip_rect, &rect2))
            {
                MAKERECT(rect1, dstx, dsty, width, height);
                if (rect_intersect(&rect2, &rect1, &draw_rect))
                {
                    libxrdp_orders_composite_blt(self->session, cache_srcidx, srcformat, srcwidth,
                                                 srcrepeat, srctransform, mskflags, cache_mskidx,
                                                 mskformat, mskwidth, mskrepeat, op, srcx, srcy,
                                                 mskx, msky, dstx, dsty, width, height, dstformat,
                                                 &draw_rect);
                }
            }
            k++;
        }
        xrdp_region_delete(region);
    }
    return 0;
}

/*****************************************************************************/
int APP_CC
xrdp_painter_line(struct xrdp_painter *self,
                  struct xrdp_bitmap *dst,
                  int x1, int y1, int x2, int y2)
{
    struct xrdp_rect clip_rect;
    struct xrdp_rect draw_rect;
    struct xrdp_rect rect;
    struct xrdp_region *region;
    int k;
    int dx;
    int dy;
    int rop;

    if (self == 0)
    {
        return 0;
    }

    /* todo data */

    if (dst->type == WND_TYPE_BITMAP)
    {
        return 0;
    }

    xrdp_bitmap_get_screen_clip(dst, self, &clip_rect, &dx, &dy);
    region = xrdp_region_create(self->wm);

    if (dst->type != WND_TYPE_OFFSCREEN)
    {
        xrdp_wm_get_vis_region(self->wm, dst, MIN(x1, x2), MIN(y1, y2),
                               g_abs(x1 - x2) + 1, g_abs(y1 - y2) + 1,
                               region, self->clip_children);
    }
    else
    {
        xrdp_region_add_rect(region, &clip_rect);
    }

    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;
    k = 0;
    rop = self->rop;

    if (rop < 0x01 || rop > 0x10)
    {
        rop = (rop & 0xf) + 1;
    }

    while (xrdp_region_get_rect(region, k, &rect) == 0)
    {
        if (rect_intersect(&rect, &clip_rect, &draw_rect))
        {
            libxrdp_orders_line(self->session, 1, x1, y1, x2, y2,
                                rop, self->bg_color,
                                &self->pen, &draw_rect);
        }

        k++;
    }

    xrdp_region_delete(region);
    return 0;
}
