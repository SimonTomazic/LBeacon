/*
Copyright (c) 2016 Academia Sinica, Institute of Information Science

License:

    GPL 3.0 : The content of this file is subject to the terms and
    conditions defined in file 'COPYING.txt', which is part of this source
    code package.

Project Name:

    BeDIS

File Description:

    This header file contains declarations of variables, structs and
    functions and definitions of global variables used in the LBeacon.c file.

File Name:

    LBeacon.h

Version:

    1.2,  20181114

Abstract:

    BeDIS uses LBeacons to deliver 3D coordinates and textual
    descriptions of their locations to users' devices. Basically, a
    LBeacon is an inexpensive, Bluetooth Smart Ready device. The 3D
    coordinates and location description of every LBeacon are retrieved
    from BeDIS (Building/environment Data and Information System) and
    stored locally during deployment and maintenance times. Once
    initialized, each LBeacon broadcasts its coordinates and location
    description to Bluetooth enabled user devices within its coverage
    area.

Authors:

    Han Wang, hollywang@iis.sinica.edu.tw
    Jake Lee, jakelee@iis.sinica.edu.tw
    Joey Zhou, joeyzhou@iis.sinica.edu.tw
    Kenneth Tang, kennethtang@iis.sinica.edu.tw
    James Huamg, jameshuang@iis.sinica.edu.tw
    Shirley Huang, shirley.huang.93@gmail.com


*/

#ifndef LBEACON_H
#define LBEACON_H

/*
* INCLUDES
*/

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <netinet/in.h>
#include <obexftp/client.h>
#include "BeDIS.h"


/*
  CONSTANTS
*/

/* The major and minor versions of LBeacon used for advertising */
#define LBEACON_MAJOR_VER 1
#define LBEACON_MINOR_VER 2

/* Command opcode pack/unpack from HCI library. ogf and ocf stand for Opcode
   group field and Opcofe command field, respectively. See Bluetooth
   specification - core version 4.0, vol.2, part E Chapter 5.4 for details.
*/
//#define cmd_opcode_pack(ogf, ocf) (uint16_t)((ocf &amp; 0x03ff) | \
//                                                        (ogf &lt;&lt; 10))
/* File path of the config file of the LBeacon */
#define CONFIG_FILE_NAME "../config/config.conf"

/* File path of the logging file*/
#define LOG_FILE_NAME "../config/zlog.conf"

/* The category defined of log file used for health report */
#define LOG_CATEGORY_HEALTH_REPORT "Health_Report"

/* The category defined for the printf during debugging */
#define LOG_CATEGORY_DEBUG "LBeacon_Debug"

/* The prefix for Beacon basic information */
#define BEACON_BASIC_PREFIX "B:" 

/* The prefix for Gateway basic information */
#define GATEWAY_BASIC_PREFIX "G:" 

/* The temporary file for uploading tracked BR data */
#define TRACKED_BR_TXT_FILE_NAME "tracked_br_txt.txt"

/* The temporary file for uploading tracked BLE data */
#define TRACKED_BLE_TXT_FILE_NAME "tracked_ble_txt.txt"

/* The log file for LBeacon health history */
#define HEALTH_REPORT_LOG_FILE_NAME "Health_Report.log"

/* BlueZ bluetooth extended inquiry response protocol: flags */
#define EIR_FLAGS 0X01

/* BlueZ bluetooth extended inquiry response protocol: Manufacturer Specific
   Data */
#define EIR_MANUFACTURE_SPECIFIC_DATA 0xFF

/* BlueZ bluetooth extended inquiry response protocol: complete local name */
#define EIR_NAME_COMPLETE 0x09

/* BlueZ bluetooth extended inquiry response protocol: short local name */
#define EIR_NAME_SHORT 0x08

/* Maximum number of characters in message file name */
#define FILE_NAME_BUFFER 64

/* Length in number of chars used for basic information */
#define LENGTH_OF_INFO 128


/* Time interval in seconds of a bluetooth device stays in the
   scanned device list. This time interval is for
   cleanup_scanned_list function
*/
#define INTERVAL_HANDLE_SCANNED_LIST_IN_SEC 30

/* Timeout in milliseconds of hci_send_req funtion */
#define HCI_SEND_REQUEST_TIMEOUT_IN_MS 1000

