 #include "e.h"

/* local function prototypes */
static void _e_surface_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *resource);
static void _e_surface_cb_attach(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *buffer_resource, int x, int y);
static void _e_surface_cb_damage(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int x, int y, int w, int h);
static void _e_surface_cb_commit(struct wl_client *client EINA_UNUSED, struct wl_resource *resource);
static void _e_surface_cb_frame(struct wl_client *client, struct wl_resource *resource, unsigned int callback);
static void _e_surface_cb_opaque_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *region_resource);
static void _e_surface_cb_input_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *region_resource);
static void _e_surface_cb_buffer_scale_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int scale);
static void _e_surface_cb_buffer_destroy(struct wl_listener *listener, void *data EINA_UNUSED);
static void _e_surface_frame_cb_destroy(struct wl_resource *resource);

/* local wayland interfaces */
static const struct wl_surface_interface _e_surface_interface = 
{
   _e_surface_cb_destroy,
   _e_surface_cb_attach,
   _e_surface_cb_damage,
   _e_surface_cb_frame,
   _e_surface_cb_opaque_set,
   _e_surface_cb_input_set,
   _e_surface_cb_commit,
   NULL, // cb_buffer_transform_set
   NULL // cb_buffer_scale_set
};

EAPI E_Surface *
e_surface_new(struct wl_client *client, unsigned int id)
{
   E_Surface *es;

   /* try to allocate space for a new surface */
   if (!(es = E_NEW(E_Surface, 1))) return NULL;

   es->wl.id = id;

   es->scale = 1;
   es->pending.scale = es->scale;

   /* initialize the destroy signal */
   wl_signal_init(&es->signals.destroy);

   /* initialize the link */
   wl_list_init(&es->wl.link);
   wl_list_init(&es->frames);
   wl_list_init(&es->pending.frames);

   pixman_region32_init(&es->bounding);
   pixman_region32_init(&es->damage);
   pixman_region32_init(&es->opaque);
   pixman_region32_init(&es->clip);
   pixman_region32_init_rect(&es->input, INT32_MIN, INT32_MIN, 
                             UINT32_MAX, UINT32_MAX);

   es->pending.buffer_destroy.notify = _e_surface_cb_buffer_destroy;

   pixman_region32_init(&es->pending.damage);
   pixman_region32_init(&es->pending.opaque);
   pixman_region32_init_rect(&es->pending.input, INT32_MIN, INT32_MIN, 
                             UINT32_MAX, UINT32_MAX);

   es->wl.resource = 
     wl_client_add_object(client, &wl_surface_interface, 
                          &_e_surface_interface, id, es);

   return es;
}

EAPI void 
e_surface_attach(E_Surface *es, E_Buffer *buffer)
{
   /* check for valid surface */
   if (!es) return;

   /* reference this buffer */
   e_buffer_reference(&es->buffer.reference, buffer);

   if (!buffer)
     {
        /* if we have no buffer to attach (buffer == NULL), 
         * then we assume that we want to unmap this surface (hide it) */

        /* FIXME: This may need to call the surface->unmap function if a 
         * shell wants to do something "different" when it unmaps */
        if (es->mapped) e_surface_unmap(es);
     }

   /* call renderer attach */
   if (_e_comp->renderer->attach) _e_comp->renderer->attach(es, buffer);
}

EAPI void 
e_surface_unmap(E_Surface *es)
{
   /* TODO */
}

EAPI void 
e_surface_damage(E_Surface *es)
{
   /* check for valid surface */
   if (!es) return;

   /* add this damage rectangle */
   pixman_region32_union_rect(&es->damage, &es->damage, 
                              0, 0, es->geometry.w, es->geometry.h);

   e_surface_repaint_schedule(es);
}

EAPI void 
e_surface_damage_below(E_Surface *es)
{
   pixman_region32_t damage;

   pixman_region32_init(&damage);
   pixman_region32_subtract(&damage, &es->bounding, &es->clip);
   pixman_region32_union(&es->plane->damage, &es->plane->damage, &damage);
   pixman_region32_fini(&damage);
}

