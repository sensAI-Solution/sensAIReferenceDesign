/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "assert.h"
#include "gard_types.h"
#include "gard_hub_iface.h"
#include "host_cmds.h"
#include "gard_hub_iface_unpacked.h"
#include "iface_support.h"
#include "utils.h"
#include "fw_globals.h"
#include "ml_ops.h"
#include "camera_capture.h"
#include "pipeline_ops.h"

enum host_request_service_state {
	REQUEST_IFACE_TO_RECV_CMD_ID = 1,
	IFACE_WAIT_FOR_CMD_ID,
	IFACE_CALCULATE_CMD_BODY_SIZE,
	REQUEST_IFACE_TO_RECV_CMD_BODY,
	IFACE_WAIT_FOR_CMD_BODY,
	EXECUTE_HOST_IFACE_CMD,

	// Following states are for SEND_DATA_TO_GARD_FOR_OFFSET command
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__START_PROCESSING,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__REQ_PAYLOAD,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__WAIT_FOR_PAYLOAD,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___REQ_EOD_MARKER,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_EOD_MARKER,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_EOD_MARKER,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_CRC,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___SEND_ACK,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_ACK_TO_SEND,
	EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__END_PROCESSING,

	// Following states are for RECV_DATA_FROM_GARD_AT_OFFSET command
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__START_PROCESSING,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_START_OF_DATA_MARKER,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_SOD_SEND,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_PAYLOAD,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_PAYLOAD_SEND,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_END_OF_DATA_MARKER,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_EOD_SEND,
	EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__END_PROCESSING,

	// Following states are for READ_REG_VALUE_FROM_GARD_AT_OFFSET command
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__START_PROCESSING,
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__VALIDATE_EOD_MARKER,
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__END_PROCESSING,

	// Following states are for WRITE_REG_VALUE_TO_GARD_AT_OFFSET command
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__START_PROCESSING,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__VALIDATE_EOD_MARKER,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__WRITE_REG_VALUE_TO_GARD,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__END_PROCESSING,

	// Following states are for GET_ML_ENGINE_STATUS command
	EXECUTE_CMD_GET_ML_ENGINE_STATUS__START_PROCESSING,
	EXECUTE_CMD_GET_ML_ENGINE_STATUS__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_GET_ML_ENGINE_STATUS__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_GET_ML_ENGINE_STATUS__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_GET_ML_ENGINE_STATUS__END_PROCESSING,

	// Following states are for GARD_DISCOVERY_OVER_IFACE command
	EXECUTE_CMD_DISCOVERY__START_PROCESSING,
	EXECUTE_CMD_DISCOVERY__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_DISCOVERY__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_DISCOVERY__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_DISCOVERY__END_PROCESSING,

	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__START_PROCESSING,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__VALIDATE_PARAMETERS,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__TRIGGER_IMAGE_CAPTURE,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_IMAGE_CAPTURE,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__END_PROCESSING,

	EXECUTE_CMD_RESUME_PIPELINE__START_PROCESSING,
	EXECUTE_CMD_RESUME_PIPELINE__VALIDATE_PARAMETERS,
	EXECUTE_CMD_RESUME_PIPELINE__TRIGGER_PIPELINE_RESUME,
	EXECUTE_CMD_RESUME_PIPELINE__COMPOSE_RESPONSE_TO_SEND,
	EXECUTE_CMD_RESUME_PIPELINE__SEND_RESPONSE_TO_HOST,
	EXECUTE_CMD_RESUME_PIPELINE__WAIT_FOR_RESPONSE_SEND,
	EXECUTE_CMD_RESUME_PIPELINE__END_PROCESSING,
};

/**
 * host_request_init initializes the host request service module by initializing
 * all the variables that it needs for its operation. It initializes all the
 * valid interfaces that are used by the host request service.
 * This function should be called before any host request processing is done.
 * It is expected that the interfaces (UART or I2C) have already been
 * initialized (by calling ifaces_init()) before this function is called.
 *
 * @return true if the initialization is successful, false otherwise.
 */
bool host_request_init(void)
{
	uint32_t iface_idx;

	// Here get all the valid interfaces. They should have already been
	// initialized before this call.

	for (iface_idx = 0; iface_idx < valid_ifaces; iface_idx++) {
		/**
		 * Initialize the instance data for each interface.
		 */
		iface_inst[iface_idx].hc_data.rx_done = false;
		iface_inst[iface_idx].hc_data.tx_done = false;
		iface_inst[iface_idx].hc_data.host_request_service_state =
			REQUEST_IFACE_TO_RECV_CMD_ID;
	}

	return (valid_ifaces > 0);
}

/**
 * set_rx_done sets the flag rx_done to the value passed as parameter.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 *
 */
void set_rx_done(struct iface_instance *inst)
{
	inst->hc_data.rx_done = true;
}

/**
 * set_tx_done sets the flag tx_done to the value passed as parameter.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 */
void set_tx_done(struct iface_instance *inst)
{
	inst->hc_data.tx_done = true;
}