/* Time interval in milliseconds between advertising by a LBeacon */
#define INTERVAL_ADVERTISING_IN_MS 3000

/* Time interval in seconds for cleanup_scanned_list busy-wait
checking in threads */
#define INTERVAL_FOR_BUSY_WAITING_CHECK_CLEANUP_SCANNED_LIST_IN_SEC 30

/* Time interval in seconds for timeout_cleanup function to cleanup
all lists. Currently, it is a periodical tasks, and we will change
this cleanup tasks to be only triggered by network connection failure
or memory allocations reach threshold situations.
*/
#define INTERVAL_FOR_CLEANUP_LISTS_IN_SEC 1800

/* Time interval in seconds for timeout_cleanup function to wait
for abnormal network situatins */
#define INTERVAL_WATCHDOG_FOR_NETWORK_DOWN_IN_SEC 10

/* Time interval in seconds for idle status in Wifi connection between
LBeacon and gateway. When this situation happens, LBeacon will send
join requset to gateway again. */
#define INTERVAL_RECEIVE_MESSAGE_FROM_GATEWAY_IN_SEC 300


/* Nominal transmission range limit. Only devices in this RSSI range are
   to be discovered and data sent. */
#define RSSI_RANGE -80

/* RSSI value of TX power for calibration and broadcast  */
#define RSSI_VALUE -50

/* Number of characters in a Bluetooth MAC address */
#define LENGTH_OF_MAC_ADDRESS 18

/* Number of digits of MAC address to compare */
#define NUM_DIGITS_TO_COMPARE 2

/* Number of worker threads in the thread pool used by communication unit */
#define NUM_WORK_THREADS 4

/* Location data of the maximum number of each type of objects (BR or BLE)
   to be transmitted at one time over wife network link */
#define MAX_NUM_OBJECTS 10


/* The macro of comparing two integer for minimum */
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })





/*
  TYPEDEF STRUCTS
*/

/* The configuration file structure */

typedef struct Config {

    /* String representation of the X coordinate of the beacon location */
    char coordinate_X[CONFIG_BUFFER_SIZE];

    /* String representation of the Y coordinate of the beacon location */
    char coordinate_Y[CONFIG_BUFFER_SIZE];

    /* String representation of the Z coordinate of the beacon location */
    char coordinate_Z[CONFIG_BUFFER_SIZE];

    /* String representation of the message file name */
    char file_name[CONFIG_BUFFER_SIZE];

    /* String representation of the path name of message file */
    char file_path[CONFIG_BUFFER_SIZE];

    /* String representation of the maximum number of devices to be
       handled by all push dongles
    */
    char maximum_number_of_devices[CONFIG_BUFFER_SIZE];

    /* String representation of number of message groups */
    char number_of_groups[CONFIG_BUFFER_SIZE];

    /* String representation of the number of messages */
    char number_of_messages[CONFIG_BUFFER_SIZE];

    /* String representation of the number of push dongles */
    char number_of_push_dongles[CONFIG_BUFFER_SIZE];

    /* String representation of the required signal strength */
    char rssi_coverage[CONFIG_BUFFER_SIZE];

    /* String representation of the universally unique identifer */
    char uuid[CONFIG_BUFFER_SIZE];

    /* The IPv4 network address of gateway */
    char gateway_addr[NETWORK_ADDR_LENGTH];
    
    /* Sepcify the UDP port of gateway connection*/
    int gateway_port;
    
    /* Specify the UDP port for LBeacon to listen and receive UDP from gateway*/
    int local_client_port;

} Config;


/* The structure for storing information and status of a thread */
typedef struct ThreadStatus {

    char scanned_mac_address[LENGTH_OF_MAC_ADDRESS];
    bool idle;
    bool is_waiting_to_send;

} ThreadStatus;


/* Struct for storing MAC address of a Bluetooth device and the time instants
   at which the address is scanned
*/
typedef struct ScannedDevice {

    char scanned_mac_address[LENGTH_OF_MAC_ADDRESS];
    long long initial_scanned_time;
    long long final_scanned_time;
    /* List entries for linking the struct to scanned_list and
       tracked_BR_object_list or to tracked_BLE_object_list, depending
       whether the device type is BR_EDR or BLE. */
    struct List_Entry sc_list_entry;
    struct List_Entry tr_list_entry;


/* Pad added to make the struct size an integer multiple of 32 byte, size of
   D-cache line.

   int pad[30];
*/

} ScannedDevice;

