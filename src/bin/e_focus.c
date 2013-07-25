#include "e.h"

/* local subsystem functions */
static Eina_Bool _e_focus_raise_timer(void *data);

/* local subsystem globals */

/* externally accessible functions */
EAPI void
e_focus_event_mouse_in(E_Client *ec)
{
   if ((e_config->focus_policy == E_FOCUS_MOUSE) ||
       (e_config->focus_policy == E_FOCUS_SLOPPY))
     {
        evas_object_focus_set(ec->frame, 1);
     }
   E_FREE_FUNC(ec->raise_timer, ecore_timer_del);
   if (e_config->use_auto_raise)
     {
        if (e_config->auto_raise_delay == 0.0)
          {
             if (!ec->lock_user_stacking)
               evas_object_raise(ec->frame);
          }
        else
          ec->raise_timer = ecore_timer_add(e_config->auto_raise_delay, _e_focus_raise_timer, ec);
     }
}

EAPI void
e_focus_event_mouse_out(E_Client *ec)
{
   if (e_config->focus_policy == E_FOCUS_MOUSE)
     {
        if (!ec->lock_focus_in)
          {
             if (ec->focused)
               evas_object_focus_set(ec->frame, 0);
          }
     }
   E_FREE_FUNC(ec->raise_timer, ecore_timer_del);
}

EAPI void
e_focus_event_mouse_down(E_Client *ec)
{
   if (e_client_focus_policy_click(ec) ||
       e_config->always_click_to_focus)
     evas_object_focus_set(ec->frame, 1);
   if (e_config->always_click_to_raise)
     {
        if (!ec->lock_user_stacking)
          evas_object_raise(ec->frame);
     }
}

EAPI void
e_focus_event_mouse_up(E_Client *ec __UNUSED__)
{
}

EAPI void
e_focus_event_focus_in(E_Client *ec)
{
   if ((e_client_focus_policy_click(ec)) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (!ec->button_grabbed) return;
        e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, e_client_util_pwin_get(ec));
        e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, e_client_util_pwin_get(ec));
        ecore_x_window_button_ungrab(e_client_util_pwin_get(ec), 1, 0, 1);
        ecore_x_window_button_ungrab(e_client_util_pwin_get(ec), 2, 0, 1);
        ecore_x_window_button_ungrab(e_client_util_pwin_get(ec), 3, 0, 1);
        e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, e_client_util_pwin_get(ec));
        e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, e_client_util_pwin_get(ec));
        ec->button_grabbed = 0;
     }
}

EAPI void
e_focus_event_focus_out(E_Client *ec)
{
   if ((e_client_focus_policy_click(ec)) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (ec->button_grabbed) return;
        ecore_x_window_button_grab(e_client_util_pwin_get(ec), 1,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(e_client_util_pwin_get(ec), 2,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(e_client_util_pwin_get(ec), 3,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ec->button_grabbed = 1;
     }
}

/* local subsystem functions */
static Eina_Bool
_e_focus_raise_timer(void *data)
{
   E_Client *ec = data;

   if (!ec->lock_user_stacking) evas_object_raise(ec->frame);
   ec->raise_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