/**
 * exec_send_data_to_gard_for_offset executes the state machine for
 * SEND_DATA_TO_GARD_FOR_OFFSET command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_send_data_to_gard_for_offset(
	struct iface_instance           *inst,
	enum host_request_service_state *current_state,
	struct _host_requests_unpked    *host_req,
	struct _host_responses_unpked   *host_resp)
{
	uint32_t                                              bytes_to_read;
	struct _send_data_to_gard_for_offset_request_unpked  *p_send_data_req;
	struct _send_data_to_gard_for_offset_response_unpked *p_send_data_resp;

	p_send_data_req = &host_req->send_data_to_gard_for_offset_request;

	// This function executes the SEND_DATA_TO_GARD_FOR_OFFSET command.
	// It handles the state transitions and data transfers as per the command
	// requirements.

	switch (*current_state) {
	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__START_PROCESSING:
	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__REQ_PAYLOAD:

		// Request interface to receive the data from the Host and write it to
		// GARD at the specified offset. Following the data the Host will also
		// send the end-of-data marker and optionally a checksum. These later 2
		// fields should be received separately in our local buffer, hence we
		// will need to split the receive operation into two steps.

		inst->hc_data.rx_done = false;  // Reset the flag to wait for new data.
		inst->read_data_async_call(
			inst, p_send_data_req->cmd.data_size,
			(uint8_t *)p_send_data_req->cmd.offset_address);

		*current_state = EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__WAIT_FOR_PAYLOAD;
		// Fall through to check if the data has arrived.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__WAIT_FOR_PAYLOAD:
		// Check if all the requested data has been received.
		if (!inst->hc_data.rx_done) {
			return false;
		}

		// Payload data has arrived, now request interface to receive the
		// end-of-data marker and optional CRC for the command.

		// Fall through.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___REQ_EOD_MARKER:

		bytes_to_read = sizeof(p_send_data_req->eod.end_of_data_marker);

		// In the current structure the end-of-data marker section is of the
		// same size for both packed and unpacked structures. We use this to
		// our advantage to read the end-of-data marker and optional CRC
		// together in in the unpacked structure so that we can avoid the
		// memcpy.
		GARD__CASSERT(
			sizeof(p_send_data_req->eod) ==
				sizeof((struct _send_data_to_gard_for_offset_request *)0)->eod,
			"Sizes of packed and unpacked structures mismatch.");
		if (p_send_data_req->cmd.control_code & CC_CHECKSUM_PRESENT) {
			// If checksum is present, we need to read the end-of-data marker
			// and the checksum.
			bytes_to_read += sizeof(p_send_data_req->eod.opt_crc);
		}

		// Reset the flag and request interface to receive the end-of-data
		// marker and optional CRC.
		inst->hc_data.rx_done = false;
		inst->read_data_async_call(
			inst, bytes_to_read,
			(uint8_t *)&p_send_data_req->eod.end_of_data_marker);

		*current_state =
			EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_EOD_MARKER;

		// Fall through to check if the data has arrived.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_EOD_MARKER:
		// Check if end-of-data marker has been received.
		if (!inst->hc_data.rx_done) {
			return false;
		}

		// Fall through to process the end-of-data marker.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_EOD_MARKER:

		if (p_send_data_req->eod.end_of_data_marker != END_OF_DATA_MARKER) {
			// Handle error in end-of-data marker.
			// This could involve logging an error or sending an error
			// response.
			*current_state =
				EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_EOD_MARKER;
			return false;  // Error in end-of-data marker, abort execution.
		} else if (!(p_send_data_req->cmd.control_code & CC_CHECKSUM_PRESENT)) {
			// Done executing the SEND_DATA_TO_GARD_FOR_OFFSET command.
			// Check if ACK is requested.
			*current_state = EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___SEND_ACK;
			break;
		}

		// CRC received, fall thru to validate it.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_CRC:

		if (p_send_data_req->eod.opt_crc !=
			calculate_checksum(p_send_data_req->data,
							   p_send_data_req->cmd.data_size)) {
			// Handle checksum error.
			// This could involve logging an error or sending an error
			// response.
			*current_state = EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___VALIDATE_CRC;
			return false;  // Error in checksum, abort execution.
		}

		// Fall through to send ACK if required.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___SEND_ACK:

		p_send_data_resp = &host_resp->send_data_to_gard_for_offset_response;

		if (!(p_send_data_req->cmd.control_code & CC_SEND_ACK_AFTER_XFER)) {
			// Command complete. Go back to start state to wait for new
			// command.
			*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
			break;
		}

		// If ACK is requested, send acknowledgment to terminate this
		// command transaction. Since the ACK byte is 1-byte long we can send it
		// in either the packed or unpacked structure. Here we use the
		// unpacked structure to send the ACK byte.
		inst->hc_data.tx_done = false;
		p_send_data_resp->opt_ack =
			ACK_BYTE;  // Set ACK byte to indicate success.
		inst->send_data_async_call(inst, sizeof(p_send_data_resp->opt_ack),
								   &p_send_data_resp->opt_ack);

		*current_state =
			EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_ACK_TO_SEND;

		// Fall through to wait for ACK to be sent.

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET___WAIT_FOR_ACK_TO_SEND:
		// Check if ACK sending done.
		if (!inst->hc_data.tx_done) {
			return false;
		}

		// Command complete. Go back to start state to wait for new
		// command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;

	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;  // No commands processed in this stub.
}

/**
 * exec_recv_data_from_gard_at_offset executes the state machine for
 * RECV_DATA_FROM_GARD_AT_OFFSET command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_recv_data_from_gard_at_offset(
	struct iface_instance           *inst,
	enum host_request_service_state *current_state,
	struct _host_requests_unpked    *host_req,
	struct _host_responses_unpked   *host_resp)
{
	uint32_t                                               bytes_to_send;
	struct _recv_data_from_gard_at_offset_request_unpked  *p_recv_data_req;
	struct _recv_data_from_gard_at_offset_response_unpked *p_recv_data_resp;
	uint8_t                                               *data_to_send_addr;

	p_recv_data_req  = &host_req->recv_data_from_gard_at_offset_request;
	p_recv_data_resp = &host_resp->recv_data_from_gard_at_offset_response;

	// This function executes the RECV_DATA_FROM_GARD_AT_OFFSET command.
	// It handles the state transitions and data transfers as per the command
	// documentation.

	switch (*current_state) {
	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__START_PROCESSING:
	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_START_OF_DATA_MARKER:
		// We need to send data at GARD offset to Host.
		// We will first send the START_OF_DATA marker to indicate the start
		// of data transfer.
		// Then we will send the data from GARD at the specified offset.
		// After the data transfer is complete, we will send the
		// END_OF_DATA marker and optionally a checksum.

		// Since the start-of-data marker is a 4-byte long field and we send
		// this one individually we use the unpacked structure to send this
		// though we could equally have used the packed structure if needed.
		p_recv_data_resp->start_of_data_marker = START_OF_DATA_MARKER;
		bytes_to_send = sizeof(p_recv_data_resp->start_of_data_marker);

		if (p_recv_data_req->control_code & CC_APP_DATA) {
			/**
			 * If the data to be sent is by the App Module then send the size of
			 * the data.
			 */
			p_recv_data_resp->data_size =
				(p_recv_data_req->data_size < app_tx_buffer_size)
					? p_recv_data_req->data_size
					: app_tx_buffer_size;
		} else {
			/**
			 * For non App Module data transfers, set the original data size.
			 */
			p_recv_data_resp->data_size = p_recv_data_req->data_size;
		}
		bytes_to_send         += sizeof(p_recv_data_resp->data_size);

		// Reset the flag to wait for data to be sent.
		inst->hc_data.tx_done  = false;
		inst->send_data_async_call(
			inst, bytes_to_send,
			(uint8_t *)&p_recv_data_resp->start_of_data_marker);

		*current_state =
			EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_SOD_SEND;

		// Fall thru to wait for the start of data marker to be sent.

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_SOD_SEND:
		if (!inst->hc_data.tx_done) {
			return false;
		}

		// Fall through to prepare to send the payload data.

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_PAYLOAD:

		// Reset the flag to wait for data to be sent.
		inst->hc_data.tx_done = false;
		if (p_recv_data_req->control_code & CC_APP_DATA) {
			/**
			 * If the data to be sent is in the App Modules buffer then use that
			 * buffer to send the data.
			 */
			data_to_send_addr = app_tx_buffer;
		} else {
			/**
			 * Otherwise use the offset address provided in the command to
			 * send the data.
			 */
			data_to_send_addr = (uint8_t *)p_recv_data_req->offset_address;
		}
		inst->send_data_async_call(inst, p_recv_data_resp->data_size,
								   data_to_send_addr);

		*current_state =
			EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_PAYLOAD_SEND;

		// Fall through to wait for payload to be sent.

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_PAYLOAD_SEND:
		if (!inst->hc_data.tx_done) {
			return false;
		}

		// Payload data has been sent,
		// Fall through to send the end-of-data marker and optionally a
		// checksum.

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__SEND_END_OF_DATA_MARKER:

		p_recv_data_req = &host_req->recv_data_from_gard_at_offset_request;

		// If the packed and unpacked versions of end-of-data structure are the
		// same then we will use the unpacked structure to send the end-of-data
		// for the response frame.
		GARD__CASSERT(
			sizeof(p_recv_data_resp->eod) ==
				sizeof((struct _recv_data_from_gard_at_offset_response *)0)
					->eod,
			"Sizes of packed and unpacked structures mismatch.");

		bytes_to_send = sizeof(p_recv_data_resp->eod.end_of_data_marker);
		p_recv_data_resp->eod.end_of_data_marker =
			END_OF_DATA_MARKER;  // Set the end of data marker.
		if (p_recv_data_req->control_code & CC_CHECKSUM_PRESENT) {
			// If checksum is present, we need to send the end-of-data
			// marker and the checksum.
			bytes_to_send += sizeof(p_recv_data_resp->eod.opt_crc);
			p_recv_data_resp->eod.opt_crc =
				calculate_checksum((uint8_t *)p_recv_data_req->offset_address,
								   p_recv_data_req->data_size);
		}

		// Reset the flag to wait for data to be sent.
		inst->hc_data.tx_done = false;
		inst->send_data_async_call(
			inst, bytes_to_send,
			(uint8_t *)&(p_recv_data_resp->eod.end_of_data_marker));

		*current_state =
			EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_EOD_SEND;

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__WAIT_FOR_EOD_SEND:
		if (!inst->hc_data.tx_done) {
			return false;
		}

		/* If App Data was sent flag the marker as Job is DONE. */
		if (p_recv_data_req->control_code & CC_APP_DATA) {
			*p_app_tx_complete = true;
		}
		/**
		 * Command complete. Go back to start state to wait for new
		 * command.
		 */
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;
	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;
}

