#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

//Our custom definitions of IOCTL operations
#include "message_slot.h"
MODULE_LICENSE("GPL");

static channel_list device_slot_list[257]; //array of all slots/channels



//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode, struct file*  file )
{
    printk("Invoking device_open(%p)\n", file);

    return SUCCESS;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    int minor;
    channel *curr_ch,*curr_node,*last_node;
    if(ioctl_command_id != MSG_SLOT_CHANNEL)
    {
        printk("Error - Invalid ioclt command id\n");
        return -EINVAL;
    }
    if(ioctl_command_id == 0)
    {
        printk("Error - Invalid ioclt command id, equal zero\n");
        return -EINVAL;
    }
    minor = iminor(file->f_inode); // getting the minor number from the inode of the file as described in the HW file
    last_node = device_slot_list[minor].head;
    if(last_node == NULL) // minor is empty
    {
        curr_ch = (channel *)kmalloc(sizeof(channel), GFP_KERNEL);
        if(curr_ch == NULL)
        {
            printk("Error - failed malloc new channel\n");
            return -ENOMEM;
        }
        curr_ch->ch_id = ioctl_param;
        curr_ch->msg_size = 0;
        curr_ch->message = NULL;
        device_slot_list[minor].head = curr_ch;
    }
    else // minor is not empty
    {
        curr_node = device_slot_list[minor].head;
        while (curr_node != NULL)
        {
            //getting the channel with the ch_id
            //getting the last node of the linked list in the minor
            if(curr_node->ch_id == ioctl_param)
                curr_ch = curr_node;
            last_node = curr_node;
            curr_node = curr_node->next;
        }
        if(curr_node == NULL)
        {
            //channel id does not exists for the minor
            curr_ch = (channel *)kmalloc(sizeof(channel), GFP_KERNEL);
            if(curr_ch == NULL)
            {
                printk("Error - failed malloc new channel\n");
                return -ENOMEM;
            }
            curr_ch->ch_id = ioctl_param;
            curr_ch->msg_size = 0;
            curr_ch->message = NULL;
            last_node->next = curr_ch;
        }
        
    }
    file->private_data = curr_ch;
    printk("succesfull ioctl for minor %ld, channel %ld\n",ioctl_param,minor);


    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    printk( "Invocing device_read(%p,%ld)\n",file, length);
    channel *curr_ch;
    size_t i;
    if (buffer == NULL){
        printk("Error - reading buffer is NULL\n");
        return -EINVAL;
    }
    curr_ch = (channel*)file->private_data;
    if(curr_ch == NULL)
    {
        printk("Error - no channel to read from the minor\n");
        return -EINVAL;
    }
    if(curr_ch->msg_size == 0)
    {
        printk("Error - no message to read\n");
        return -EWOULDBLOCK;
    }
    if (curr_ch->msg_size > length){
        printk("Error - too long length to read\n");
        return -ENOSPC;
    }
    for (i = 0; i < length; i++){
        if (put_user(cur_channel->message[i], &buffer[i]) != 0)
        {
            printk("Error - put_user failed after %ld, reading out of %ld\n",i,length);
            return -EFAULT;
        }
    }
    printk("succesfull %ld out of %ld bytes reading\n",i,length);
    return SUCCESS;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    size_t i,j;
    char temp[BUFFER_LEN];
    channel curr_ch;
    printk("Invoking device_write(%p,%ld)\n", file, length);
    if (buffer == NULL){
        printk("Error - writing buffer is NULL\n");
        return -EINVAL;
    }
    if (length == 0 || length > BUFFER_LEN){
        printk("Error - message length is 0 or more than 128\n");
        return -EMSGSIZE;
    }
    curr_ch = (channel*)file->private_data;
    if(curr_ch == NULL)
    {
        printk("Error - no channel to write from the minor\n");
        return -EINVAL;
    }
    for (j = 0; j < length; j++){
        if (get_user(temp[i], &buffer[i]) != 0){
            printk("Error - get_user\n");
            return -EFAULT;
        }
    }
    for(i=0; i < j; i++)
        curr_ch.message[i] = temp[i];
    curr_ch.msg_size = i;
    printk("succesfull %ld out of %ld bytes writing\n",i,length);
    return i;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;
    int i;
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 ) {
    printk( KERN_ALERT "%s registraion failed for  %d\n",DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
    }

    for(i = 0; i < 257; i++)
    {
        device_slot_list[i].head = NULL;
    }

    printk( "Registeration is successful. \n");
    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    //free all array and linked lists
    int i;
    channel *curr_ch;
    channel *temp;
    for(i=0; i<257; i++)
    {
        curr_ch = device_slot_list[i].head;
        while(curr_ch != NULL)
        {
            temp = curr_ch;
            curr_ch = curr_ch->next;
            kfree(temp);
        }
    }

    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

    
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