EAPI void 
e_surface_destroy(E_Surface *es)
{
   E_Surface_Frame *cb, *cbnext;

   /* check for valid surface */
   if (!es) return;

   wl_signal_emit(&es->signals.destroy, &es->wl.resource);

   /* if this surface is mapped, unmap it */
   if (es->mapped) e_surface_unmap(es);

   /* remove any pending frame callbacks */
   wl_list_for_each_safe(cb, cbnext, &es->pending.frames, link)
     wl_resource_destroy(cb->resource);

   pixman_region32_fini(&es->pending.damage);
   pixman_region32_fini(&es->pending.opaque);
   pixman_region32_fini(&es->pending.input);

   /* destroy pending buffer */
   if (es->pending.buffer)
     wl_list_remove(&es->pending.buffer_destroy.link);

   /* remove any buffer references */
   e_buffer_reference(&es->buffer.reference, NULL);

   if (_e_comp->renderer->surface_destroy)
     _e_comp->renderer->surface_destroy(es);

   /* free regions */
   pixman_region32_fini(&es->bounding);
   pixman_region32_fini(&es->damage);
   pixman_region32_fini(&es->opaque);
   pixman_region32_fini(&es->input);
   pixman_region32_fini(&es->clip);

   /* remove any active frame callbacks */
   wl_list_for_each_safe(cb, cbnext, &es->frames, link)
     wl_resource_destroy(cb->resource);

   /* EINA_LIST_FOREACH(_e_comp->inputs, l, seat) */
   /*   { */
   /*      kbd = seat->keyboard; */

   /*      if (kbd->focus) printf("\tCurrently Focused: %p\n", kbd->focus); */

   /*      if ((kbd->focus) && (kbd->focus == es)) */
   /*        { */
   /*           if (seat->kbd.saved_focus) */
   /*             { */
   /*                printf("\tSaved Focus: %p\n", seat->kbd.saved_focus); */
   /*                wl_list_remove(&seat->kbd.focus_listener.link); */
   /*                e_input_keyboard_focus_set(kbd, seat->kbd.saved_focus); */
   /*                seat->kbd.saved_focus = NULL; */
   /*             } */
   /*        } */
   /*   } */

   /* free the surface structure */
   E_FREE(es);
}

EAPI void 
e_surface_damage_calculate(E_Surface *es, pixman_region32_t *opaque)
{
   /* check for valid surface */
   if (!es) return;

   /* check for referenced buffer */
   if (es->buffer.reference.buffer)
     {
        /* if this is an shm buffer, flush any pending damage */
        if (wl_shm_buffer_get(es->buffer.reference.buffer->wl.resource))
          {
             if (_e_comp->renderer->damage_flush)
               _e_comp->renderer->damage_flush(es);
          }
     }

   /* TODO: handle transforms */

   pixman_region32_translate(&es->damage, 
                             es->geometry.x - es->plane->x, 
                             es->geometry.y - es->plane->y);

   pixman_region32_subtract(&es->damage, &es->damage, opaque);
   pixman_region32_union(&es->plane->damage, &es->plane->damage, &es->damage);

   pixman_region32_fini(&es->damage);
   pixman_region32_init(&es->damage);

   pixman_region32_copy(&es->clip, opaque);
}

EAPI void 
e_surface_show(E_Surface *es)
{
   /* check for valid surface */
   if (!es) return;

   /* ecore_evas_show(es->ee); */
}

EAPI void 
e_surface_repaint_schedule(E_Surface *es)
{
   E_Output *output;
   Eina_List *l;

   EINA_LIST_FOREACH(_e_comp->outputs, l, output)
     {
        if ((es->output == output) || (es->output_mask & (1 << output->id)))
          e_output_repaint_schedule(output);
     }
}

EAPI void 
e_surface_output_assign(E_Surface *es)
{
   E_Output *output, *noutput = NULL;
   pixman_region32_t region;
   pixman_box32_t *box;
   unsigned int area, mask = 0, max = 0;
   Eina_List *l;

   pixman_region32_fini(&es->bounding);
   pixman_region32_init_rect(&es->bounding, es->geometry.x, es->geometry.y, 
                             es->geometry.w, es->geometry.h);

   pixman_region32_init(&region);
   EINA_LIST_FOREACH(_e_comp->outputs, l, output)
     {
        pixman_region32_intersect(&region, &es->bounding, 
                                  &output->repaint.region);
        box = pixman_region32_extents(&region);
        area = ((box->x2 - box->x1) * (box->y2 - box->y1));
        if (area > 0) mask |= (1 << output->id);
        if (area >= max)
          {
             noutput = output;
             max = area;
          }
     }
   pixman_region32_fini(&region);
   es->output = noutput;
   es->output_mask = mask;
}

