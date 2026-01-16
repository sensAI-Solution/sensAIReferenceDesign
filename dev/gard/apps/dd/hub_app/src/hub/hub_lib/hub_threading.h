/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_THREADING_H__
#define __HUB_THREADING_H__

#include <pthread.h>
#include <errno.h>

typedef pthread_t       hub_thread_hdl_t;
typedef pthread_attr_t  hub_thread_attr_t;

typedef pthread_cond_t  hub_cond_var_t;
typedef pthread_mutex_t hub_mutex_t;

typedef void           *(*hub_thread_worker_func_t)(void *);

enum hub_ret_code       hub_thread_create(hub_thread_hdl_t        *p_thread_hdl,
										  hub_thread_attr_t       *p_thread_attr,
										  hub_thread_worker_func_t worker_func,
										  void                    *p_thread_args);

enum hub_ret_code hub_thread_join(hub_thread_hdl_t thread_hdl, void **retval);

enum hub_ret_code hub_thread_cancel(hub_thread_hdl_t thread_hdl);

enum hub_ret_code hub_mutex_init(hub_mutex_t *p_mutex);
enum hub_ret_code hub_mutex_lock(hub_mutex_t *p_mutex);
enum hub_ret_code hub_mutex_unlock(hub_mutex_t *p_mutex);
enum hub_ret_code hub_mutex_try_unlock(hub_mutex_t *p_mutex);
enum hub_ret_code hub_mutex_destroy(hub_mutex_t *p_mutex);

enum hub_ret_code hub_cond_var_init(hub_cond_var_t *p_cond_var);
enum hub_ret_code hub_cond_var_wait(hub_cond_var_t *p_cond_var,
									hub_mutex_t    *p_mutex);
enum hub_ret_code hub_cond_var_signal(hub_cond_var_t *p_cond_var);
enum hub_ret_code hub_cond_var_destroy(hub_cond_var_t *p_cond_var);

#endif /* __HUB_THREADING_H__ */