/**
 * exec_read_reg_value_from_gard_at_offset executes the state machine for
 * READ_REG_VALUE_FROM_GARD_AT_OFFSET command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_read_reg_value_from_gard_at_offset(
	struct iface_instance           *inst,
	enum host_request_service_state *current_state,
	struct _host_requests           *host_req,
	struct _host_responses_unpked   *host_resp)
{
	struct _read_reg_value_from_gard_at_offset_request         *p_read_reg_req;
	struct _read_reg_value_from_gard_at_offset_response_unpked *p_read_reg_resp;
	uint32_t                                                   *p_reg;

	p_read_reg_req = &host_req->read_reg_value_from_gard_at_offset_request;

	// This function executes the READ_REG_VALUE_FROM_GARD_AT_OFFSET command.
	// It handles the state transitions and data transfers as per the command
	// documentation.

	switch (*current_state) {
	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__START_PROCESSING:
	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__VALIDATE_EOD_MARKER:
		// We need to read a register value from GARD at the specified offset.
		// The command will provide the offset address, and we will read the
		// register value from that address.

		if (p_read_reg_req->end_of_data_marker != END_OF_DATA_MARKER) {
			// Handle error in end-of-data marker.
			// This could involve logging an error or sending an error
			// response.
			*current_state =
				EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__VALIDATE_EOD_MARKER;
			return false;  // Error in end-of-data marker, abort execution.
		}

		// Fall through to compose the response to send.

	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__COMPOSE_RESPONSE_TO_SEND:

		p_read_reg_resp =
			&host_resp->read_reg_value_from_gard_at_offset_response;
		p_reg = (uint32_t *)p_read_reg_req->offset_address;

		p_read_reg_resp->start_of_data_marker =
			START_OF_DATA_MARKER;  // Set the start of data marker.
		p_read_reg_resp->reg_value =
			*p_reg;  // Read the register value from GARD.
		p_read_reg_resp->end_of_data_marker =
			END_OF_DATA_MARKER;  // Set the end of data marker.

		// If the packed and unpacked versions of response structure are the
		// same then we will use the unpacked structure to send the response
		// frame.
		GARD__CASSERT(
			sizeof(*p_read_reg_resp) ==
				sizeof(struct _read_reg_value_from_gard_at_offset_response),
			"Sizes of packed and unpacked structures mismatch.");

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.
		inst->send_data_async_call(inst, sizeof(*p_read_reg_resp),
								   (uint8_t *)p_read_reg_resp);

		*current_state =
			EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.

	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND:
		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Response has been sent, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;
	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;
}

/**
 * exec_write_reg_value_to_gard_at_offset executes the state machine for
 * WRITE_REG_VALUE_TO_GARD_AT_OFFSET command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_write_reg_value_to_gard_at_offset(
	struct iface_instance           *inst,
	enum host_request_service_state *current_state,
	struct _host_requests_unpked    *host_req,
	struct _host_responses_unpked   *host_resp)
{
	struct _write_reg_value_to_gard_at_offset_request_unpked  *p_write_reg_req;
	struct _write_reg_value_to_gard_at_offset_response_unpked *p_write_reg_resp;
	uint32_t                                                  *p_reg;

	p_write_reg_req = &host_req->write_reg_value_to_gard_at_offset_request;

	// This function executes the WRITE_REG_VALUE_TO_GARD_AT_OFFSET command.
	// It handles the state transitions and data transfers as per the command
	// documentation.

	switch (*current_state) {
	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__START_PROCESSING:
	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__VALIDATE_EOD_MARKER:
		if (p_write_reg_req->end_of_data_marker != END_OF_DATA_MARKER) {
			// Handle error in end-of-data marker.
			// This could involve logging an error or sending an error
			// response.
			*current_state =
				EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__VALIDATE_EOD_MARKER;
			return false;  // Error in end-of-data marker, abort execution.
		}

		// Fall through to write the value to the GARD memory address space.

	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__WRITE_REG_VALUE_TO_GARD:
		p_reg  = (uint32_t *)p_write_reg_req->offset_address;

		*p_reg = p_write_reg_req->data;  // Write the register value to GARD.

		// Fall through to compose the response to send.

	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__COMPOSE_RESPONSE_TO_SEND:
		p_write_reg_resp =
			&host_resp->write_reg_value_to_gard_at_offset_response;

		p_write_reg_resp->ack = ACK_BYTE;  // Set ACK byte to indicate success.

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.
		inst->send_data_async_call(inst, sizeof(*p_write_reg_resp),
								   (uint8_t *)p_write_reg_resp);

		*current_state =
			EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.

	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__WAIT_FOR_RESPONSE_SEND:
		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Write is done, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;
	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;
}

/**
 * exec_gel_ml_engine_status executes the state machine for GET_ML_ENGINE_STATUS
 * command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool
	exec_gel_ml_engine_status(struct iface_instance           *inst,
							  enum host_request_service_state *current_state,
							  struct _host_requests_unpked    *host_req,
							  struct _host_responses_unpked   *host_resp)
{
	struct _get_ml_engine_status_request_unpked  *p_get_ml_eng_stat_req;
	struct _get_ml_engine_status_response_unpked *p_get_ml_eng_stat_resp;

	p_get_ml_eng_stat_resp = &host_resp->get_ml_engine_status_response;

	// This function executes the GET_ML_ENGINE_STATUS command.
	// It handles the state transitions and data transfers as per the command
	// documentation.

	switch (*current_state) {
	case EXECUTE_CMD_GET_ML_ENGINE_STATUS__START_PROCESSING:
	case EXECUTE_CMD_GET_ML_ENGINE_STATUS__COMPOSE_RESPONSE_TO_SEND:
		p_get_ml_eng_stat_req = &host_req->get_ml_engine_status_request;

		// If the packed and unpacked versions of get_ml_engine_status_response
		// structure are the same then we will use the unpacked structure to
		// send the end-of-data for the response frame.
		GARD__CASSERT(sizeof(*p_get_ml_eng_stat_resp) ==
						  sizeof(struct _get_ml_engine_status_response),
					  "Sizes of packed and unpacked structures mismatch.");

		p_get_ml_eng_stat_resp->start_of_data_marker =
			START_OF_DATA_MARKER;  // Set the start of data marker.
		p_get_ml_eng_stat_resp->status_code = get_ml_engine_status(
			p_get_ml_eng_stat_req->engine_id);  // Get the ML engine status
		p_get_ml_eng_stat_resp->end_of_data_marker =
			END_OF_DATA_MARKER;  // Set the end of data marker.

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_GET_ML_ENGINE_STATUS__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.
		inst->send_data_async_call(inst, sizeof(*p_get_ml_eng_stat_resp),
								   (uint8_t *)p_get_ml_eng_stat_resp);

		*current_state =
			EXECUTE_CMD_GET_ML_ENGINE_STATUS__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.

	case EXECUTE_CMD_GET_ML_ENGINE_STATUS__WAIT_FOR_RESPONSE_SEND:
		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Response has been sent, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;
	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;
}

/**
 * exec_gard_discovery executes the state machine for GARD_DISCOVERY_OVER_IFACE
 * command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_gard_discovery(struct iface_instance           *inst,
								enum host_request_service_state *current_state,
								struct _host_requests_unpked    *host_req,
								struct _host_responses_unpked   *host_resp)
{
	struct _gard_discovery_response_unpked *p_disc_resp =
		&host_resp->gard_discovery_response;
	struct _gard_discovery_response temp = {.start_of_data_marker = 0x50DB50DBU,
											.signature            = "I AM GARD",
											.end_of_data_marker =
												END_OF_DATA_MARKER};

	// In the interest of space we will use the unpacked structure instead of
	// the packed structure to send the response frame. This is because we know
	// that the unpacked structure can be greater or equal to the size of his
	// packed cousin. This way we know we will not be overflowing the buffer
	// when we send the response frame.
	GARD__CASSERT(sizeof(*p_disc_resp) >=
					  sizeof(struct _gard_discovery_response),
				  "Sizes of packed and unpacked structures mismatch.");

	switch (*current_state) {
	case EXECUTE_CMD_DISCOVERY__START_PROCESSING:
	case EXECUTE_CMD_DISCOVERY__COMPOSE_RESPONSE_TO_SEND:
		*((struct _gard_discovery_response *)p_disc_resp) =
			temp;  // Copy the temporary response to the actual
				   // response structure.

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_DISCOVERY__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.
		inst->send_data_async_call(inst, sizeof(temp), (uint8_t *)p_disc_resp);

		*current_state = EXECUTE_CMD_DISCOVERY__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.

	case EXECUTE_CMD_DISCOVERY__WAIT_FOR_RESPONSE_SEND:
		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Response has been sent, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;

#if defined(TEST_APP_MOD_DATA_XFER_TRIGGER)
		/**
		 * To test App_Mod data transfer functionality send some dummy data
		 * after receiving the GARD_DISCOVERY command. This data sending will
		 * start 5 seconds after the GARD_DISCOVERY response is sent.
		 */
		set_timer_for_time(5000);