EAPI void 
e_surface_activate(E_Surface *es, E_Input *seat)
{
   E_Compositor *comp;

   if (!(comp = seat->compositor)) return;

   printf("Surface Activate: %p\n", es);

   if ((seat->keyboard) && (comp->focus))
     e_input_keyboard_focus_set(seat->keyboard, es);
   else if (seat->keyboard)
     {
        seat->kbd.saved_focus = es;
        seat->kbd.focus_listener.notify = e_input_keyboard_focus_destroy;
        wl_signal_add(&seat->kbd.saved_focus->signals.destroy, 
                      &seat->kbd.focus_listener);
     }

   wl_signal_emit(&comp->signals.activate, es);
}

EAPI int 
e_surface_buffer_width(E_Surface *es)
{
   return es->buffer.reference.buffer->w / es->scale;
}

EAPI int 
e_surface_buffer_height(E_Surface *es)
{
   return es->buffer.reference.buffer->h / es->scale;
}

/* local functions */
static void 
_e_surface_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   wl_resource_destroy(resource);
}

static void 
_e_surface_cb_attach(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *buffer_resource, int x, int y)
{
   E_Surface *es;
   E_Buffer *buffer = NULL;

   printf("E_Surface Attach\n");

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource)))
     {
        printf("\tCOULD NOT GET SURFACE FROM RESOURCE !!\n");
        return;
     }

   printf("\tHave Surface: %p\n", es);

   /* if we have a buffer resource, get a wl_buffer from it */
   if (buffer_resource) buffer = e_buffer_resource_get(buffer_resource);

   if (buffer) printf("\tHave Buffer\n");
   else printf("\tNO BUFFER !!!\n");

   if (buffer)
     {
        printf("\tBuffer Size: %d %d\n", buffer->w, buffer->h);
     }

   /* if we have a previous pending buffer, remove it
    * 
    * NB: This means that attach was called more than once without calling 
    * a commit in between, so we disgard any old buffers and just deal with 
    * the most current request */
   if (es->pending.buffer) wl_list_remove(&es->pending.buffer_destroy.link);

   /* set surface pending properties */
   es->pending.x = x;
   es->pending.y = y;
   es->pending.buffer = buffer;
   es->pending.new_attach = EINA_TRUE;

   /* if we have a valid pending buffer, setup a destroy listener */
   if (buffer) 
     wl_signal_add(&buffer->signals.destroy, &es->pending.buffer_destroy);
}

static void 
_e_surface_cb_damage(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int x, int y, int w, int h)
{
   E_Surface *es;

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource))) return;

   /* add this damage rectangle */
   pixman_region32_union_rect(&es->pending.damage, &es->pending.damage, 
                              x, y, w, h);
}

