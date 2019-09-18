/*******************************************************************************

  This software program is available to you under a choice of one of two 
  licenses. You may choose to be licensed under either the GNU General Public 
  License (GPL) Version 2, June 1991, available at 
  http://www.fsf.org/copyleft/gpl.html, or the Intel BSD + Patent License, the 
  text of which follows:
  
  Recipient has requested a license and Intel Corporation ("Intel") is willing
  to grant a license for the software entitled Linux Base Driver for the 
  Intel(R) PRO/1000 Family of Adapters (e1000) (the "Software") being provided
  by Intel Corporation. The following definitions apply to this license:
  
  "Licensed Patents" means patent claims licensable by Intel Corporation which 
  are necessarily infringed by the use of sale of the Software alone or when 
  combined with the operating system referred to below.
  
  "Recipient" means the party to whom Intel delivers this Software.
  
  "Licensee" means Recipient and those third parties that receive a license to 
  any operating system available under the GNU Public License version 2.0 or 
  later.
  
  Copyright (c) 1999 - 2002 Intel Corporation.
  All rights reserved.
  
  The license is provided to Recipient and Recipient's Licensees under the 
  following terms.
  
  Redistribution and use in source and binary forms of the Software, with or 
  without modification, are permitted provided that the following conditions 
  are met:
  
  Redistributions of source code of the Software may retain the above 
  copyright notice, this list of conditions and the following disclaimer.
  
  Redistributions in binary form of the Software may reproduce the above 
  copyright notice, this list of conditions and the following disclaimer in 
  the documentation and/or materials provided with the distribution.
  
  Neither the name of Intel Corporation nor the names of its contributors 
  shall be used to endorse or promote products derived from this Software 
  without specific prior written permission.
  
  Intel hereby grants Recipient and Licensees a non-exclusive, worldwide, 
  royalty-free patent license under Licensed Patents to make, use, sell, offer 
  to sell, import and otherwise transfer the Software, if any, in source code 
  and object code form. This license shall include changes to the Software 
  that are error corrections or other minor changes to the Software that do 
  not add functionality or features when the Software is incorporated in any 
  version of an operating system that has been distributed under the GNU 
  General Public License 2.0 or later. This patent license shall apply to the 
  combination of the Software and any operating system licensed under the GNU 
  Public License version 2.0 or later if, at the time Intel provides the 
  Software to Recipient, such addition of the Software to the then publicly 
  available versions of such operating systems available under the GNU Public 
  License version 2.0 or later (whether in gold, beta or alpha form) causes 
  such combination to be covered by the Licensed Patents. The patent license 
  shall not apply to any other combinations which include the Software. NO 
  hardware per se is licensed hereunder.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MECHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR IT CONTRIBUTORS BE LIABLE FOR ANY 
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
  (INCLUDING, BUT NOT LIMITED, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  ANY LOSS OF USE; DATA, OR PROFITS; OR BUSINESS INTERUPTION) HOWEVER CAUSED 
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR 
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/***************************************************************************/
/*       /proc File System Interaface Support Functions                    */
/***************************************************************************/

#include "e1000.h"
extern char e1000_driver_name[];
extern char e1000_driver_version[];
#include "e1000_proc.h"
#include <linux/proc_fs.h>

struct proc_dir_entry *e1000_proc_dir;

#define CABLE_LENGTH_TO_STRING() \
   msg = \
   (adapter->phy_info.cable_length == e1000_cable_length_50)      ? "0-50 Meters (+/- 20 Meters)"  : \
   (adapter->phy_info.cable_length == e1000_cable_length_50_80)   ? "50-80 Meters (+/- 20 Meters)"  : \
   (adapter->phy_info.cable_length == e1000_cable_length_80_110)  ? "80-110 Meters (+/- 20 Meters)" : \
   (adapter->phy_info.cable_length == e1000_cable_length_110_140) ? "110-140 Meters (+/- 20 Meters)" : \
   (adapter->phy_info.cable_length == e1000_cable_length_140)     ? "> 140 Meters (+/- 20 Meters)" : \
   "Unknown";

#define EXTENDED_10BASE_T_DISTANCE_TO_STRING() \
    msg = \
    (adapter->phy_info.extended_10bt_distance == \
            e1000_10bt_ext_dist_enable_normal) ? "Disabled" : \
    (adapter->phy_info.extended_10bt_distance == \
                    e1000_10bt_ext_dist_enable_lower) ? "Enabled" : "Unknown"; 

#define CABLE_POLARITY_TO_STRING() \
    msg = \
    (adapter->phy_info.cable_polarity == e1000_rev_polarity_normal) ? "Normal" : \
    (adapter->phy_info.cable_polarity == e1000_rev_polarity_reversed) ? \
                                        "Reversed" : "Unknown";

#define POLARITY_CORRECTION_TO_STRING() \
    msg = \
    (adapter->phy_info.polarity_correction == \
                    e1000_polarity_reversal_enabled) ? "Disabled" : \
    (adapter->phy_info.polarity_correction == \
                    e1000_polarity_reversal_disabled) ? "Enabled" : "Undefined";

#define LINK_RESET_TO_STRING() \
    msg = \
    (adapter->phy_info.link_reset == e1000_down_no_idle_no_detect) ? "Disabled" : \
    (adapter->phy_info.link_reset == e1000_down_no_idle_detect) ? "Enabled" : \
    "Unknown"; 

#define MDI_X_MODE_TO_STRING() \
    msg = (adapter->phy_info.mdix_mode == 0) ? "MDI" :  "MDI-X";