#endif

		break;
	default:
		// Invalid state, reset to start state.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		return false;  // Error in state machine, abort execution.
	}

	return true;
}

/**
 * exec_capture_rescaled_image executes the state machine for
 * CAPTURE_RESCALED_IMAGE command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool
	exec_capture_rescaled_image(struct iface_instance           *inst,
								enum host_request_service_state *current_state,
								struct _host_requests_unpked    *host_req,
								struct _host_responses_unpked   *host_resp)
{
	struct _capture_rescaled_image_request_unpked  *p_capture_img_req;
	struct _capture_rescaled_image_response_unpked *p_capture_img_resp;
	struct image_info                               captured_img_info;

	p_capture_img_req  = &host_req->capture_rescaled_image_request_unpked;
	p_capture_img_resp = &host_resp->capture_rescaled_image_response_unpked;

	switch (*current_state) {
	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__START_PROCESSING:
	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__VALIDATE_PARAMETERS:

		if (p_capture_img_req->end_of_data_marker != END_OF_DATA_MARKER) {
			// Handle error in end-of-data marker.
			// This could involve logging an error or sending an error
			// response.
			*current_state =
				EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__VALIDATE_PARAMETERS;
			return false;  // Error in end-of-data marker, abort execution.
		}

		// Currently only camera ID 0 is supported.
		GARD__DBG_ASSERT(p_capture_img_req->camera_id == 0,
						 "Unsupported camera ID");

		if (!camera_started) {
			*current_state =
				EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__COMPOSE_RESPONSE_TO_SEND;
			return false;  // Camera not started, skip calling capture.
		}

		// Fall through to trigger the capture image process.

	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__TRIGGER_IMAGE_CAPTURE:

		capture_rescaled_image_async();

		*current_state =
			EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_IMAGE_CAPTURE;

		// Fall through to wait for image capture to complete.

	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_IMAGE_CAPTURE:
		if (!is_rescaled_image_captured()) {
			return false;  // Wait for image capture to complete.
		}

		*current_state =
			EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__COMPOSE_RESPONSE_TO_SEND;

		// Fall through to collect image information and compose response.

	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__COMPOSE_RESPONSE_TO_SEND:
		get_rescaled_image_info(&captured_img_info);

		p_capture_img_resp->start_of_data_marker = START_OF_DATA_MARKER;

		// Fill the data related to the captured image.
		p_capture_img_resp->image_buffer_address =
			(uint32_t)captured_img_info.image_data;

		p_capture_img_resp->image_buffer_size      = captured_img_info.size;

		p_capture_img_resp->h_size                 = captured_img_info.width;

		p_capture_img_resp->v_size                 = captured_img_info.height;

		p_capture_img_resp->image_format           = captured_img_info.format;

		p_capture_img_resp->eod.end_of_data_marker = END_OF_DATA_MARKER;

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.
		inst->send_data_async_call(inst, sizeof(*p_capture_img_resp),
								   (uint8_t *)p_capture_img_resp);
		*current_state =
			EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.
	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__WAIT_FOR_RESPONSE_SEND:
		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Response has been sent, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;
		break;

	default:
		break;
	}

	return true;  // Command execution complete.
}

/**
 * exec_resume_pipeline executes the state machine for RESUME_PIPELINE command.
 *
 * @param inst: Pointer to the interface instance structure that contains
 *              the interface details and the host command data.
 * @param current_state: Pointer to the current state of the command execution.
 * @param host_req: Pointer to the host request structure containing the request
 *                  details.
 * @param host_resp: Pointer to the host response structure containing the
 * 					response details.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool exec_resume_pipeline(struct iface_instance           *inst,
								 enum host_request_service_state *current_state,
								 struct _host_requests_unpked    *host_req,
								 struct _host_responses_unpked   *host_resp)
{
	struct _resume_pipeline_request_unpked  *p_resume_pipe_req;
	struct _resume_pipeline_response_unpked *p_resume_pipe_resp;

	p_resume_pipe_req  = &host_req->resume_pipeline_request_unpked;
	p_resume_pipe_resp = &host_resp->resume_pipeline_response_unpked;

	switch (*current_state) {
	case EXECUTE_CMD_RESUME_PIPELINE__START_PROCESSING:
	case EXECUTE_CMD_RESUME_PIPELINE__VALIDATE_PARAMETERS:

		if (p_resume_pipe_req->end_of_data_marker != END_OF_DATA_MARKER) {
			// Handle error in end-of-data marker.
			// This could involve logging an error or sending an error
			// response.
			*current_state = EXECUTE_CMD_RESUME_PIPELINE__VALIDATE_PARAMETERS;

			return false;
		}

		// Currently only camera ID 0 is supported.
		GARD__DBG_ASSERT(p_resume_pipe_req->camera_id == 0,
						 "Unsupported camera ID");

		// Fall through to trigger the pipeline resume.

	case EXECUTE_CMD_RESUME_PIPELINE__TRIGGER_PIPELINE_RESUME:
		resume_pipeline_async();

		*current_state = EXECUTE_CMD_RESUME_PIPELINE__COMPOSE_RESPONSE_TO_SEND;
		// Fall through to compose response.

	case EXECUTE_CMD_RESUME_PIPELINE__COMPOSE_RESPONSE_TO_SEND:
		p_resume_pipe_resp->ack_or_nak = ACK_BYTE;

		// Fall through to send the composed response to Host.

	case EXECUTE_CMD_RESUME_PIPELINE__SEND_RESPONSE_TO_HOST:
		inst->hc_data.tx_done =
			false;  // Reset the flag to wait for data to be sent.

		inst->send_data_async_call(inst, sizeof(*p_resume_pipe_resp),
								   (uint8_t *)p_resume_pipe_resp);

		*current_state = EXECUTE_CMD_RESUME_PIPELINE__WAIT_FOR_RESPONSE_SEND;

		// Fall through to check if the response is sent.

	case EXECUTE_CMD_RESUME_PIPELINE__WAIT_FOR_RESPONSE_SEND:

		if (!inst->hc_data.tx_done) {
			return false;  // Wait for response to be sent.
		}

		// Response has been sent, go back to start state to wait for
		// new command.
		*current_state = REQUEST_IFACE_TO_RECV_CMD_ID;

		break;

	default:
		break;
	}

	return true;  // Command execution complete.
}

/**
 * service_host_requests processes the host requests that originate
 * over UART/I2C or other slow serial interfaces.
 *
 * This function implements a state machine to handle various host commands
 * received over serial interfaces. It reads the command ID, calculates the
 * command body size based on the command ID, and then reads the command body
 * from the interface. Once the command body is fully received, it executes the
 * command based on the command ID. For different commands it offloads the
 * responsibility to sub-functions that handle the specific command execution
 * logic.
 *
 * @return true if the command execution is complete, false if more data is
 *         expected or if an error occurs.
 */
