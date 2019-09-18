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

/* /proc definitions */
#include <linux/proc_fs.h>

#define ADAPTERS_PROC_DIR           "PRO_LAN_Adapters"

#define DESCRIPTION_TAG             "Description"
#define PART_NUMBER_TAG             "Part_Number"
#define DRVR_NAME_TAG               "Driver_Name"
#define DRVR_VERSION_TAG            "Driver_Version"
#define PCI_VENDOR_TAG              "PCI_Vendor"
#define PCI_DEVICE_ID_TAG           "PCI_Device_ID"
#define PCI_SUBSYSTEM_VENDOR_TAG    "PCI_Subsystem_Vendor"
#define PCI_SUBSYSTEM_ID_TAG        "PCI_Subsystem_ID"
#define PCI_REVISION_ID_TAG         "PCI_Revision_ID"
#define PCI_BUS_TAG                 "PCI_Bus"
#define PCI_SLOT_TAG                "PCI_Slot"
#define PCI_BUS_TYPE_TAG            "PCI_Bus_Type"
#define PCI_BUS_SPEED_TAG           "PCI_Bus_Speed"
#define PCI_BUS_WIDTH_TAG           "PCI_Bus_Width"
#define IRQ_TAG                     "IRQ"
#define SYSTEM_DEVICE_NAME_TAG      "System_Device_Name"
#define CURRENT_HWADDR_TAG          "Current_HWaddr"
#define PERMANENT_HWADDR_TAG        "Permanent_HWaddr"

#define LINK_TAG                    "Link"
#define SPEED_TAG                   "Speed"
#define DUPLEX_TAG                  "Duplex"
#define STATE_TAG                   "State"

#define RX_PACKETS_TAG              "Rx_Packets"
#define TX_PACKETS_TAG              "Tx_Packets"
#define RX_BYTES_TAG                "Rx_Bytes"
#define TX_BYTES_TAG                "Tx_Bytes"
#define RX_ERRORS_TAG               "Rx_Errors"
#define TX_ERRORS_TAG               "Tx_Errors"
#define RX_DROPPED_TAG              "Rx_Dropped"
#define TX_DROPPED_TAG              "Tx_Dropped"
#define MULTICAST_TAG               "Multicast"
#define COLLISIONS_TAG              "Collisions"
#define RX_LENGTH_ERRORS_TAG        "Rx_Length_Errors"
#define RX_OVER_ERRORS_TAG          "Rx_Over_Errors"
#define RX_CRC_ERRORS_TAG           "Rx_CRC_Errors"
#define RX_FRAME_ERRORS_TAG         "Rx_Frame_Errors"
#define RX_FIFO_ERRORS_TAG          "Rx_FIFO_Errors"
#define RX_MISSED_ERRORS_TAG        "Rx_Missed_Errors"
#define TX_ABORTED_ERRORS_TAG       "Tx_Aborted_Errors"
#define TX_CARRIER_ERRORS_TAG       "Tx_Carrier_Errors"
#define TX_FIFO_ERRORS_TAG          "Tx_FIFO_Errors"
#define TX_HEARTBEAT_ERRORS_TAG     "Tx_Heartbeat_Errors"
#define TX_WINDOW_ERRORS_TAG        "Tx_Window_Errors"

#define RX_TCP_CHECKSUM_GOOD_TAG    "Rx_TCP_Checksum_Good"
#define RX_TCP_CHECKSUM_BAD_TAG     "Rx_TCP_Checksum_Bad"
#define TX_TCP_CHECKSUM_GOOD_TAG    "Tx_TCP_Checksum_Good"
#define TX_TCP_CHECKSUM_BAD_TAG     "Tx_TCP_Checksum_Bad"

#define TX_LATE_COLL_TAG            "Tx_Abort_Late_Coll"
#define TX_DEFERRED_TAG             "Tx_Deferred_Ok"
#define TX_SINGLE_COLL_TAG          "Tx_Single_Coll_Ok"
#define TX_MULTI_COLL_TAG           "Tx_Multi_Coll_Ok"
#define RX_LONG_ERRORS_TAG          "Rx_Long_Length_Errors"
#define RX_SHORT_ERRORS_TAG         "Rx_Short_Length_Errors"
#define RX_ALIGN_ERRORS_TAG         "Rx_Align_Errors"
#define RX_XON_TAG                  "Rx_Flow_Control_XON"
#define RX_XOFF_TAG                 "Rx_Flow_Control_XOFF"
#define TX_XON_TAG                  "Tx_Flow_Control_XON"
#define TX_XOFF_TAG                 "Tx_Flow_Control_XOFF"
#define RX_CSUM_GOOD_TAG            "Rx_Csum_Offload_Good"
#define RX_CSUM_ERROR_TAG           "Rx_Csum_Offload_Errors"

/* what is the cable length (only for 100/1000 modes)? - 50, 50-80, 80-110, 110-140 and > 140 meters */
#define CABLE_LENGTH_TAG                 "PHY_Cable_Length"

/* Media Type Copper/Fiber */
#define MEDIA_TYPE_TAG                   "PHY_Media_Type"

/* Is extended 10 Base-T distance feature enabled? This is done by lowering the receive threshold - enabled/disabled */
#define EXTENDED_10BASE_T_DISTANCE_TAG   "PHY_Extended_10Base_T_Distance"

/* Cable polarity Normal/Reversed */
#define CABLE_POLARITY_TAG               "PHY_Cable_Polarity"

/* Is Polarity reversal enabled? Enabled/Disabled */
#define CABLE_POLARITY_CORRECTION_TAG    "PHY_Disable_Polarity_Correction"

/* Number of IDLE Errors */
#define IDLE_ERRORS_TAG                  "PHY_Idle_Errors"

/* Should the link be brought down if an IDLE is not seen within 1 msec while in 1000mbps mode? Enabled/Disabled */
#define LINK_RESET_ENABLED_TAG           "PHY_Link_Reset_Enabled"

/* Number of receive errors */
#define RECEIVE_ERRORS_TAG               "PHY_Receive_Errors"

/* MDI-X Support Enabled? Auto, Manual(MDI) or Manual(MDI-X) */
#define MDI_X_ENABLED_TAG                "PHY_MDI_X_Enabled"

/* Local Receiver OK? OK/NOT_OK */
#define LOCAL_RECEIVER_STATUS_TAG        "PHY_Local_Receiver_Status"

/* Remote Receiver OK? OK/NOT_OK */
#define REMOTE_RECEIVER_STATUS_TAG       "PHY_Remote_Receiver_Status"

/* symbols exported to e1000_main */
extern struct proc_dir_entry *e1000_proc_dir;
extern int e1000_create_proc_dev(struct e1000_adapter * Adapter);
extern void e1000_remove_proc_dev(struct net_device *dev);