#define LOCAL_RECEIVER_STATUS_TO_STRING() \
    msg = \
    (adapter->phy_info.local_rx == e1000_1000t_rx_status_not_ok) ? "NOT_OK" : \
    (adapter->phy_info.local_rx == e1000_1000t_rx_status_ok) ? "OK" : \
    "Unknown";

#define REMOTE_RECEIVER_STATUS_TO_STRING() \
    msg = \
    (adapter->phy_info.remote_rx == e1000_1000t_rx_status_not_ok) ? "NOT_OK" : \
    (adapter->phy_info.remote_rx == e1000_1000t_rx_status_ok) ? "OK" : \
    "Unknown";

static void e1000_link_update(struct e1000_adapter * adapter) {

    e1000_check_for_link(&adapter->shared);
    if(E1000_READ_REG(&adapter->shared, STATUS) & E1000_STATUS_LU)
        adapter->link_active = 1;
    else
        adapter->link_active = 0;

    if (adapter->link_active) {
        e1000_get_speed_and_duplex(&adapter->shared, &adapter->link_speed, &adapter->link_duplex);
    } else {
        adapter->link_speed = 0;
        adapter->link_duplex = 0;
    }
    return;
}

static int e1000_generic_read(char *page, char **start, off_t off,
                              int count, int *eof)
{
    int len;

    len = strlen(page);
    page[len++] = '\n';

    if (len <= off + count)
        *eof = 1;
    *start = page + off;
    len -= off;
    if (len > count)
        len = count;
    if (len < 0)
        len = 0;
    return len;
}