static bool service_host_request_over_iface(struct iface_instance *inst)
{
	// The following variables are used in the state machine and need to
	// maintain their values across function calls.
	enum host_request_service_state *current_state =
		(enum host_request_service_state *)&inst->hc_data
			.host_request_service_state;

	/**
	 * The next two variables are used within this file to work on the received
	 * command. These variables are unpacked versions of their packed cousins
	 * that are filled with the data arriving over the interface.
	 */
	struct _host_requests_unpked  *host_req  = &inst->hc_data.host_req;
	struct _host_responses_unpked *host_resp = &inst->hc_data.host_resp;

	/**
	 * The next variable is used to receive the command body over the
	 * interface.
	 */
	struct _host_requests *iface_host_req    = &inst->hc_data.iface_host_req;
	uint32_t               bytes_to_read;

	switch (*current_state) {
	case REQUEST_IFACE_TO_RECV_CMD_ID:
		iface_host_req->command_id = 0;

		inst->hc_data.rx_done = false;  // Reset the flag to wait for new data.

		// Request interface to receive 1 byte command code.
		inst->read_data_async_call(inst, sizeof(iface_host_req->command_id),
								   (uint8_t *)&iface_host_req->command_id);

		// Wait for the command code to be received.
		*current_state = IFACE_WAIT_FOR_CMD_ID;

		// Fall through to the next state.

	case IFACE_WAIT_FOR_CMD_ID:
		// Check if command code is available.
		if (!inst->hc_data.rx_done) {
			return false;
		}

		// Fall through to the next state to calcaulate the size of command
		// body.

	case IFACE_CALCULATE_CMD_BODY_SIZE:

		// Calculate the size of the command body based on the command ID.

		switch (iface_host_req->command_id) {
		case SEND_DATA_TO_GARD_FOR_OFFSET:
			bytes_to_read = sizeof(
				iface_host_req->send_data_to_gard_for_offset_request.cmd);
			break;

		case RECV_DATA_FROM_GARD_AT_OFFSET:
			bytes_to_read =
				sizeof(iface_host_req->recv_data_from_gard_at_offset_request);
			break;

		case READ_REG_VALUE_FROM_GARD_AT_OFFSET:
			bytes_to_read = sizeof(
				iface_host_req->read_reg_value_from_gard_at_offset_request);
			break;

		case WRITE_REG_VALUE_TO_GARD_AT_OFFSET:
			bytes_to_read = sizeof(
				iface_host_req->write_reg_value_to_gard_at_offset_request);
			break;

		case GET_ML_ENGINE_STATUS:
			bytes_to_read =
				sizeof(iface_host_req->get_ml_engine_status_request);
			break;

		case GARD_DISCOVERY:
			bytes_to_read = sizeof(iface_host_req->gard_discovery_request);
			break;

		case CAPTURE_RESCALED_IMAGE:
			bytes_to_read =
				sizeof(iface_host_req->capture_rescaled_image_request);
			break;

		case RESUME_PIPELINE:
			bytes_to_read = sizeof(iface_host_req->resume_pipeline_request);
			break;

		default:
			bytes_to_read = 0;
			break;
		}

		// Fall through to receive command body (minus the command code).

	case REQUEST_IFACE_TO_RECV_CMD_BODY:

		if (bytes_to_read == 0) {
			// No command body to read, we jump to execute the command.
			*current_state = EXECUTE_HOST_IFACE_CMD;
			return true;  // No command body to read, return success.
		}

		inst->hc_data.rx_done = false;  // Reset the flag to wait for new data.

		// Request interface to receive remaining command body.
		inst->read_data_async_call(inst, bytes_to_read,
								   (uint8_t *)&iface_host_req->command_body);

		// Wait for complete command body to be received.
		*current_state = IFACE_WAIT_FOR_CMD_BODY;

		// Fall through to the next state.

	case IFACE_WAIT_FOR_CMD_BODY:
		// Check if command body has been received over the interface.
		if (!inst->hc_data.rx_done) {
			return false;
		}

		// The command body has been received, but it is a packed structure
		// containing unaligned fields. We need to unpack it to a similar padded
		// structure so that RISC-V can access the fields without dying. The
		// unpacking process is delegated to the individual command handlers
		// instead of executing the code in this common routine.

		// Fall through to the next state to unpack the command body and start
		// executing the command

	case EXECUTE_HOST_IFACE_CMD: {
		// Copy the command ID between the packed and unpacked structures. This
		// is done here since the command ID is common to all commands.
		// For the next line we assume the code is running on a little-endian
		// architecture.
		*((uint8_t *)&host_req->command_id) = iface_host_req->command_id;

		switch (iface_host_req->command_id) {
		case SEND_DATA_TO_GARD_FOR_OFFSET:

			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Currently _host_requests.send_data_to_gard_for_offset_request.cmd
			// has the same size for both packed and unpacked versions so
			// calling memcpy once to copy the entire structure is done. In
			// future if these sizes differ, then individual fields should be
			// copied between these structures.
			GARD__CASSERT(
				sizeof(host_req->send_data_to_gard_for_offset_request.cmd) ==
					sizeof(iface_host_req->send_data_to_gard_for_offset_request
							   .cmd),
				"Sizes of packed and unpacked structures mismatch.");

			memcpy(
				(uint8_t *)&host_req->send_data_to_gard_for_offset_request.cmd,
				(const uint8_t *)&iface_host_req
					->send_data_to_gard_for_offset_request.cmd,
				sizeof(
					iface_host_req->send_data_to_gard_for_offset_request.cmd));

			*current_state =
				EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__START_PROCESSING;
			break;

		case RECV_DATA_FROM_GARD_AT_OFFSET:

			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Currently _host_requests.recv_data_from_gard_at_offset_request
			// has the same size for both packed and unpacked versions, hence
			// using the packed structure to capture the body for this command.
			GARD__CASSERT(
				sizeof(host_req->recv_data_from_gard_at_offset_request) ==
					sizeof(
						iface_host_req->recv_data_from_gard_at_offset_request),
				"Sizes of packed and unpacked structures mismatch.");

			memcpy(
				(uint8_t *)&host_req->recv_data_from_gard_at_offset_request,
				(const uint8_t *)&iface_host_req
					->recv_data_from_gard_at_offset_request,
				sizeof(iface_host_req->recv_data_from_gard_at_offset_request));

			*current_state =
				EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__START_PROCESSING;
			break;

		case READ_REG_VALUE_FROM_GARD_AT_OFFSET:

			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Currently
			// _host_requests.read_reg_value_from_gard_at_offset_request has the
			// same size for both packed and unpacked versions so we use the
			// packed version instead.
			GARD__CASSERT(
				sizeof(host_req->read_reg_value_from_gard_at_offset_request) ==
					sizeof(iface_host_req
							   ->read_reg_value_from_gard_at_offset_request),
				"Sizes of packed and unpacked structures mismatch.");

			*current_state =
				EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__START_PROCESSING;
			break;

		case WRITE_REG_VALUE_TO_GARD_AT_OFFSET:

			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Currently
			// _host_requests.write_reg_value_to_gard_at_offset_request has the
			// same size for both packed and unpacked versions so calling
			// memcpy once to copy the entire structure is done. In future if
			// these sizes differ, then individual fields should be copied
			// between these structures.
			GARD__CASSERT(
				sizeof(host_req->write_reg_value_to_gard_at_offset_request) ==
					sizeof(iface_host_req
							   ->write_reg_value_to_gard_at_offset_request),
				"Sizes of packed and unpacked structures mismatch.");

			memcpy(
				(uint8_t *)&host_req->write_reg_value_to_gard_at_offset_request,
				(const uint8_t *)&iface_host_req
					->write_reg_value_to_gard_at_offset_request,
				sizeof(
					iface_host_req->write_reg_value_to_gard_at_offset_request));

			*current_state =
				EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__START_PROCESSING;
			break;

		case GET_ML_ENGINE_STATUS:

			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Currently
			// _host_requests.get_ml_engine_status_request has the
			// same size for both packed and unpacked versions so calling
			// memcpy once to copy the entire structure is done. In future if
			// these sizes differ, then individual fields should be copied
			// between these structures.
			GARD__CASSERT(
				sizeof(host_req->get_ml_engine_status_request) ==
					sizeof(iface_host_req->get_ml_engine_status_request),
				"Sizes of packed and unpacked structures mismatch.");

			memcpy(
				(uint8_t *)&host_req->get_ml_engine_status_request,
				(const uint8_t *)&iface_host_req->get_ml_engine_status_request,
				sizeof(iface_host_req->get_ml_engine_status_request));

			*current_state = EXECUTE_CMD_GET_ML_ENGINE_STATUS__START_PROCESSING;
			break;

		case GARD_DISCOVERY:

			// Currently
			// _host_requests.gard_discovery_request is empty so no data
			// unpacking is needed. In future if the need arises please refer to
			// the previous states for code template to be used here.
			*current_state = EXECUTE_CMD_DISCOVERY__START_PROCESSING;
			break;

		case CAPTURE_RESCALED_IMAGE:
			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Move received parameters to the unpacked structure in optimized
			// way as the received packed structure has made an attempt to align
			// fields. Use assert to capture if this is not the case.
			GARD__CASSERT(
				(GET_MEMBER_SIZE(struct _host_requests,
								 capture_rescaled_image_request.camera_id) ==
				 sizeof(uint8_t)) &&
					(GET_MEMBER_SIZE(
						 struct _host_requests,
						 capture_rescaled_image_request.end_of_data_marker) ==
					 sizeof(uint32_t)) &&
					((GET_MEMBER_OFFSET(
						  struct _host_requests,
						  capture_rescaled_image_request.end_of_data_marker) %
					  sizeof(cpu_size_t)) == 0),
				"Sizes or offsets of fields in packed structure have changed, "
				"update the unpacking code.");

			host_req->capture_rescaled_image_request_unpked.camera_id =
				iface_host_req->capture_rescaled_image_request.camera_id;

			host_req->capture_rescaled_image_request_unpked.end_of_data_marker =
				iface_host_req->capture_rescaled_image_request
					.end_of_data_marker;

			*current_state =
				EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__START_PROCESSING;
			break;

		case RESUME_PIPELINE:
			// Dummy statement to make compiler not complain about the
			// GARD__CASSERT statement which follows.
			iface_host_req->command_id = iface_host_req->command_id;

			// Move received parameters to the unpacked structure in optimized
			// way as the received packed structure has made an attempt to align
			// fields. Use assert to capture if this is not the case.
			GARD__CASSERT(
				(GET_MEMBER_SIZE(struct _host_requests,
								 resume_pipeline_request.camera_id) ==
				 sizeof(uint8_t)) &&
					(GET_MEMBER_SIZE(
						 struct _host_requests,
						 resume_pipeline_request.end_of_data_marker) ==
					 sizeof(uint32_t)) &&
					((GET_MEMBER_OFFSET(
						  struct _host_requests,
						  resume_pipeline_request.end_of_data_marker) %
					  sizeof(cpu_size_t)) == 0),
				"Sizes or offsets of fields in packed structure have changed, "
				"update the unpacking code.");

			host_req->capture_rescaled_image_request_unpked.camera_id =
				iface_host_req->capture_rescaled_image_request.camera_id;

			host_req->capture_rescaled_image_request_unpked.end_of_data_marker =
				iface_host_req->capture_rescaled_image_request
					.end_of_data_marker;

			*current_state = EXECUTE_CMD_RESUME_PIPELINE__START_PROCESSING;
			break;

		default:
			// Unsupported command ID, we should ASSERT here or handle
			// the error appropriately.
			return false;  // Unsupported command ID, abort execution.
		}

		break;
	}

	case EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__START_PROCESSING ... EXEC_SEND_DATA_TO_GARD_FOR_OFFSET__END_PROCESSING:

		return exec_send_data_to_gard_for_offset(inst, current_state, host_req,
												 host_resp);

	case EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__START_PROCESSING ... EXEC_CMD_RECV_DATA_FROM_GARD_AT_OFFSET__END_PROCESSING:

		return exec_recv_data_from_gard_at_offset(inst, current_state, host_req,
												  host_resp);

	case EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__START_PROCESSING ... EXECUTE_CMD_READ_REG_VALUE_FROM_GARD_AT_OFFSET__END_PROCESSING:

		return exec_read_reg_value_from_gard_at_offset(
			inst, current_state, iface_host_req, host_resp);

	case EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__START_PROCESSING ... EXECUTE_CMD_WRITE_REG_VALUE_TO_GARD_AT_OFFSET__END_PROCESSING:

		return exec_write_reg_value_to_gard_at_offset(inst, current_state,
													  host_req, host_resp);

	case EXECUTE_CMD_GET_ML_ENGINE_STATUS__START_PROCESSING ... EXECUTE_CMD_GET_ML_ENGINE_STATUS__END_PROCESSING:

		return exec_gel_ml_engine_status(inst, current_state, host_req,
										 host_resp);

	case EXECUTE_CMD_DISCOVERY__START_PROCESSING ... EXECUTE_CMD_DISCOVERY__END_PROCESSING:

		return exec_gard_discovery(inst, current_state, host_req, host_resp);

	case EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__START_PROCESSING ... EXECUTE_CMD_CAPTURE_RESCALED_IMAGE__END_PROCESSING:

		return exec_capture_rescaled_image(inst, current_state, host_req,
										   host_resp);

	case EXECUTE_CMD_RESUME_PIPELINE__START_PROCESSING ... EXECUTE_CMD_RESUME_PIPELINE__END_PROCESSING:

		return exec_resume_pipeline(inst, current_state, host_req, host_resp);

	default:
		// ERROR - We should ASSERT here.
		break;
	}

	return false;  // No commands processed in this stub.
}

bool service_host_requests(void)
{
	uint32_t idx;
	bool     work_done = false;

	for (idx = 0; idx < valid_ifaces; idx++) {
		// Call the service_host_request_over_iface function for each valid
		// interface.
		if (service_host_request_over_iface(&iface_inst[idx])) {
			// At least one command was processed on one interface.
			work_done = true;
		}
	}

	return work_done;
}