/* struct for device list head. */
typedef struct object_list_head{

  struct List_Entry list_entry;
  DeviceType device_type;

} ObjectListHead;



/*
  EXTERN STRUCTS
*/

/* In sys/poll.h, the struct for controlling the events. */
//extern struct pollfd;

/* In hci_sock.h, the struct for callback event from the socket. */
//extern struct hci_filter;



/*
  GLOBAL VARIABLES
*/

/* Struct for storing config information from the input file */
Config g_config;

/* Path of the object push file */
char *g_push_file_path;


/* The struct of UDP configuration */
sudp_config_beacon udp_config;


/* Heads of three lists of structs for recording scanned devices */

/* Head of scanned_list that holds the scanned device structs of
   BR/EDR devices found in recent scans. The MAC address elements of all the
   structs in this list are distinct.
*/
ObjectListHead scanned_list_head;

/* Head of tracking_BR_object_list that holds the scanned device structs of
   Bluetooth devices discovered in recent scans. The MAC address elements of
   some structs in the list may be identical but their associated timestamps
   indicate disjoint time intervals. The contents of the list await to be
   sent via the gateway to the server to be processed there.
*/
ObjectListHead BR_object_list_head;

/* Head of tracking_BLE_object_list that holds the scanned device structs
   of BLE devices discovered in recent scans. The MAC address elements of
   some structs in the list may be identical but their associated timestamps
   indicate disjoint time intervals. The contents of the list await to be
   sent via the gateway to the server to be processed there.
*/
ObjectListHead BLE_object_list_head;


/* Global flags for communication among threads */

/* A global flag that is initially set to false by the main thread. When there
is any error of the network connection */
bool network_is_down;


/* The memory pool for the allocation of all nodes in scanned device list and
   tracked object lists. */
Memory_Pool mempool;

/* The lock that controls access to lists */
pthread_mutex_t  list_lock;

#ifdef Bluetooth_classic

/* An array of struct for storing information and status of threads */
ThreadStatus g_idle_handler[MAX_NUM_OBJECTS];

#endif

/*
  EXTERNAL GLOBAL VARIABLES
*/

extern int errno;



/*
  FUNCTIONS
*/

/*
  get_config:

      This function reads the specified config file line by line until the
      end of file and copies the data in the lines into the Config struct
      global variable.

  Parameters:
      config - Config struct including file path, coordinates, etc.
      file_name - the name of the config file that stores all the beacon data

  Return value:

      ErrorCode - indicate the result of execution, the expected return code is
  WORK_SUCCESSFULLY
*/

ErrorCode get_config(Config *config, char *file_name);




/*
  print_RSSI_value:

      This function prints the RSSI value along with the MAC address of the
      user's scanned bluetooth device.

  Parameters:

      bluetooth_device_address - MAC address of a bluetooth device
      has_rssi - a flag indicating whether the bluetooth device has an RSSI
                 value
      rssi - RSSI value of bluetooth device

  Return value:

      None
*/

void print_RSSI_value(bdaddr_t *bluetooth_device_address, bool has_rssi,
    int rssi);


/*
  start_ble_scanning:

      This function scans continuously for BLE bluetooth devices under the
      coverage of the  beacon until scanning is cancelled. To reduce the
      traffic among BeDIS system, this function only tracks the tags with
      our specific name. When one tag with specific name are scanned, this
      function calls send_to_push_dongle to either add a new ScannedDevice
      struct of the device to ble_object_list or update the final scan time
      of a struct in the list.

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      None
*/

void *start_ble_scanning(void *param);


/*
  start_br_scanning:

      This function scans continuously for bluetooth devices under the
      coverage of the  beacon until scanning is cancelled. When the RSSI
      value of the device is within the threshold, this function calls
      send_to_push_dongle to either add a new ScannedDevice struct of the
      device to scanned list and track_object_list or update the final scan
      time of a struct in the lists.

      [N.B. This function is executed by the main thread. ]

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      None
*/
void *start_br_scanning(void *param);



