#include "protocol.h"


void XerxesBindRegister(uint8_t *parameter_register)
{
    param_reg = parameter_register;
}

void XerxesBusSetup(uint8_t (*receive)(), void (*send)(uint8_t), void (*tx_enable)(), void (*tx_disable)(), bool (*tx_is_done)(), bool (*rx_is_ready)())
{
    send_char = send;
    read_char = receive;
    TX_en = tx_enable;
    TX_dis = tx_disable;
    TX_done = tx_is_done;
    Rx_ready = rx_is_ready;
}

void XerxesDeviceSetup(uint8_t dev_id, void (*fetch_handler)(uint8_t), void (*broadcast_handler)())
{
    this_device_id = dev_id;
    FetchHandler = fetch_handler;
    BroadcastHandler = broadcast_handler;
}

bool XerxesSync()
{
    struct packet_s inbound;
    uint8_t header;

    if (Rx_ready())
    {
        header = read_char();
        if (header == 0x01)
        {
            // read from UART
            if (XerxesReceive(&inbound))
            {
                // message was received successfully, check destination address
                if (inbound.dst == ADDR_BROADCAST)
                {
                    BroadcastHandler();
                    return true;
                }

                else if (inbound.dst == this_dev_addr)
                {
                    if (inbound.msgid.msgid_16 == MSGID_PING)
                    {
                        // ping was received, reply with device identification
                        uint8_t vals[3] = {this_device_id, PROTOCOL_VERSION_MAJ, PROTOCOL_VERSION_MIN}; //
                        uint8_t *message = vals;
                        XerxesSend(inbound.src, MSGID_PING_REPLY, message, 3);
                        return true;
                    }

                    else if (inbound.msgid.msgid_16 == MSGID_FETCH_MEASUREMENT)
                    {
                        FetchHandler(inbound.src);
                        return true;
                    }

                    else if (inbound.msgid.msgid_16 == MSGID_READ)
                    {
                        // read required register
                        // The request prototype is <MSGID_READ> <REG_ID> <LEN>

                        uint8_t offset = inbound.payload[0];
                        uint8_t len = inbound.payload[1];
                        uint8_t vals[10];
                        for (int i = 0; i < len; i++)
                        {
                            vals[i] = param_reg[i + offset];
                        }

                        XerxesSend(inbound.src, MSGID_READ_VALUE, vals, len);

                        return true;
                    }

                    else if (inbound.msgid.msgid_16 == MSGID_SET)
                    {
                        // The message prototype is <MSGID_SET> <REG_ID> <LEN> <BYTE_1> ... <BYTE_N>
                        uint8_t offset = inbound.payload[0];
                        uint8_t len = inbound.payload[1];
                        for (int i = 0; i < len; i++)
                        {
                            param_reg[offset + i] = inbound.payload[2 + i];
                        }

                        XerxesSend(inbound.src, MSGID_ACK_OK, &len, 1);
                        return true;
                    }

                    else if (inbound.msgid.msgid_16 == MSGID_SYNC)
                    {
                        // do something with sync packet
                        sync = true;
                        XerxesSend(inbound.src, MSGID_ACK_OK, NULL, 0);
                    }

                    else
                    {
                        // unknown MSGID ???
                        return false;
                    }
                }

                else
                {
                    // message was not directed to me
                    // do nothing = discard
                }
            }
            else
            {
                // checksum of the message is invalid
                // do nothing, I guess
            }
        }
    }
    return false;
}

bool XerxesReceive(struct packet_s *packet)
{
    uint8_t sum = 0;
    packet->len = read_char();
    packet->src = read_char();
    packet->dst = read_char();

    packet->msgid.msgid_8.msg_id_h = read_char();
    packet->msgid.msgid_8.msg_id_l = read_char();
    sum += packet->msgid.msgid_8.msg_id_h;
    sum += packet->msgid.msgid_8.msg_id_l;

    for (uint8_t i = 0; i < 64; i++)
    {
        // clean buffer
        packet->payload[i] = 0;
    }

    for (uint8_t i = 0; i < packet->len - 7; i++)
    {
        packet->payload[i] = read_char();
        sum += packet->payload[i];
    }

    // receive and compare checksum
    uint8_t checksum = read_char();
    sum += 0x01 + packet->len + packet->src + packet->dst + checksum;

    if (sum == 0)
    {
        // checksum passed
        return true;
    }
    else
    {
        // checksum failed
        return false;
    }
}

uint16_t send_word(uint16_t word, void (*send_char)(uint8_t))
{
    uint8_t hbyte = word >> 8;
    uint8_t lbyte = word & 0xFF;
    (*send_char)(hbyte);
    (*send_char)(lbyte);

    return lbyte + hbyte;
}

uint8_t XerxesSend(uint8_t dest_addr, msg_id_16_t msgid, uint8_t *buffer, uint8_t payload_len)
{

    volatile uint8_t tx_len, checksum;
    volatile uint64_t sum;

    tx_len = (payload_len) + 7;
    sum = 0x01 + tx_len + this_dev_addr + dest_addr; // SOH + address

    // RS485_TX_EN_SetHigh();
    TX_en();

    send_char(0x01);          // SOH
    send_char(tx_len);        // length of message
    send_char(this_dev_addr); // from
    send_char(dest_addr);     // destination

    sum += send_word(msgid, send_char);

    for (uint8_t i = 0; i < payload_len; i++)
    {
        sum += *buffer;
        send_char(*buffer++);
    }

    checksum = ~(sum & 0xFF) + 1; // two's complement
    send_char(checksum);          // send checksum

    // while(!UART1_is_tx_done());
    // RS485_TX_EN_SetLow();
    while (!TX_done())
        ;
    TX_dis();

    return checksum;
}