static int e1000_read_ulong(char *page, char **start, off_t off,
               int count, int *eof, unsigned long l)
{
    sprintf(page, "%lu", l);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_ulong_hex(char *page, char **start, off_t off,
                   int count, int *eof, unsigned long l)
{
    sprintf(page, "0x%04lx", l);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_ullong(char *page, char **start, off_t off,
               int count, int *eof, unsigned long long l)
{
    sprintf(page, "%Lu", l);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_hwaddr(char *page, char **start, off_t off,
                int count, int *eof, unsigned char *hwaddr)
{
    sprintf(page, "%02X:%02X:%02X:%02X:%02X:%02X",
            hwaddr[0], hwaddr[1], hwaddr[2],
            hwaddr[3], hwaddr[4], hwaddr[5]);

    return e1000_generic_read(page, start, off, count, eof);
}

/* need to check page boundaries !!! */
static int e1000_read_info(char *page, char **start, off_t off,
              int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    struct net_device_stats *stats = &adapter->net_stats;
    unsigned char *hwaddr;
    char *pagep = page;
    char *msg;

    page += sprintf(page, "%-32s %s\n", DESCRIPTION_TAG, adapter->id_string);
    page += sprintf(page, "%-32s %06lx-%03x\n",
                    PART_NUMBER_TAG, 
                    (unsigned long )adapter->part_num >> 8, 
                    adapter->part_num & 0x000000FF);

    page += sprintf(page, "%-32s %s\n", DRVR_NAME_TAG, e1000_driver_name);

    page += sprintf(page, "%-32s %s\n", DRVR_VERSION_TAG, e1000_driver_version);

    page += sprintf(page, "%-32s 0x%04lx\n",
                    PCI_VENDOR_TAG, (unsigned long) adapter->vendor_id);
    page += sprintf(page, "%-32s 0x%04lx\n",
                    PCI_DEVICE_ID_TAG, (unsigned long) adapter->device_id);
    page += sprintf(page, "%-32s 0x%04lx\n",
                    PCI_SUBSYSTEM_VENDOR_TAG,
                    (unsigned long) adapter->subven_id);
    page += sprintf(page, "%-32s 0x%04lx\n",
                    PCI_SUBSYSTEM_ID_TAG,
                    (unsigned long) adapter->subsys_id);
    page += sprintf(page, "%-32s 0x%02lx\n",
                    PCI_REVISION_ID_TAG,
                    (unsigned long) adapter->rev_id);
    
    page += sprintf(page, "%-32s %lu\n",
                    PCI_BUS_TAG,
                    (unsigned long) (adapter->pdev->bus->number));
    page += sprintf(page, "%-32s %lu\n",
                    PCI_SLOT_TAG,
                    (unsigned
                     long) (PCI_SLOT((adapter->pdev->devfn))));
       
    if(adapter->shared.mac_type >= e1000_82543) {
        page += sprintf(page, "%-32s %s\n",
                PCI_BUS_TYPE_TAG,
                (adapter->shared.bus_type == e1000_bus_type_pci)  ? "PCI"   :
                (adapter->shared.bus_type == e1000_bus_type_pcix) ? "PCI-X" :
                "UNKNOWN");
    
        page += sprintf(page, "%-32s %s\n",
            PCI_BUS_SPEED_TAG,
            (adapter->shared.bus_speed == e1000_bus_speed_33)  ? "33MHz"  :
            (adapter->shared.bus_speed == e1000_bus_speed_66)  ? "66MHz"  :
            (adapter->shared.bus_speed == e1000_bus_speed_100) ? "100MHz" :
            (adapter->shared.bus_speed == e1000_bus_speed_133) ? "133MHz" :
                "UNKNOWN");
    
        page += sprintf(page, "%-32s %s\n",
                PCI_BUS_WIDTH_TAG,
                (adapter->shared.bus_width == e1000_bus_width_32) ? "32-bit" :
                (adapter->shared.bus_width == e1000_bus_width_64) ? "64-bit" :
                "UNKNOWN");
    }
    
    page +=
        sprintf(page, "%-32s %lu\n", IRQ_TAG,
                (unsigned long) (adapter->pdev->irq));
    page +=
        sprintf(page, "%-32s %s\n", SYSTEM_DEVICE_NAME_TAG,
                adapter->netdev->name);

    hwaddr = adapter->netdev->dev_addr;
    page += sprintf(page, "%-32s %02X:%02X:%02X:%02X:%02X:%02X\n",
                    CURRENT_HWADDR_TAG,
                    hwaddr[0], hwaddr[1], hwaddr[2],
                    hwaddr[3], hwaddr[4], hwaddr[5]);

    hwaddr = adapter->perm_net_addr;
    page += sprintf(page, "%-32s %02X:%02X:%02X:%02X:%02X:%02X\n",
                    PERMANENT_HWADDR_TAG,
                    hwaddr[0], hwaddr[1], hwaddr[2],
                    hwaddr[3], hwaddr[4], hwaddr[5]);

    page += sprintf(page, "\n");
    
    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    if (adapter->link_active == 1)
        msg = "up";
    else
        msg = "down";
    page += sprintf(page, "%-32s %s\n", LINK_TAG, msg);

    if (adapter->link_speed)
        page += sprintf(page, "%-32s %lu\n",
                        SPEED_TAG,
                        (unsigned long) (adapter->link_speed));
    else
        page += sprintf(page, "%-32s %s\n", SPEED_TAG, "N/A");

    msg = adapter->link_duplex == FULL_DUPLEX ? "full" :
        ((adapter->link_duplex == 0) ? "N/A" : "half");
    page += sprintf(page, "%-32s %s\n", DUPLEX_TAG, msg);

    if (adapter->netdev->flags & IFF_UP)
        msg = "up";
    else
        msg = "down";
    page += sprintf(page, "%-32s %s\n", STATE_TAG, msg);

    page += sprintf(page, "\n");

    page += sprintf(page, "%-32s %lu\n",
                    RX_PACKETS_TAG, (unsigned long) stats->rx_packets);
    page += sprintf(page, "%-32s %lu\n",
                    TX_PACKETS_TAG, (unsigned long) stats->tx_packets);
    page += sprintf(page, "%-32s %lu\n",
                    RX_BYTES_TAG, (unsigned long) stats->rx_bytes);
    page += sprintf(page, "%-32s %lu\n",
                    TX_BYTES_TAG, (unsigned long) stats->tx_bytes);
    page += sprintf(page, "%-32s %lu\n",
                    RX_ERRORS_TAG, (unsigned long) stats->rx_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_ERRORS_TAG, (unsigned long) stats->tx_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_DROPPED_TAG, (unsigned long) stats->rx_dropped);
    page += sprintf(page, "%-32s %lu\n",
                    TX_DROPPED_TAG, (unsigned long) stats->tx_dropped);
    page += sprintf(page, "%-32s %lu\n",
                    MULTICAST_TAG, (unsigned long) stats->multicast);
    page += sprintf(page, "%-32s %lu\n",
                    COLLISIONS_TAG, (unsigned long) stats->collisions);
    page += sprintf(page, "%-32s %lu\n",
                    RX_LENGTH_ERRORS_TAG,
                    (unsigned long) stats->rx_length_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_OVER_ERRORS_TAG,
                    (unsigned long) stats->rx_over_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_CRC_ERRORS_TAG,
                    (unsigned long) stats->rx_crc_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_FRAME_ERRORS_TAG,
                    (unsigned long) stats->rx_frame_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_FIFO_ERRORS_TAG,
                    (unsigned long) stats->rx_fifo_errors);
    page += sprintf(page, "%-32s %lu\n",
                    RX_MISSED_ERRORS_TAG,
                    (unsigned long) stats->rx_missed_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_ABORTED_ERRORS_TAG,
                    (unsigned long) stats->tx_aborted_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_CARRIER_ERRORS_TAG,
                    (unsigned long) stats->tx_carrier_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_FIFO_ERRORS_TAG,
                    (unsigned long) stats->tx_fifo_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_HEARTBEAT_ERRORS_TAG,
                    (unsigned long) stats->tx_heartbeat_errors);
    page += sprintf(page, "%-32s %lu\n",
                    TX_WINDOW_ERRORS_TAG,
                    (unsigned long) stats->tx_window_errors);

    page += sprintf(page, "\n");

    /* 8254x specific stats */
    page += sprintf(page, "%-32s %Lu\n",
                    TX_LATE_COLL_TAG,
                    (unsigned long long)adapter->stats.latecol);
    page += sprintf(page, "%-32s %Lu\n",
                    TX_DEFERRED_TAG,
                    (unsigned long long)adapter->stats.dc);
    page += sprintf(page, "%-32s %Lu\n",
                    TX_SINGLE_COLL_TAG,
                    (unsigned long long)adapter->stats.scc);
    page += sprintf(page, "%-32s %Lu\n",
                    TX_MULTI_COLL_TAG,
                    (unsigned long long)adapter->stats.mcc);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_LONG_ERRORS_TAG,
                    (unsigned long long)adapter->stats.roc);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_SHORT_ERRORS_TAG,
                    (unsigned long long)adapter->stats.ruc);
    /* The 82542 does not have an alignment error count register */
    /* ALGNERRC is only valid in MII mode at 10 or 100 Mbps */
    if(adapter->shared.mac_type >= e1000_82543)
        page += sprintf(page, "%-32s %Lu\n",
                        RX_ALIGN_ERRORS_TAG,
                        (unsigned long long)adapter->stats.algnerrc);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_XON_TAG,
                    (unsigned long long)adapter->stats.xonrxc);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_XOFF_TAG,
                    (unsigned long long)adapter->stats.xoffrxc);
    page += sprintf(page, "%-32s %Lu\n",
                    TX_XON_TAG,
                    (unsigned long long)adapter->stats.xontxc);
    page += sprintf(page, "%-32s %Lu\n",
                    TX_XOFF_TAG,
                    (unsigned long long)adapter->stats.xofftxc);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_CSUM_GOOD_TAG,
                    (unsigned long long)adapter->XsumRXGood);
    page += sprintf(page, "%-32s %Lu\n",
                    RX_CSUM_ERROR_TAG,
                    (unsigned long long)adapter->XsumRXError);

    if (adapter->shared.media_type == e1000_media_type_copper)
        msg = "Copper";
    else
        msg = "Fiber";
    page += sprintf(page, "\n%-32s %s\n", MEDIA_TYPE_TAG, msg);
    
    if (adapter->shared.media_type == e1000_media_type_copper) {
       CABLE_LENGTH_TO_STRING();
       page += sprintf(page, "%-32s %s\n", CABLE_LENGTH_TAG, msg);

       EXTENDED_10BASE_T_DISTANCE_TO_STRING();
       page += sprintf(page, "%-32s %s\n", EXTENDED_10BASE_T_DISTANCE_TAG, msg);
 
       CABLE_POLARITY_TO_STRING();
       page += sprintf(page, "%-32s %s\n", CABLE_POLARITY_TAG, msg);

       POLARITY_CORRECTION_TO_STRING();
       page += sprintf(page, "%-32s %s\n",  CABLE_POLARITY_CORRECTION_TAG, msg);

       page += sprintf(page, "%-32s %lu\n", IDLE_ERRORS_TAG, (unsigned long)adapter->phy_stats.idle_errors );

       LINK_RESET_TO_STRING();
       page += sprintf(page, "%-32s %s\n", LINK_RESET_ENABLED_TAG, msg);

       page += sprintf(page, "%-32s %lu\n", RECEIVE_ERRORS_TAG, (unsigned long)adapter->phy_stats.receive_errors);

       MDI_X_MODE_TO_STRING();
       page += sprintf(page, "%-32s %s\n", MDI_X_ENABLED_TAG, msg);

       LOCAL_RECEIVER_STATUS_TO_STRING();
       page += sprintf(page, "%-32s %s\n", LOCAL_RECEIVER_STATUS_TAG, msg);

       REMOTE_RECEIVER_STATUS_TO_STRING();
       page += sprintf(page, "%-32s %s\n", REMOTE_RECEIVER_STATUS_TAG, msg);
    }

    *page = 0;
    return e1000_generic_read(pagep, start, off, count, eof);
}