/*
  send_to_push_dongle:

      When called, this functions first checks whether there is a
      ScannedDevice struct in the scanned list or ble_object_list with MAC
      address matching the input bluetooth device address depending on
      whether the device is a BR/EDR type or BLE type. If there is no such
      struct, this function allocates from memory pool space for a
      ScannedDeivce struct, sets the MAC address of the new struct to the
      input MAC address, the initial scanned time and final scanned time to
      the current time, and inserts the sruct at the head of the scanned_list
      if the device is of BR/EDR type, and tail of the tracked object list
      for the device type. If a struct with MAC address matching the input
      device address is found, this function sets the final scanned time of
      the struct to current time.

  Parameters:

      bluetooth_device_address - MAC address of a bluetooth device discovered
                                 during inquiry
      is_ble - the indicator to show whether the input address is that of a
               BLE device

  Return value:

      None
*/

void send_to_push_dongle(bdaddr_t *bluetooth_device_address, bool is_ble);




/*
  check_is_in_list:

      This function checks whether the MAC address given as input is in the
      specified list. If a node with MAC address matching the input address
      is found in the list, the function returns the pointer to the node
      with matching address; otherwise it returns NULL.

  Parameters:

      address - MAC address of a bluetooth device
      list_head - the head of a specified list


  Return value:

  match_node - A pointer to the node found with MAC address identical to
               the input address.
               or NULL
*/

struct ScannedDevice *check_is_in_list(char address[],
                                       ObjectListHead *list);




/*
  enable_advertising:

      This function enables the LBeacon to start advertising, sets the time
      interval for advertising, and calibrates the RSSI value.

  Parameters:

      advertising_interval - the time interval during which the LBeacon can
                         advertise
      advertising_uuid - universally unique identifier for advertising
      major_number - major version number of LBeacon
      minor_number - minor version number of LBeacon
      rssi_value - RSSI value of the bluetooth device

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise

*/

ErrorCode enable_advertising(int advertising_interval,
                             char *advertising_uuid,
           int major_number,
           int minor_number,
                             int rssi_value);


/*
  disable_advertising:

      This function disables the advertising capabilities of the beacon.

  Parameters:

      None

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode disable_advertising();


/*
  cleanup_scanned_list:

      This function checks each ScannedDevice node in the scanned list to
      determine whether the node has been in the list for over TIMEOUT unit
      of time. If yes, the function removes the ScannedDevice struct from
      the list. If the struct is no longer in the tracked_object_list also,
      the function calls the memory pool to release the memory space used by
      the struct.

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      None
*/

void *cleanup_scanned_list(void *param);


/*
  timeout_cleanup:

      This function sets a timer to countdown a specific time. When timer
      expires, cleans up tracked object lists to make sure the memory space
      is always available.

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      None
*/

void *timeout_cleanup(void *param);


/*
  manage_communication:

      This is the start function of the main thread in the communication
      unit of LBeacon. After initializing the wifi network, it creates a
      thread pool with NUM_WORK_THREADS worker threads; then while the beacon
      is ready to work, the function waits for poll from the gateway, when
      polled, the function creates appropriate work items to be executed by
      a worker thread.

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      None
*/

void *manage_communication(void *param);


/*
  copy_object_data_to_file:

      This function copies the data on tracked objects captured in the
      specifed tracked object list to file to be transfer to gateway. The
      output file contains for each ScannedDevice struct found in the list,
      the MAC address and the initial and final timestamps.

  Parameters:

      file_name - name of the file containing data stored in all
                  ScannedDevice struct found in specified tracked object
                  list.
      list - head of the tracked object list from which data is to be
             copied.

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode copy_object_data_to_file(char *file_name, ObjectListHead *list);


/*
  consolidate_tracked_data:

      This function consolidate the data on tracked objects captured in the
      specifed tracked object list to message to be transfer to gateway. The
      output message buffer contains for each ScannedDevice struct found in
      the list, the MAC address and the initial and final timestamps.

  Parameters:

      list - head of the tracked object list from which data is to be
             copied.

      msg_buf - message buffer to contain the consolidated data

      msg_size - size of bytes of msg_buf

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode consolidate_tracked_data(ObjectListHead *list, char *msg_buf, size_t msg_size);

/*
  free_list:

      This function removes nodes from the specified list and if the removed
      node is not in any list, calls memory pool to release memory used by
      the node.

  Parameters:

      list_entry - the head of a specified list.
      device_type - type of device with data contained in the list

  Return value:

      None
*/

void free_list(List_Entry *list_entry, DeviceType device_type);

