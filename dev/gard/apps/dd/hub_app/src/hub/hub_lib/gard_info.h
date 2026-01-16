/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __GARD_INFO_H__
#define __GARD_INFO_H__

#include "hub.h"
#include "gard_hub_iface.h"
#include "types.h"
#include "hub_threading.h"

/**
 * MIN MAX functions for uint32_t and int32_t
 */
static inline uint32_t hub_min_uint32(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

static inline uint32_t hub_max_uint32(uint32_t a, uint32_t b)
{
	return (a > b) ? a : b;
}

static inline int32_t hub_min_int32(int32_t a, int32_t b)
{
	return (a < b) ? a : b;
}

static inline int32_t hub_max_int32(int32_t a, int32_t b)
{
	return (a > b) ? a : b;
}

/* HUB handle / context states */
enum hub_ctx_state {
	HUB_IN_ERROR = -1,
	HUB_BEGIN    = 0,
	HUB_PREINIT_DONE,
	HUB_DISCOVER_DONE,
	HUB_INIT_DONE
};

/* Possible methods for probing a particular bus. Not used as of now. */
enum hub_gard_probe_method {
	HUB_GARD_PROBE_SYSTEM_WIDE = 0,
	HUB_GARD_PROBE_BUS_WIDE,
	HUB_GARD_PROBE_EXPLICIT,
	NR_HUB_GARD_PROBE_METHODS,
};

/* HUB info prints */
#define hub_pr_info(fmt, args...)                                              \
	fprintf(stdout, "HUB_INFO: %s:%d:%s(): " fmt, __FILE__, __LINE__,          \
			__func__, ##args)

/**
 * HUB debug prints: #define HUB_DEBUG for debug prints in HUB's Makefile.vars
 */
#if defined(HUB_DEBUG)
#define hub_pr_dbg(fmt, args...)                                               \
	fprintf(stdout, "HUB_DBG: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
			##args)
#else
#define hub_pr_dbg(fmt, args...) /* Don't do anything in non-debug builds */
#endif

/* HUB error prints */
#define hub_pr_err(fmt, args...)                                               \
	fprintf(stderr, "HUB_ERR: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
			##args)

/* HUB warn prints */
#define hub_pr_warn(fmt, args...)                                              \
	fprintf(stdout, "HUB_WARN: %s:%d:%s(): " fmt, __FILE__, __LINE__,          \
			__func__, ##args)

/* Properties of an I2C bus on HUB */
struct hub_gard_bus_i2c_props {
	int      bus_hdl;
	bool     is_open;
	uint32_t num;
	uint32_t slave_id;
	uint32_t speed;
};

/* Properties of a UART bus on HUB */
struct hub_gard_bus_uart_props {
	int      bus_hdl;
	bool     is_open;
	char     bus_dev[PATH_MAX];
	uint32_t baudrate;
};

/* Properties of a USB bus on HUB */
struct hub_gard_bus_usb_props {
	int      bus_hdl;
	bool     is_open;
	uint32_t vendor_id;
	uint32_t product_id;
};

/**
 * This is a special struct used only for USB ops
 * Since we have to adhere to the Python API for write/read
 * as of now, we bundle the buffer and address into
 * an opaque pointer for USB read_blob and write_blob
 */
struct hub_usb_ops_map {
	void    *p_buffer;
	uint32_t addr;
};

/**
 * The file operations structure for a HUB bus.
 * Functions for all busses need to adhere to this API.
 */
struct hub_gard_bus_fops {
	int32_t (*device_open)(void *params);
	int32_t (*device_read)(int bus_hdl, void *p_buffer, uint32_t count);
	int32_t (*device_write)(int bus_hdl, const void *p_buffer, uint32_t count);
	int32_t (*device_close)(int bus_hdl);
};

/**
 * Structure for holding details and context of a HUB bus
 *
 * Note that we define an anon union for the exact bus, since
 * the bus can be only 1 of all possible types defined
 * in HUB.
 */
struct hub_gard_bus {
	enum hub_gard_bus_types types;

	union {
		struct hub_gard_bus_i2c_props  i2c;
		struct hub_gard_bus_uart_props uart;
		struct hub_gard_bus_usb_props  usb;
	};
	struct hub_gard_bus_fops fops;
	hub_mutex_t              bus_mutex;
};

/**
 * This is the back-end of the gard_handle_t.
 * Used internally for GARD operations.
 *
 * For every GARD, HUB defines 2 bus types:
 * 1. A control bus for register reads/writes
 * 2. A data bus for buffer reads/writes
 *
 * TBD-DPN:
 * Presently, which bus to use for what is defined by the
 * GARD's .json file parsed by HUB.
 * In future, HUB routines will maintain a state of bus state
 * and bus load, and switch from one bus to the other
 * in a transparent manner.
 */
struct hub_gard_info {
	hub_handle_t         hub;
	uint32_t             gard_id;
	char                 gard_name[PATH_MAX];
	char                 gpio_chip[PATH_MAX];
	uint32_t             num_gpio_inputs;
	uint32_t             num_gpio_outputs;
	uint32_t            *gpio_inputs;
	uint32_t            *gpio_outputs;
	struct hub_gard_bus *control_bus;
	struct hub_gard_bus *data_bus;
};

/**
 * This is the back-end of the hub_handle_t.
 * Used internally for HUB operations.
 *
 * It contains the entire context of a current HUB instance:
 * 1. hub_state
 * 2. number of busses found in the host config file
 * 3. number of GARDs that responded to discovery command
 * 4. full path to the GARD json dir
 * 5. detailed properties of each bus found
 * 6. a listing of all valid GARDs parsed from the relevant GARD json file
 * 7. HUB GPIO monitor ctx
 * 8. HUB GPIO event ctx's
 */
struct hub_ctx {
	enum hub_ctx_state         hub_state;
	uint32_t                   num_busses;
	uint32_t                   num_gards;
	uint32_t                   num_gpio_threads;
	char                       gard_json_dir[PATH_MAX];
	struct hub_gard_bus       *p_bus_props;
	struct hub_gard_info      *p_gards;
	struct hub_gpio_mon_ctx   *p_gpio_mon_ctx;
	struct hub_gpio_event_ctx *p_gpio_event_ctx;
};

#endif /* __GARD_INFO_H__ */