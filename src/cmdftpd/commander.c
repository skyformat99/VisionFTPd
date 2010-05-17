#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "main.h"
#include "net_common.h"
#include "commander.h"

#include "net_threader.h"
#include "net_messenger.h"
#include "unsigned_string.h"

#include "ftpd.h"

#include "vfs.h"


/****************************************************************************************************

 ***********  ***********  **********
 *                 *       *         *               
 *                 *       *         *              
 *                 *       *         *              
 *******           *       **********                      
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        

****************************************************************************************************/



int commander_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = 0;


    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        scar_log (1, "%s: Error: the ftp_state object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

#ifdef DEBUG
    scar_log (1, "   << %s\n", read_buffer_state -> buffer);
#endif

    /* Init phase for the FTP command channel */
    if (ftp_state -> init == 0)
    {
        rc = handle_ftp_initialization (ftp_state, NULL, write_buffer_state);
        goto finalize_message_handling;
    }

    /* Handle ABOR message */
    if ((rc = handle_ftp_ABOR (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle QUIT message */
    if ((rc = handle_ftp_QUIT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle USER message */
    if ((rc = handle_ftp_USER (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PASS message */
    if ((rc = handle_ftp_PASS (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle SYST message */
    if ((rc = handle_ftp_SYST (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle FEAT message */
    if ((rc = handle_ftp_FEAT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PWD  message */
    if ((rc = handle_ftp_PWD  (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle CWD  message */
    if ((rc = handle_ftp_CWD  (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle TYPE message */
    if ((rc = handle_ftp_TYPE (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle SIZE message */
    if ((rc = handle_ftp_SIZE (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle STAT message */
    if ((rc = handle_ftp_STAT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle LIST message */
    if ((rc = handle_ftp_LIST (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle TYPE message */
    if ((rc = handle_ftp_TYPE (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle EPRT message */
    if ((rc = handle_ftp_EPRT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle LPRT message */
    if ((rc = handle_ftp_LPRT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PORT message */
    if ((rc = handle_ftp_PORT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle NOOP message */
    if ((rc = handle_ftp_NOOP (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;


    /* 500 - Syntax error, command unrecognized. */
    if ((rc = handle_message_not_understood (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int commander_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = NET_RC_IDLE;
    net_msg_t * msg_to_send = NULL;


    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        scar_log (1, "%s: Error: the ftp_state object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

    /* Init phase for the FTP command channel */
    if (ftp_state -> init == 0)
    {
        rc = handle_ftp_initialization (ftp_state, NULL, write_buffer_state);
        goto finalize_message_handling;
    }


    /* Handle message queue */
    if ((msg_to_send = net_msg_pop_from_queue (ftp_state -> output_q)))
    {
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, 
                                                    write_buffer_state -> buffer_size,
                                                    "%s",
                                                    msg_to_send -> msg -> buffer);
        /* Remove popped message */
        net_msg_delete_msg (msg_to_send);
        msg_to_send = NULL;

        /* Buffer overrun */ 
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            rc = NET_RC_DISCONNECT;
        }
        else
        {
            rc = NET_RC_MUST_WRITE;
        }
        goto finalize_message_handling;
    }

finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int commander_state_liberator (void ** state)
{
    ftp_state_t *     ftp_state  = *(ftp_state_t **)state;
    file_transfer_t * helper     = NULL;
    
    if (ftp_state)
    {
        ftp_state -> init       = 0;
        ftp_state -> mode       = ASCII;
        free(ftp_state -> ftp_user);
        ftp_state -> ftp_user   = NULL;
        free(ftp_state -> ftp_passwd);
        ftp_state -> ftp_passwd = NULL;
        free(ftp_state -> cwd);
        ftp_state -> cwd        = NULL;

        net_msg_queue_delete (&(ftp_state -> input_q));
        net_msg_queue_delete (&(ftp_state -> output_q));

        while (ftp_state -> in_transfer)
        {
            ftp_state -> in_transfer -> size = 0;
            free(ftp_state -> in_transfer -> path);
            ftp_state -> in_transfer -> path = NULL;
            ftp_state -> in_transfer -> data_address_family = '\0';
            free(ftp_state -> in_transfer -> data_address);
            ftp_state -> in_transfer -> data_address = NULL;
            ftp_state -> in_transfer -> data_port = 0;

            helper = ftp_state -> in_transfer;
            ftp_state -> in_transfer = ftp_state -> in_transfer -> next;
            free(helper);
        }

        ftp_state -> vfs_root = NULL;
    }
    return 0;
}

int commander_state_initiator (void ** state, void * vfs)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;

    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        ftp_state = malloc (sizeof (ftp_state_t));
    }
    
    if (ftp_state == NULL)
    {
        /* Out of memory! */
        return 1;
    }

    
    memset (ftp_state, 0, sizeof (ftp_state_t));
    if (ftp_state)
    {
        ftp_state -> init       = 0;
        ftp_state -> mode       = ASCII;
        free(ftp_state -> ftp_user);
        ftp_state -> ftp_user   = NULL;
        free(ftp_state -> ftp_passwd);
        ftp_state -> ftp_passwd = NULL;
        free(ftp_state -> cwd);
        ftp_state -> cwd        = NULL;

        ftp_state -> in_transfer = NULL;

        ftp_state -> vfs_root    = (vfs_t *) vfs;

        ftp_state -> input_q  = net_msg_queue_create();
        ftp_state -> output_q = net_msg_queue_create();

        /* DEBUG */
        /* VFS_print (ftp_state -> vfs_root); */

        *state = ftp_state;
    }
    return 0;
}


/* Main daemon start for the commander */
/* int startCommander (int port, int max_clients, char * ftp_banner) */
void * startCommander (void * arg)
{
    commander_options_t * commander_options = *(commander_options_t **) arg;

    set_ftp_service_banner(commander_options -> ftp_banner);


    threadingDaemonStart (commander_options -> port, 
                          commander_options -> max_clients, 
                          4096, 
                          4096, 
                          commander_active_io, 
                          commander_idle_io,
                          commander_state_initiator,
                          commander_options -> vfs_root,
                          commander_state_liberator);
    return NULL;
}