/*
  cleanup_exit:

      This function releases all the resources.

  Parameters:

      err_code - the error code of hard errors which causes program to
      terminate

  Return value:

      None
*/

void cleanup_exit(ErrorCode err_code);





/*
  EXTERNAL FUNCTIONS
*/



/*
  opendir:

      This function is called to open a specified directory.

  Parameters:

      dirname - the name of the directory which is goning to be opened.

  Return value:

      dirp - a pointer to the directory stream.
*/
extern DIR *opendir(const char *dirname);

/*
  memset:

      This function is called to fill a block of memory.

  Parameters:

      ptr - the pointer points to the memory area
      value - the constant byte to replace the memory area
      number - number of bytes in the memory area starting from ptr to be
               filled

  Return value:

      dst - a pointer to the memory area
*/
extern void * memset(void * ptr, int value, size_t number);


/*
  hci_open_dev:

      This function is called to open a Bluetooth socket with the specified
      resource number.

  Parameters:

      dev_id - the id of the Bluetooth socket device

  Return value:

      dd - device descriptor of the Bluetooth socket
*/
extern int hci_open_dev(int dev_id);


/*
  hci_filter_clear:

      This function is called to clear filter.

  Parameters:

      f - the filter to be cleaned

  Return value:

      None
*/
extern void hci_filter_clear(struct hci_filter *f);


/*
  hci_filter_set_ptype:

      This function is called to let filter set ptype.

  Parameters:

      t - the type
      f - the filter to be set

  Return value:

      None
*/
extern void hci_filter_set_ptype(int t, struct hci_filter *f);


/*
  hci_filter_set_event:

      This function is called to let filter set event

  Parameters:

      e - the event
      f - the filter to be set

  Return value:

      None
*/
extern void hci_filter_set_event(int e, struct hci_filter *f);


/*
  hci_write_inquiry_mode:

      This function is called to configure inquiry mode

  Parameters:

      dd - device descriptor of the open HCI socket
      mode - new inquiry mode
      to - send request to this

  Return value:

      None
*/
extern int hci_write_inquiry_mode(int dd, uint8_t mode, int to);


/*
  hci_send_cmd:

      This function is called to send cmd

  Parameters:

      dd - device descriptor of the open HCI socket
      ogf - opcode group field
      ocf - opcode command field
      plen - the length of the command parameters
      param - the parameters that function runs with

  Return value:

      0 for success. error number for error.
*/
extern int  hci_send_cmd(int dd, uint16_t ogf, uint16_t ocf, uint8_t plen,
    void *param);

/* Functions for communication via BR/EDR path to Bluetooth
   classic devices */
#ifdef Bluetooth_classic

/*
  obexftp_open:

      This function is called to create an obexftp client.

  Parameters:

      transport - the transport protocol that will be used
      ctrans - optional custom transport protocol
      infocb - optional info callback
      infocb_data - optional info callback data

  Return value:

      cli - a new allocated ObexFTP client instance, or NULL on error.
*/
extern obexftp_client_t * obexftp_open(int transport, obex_ctrans_t *ctrans,
    obexftp_info_cb_t infocb, void *infocb_data);

/*
  send_data:

      When called, this function sends a packet that containing the specified
      message to the gateway via xbee module.

  Parameters:

    message - the message to be sent via xbee module

  Return value:

      None

*/
extern void *send_data(char *message);


/*
  choose_file:

    This function receives the name of a message file and returns the file
    path where the message is located. It goes through each directory in the
    messages folder and in each category, it reads each file name to find
    the designated message to be broadcast to the users under the beacon.

  Parameters:

    message_to_send - name of the message file we want to retrieve

  Return value:

    eturn_value - message file path
*/


char *choose_file(char *message_to_send);

/*
  send_file:

    This function pushes a message asynchronously to devices. It is the
    thread function of the specified thread.

    [N.B. The beacon may still be scanning for other bluetooth devices.]

  Parameters:

    id - ID of the thread used to send the push message

  Return value:

    None
*/

void *send_file(void *id);


/*
  start_classic_pushing:

    This function creates threads per devices to push the data or file to
    the scanned classic Bluetooth devices via BR/EDR path.

    [N.B. The code in this function was orignally put in the main function]

  Parameters:

    None

  Return value:

    None
*/
void start_classic_pushing(void);

#endif // Bluetooth_classic

#endif
