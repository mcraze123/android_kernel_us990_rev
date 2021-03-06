/*
                
  
 */

/*
                              
 */
#include "felica_rws.h"
#include "felica_gpio.h"

#include "felica_test.h"

/*
          
 */
enum{
  RWS_AVAILABLE = 0,
  RWS_NOT_AVAILABLE,
};

/*              */
/*                                                */
#define FELICA_INTENT "com.felicanetworks.mfc/com.felicanetworks.adhoc.AdhocReceiver"

/*
                         
 */
static int isopen = 0; //                     


/*
                        
 */
static void felica_int_low_work(struct work_struct *data);
static DECLARE_DELAYED_WORK(felica_int_work, felica_int_low_work);

/*
                         
 */
static int invoke_felica_apk(void)
{
  char *argv[] = { "/system/bin/sh","/system/bin/am", "start", "-n", FELICA_INTENT, "--activity-clear-top", NULL };

  //                                                                                      
  static char *envp[] = {FELICA_PATH, NULL };
  int rc = 0;

  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] invoke felica app... \n");

  rc = call_usermodehelper( argv[0], argv, envp, UMH_WAIT_EXEC );

  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] felica app result : %d \n", rc);

  return rc;
}

static void felica_int_low_work(struct work_struct *data)
{
  int rc = 0;

  lock_felica_wake_lock();
  disable_irq_nosync(gpio_to_irq(felica_get_int_gpio_num()));

  usermodehelper_enable();

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_int_low_work - start \n");


  rc = invoke_felica_apk();

  if(rc)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] Error - invoke app \n");

    unlock_felica_wake_lock();
  }

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_int_low_work - end \n");


  enable_irq(gpio_to_irq(felica_get_int_gpio_num()));
}

static irqreturn_t felica_int_low_isr(int irq, void *dev_id)
{
  schedule_delayed_work(&felica_int_work,0);

  return IRQ_HANDLED;
}
/*
                                                                                                 
                                                             
              
                                   
*/
static int felica_rws_open (struct inode *inode, struct file *fp)
{
  int rc = 0;

  if(1 == isopen)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] felica_rws_open - already open \n");

    return -1;
  }
  else
  {
    FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_open - start \n");

    isopen = 1;
  }

  rc = felica_gpio_open(felica_get_int_gpio_num(), GPIO_DIRECTION_IN, GPIO_HIGH_VALUE);

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_open - end \n");

#ifdef FELICA_FN_DEVICE_TEST
  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_open - result(%d) \n",result_open_rws);
  return result_open_rws;
#else
    return rc;
#endif
}

/*
                                                                                              
              
                                                           
 */
static ssize_t felica_rws_read(struct file *fp, char *buf, size_t count, loff_t *pos)
{
  int rc = 0;
  int getvalue = GPIO_HIGH_VALUE;    /*               */

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_read - start \n");

  /*             */
  if(NULL == fp)
	{
	  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] ERROR fp is NULL \n");

	  return -1;
	}
  
	if(NULL == buf)
	{
	  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] ERROR buf is NULL \n");

	  return -1;
	}
  
	if(1 != count)
	{
	  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] ERROR count(%d) \n",count);

	  return -1;
	}
  
	if(NULL == pos)
	{
	  FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] ERROR pos is NULL \n");

	  return -1;
	}


/*                */
  getvalue = felica_gpio_read(felica_get_int_gpio_num());

  if((GPIO_LOW_VALUE != getvalue)&&(GPIO_HIGH_VALUE != getvalue))
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RFS] ERROR - getvalue is out of range \n");

    return -1;
  }

/*                                */
  getvalue = getvalue ? RWS_AVAILABLE : RWS_NOT_AVAILABLE;

  FELICA_DEBUG_MSG_MED("[FELICA_RWS] RWS status : %d \n", getvalue);


/*                           */
  rc = copy_to_user((void *)buf, (void *)&getvalue, count);
  if(rc)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] ERROR -  copy_to_user \n");

    return rc;
  }

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rfs_read - end \n");

#ifdef FELICA_FN_DEVICE_TEST
  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rfs_read - result(%d) \n",result_read_rws);
  if(result_read_rws != -1)
    result_read_rws = count;
  return result_read_rws;
#else
  return count;
#endif
}

/*
                                                                                                  
                                                             
              
                                   
*/
static int felica_rws_release (struct inode *inode, struct file *fp)
{
  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_release - start \n");

  if(0 == isopen)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] felica_rws_release - not open \n");

    return -1;
  }
  else
  {
    isopen = 0;

    FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_release - end \n");
  }

#ifdef FELICA_FN_DEVICE_TEST
  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_release - result(%d) \n",result_close_rws);
  return result_close_rws;
#else
    return 0;
#endif
}

/*
                       
 */


static struct file_operations felica_rws_fops =
{
  .owner    = THIS_MODULE,
  .open      = felica_rws_open,
  .read      = felica_rws_read,
  .release  = felica_rws_release,
};

static struct miscdevice felica_rws_device =
{
  .minor = MINOR_NUM_FELICA_RWS,
  .name = FELICA_RWS_NAME,
  .fops = &felica_rws_fops,
};

static int felica_rws_init(void)
{
  int rc = 0;

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_init - start \n");

  /*                          */
  rc = misc_register(&felica_rws_device);
  if (rc)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] FAIL!! can not register felica_int \n");

    return rc;
  }

  rc= request_irq(gpio_to_irq(felica_get_int_gpio_num()), felica_int_low_isr, IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND, FELICA_RWS_NAME, NULL);

  if (rc)
  {
    FELICA_DEBUG_MSG_HIGH("[FELICA_RWS] FAIL!! can not request_irq = %d \n",rc);

    return rc;
  }

/*                                                               */
  irq_set_irq_wake(gpio_to_irq(felica_get_int_gpio_num()),1);

  FELICA_DEBUG_MSG_LOW("[FELICA_RWS] felica_rws_init - end \n");

  return 0;
}

static void felica_rws_exit(void)
{
  FELICA_DEBUG_MSG_LOW("[FELICA_INT] felica_rws_exit - start \n");

  free_irq(gpio_to_irq(felica_get_int_gpio_num()), NULL);

  misc_deregister(&felica_rws_device);

  FELICA_DEBUG_MSG_LOW("[FELICA_INT] felica_rws_exit - end \n");
}

module_init(felica_rws_init);
module_exit(felica_rws_exit);

MODULE_LICENSE("Dual BSD/GPL");