static int e1000_read_descr(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    
    strncpy(page, adapter->id_string, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_partnum(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    sprintf(page, "%06lx-%03x",
            (unsigned long)adapter->part_num >> 8,
            adapter->part_num & 0x000000FF);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_drvr_name(char *page, char **start, off_t off,
                   int count, int *eof, void *data)
{
    strncpy(page, e1000_driver_name, PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_drvr_ver(char *page, char **start, off_t off,
                  int count, int *eof, void *data)
{
    strncpy(page, e1000_driver_version, PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_pci_vendor(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong_hex(page, start, off, count, eof,
                          (unsigned long) adapter->vendor_id);
}

static int e1000_read_pci_device(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong_hex(page, start, off, count, eof,
                          (unsigned long) adapter->device_id);
}

static int e1000_read_pci_sub_vendor(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong_hex(page, start, off, count, eof,
                          (unsigned long) adapter->subven_id);
}

static int e1000_read_pci_sub_device(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong_hex(page, start, off, count, eof,
                          (unsigned long) adapter->subsys_id);
}

static int e1000_read_pci_revision(char *page, char **start, off_t off,
                      int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong_hex(page, start, off, count, eof,
                          (unsigned long) adapter->rev_id);
}

static int e1000_read_dev_name(char *page, char **start, off_t off,
                  int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    strncpy(page, adapter->netdev->name, PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_pci_bus(char *page, char **start, off_t off,
                 int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) (adapter->pdev->bus->number));
}

static int e1000_read_pci_slot(char *page, char **start, off_t off,
                  int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned
                       long) (PCI_SLOT((adapter->pdev->devfn))));
}

static int e1000_read_pci_bus_type(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    strncpy(page,
        (adapter->shared.bus_type == e1000_bus_type_pci)  ? "PCI"   :
        (adapter->shared.bus_type == e1000_bus_type_pcix) ? "PCI-X" :
        "UNKNOWN", PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_pci_bus_speed(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    strncpy(page, 
        (adapter->shared.bus_speed == e1000_bus_speed_33)  ? "33MHz"  :
        (adapter->shared.bus_speed == e1000_bus_speed_66)  ? "66MHz"  :
        (adapter->shared.bus_speed == e1000_bus_speed_100) ? "100MHz" :
        (adapter->shared.bus_speed == e1000_bus_speed_133) ? "133MHz" :
        "UNKNOWN", PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_pci_bus_width(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    strncpy(page, 
        (adapter->shared.bus_width == e1000_bus_width_32) ? "32-bit" :
        (adapter->shared.bus_width == e1000_bus_width_64) ? "64-bit" :
        "UNKNOWN", PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_irq(char *page, char **start, off_t off,
             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) (adapter->pdev->irq));
}

static int e1000_read_current_hwaddr(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    unsigned char *hwaddr = adapter->netdev->dev_addr;

    return e1000_read_hwaddr(page, start, off, count, eof, hwaddr);
}

static int e1000_read_permanent_hwaddr(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    unsigned char *hwaddr = adapter->perm_net_addr;

    return e1000_read_hwaddr(page, start, off, count, eof, hwaddr);
}

static int e1000_read_link_status(char *page, char **start, off_t off,
                     int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    if (adapter->link_active == 1)
        strncpy(page, "up", PAGE_SIZE);
    else
        strncpy(page, "down", PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_speed(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    if (adapter->link_speed)
        return e1000_read_ulong(page, start, off, count, eof,
                          (unsigned long) (adapter->link_speed));
    strncpy(page, "N/A", PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_dplx_mode(char *page, char **start, off_t off,
                   int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    char *dplx_mode;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    dplx_mode = adapter->link_duplex == FULL_DUPLEX ? "full" :
        ((adapter->link_duplex == 0) ? "N/A" : "half");
    strncpy(page, dplx_mode, PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_state(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    if (adapter->netdev->flags & IFF_UP)
        strncpy(page, "up", PAGE_SIZE);
    else
        strncpy(page, "down", PAGE_SIZE);

    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_rx_packets(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_packets);
}

static int e1000_read_tx_packets(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_packets);
}

static int e1000_read_rx_bytes(char *page, char **start, off_t off,
                  int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_bytes);
}

static int e1000_read_tx_bytes(char *page, char **start, off_t off,
                  int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_bytes);
}

static int e1000_read_rx_errors(char *page, char **start, off_t off,
                   int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_errors);
}

static int e1000_read_tx_errors(char *page, char **start, off_t off,
                   int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_errors);
}

static int e1000_read_rx_dropped(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_dropped);
}

static int e1000_read_tx_dropped(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_dropped);
}

static int e1000_read_rx_multicast_packets(char *page, char **start, off_t off,
                              int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.multicast);
}

static int e1000_read_collisions(char *page, char **start, off_t off,
                    int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.collisions);
}

static int e1000_read_rx_length_errors(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_length_errors);
}

static int e1000_read_rx_over_errors(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_over_errors);
}

static int e1000_read_rx_crc_errors(char *page, char **start, off_t off,
                       int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_crc_errors);
}

static int e1000_read_rx_frame_errors(char *page, char **start, off_t off,
                         int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_frame_errors);
}

static int e1000_read_rx_fifo_errors(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_fifo_errors);
}

static int e1000_read_rx_missed_errors(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.rx_missed_errors);
}

