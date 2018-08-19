/*
 Copyright (c) 2016 Academia Sinica, Institute of Information Science

 License:

      GPL 3.0 : The content of this file is subject to the terms and
      cnditions defined in file 'COPYING.txt', which is part of this source
      code package.

 Project Name:

      BeDIPS

 File Description:

      This file contains the program to allow transmission between 
       LBeacon and Gateway. 

 File Name:

      Communication.c

 Version:
 
       1.2

 Abstract:

      BeDIPS uses LBeacons to deliver 3D coordinates and textual
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
      Gary Xiao, garyh0205@hotmail.com      
      
*/


#include "Communication.h"



ErrorCode_Xbee zigbee_init(Zigbee *zigbee){

     /* Parameters for Zigbee initialization */
    char* xbee_mode = "xbeeZB";
    char* xbee_device = "/dev/ttyAMA0";

    /* Initialize Zigbee */
    xbee_initial(xbee_mode, xbee_device, XBEE_BAUDRATE, 
                 &zigbee->xbee, &zigbee->send_queue, &zigbee->received_queue);
   
#ifdef Debugging 

    zlog_debug(category_debug, "Establishing Connection...");
      
#endif

    xbee_connector(&zigbee->xbee, 
                   &zigbee->con, 
                   &zigbee->send_queue,
                   &zigbee->received_queue);
    
#ifdef Debugging 
   
    zlog_debug(category_debug, 
               "Zigbee Connection Successfully Established");
      
#endif

    zlog_info(category_health_report, 
              "Zigbee Connection Successfully Established");

    /* Start the chain reaction                                             */
    if((ret = xbee_conValidate(zigbee->con)) != XBEE_ENONE){
        
        xbee_log(zigbee->xbee, 1, "con unvalidate ret : %d", ret);
        
        perror(error_xbee[E_XBEE_VALIDATE].message);
        zlog_info(category_health_report, 
                  error_xbee[E_XBEE_VALIDATE].message);
        
        return E_XBEE_VALIDATE;
    }

    return XBEE_SUCCESSFULLY;
}


int receive_call_back(Zigbee *zigbee){
  
    /* Check the connection of call back is enable */ 
    if(xbee_check_CallBack(zigbee->con, &zigbee->send_queue, false)){
      
      perror(error_xbee[E_CALL_BACK].message);
      zlog_info(category_health_report, 
                error_xbee[E_CALL_BACK].message);
      
      return NULL;
    
    };

    /* Get the polled type form the gateway */
    pPkt temppkt = get_pkt(&zigbee->received_queue);
    
    
    if(temppkt != NULL){

      printf("Geting the call back\n");

        /* If data[0] == '@', callback will be end.                       */
        if(temppkt -> content[0] == 'T'){

          return TRACK_OBJECT_DATA;

        }else if(temppkt -> content[0] == 'H'){

          printf("HEALTH_REPORT \n");
          return HEALTH_REPORT; 

        }else if(temppkt -> content[0] == '@'){

            xbee_conCallbackSet(zigbee->con, NULL, NULL);

            printf("*** DISABLED CALLBACK... ***\n");

        }

            delpkt(&zigbee->received_queue);

    }

    return NOT_YET_POLLED;

}

void zigbee_send_file(Zigbee *zigbee){
    
    printf("In the send file \n");
    
        xbee_connector(&zigbee->xbee, 
                   &zigbee->con, 
                   &zigbee->send_queue,
                   &zigbee->received_queue);

    /* Add the content that to be sent to the gateway to the packet queue */
    addpkt(&zigbee->send_queue, Data, Gateway, zigbee->zig_message);

    /* If there are remain some packet need to send in the Queue,send the 
    packet */                                      
    xbee_send_pkt(zigbee->con, &zigbee->send_queue);

    usleep(XBEE_TIMEOUT);
        



   return;
}


ErrorCode_Xbee zigbee_free(Zigbee *zigbee){


    /* Close connection                                                      */
    if ((ret = xbee_conEnd(zigbee->con)) != XBEE_ENONE) {
        
        xbee_log(zigbee->xbee, 10, "xbee_conEnd() returned: %d", ret);
        
        perror(error_xbee[E_CONNECT].message);
        zlog_info(category_health_report, 
                  error_xbee[E_CONNECT].message);
        
        return;
    }

    /* Free Send_Queue for zigbee connection */
    Free_Packet_Queue(&zigbee->send_queue);

    /* Free received_Queue for zigbee connection */
    Free_Packet_Queue(&zigbee->received_queue);
    
    printf("Stop Xbee connection Succeeded\n");
    zlog_info(category_health_report, 
              "Stop Xbee connection Succeeded\n");

    /* Close xbee                                                            */
    xbee_shutdown(zigbee->xbee);
    printf("Shutdown Xbee Succeeded\n");
    zlog_info(category_health_report, 
              "Shutdown Xbee Succeeded\n");

}