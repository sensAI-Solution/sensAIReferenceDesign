/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_info.h"
#include "hub_threading.h"

enum hub_ret_code hub_thread_create(hub_thread_hdl_t        *p_thread_hdl,
									hub_thread_attr_t       *p_thread_attr,
									hub_thread_worker_func_t worker_func,
									void                    *p_thread_args)
{
	int ret;

	/* TBD-DPN: Put error checks for failures here */
	ret = pthread_attr_init(p_thread_attr);

	/* TBD-DPN: By default we create all HUB threads as joinable */
	ret = pthread_attr_setdetachstate(p_thread_attr, PTHREAD_CREATE_JOINABLE);

	ret = pthread_create(p_thread_hdl, (const hub_thread_attr_t *)p_thread_attr,
						 worker_func, p_thread_args);

	if (ret < 0) {
		hub_pr_err("Error creating thread: %d", errno);
		return HUB_FAILURE_THREAD_CREATE;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_thread_join(hub_thread_hdl_t thread_hdl, void **retval)
{
	int ret = pthread_join(thread_hdl, retval);

	if (ret) {
		hub_pr_err("Error joining thread: %d", errno);
		return HUB_FAILURE_THREAD_JOIN;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_thread_cancel(hub_thread_hdl_t thread_hdl)
{
	int ret = pthread_cancel(thread_hdl);

	if (ret) {
		hub_pr_err("Error cancelling thread: %d\n", errno);
		return HUB_FAILURE_THREAD_CANCEL;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_mutex_init(hub_mutex_t *p_mutex)
{
	int ret = pthread_mutex_init(p_mutex, NULL);

	if (ret) {
		hub_pr_err("Error initing mutex: %d\n", errno);
		return HUB_FAILURE_MUTEX_INIT;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_mutex_lock(hub_mutex_t *p_mutex)
{
	int ret = pthread_mutex_lock(p_mutex);

	if (ret) {
		hub_pr_err("Error locking mutex: %d\n", errno);
		return HUB_FAILURE_MUTEX_LOCK;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_mutex_unlock(hub_mutex_t *p_mutex)
{
	int ret = pthread_mutex_unlock(p_mutex);

	if (ret) {
		hub_pr_err("Error unlocking mutex: %d\n", errno);
		return HUB_FAILURE_MUTEX_UNLOCK;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_mutex_try_unlock(hub_mutex_t *p_mutex)
{
	int ret = pthread_mutex_unlock(p_mutex);

	if (ret) {
		/**
		 * EPERM - The mutex is not owned by the calling thread.
		 * Not an error, return SUCCESS and let OS clean up.
		 */
		if (ret == EPERM) {
			return HUB_SUCCESS;
		}
		/* Other errors (like EINVAL) are still reported */
		hub_pr_err("Error unlocking mutex: %d\n", errno);
		return HUB_FAILURE_MUTEX_UNLOCK;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_mutex_destroy(hub_mutex_t *p_mutex)
{
	int ret = pthread_mutex_destroy(p_mutex);

	if (ret) {
		/* EINVAL means mutex is invalid (already destroyed or not initialized) */
		/* EBUSY means mutex is still locked - expected in some cleanup scenarios */
		/* These are expected in cleanup scenarios, so don't print error */
		if (ret != EINVAL && ret != EBUSY) {
			hub_pr_err("Error destroying mutex: %d, ret : %d\n", errno, ret);
		}
		return HUB_FAILURE_MUTEX_DESTROY;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_cond_var_init(hub_cond_var_t *p_cond_var)
{
	int ret = pthread_cond_init(p_cond_var, NULL);

	if (ret) {
		hub_pr_err("Error initing cond_var: %d\n", errno);
		return HUB_FAILURE_CONDVAR_INIT;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_cond_var_wait(hub_cond_var_t *p_cond_var,
									hub_mutex_t    *p_mutex)
{
	int ret = pthread_cond_wait(p_cond_var, p_mutex);

	if (ret) {
		hub_pr_err("Error in cond_var wait: %d\n", errno);
		return HUB_FAILURE_CONDVAR_WAIT;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_cond_var_signal(hub_cond_var_t *p_cond_var)
{
	int ret = pthread_cond_signal(p_cond_var);

	if (ret) {
		hub_pr_err("Error in cond_var singal: %d\n", errno);
		return HUB_FAILURE_CONDVAR_SIGNAL;
	}

	return HUB_SUCCESS;
}

enum hub_ret_code hub_cond_var_destroy(hub_cond_var_t *p_cond_var)
{
	int ret = pthread_cond_destroy(p_cond_var);

	if (ret) {
		hub_pr_err("Error destroying cond_var: %d\n", errno);
		return HUB_FAILURE_CONDVAR_DESTROY;
	}

	return HUB_SUCCESS;
}