static int e1000_read_tx_aborted_errors(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_aborted_errors);
}

static int e1000_read_tx_carrier_errors(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_carrier_errors);
}

static int e1000_read_tx_fifo_errors(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_fifo_errors);
}

static int e1000_read_tx_heartbeat_errors(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_heartbeat_errors);
}

static int e1000_read_tx_window_errors(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    return e1000_read_ulong(page, start, off, count, eof,
                      (unsigned long) adapter->net_stats.tx_window_errors);
}

/* 8254x specific stats */
static int e1000_read_tx_late_coll(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.latecol);
}

static int e1000_read_tx_defer_events(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.dc);
}
static int e1000_read_tx_single_coll(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.scc);
}
static int e1000_read_tx_multi_coll(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.mcc);
}
static int e1000_read_rx_oversize(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.roc);
}
static int e1000_read_rx_undersize(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.ruc);
}
static int e1000_read_rx_align_err(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.algnerrc);
}
static int e1000_read_rx_xon(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.xonrxc);
}
static int e1000_read_rx_xoff(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.xoffrxc);
}
static int e1000_read_tx_xon(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.xontxc);
}
static int e1000_read_tx_xoff(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;
    return e1000_read_ullong(page, start, off, count, eof, adapter->stats.xofftxc);
}

static struct proc_dir_entry *e1000_create_proc_read(char *name,
    struct e1000_adapter * adapter,
    struct proc_dir_entry *parent,
    read_proc_t * read_proc)
{
    struct proc_dir_entry *pdep;

    if (!(pdep = create_proc_entry(name, S_IFREG, parent)))
        return NULL;
    pdep->read_proc = read_proc;
    pdep->data = adapter;
    return pdep;
}