static void 
_e_surface_cb_commit(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_Surface *es;
   Evas_Coord bw = 0, bh = 0;
   pixman_region32_t opaque;

   printf("Surface Commit\n");

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource))) return;

   printf("\tHave Surface: %p\n", es);

   es->scale = es->pending.scale;

   /* if we have a pending buffer, attach it */
   if ((es->pending.buffer) || (es->pending.new_attach))
     e_surface_attach(es, es->pending.buffer);

   /* if we have a referenced buffer, get it's size */
   if (es->buffer.reference.buffer)
     {
        bw = e_surface_buffer_width(es);
        bh = e_surface_buffer_height(es);
     }

   /* if we attached a new buffer, call the surface configure function */
   if ((es->pending.new_attach) && (es->configure))
     es->configure(es, es->pending.x, es->pending.y, bw, bh);

   /* remove the destroy listener for the pending buffer */
   if (es->pending.buffer) wl_list_remove(&es->pending.buffer_destroy.link);

   /* clear surface pending properties */
   es->pending.buffer = NULL;
   es->pending.x = 0;
   es->pending.y = 0;
   es->pending.new_attach = EINA_FALSE;

   /* combine any pending damage */
   pixman_region32_union(&es->damage, &es->damage, &es->pending.damage);
   pixman_region32_intersect_rect(&es->damage, &es->damage, 
                                  0, 0, es->geometry.w, es->geometry.h);

   /* free any pending damage */
   pixman_region32_fini(&es->pending.damage);
   pixman_region32_init(&es->pending.damage);

   /* combine any pending opaque */
   pixman_region32_init_rect(&opaque, 0, 0, es->geometry.w, es->geometry.h);
   pixman_region32_intersect(&opaque, &opaque, &es->pending.opaque);
   if (!pixman_region32_equal(&opaque, &es->opaque))
     {
        pixman_region32_copy(&es->opaque, &opaque);
        es->geometry.changed = EINA_TRUE;
     }
   pixman_region32_fini(&opaque);

   /* combine any pending input */
   pixman_region32_fini(&es->input);
   pixman_region32_init_rect(&es->input, 0, 0, es->geometry.w, es->geometry.h);
   pixman_region32_intersect(&es->input, &es->input, &es->pending.input);

   /* add any pending frame callbacks to main list and free pending */
   wl_list_insert_list(&es->frames, &es->pending.frames);
   wl_list_init(&es->pending.frames);

   /* schedule repaint */
   e_surface_repaint_schedule(es);
}

static void 
_e_surface_cb_frame(struct wl_client *client, struct wl_resource *resource, unsigned int callback)
{
   E_Surface *es;
   E_Surface_Frame *cb;

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource))) return;

   /* try to create a new frame callback */
   if (!(cb = E_NEW_RAW(E_Surface_Frame, 1)))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   cb->resource = 
     wl_client_add_object(client, &wl_callback_interface, NULL, callback, cb);

   wl_resource_set_destructor(cb->resource, _e_surface_frame_cb_destroy);

   /* append the callback to pending frames */
   wl_list_insert(es->pending.frames.prev, &cb->link);
}

static void 
_e_surface_cb_opaque_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *region_resource)
{
   E_Surface *es;

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource))) return;

   if (region_resource)
     {
        E_Region *reg;

        /* try to cast this resource to our region */
        if (!(reg = wl_resource_get_user_data(region_resource)))
          return;
        pixman_region32_copy(&es->pending.opaque, &reg->region);
     }
   else
     {
        pixman_region32_fini(&es->pending.opaque);
        pixman_region32_init(&es->pending.opaque);
     }
}

static void 
_e_surface_cb_input_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, struct wl_resource *region_resource)
{
   E_Surface *es;

   /* try to cast the resource to our surface */
   if (!(es = wl_resource_get_user_data(resource))) return;

   if (region_resource)
     {
        E_Region *reg;

        /* try to cast this resource to our region */
        if (!(reg = wl_resource_get_user_data(region_resource)))
          return;
        pixman_region32_copy(&es->pending.input, &reg->region);
     }
   else
     {
        pixman_region32_fini(&es->pending.input);
        pixman_region32_init_rect(&es->pending.input, INT32_MIN, INT32_MIN, 
                                  UINT32_MAX, UINT32_MAX);
     }
}

static void 
_e_surface_cb_buffer_scale_set(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int scale)
{
   E_Surface *es;

   if (!(es = wl_resource_get_user_data(resource))) return;
   es->pending.scale = scale;
}

static void 
_e_surface_cb_buffer_destroy(struct wl_listener *listener, void *data EINA_UNUSED)
{
   E_Surface *es;

   /* try to get the surface structure from the listener */
   if (!(es = container_of(listener, E_Surface, pending.buffer_destroy)))
     return;

   /* clear the pending buffer */
   es->pending.buffer = NULL;
}

static void 
_e_surface_frame_cb_destroy(struct wl_resource *resource)
{
   E_Surface_Frame *cb;

   /* try to cast the resource to our callback */
   if (!(cb = wl_resource_get_user_data(resource))) return;

   wl_list_remove(&cb->link);

   E_FREE(cb);
}