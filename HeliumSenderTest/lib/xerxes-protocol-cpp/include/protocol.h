/*
 * File:   utils.h
 * Author: srubi
 *
 * Created on December 10, 2021, 12:43 PM
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <math.h>
#include "msgid.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Address definitions:

// Broadcast address
#define ADDR_BROADCAST 0xFF

#define PROTOCOL_VERSION_MAJ 1
#define PROTOCOL_VERSION_MIN 2

extern uint8_t this_dev_addr;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Xerxes network standard packet
     * @len message length including header 0x01
     * @src source address (sender)
     * @dst destination address (receiver)
     * @msgid message id union (2x uint8_t or uint16_t)
     * @payload uchar array containing message data
     */
    struct packet_s
    {
        uint8_t len;
        uint8_t src;
        uint8_t dst;
        union msg_id_u msgid;
        uint8_t payload[64];
    };

    /**
     * declare RS485 primitives
     */
    static void (*send_char)(uint8_t);
    static uint8_t (*read_char)();
    static void (*TX_en)();
    static void (*TX_dis)();
    static bool (*TX_done)();
    static bool (*Rx_ready)();

    static bool sync = false;

    /**
     * declare device parameters
     */
    static uint8_t this_device_id;
    static void (*FetchHandler)(uint8_t);
    static void (*BroadcastHandler)();

    /**
     declare device register for R/W operations*/
    static uint8_t *param_reg = NULL;

    /**
     * Setup register (array) to use for R/W operations on device
     * @param parameter_register
     */
    void XerxesBindRegister(uint8_t *parameter_register);

    /**
     * Setup protocol primitives
     * @param send - function for transmitting one char at a time
     * @param tx_enable - enable RS485 line
     * @param tx_disable - disable RS485 line
     * @param tx_is_done - check whether transmission is over
     */
    void XerxesBusSetup(uint8_t (*receive)(), void (*send)(uint8_t), void (*tx_enable)(), void (*tx_disable)(), bool (*tx_is_done)(), bool (*rx_is_ready)());

    /**
     * Setup device id and handler functions
     * @param dev_id
     * @param fetch_handler
     */
    void XerxesDeviceSetup(uint8_t dev_id, void (*fetch_handler)(uint8_t), void (*broadcast_handler)());

    /**
     * Check for incoming packet and handle it with either broadcast or fetch handler
     * @return true if something was handled
     */
    bool XerxesSync();

    /**
     * Receive packet from Xerxes network
     * @param packet - containing message headers and payload
     * @return true if message is OK, false if corrupted
     */
    bool XerxesReceive(struct packet_s *packet);

    /**
     * send 2 bytes = WORD, MSB first
     * @param word
     */
    uint16_t send_word(uint16_t word, void (*send_char)(uint8_t));

    uint8_t XerxesSend(uint8_t dest_addr, msg_id_16_t msgid, uint8_t *message, uint8_t payload_len);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