static int e1000_read_cable_length (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    CABLE_LENGTH_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_media_type (char *page, char **start,
                   off_t off, int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    switch (adapter->shared.media_type) {
        case e1000_media_type_copper: strncpy(page,"Copper", PAGE_SIZE); break;
        case e1000_media_type_fiber: strncpy(page, "Fiber", PAGE_SIZE); break;
        default: strncpy(page, "Unknown", PAGE_SIZE);    
    }
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_extended_10base_t_distance (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    EXTENDED_10BASE_T_DISTANCE_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_cable_polarity (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    CABLE_POLARITY_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_cable_polarity_correction (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    POLARITY_CORRECTION_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_idle_errors (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    return e1000_read_ulong(page, start, off, count, eof, adapter->phy_stats.idle_errors);
}

static int e1000_read_link_reset_enabled (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    LINK_RESET_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_receive_errors (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    return e1000_read_ulong(page, start, off, count, eof, adapter->phy_stats.receive_errors);
}

static int e1000_read_mdi_x_enabled (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    MDI_X_MODE_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_local_receiver_status (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);

    LOCAL_RECEIVER_STATUS_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

static int e1000_read_remote_receiver_status (char *page, char **start, 
                off_t off, int count, int *eof, void *data)
{
    char *msg;
    struct e1000_adapter * adapter = (struct e1000_adapter *) data;

    /* If board is not open yet, */
    if(!test_bit(E1000_BOARD_OPEN, &adapter->flags)) 
        e1000_link_update(adapter);
    
    REMOTE_RECEIVER_STATUS_TO_STRING();
    strncpy (page, msg, PAGE_SIZE);
    return e1000_generic_read(page, start, off, count, eof);
}

int e1000_create_proc_dev(struct e1000_adapter * adapter)
{
    struct proc_dir_entry *dev_dir;
    char info[256];
    int len;

    dev_dir = create_proc_entry(adapter->netdev->name, S_IFDIR, e1000_proc_dir);

    strncpy(info, adapter->netdev->name, sizeof(info));
    len = strlen(info);
    strncat(info + len, ".info", sizeof(info) - len);

    /* info */
    if (!(e1000_create_proc_read(info, adapter, e1000_proc_dir, e1000_read_info)))
        return -1;

    /* description */
    if (!(e1000_create_proc_read(DESCRIPTION_TAG, adapter, dev_dir, e1000_read_descr)))
        return -1;
    /* part number */
    if (!(e1000_create_proc_read(PART_NUMBER_TAG, adapter, dev_dir, e1000_read_partnum)))
        return -1;
    /* driver name */
    if (!(e1000_create_proc_read(DRVR_NAME_TAG, adapter, dev_dir, e1000_read_drvr_name)))
        return -1;
    /* driver version */
    if (!(e1000_create_proc_read(DRVR_VERSION_TAG, adapter, dev_dir, e1000_read_drvr_ver)))
        return -1;
    /* pci vendor */
    if (!(e1000_create_proc_read(PCI_VENDOR_TAG, adapter, dev_dir, e1000_read_pci_vendor)))
        return -1;
    /* pci device id */
    if (!(e1000_create_proc_read(PCI_DEVICE_ID_TAG, adapter, dev_dir,
                           e1000_read_pci_device))) return -1;
    /* pci sub vendor */
    if (!(e1000_create_proc_read(PCI_SUBSYSTEM_VENDOR_TAG, adapter, dev_dir,
                           e1000_read_pci_sub_vendor))) return -1;
    /* pci sub device id */
    if (!(e1000_create_proc_read(PCI_SUBSYSTEM_ID_TAG, adapter, dev_dir,
                           e1000_read_pci_sub_device))) return -1;
    /* pci revision id */
    if (!(e1000_create_proc_read(PCI_REVISION_ID_TAG, adapter, dev_dir,
                           e1000_read_pci_revision))) return -1;
    /* device name */
    if (!(e1000_create_proc_read(SYSTEM_DEVICE_NAME_TAG, adapter, dev_dir,
                           e1000_read_dev_name))) return -1;
    /* pci bus */
    if (!(e1000_create_proc_read(PCI_BUS_TAG, adapter, dev_dir, e1000_read_pci_bus)))
        return -1;
    /* pci slot */
    if (!(e1000_create_proc_read(PCI_SLOT_TAG, adapter, dev_dir, e1000_read_pci_slot)))
        return -1;
    /* pci bus type */
    if (!(e1000_create_proc_read(PCI_BUS_TYPE_TAG, adapter, dev_dir, 
                               e1000_read_pci_bus_type))) return -1;
    /* pci bus speed */
    if (!(e1000_create_proc_read(PCI_BUS_SPEED_TAG, adapter, dev_dir, 
                               e1000_read_pci_bus_speed))) return -1;
    /* pci bus width */
    if (!(e1000_create_proc_read(PCI_BUS_WIDTH_TAG, adapter, dev_dir, 
                               e1000_read_pci_bus_width))) return -1;
    /* irq */
    if (!(e1000_create_proc_read(IRQ_TAG, adapter, dev_dir, e1000_read_irq)))
        return -1;
    /* current hwaddr */
    if (!(e1000_create_proc_read(CURRENT_HWADDR_TAG, adapter, dev_dir,
                           e1000_read_current_hwaddr))) return -1;
    /* permanent hwaddr */
    if (!(e1000_create_proc_read(PERMANENT_HWADDR_TAG, adapter, dev_dir,
                           e1000_read_permanent_hwaddr))) return -1;

    /* link status */
    if (!(e1000_create_proc_read(LINK_TAG, adapter, dev_dir, e1000_read_link_status)))
        return -1;
    /* speed */
    if (!(e1000_create_proc_read(SPEED_TAG, adapter, dev_dir, e1000_read_speed)))
        return -1;
    /* duplex mode */
    if (!(e1000_create_proc_read(DUPLEX_TAG, adapter, dev_dir, e1000_read_dplx_mode)))
        return -1;
    /* state */
    if (!(e1000_create_proc_read(STATE_TAG, adapter, dev_dir, e1000_read_state)))
        return -1;
    /* rx packets */
    if (!(e1000_create_proc_read(RX_PACKETS_TAG, adapter, dev_dir, e1000_read_rx_packets)))
        return -1;
    /* tx packets */
    if (!(e1000_create_proc_read(TX_PACKETS_TAG, adapter, dev_dir, e1000_read_tx_packets)))
        return -1;
    /* rx bytes */
    if (!(e1000_create_proc_read(RX_BYTES_TAG, adapter, dev_dir, e1000_read_rx_bytes)))
        return -1;
    /* tx bytes */
    if (!(e1000_create_proc_read(TX_BYTES_TAG, adapter, dev_dir, e1000_read_tx_bytes)))
        return -1;
    /* rx errors */
    if (!(e1000_create_proc_read(RX_ERRORS_TAG, adapter, dev_dir, e1000_read_rx_errors)))
        return -1;
    /* tx errors */
    if (!(e1000_create_proc_read(TX_ERRORS_TAG, adapter, dev_dir, e1000_read_tx_errors)))
        return -1;
    /* rx dropped */
    if (!(e1000_create_proc_read(RX_DROPPED_TAG, adapter, dev_dir, e1000_read_rx_dropped)))
        return -1;
    /* tx dropped */
    if (!(e1000_create_proc_read(TX_DROPPED_TAG, adapter, dev_dir, e1000_read_tx_dropped)))
        return -1;
    /* multicast packets */
    if (!(e1000_create_proc_read(MULTICAST_TAG, adapter, dev_dir, 
                                    e1000_read_rx_multicast_packets)))
        return -1;

    /* collisions */
    if (!(e1000_create_proc_read (COLLISIONS_TAG, adapter, dev_dir, e1000_read_collisions))) 
        return -1;
             
    /* rx length errors */
    if (!(e1000_create_proc_read(RX_LENGTH_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_length_errors))) return -1;
    /* rx over errors */
    if (!(e1000_create_proc_read(RX_OVER_ERRORS_TAG, adapter, dev_dir,
                               e1000_read_rx_over_errors))) return -1;
    /* rx crc errors */
    if (!(e1000_create_proc_read(RX_CRC_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_crc_errors))) return -1;
    /* rx frame errors */
    if (!(e1000_create_proc_read(RX_FRAME_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_frame_errors))) return -1;
    /* rx fifo errors */
    if (!(e1000_create_proc_read(RX_FIFO_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_fifo_errors))) return -1;
    /* rx missed errors */
    if (!(e1000_create_proc_read(RX_MISSED_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_missed_errors))) return -1;
    /* tx aborted errors */
    if (!(e1000_create_proc_read(TX_ABORTED_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_tx_aborted_errors))) return -1;
    /* tx carrier errors */
    if (!(e1000_create_proc_read(TX_CARRIER_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_tx_carrier_errors))) return -1;
    /* tx fifo errors */
    if (!(e1000_create_proc_read(TX_FIFO_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_tx_fifo_errors))) return -1;
    /* tx heartbeat errors */
    if (!(e1000_create_proc_read(TX_HEARTBEAT_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_tx_heartbeat_errors))) return -1;
    /* tx window errors */
    if (!(e1000_create_proc_read(TX_WINDOW_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_tx_window_errors))) return -1;

    if (!(e1000_create_proc_read(TX_LATE_COLL_TAG, adapter, dev_dir,
                           e1000_read_tx_late_coll))) return -1;
    if (!(e1000_create_proc_read(TX_DEFERRED_TAG, adapter, dev_dir,
                           e1000_read_tx_defer_events))) return -1;
    if (!(e1000_create_proc_read(TX_SINGLE_COLL_TAG, adapter, dev_dir,
                           e1000_read_tx_single_coll))) return -1;
    if (!(e1000_create_proc_read(TX_MULTI_COLL_TAG, adapter, dev_dir,
                           e1000_read_tx_multi_coll))) return -1;
    if (!(e1000_create_proc_read(RX_LONG_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_oversize))) return -1;
    if (!(e1000_create_proc_read(RX_SHORT_ERRORS_TAG, adapter, dev_dir,
                           e1000_read_rx_undersize))) return -1;
    if(adapter->shared.mac_type >= e1000_82543)
        if (!(e1000_create_proc_read(RX_ALIGN_ERRORS_TAG, adapter, dev_dir,
                               e1000_read_rx_align_err))) return -1;
    if (!(e1000_create_proc_read(RX_XON_TAG, adapter, dev_dir,
                           e1000_read_rx_xon))) return -1;
    if (!(e1000_create_proc_read(RX_XOFF_TAG, adapter, dev_dir,
                           e1000_read_rx_xoff))) return -1;
    if (!(e1000_create_proc_read(TX_XON_TAG, adapter, dev_dir,
                           e1000_read_tx_xon))) return -1;
    if (!(e1000_create_proc_read(TX_XOFF_TAG, adapter, dev_dir,
                           e1000_read_tx_xoff))) return -1;

    if (!(e1000_create_proc_read(MEDIA_TYPE_TAG, adapter, dev_dir,
                    e1000_read_media_type))) return -1;
    
    if (adapter->shared.media_type == e1000_media_type_copper) {
       if (!(e1000_create_proc_read(CABLE_LENGTH_TAG, adapter, dev_dir,
                    e1000_read_cable_length))) return -1;
   
       if (!(e1000_create_proc_read(EXTENDED_10BASE_T_DISTANCE_TAG, 
                   adapter, dev_dir,
                   e1000_read_extended_10base_t_distance))) return -1;
        
       if (!(e1000_create_proc_read(CABLE_POLARITY_TAG, adapter, dev_dir,
                    e1000_read_cable_polarity))) return -1;
   
       if (!(e1000_create_proc_read(CABLE_POLARITY_CORRECTION_TAG, adapter, dev_dir,
                   e1000_read_cable_polarity_correction))) return -1;
   
       if (!(e1000_create_proc_read(IDLE_ERRORS_TAG, adapter, dev_dir,
                    e1000_read_idle_errors))) return -1;
   
       if (!(e1000_create_proc_read(LINK_RESET_ENABLED_TAG, adapter, dev_dir,
                   e1000_read_link_reset_enabled))) return -1;
   
       if (!(e1000_create_proc_read(RECEIVE_ERRORS_TAG, adapter, dev_dir,
                    e1000_read_receive_errors))) return -1;
   
       if (!(e1000_create_proc_read(MDI_X_ENABLED_TAG, adapter, dev_dir,
                   e1000_read_mdi_x_enabled))) return -1;
  
       if (!(e1000_create_proc_read(LOCAL_RECEIVER_STATUS_TAG, adapter, dev_dir,
                   e1000_read_local_receiver_status))) return -1;
   
       if (!(e1000_create_proc_read(REMOTE_RECEIVER_STATUS_TAG, adapter, dev_dir,
                   e1000_read_remote_receiver_status))) return -1;
    }
    
    return 0;
}

void e1000_remove_proc_dev(struct net_device *dev)
{
    struct proc_dir_entry *de;
    struct e1000_adapter * adapter = dev->priv;
    char info[256];
    int len;

    len = strlen(dev->name);
    strncpy(info, dev->name, sizeof(info));
    strncat(info + len, ".info", sizeof(info) - len);

    for (de = e1000_proc_dir->subdir; de; de = de->next) {
        if ((de->namelen == len) && (!memcmp(de->name, dev->name, len)))
            break;
    }
    if (de) {
        remove_proc_entry(DESCRIPTION_TAG, de);
        remove_proc_entry(PART_NUMBER_TAG, de);
        remove_proc_entry(DRVR_NAME_TAG, de);
        remove_proc_entry(DRVR_VERSION_TAG, de);
        remove_proc_entry(PCI_VENDOR_TAG, de);
        remove_proc_entry(PCI_DEVICE_ID_TAG, de);
        remove_proc_entry(PCI_SUBSYSTEM_VENDOR_TAG, de);
        remove_proc_entry(PCI_SUBSYSTEM_ID_TAG, de);
        remove_proc_entry(PCI_REVISION_ID_TAG, de);
        remove_proc_entry(SYSTEM_DEVICE_NAME_TAG, de);
        remove_proc_entry(PCI_BUS_TAG, de);
        remove_proc_entry(PCI_SLOT_TAG, de);
        remove_proc_entry(PCI_BUS_TYPE_TAG, de);
        remove_proc_entry(PCI_BUS_SPEED_TAG, de);
        remove_proc_entry(PCI_BUS_WIDTH_TAG, de);
        remove_proc_entry(IRQ_TAG, de);
        remove_proc_entry(CURRENT_HWADDR_TAG, de);
        remove_proc_entry(PERMANENT_HWADDR_TAG, de);

        remove_proc_entry(LINK_TAG, de);
        remove_proc_entry(SPEED_TAG, de);
        remove_proc_entry(DUPLEX_TAG, de);
        remove_proc_entry(STATE_TAG, de);

        remove_proc_entry(RX_PACKETS_TAG, de);
        remove_proc_entry(TX_PACKETS_TAG, de);
        remove_proc_entry(RX_BYTES_TAG, de);
        remove_proc_entry(TX_BYTES_TAG, de);
        remove_proc_entry(RX_ERRORS_TAG, de);
        remove_proc_entry(TX_ERRORS_TAG, de);
        remove_proc_entry(RX_DROPPED_TAG, de);
        remove_proc_entry(TX_DROPPED_TAG, de);
        remove_proc_entry(MULTICAST_TAG, de);
        remove_proc_entry(COLLISIONS_TAG, de);
        remove_proc_entry(RX_LENGTH_ERRORS_TAG, de);
        remove_proc_entry(RX_OVER_ERRORS_TAG, de);
        remove_proc_entry(RX_CRC_ERRORS_TAG, de);
        remove_proc_entry(RX_FRAME_ERRORS_TAG, de);
        remove_proc_entry(RX_FIFO_ERRORS_TAG, de);
        remove_proc_entry(RX_MISSED_ERRORS_TAG, de);
        remove_proc_entry(TX_ABORTED_ERRORS_TAG, de);
        remove_proc_entry(TX_CARRIER_ERRORS_TAG, de);
        remove_proc_entry(TX_FIFO_ERRORS_TAG, de);
        remove_proc_entry(TX_HEARTBEAT_ERRORS_TAG, de);
        remove_proc_entry(TX_WINDOW_ERRORS_TAG, de);
        remove_proc_entry(TX_LATE_COLL_TAG, de);
        remove_proc_entry(TX_DEFERRED_TAG, de);
        remove_proc_entry(TX_SINGLE_COLL_TAG, de);
        remove_proc_entry(TX_MULTI_COLL_TAG, de);
        remove_proc_entry(RX_LONG_ERRORS_TAG, de);
        remove_proc_entry(RX_SHORT_ERRORS_TAG, de);
        remove_proc_entry(RX_XON_TAG, de);
        remove_proc_entry(RX_XOFF_TAG, de);
        remove_proc_entry(TX_XON_TAG, de);
        remove_proc_entry(TX_XOFF_TAG, de);

        remove_proc_entry(MEDIA_TYPE_TAG, de);
        if (adapter->shared.media_type == e1000_media_type_copper) {
           remove_proc_entry(CABLE_LENGTH_TAG, de);
           remove_proc_entry(EXTENDED_10BASE_T_DISTANCE_TAG, de);
           remove_proc_entry(CABLE_POLARITY_TAG, de);
           remove_proc_entry(CABLE_POLARITY_CORRECTION_TAG, de);
           remove_proc_entry(IDLE_ERRORS_TAG, de);
           remove_proc_entry(LINK_RESET_ENABLED_TAG, de);
           remove_proc_entry(RECEIVE_ERRORS_TAG, de);
           remove_proc_entry(MDI_X_ENABLED_TAG, de);
           remove_proc_entry(LOCAL_RECEIVER_STATUS_TAG, de);
           remove_proc_entry(REMOTE_RECEIVER_STATUS_TAG, de);
        }
    }

    remove_proc_entry(info, e1000_proc_dir);
    remove_proc_entry(dev->name, e1000_proc_dir);
}
