/*

Template for a character device driver for a PCI device,
based on the templates from the book "Linux Treiber entwickeln" and
"Linux Device Drivers".

2 in 1 driver: PCI device driver (incl. ISR) +character device driver.

For a real driver replace _template by the name of your hardware, e. g. _foobar,
and fill in the correct vendor and device id.
Makefile and pci_template.sh have to be edited similar (only one replacment per file).

For the "kernel: pci_template: unsupported module, tainting kernel." kernel
message from loading the Module see
http://www.novell.com/coolsolutions/feature/14910.html

Rolf Freitag 2005

*/
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         */
/* GNU General Public License for more details.                         */
/*                                                                      */
/************************************************************************/
#include <linux/fs.h>
#include <linux/module.h>                         // MOD_DEVICE_TABLE,
#include <linux/init.h>
#include <linux/pci.h>                            // pci_device_id,
#include <linux/interrupt.h>
#include <linux/uaccess.h>  
#include <asm/io.h>                        // copy_to_user,
#include <linux/version.h>                        // KERNEL_VERSION,
#include <iso646.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include <linux/semaphore.h>
//#include <linux/config.h>
//#include <linux/init.h>

// static int i_foo;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rolf Freitag, rolf dot freitag at email.de");
MODULE_DESCRIPTION("PCI  +character device driver template");
// module_param(i_foo, int, 0444);                   // Module parameter, unused
// MODULE_PARM_DESC(i_foo, "foo frequency");         // unused

// vendor and device id of the PCI device
#define VENDOR_ID 0x1234                          // vendor id: buggy Kolter card: official 0x1001 but real 0x0001
#define DEVICE_ID 0xdeba                          // device id of the Proto-3 card
#define CDEV_SIZE 0x8000 

// for first and second i/o region (pci memory):
// static unsigned long ioport=0L, iolen=0L, memstart=0L, memlen=0L;
static int i_template_major = 231;
// static void * device_access;
// static char *kbuf;
// static dma_addr_t handle;



#define DEVNAME "hello_crypto"

// static int major;
// atomic_t  device_opened;
// static struct class *demo_class;
// struct device *demo_device;
// static struct pci_driver* devp;


static struct
pci_device_id pci_drv_ids[] =
{
  // { VENDOR_ID, DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
  { PCI_DEVICE(VENDOR_ID, DEVICE_ID), },
  { 0, }
};
MODULE_DEVICE_TABLE(pci, pci_drv_ids);
// MORE declarations
static int major;
atomic_t  device_opened;
static struct class *demo_class;
struct device *demo_device;
static u8 interrupt_stat = 0;
static u8 mmio_or_dma = 0;
static u8 k1=0;
static u8 k2=0;
// declarations for fops, pci_driver
static int demo_open (struct inode *, struct file *);
static int demo_release (struct inode *, struct file *);
static ssize_t demo_read (struct file *, char *, size_t, loff_t *);
static ssize_t demo_write (struct file *, const char *, size_t, loff_t *);
static int demo_mmap (struct file *, struct vm_area_struct *);
static int device_init(struct pci_dev *, const struct pci_device_id *);
static void device_deinit( struct pci_dev *);
static int __init pci_drv_init(void);
static void __exit pci_drv_exit(void);
// static void* encrypt_with_interrupt2(void);
module_init(pci_drv_init);
module_exit(pci_drv_exit);
//static struct pci_driver hello_crypto;
struct crypto_dev_mem {
    const char *name;
    resource_size_t start;
    resource_size_t size;
   void __iomem *kstart;
   void  *dma_start;
   u64 paddr;
};


struct crypto_dev_info {
    struct crypto_dev_mem mem[1];
    u8 irq;
};
static struct crypto_dev_info *info;
static struct semaphore *sem;
static struct semaphore *ksem;

static struct
pci_driver hello_crypto =
{
  .name= "hello_crypto2",
  .id_table= pci_drv_ids,
  .probe= device_init,
  .remove= device_deinit,
};
static char *demo_devnode(struct device *dev, umode_t *mode)
{
        if (mode && dev->devt == MKDEV(major, 0))
                *mode = 0666;
        return NULL;
}



static struct file_operations fops = {
        .read = demo_read,
        .write = demo_write,
        .open = demo_open,
        .release = demo_release,
        .mmap = demo_mmap,
};

// example for reading a PCI config byte
// static unsigned char
// get_revision(struct pci_dev *dev)
// {
//   u8 revision;

//   pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
//   printk(KERN_ERR "revision id = %d\n" , revision);
//   return (revision);
//}

static irqreturn_t hello_crypto_handler(int irq, void *dev_info)
{
    struct crypto_dev_info *dinfo = (struct crypto_dev_info *) dev_info;
    /* is it our device throwing an interrupt ? */
    // pr_alert("interrupt...handling..\n");
    if ((readl(info->mem[0].kstart+0x24) & 0x001) == 0x001) {
        /* deassert it */

	    writel(0x001,info->mem[0].kstart+0x64);
        writel(0x0,info->mem[0].kstart+0x20);
        up(sem);
        // pr_alert("IRQ %d handled\n", irq);
    
        return IRQ_HANDLED;
    } 
    else if((readl(info->mem[0].kstart+0x24) & 0x100) == 0x100){
        // pr_alert("interrupt..dma..handling..\n");
        writel(0x100,info->mem[0].kstart+0x64);
        writeq(0x0,info->mem[0].kstart+0xa0);
        up(sem);
        // pr_alert("IRQ %d handled\n", irq);
    
        return IRQ_HANDLED;
    }
    else{
        // pr_alert("hmm\n");
        return IRQ_NONE;
    }
}



// static char *demo_devnode(struct device *dev, umode_t *mode)
// {
//         if (mode && dev->devt == MKDEV(i_template_major, 0))
//                 *mode = 0666;
//         return NULL;
//}

static int device_set_keys(struct crypto_dev_info *info,u8 a, u8 b){
    u32 key = (a << 8) | b;
    // pr_alert("%x\n",key);
    writel(key,info->mem[0].kstart+0x08);
    return 0;
}

// mode 0 for mmio and 1 for dma, returns 0 if device is free, else returns 1
static int device_operation_status(struct crypto_dev_info *info, int mode){
    if (mode==0){
        u32 status = readl(info->mem[0].kstart+0x20);
        // pr_alert("s : %x\n",status);
        if ((status & 0x01) == 0x01) return 1;
        else return 0;
    }
    if (mode==1){
        u64 status = readq(info->mem[0].kstart+0xa0);
        // pr_alert("s : %x\n",status);
        if ((status & 0x01) == 0x01) return 1;
        else return 0;
    }
    else return -1; // to be completed in future
}
// return 0 for success else -1
static int device_set_data_length(struct crypto_dev_info *info, resource_size_t len, int mode){
    if (mode==0){
        writel((u32)len,info->mem[0].kstart+0x0c);
        // pr_alert("%x\n",readl(info->mem[0].kstart+0x0c));
        return 0;
    }
    else if(mode==1){
        writeq(len,info->mem[0].kstart+0x98);
        return 0;
    }
    else return -1;
}
// return 0 for success else -1
static int device_set_data_address(struct crypto_dev_info *info, u64 offset, int mode){
    if (mode==0){
        writeq(offset,info->mem[0].kstart+0x80);
        // while (device_operation_status(info,0));
        // pr_alert("check %x %x\n",offset,readq(info->mem[0].kstart+0x80));
        return 0;
    }
    else if(mode==1){
        writeq(offset,info->mem[0].kstart+0x90);
        return 0;
    }
    else return -1;
}

static int device_data_encrpyt_mmio(struct crypto_dev_info *info, u64 offset, resource_size_t len, u32 is_interrupt,u8 a,u8 b){
    if (device_set_data_length(info,len,0)<0) return -1;
    // u32 status = readl(info->mem[0].kstart+0x20);
    device_set_keys(info,a,b);
    if(is_interrupt){
        writel(0x80, info->mem[0].kstart+0x20);
        // writel(0x001,info->mem[0].kstart+0x64);
        if (device_set_data_address(info,offset,0) < 0) return -1;
        down(sem);
    } 
    else{
        writel(0x0, info->mem[0].kstart+0x20);
        if (device_set_data_address(info,offset,0) < 0) return -1;
        while (device_operation_status(info,0));
    }
    // pr_alert("encyption bit:%x\n",readl(info->mem[0].kstart+0x20));
    return 0;
}

static int device_data_decrpyt_mmio(struct crypto_dev_info *info, u64 offset, resource_size_t len,u32 is_interrupt,u8 a,u8 b){
    if (device_set_data_length(info,len,0)<0) return -1;
    // u32 status = readl(info->mem[0].kstart+0x20);
    device_set_keys(info,a,b);
    if(is_interrupt){
        writel(0x02 | 0x80, info->mem[0].kstart+0x20);
        // writel(0x001,info->mem[0].kstart+0x64);
        if (device_set_data_address(info,offset,0) < 0) return -1;
        down(sem);
    }
    else{
        writel(0x02, info->mem[0].kstart+0x20);
        if (device_set_data_address(info,offset,0) < 0) return -1;
        while (device_operation_status(info,0));
    }
    // pr_alert("sta : %x\n",readl(info->mem[0].kstart+0x20));
    // if (device_set_data_address(info,offset,0) < 0) return -1;
    return 0;
}

static int device_data_encrpyt_dma(struct crypto_dev_info *info, u64 paddr, resource_size_t len, u32 is_interrupt,u8 a,u8 b){
    if (device_set_data_address(info,paddr,1) < 0) return -1;
    if (device_set_data_length(info,len,1)<0) return -1;
    // u64 status = readq(info->mem[0].kstart+0xa0);
    device_set_keys(info,a,b);
    if(is_interrupt){
        writeq(0x05, info->mem[0].kstart+0xa0); 
        // writel(0x001,info->mem[0].kstart+0x64);
        // pr_alert("hi\n");
        down(sem);
    }
    else{
        writeq(0x01, info->mem[0].kstart+0xa0);
        while (device_operation_status(info,1));
    }
    // pr_alert("encyption bit:%x\n",readq(info->mem[0].kstart+0xa0));
    return 0;
}

static int device_data_decrpyt_dma(struct crypto_dev_info *info, u64 paddr, resource_size_t len, u32 is_interrupt,u8 a,u8 b){
    if (device_set_data_address(info,paddr,1) < 0) return -1;
    if (device_set_data_length(info,len,1)<0) return -1;
    // u64 status = readq(info->mem[0].kstart+0xa0);
    device_set_keys(info,a,b);
    if(is_interrupt){
        writeq(0x07, info->mem[0].kstart+0xa0);
        // writel(0x001,info->mem[0].kstart+0x64);
        down(sem);
    }
    else{
        writeq(0x01 | 0x02, info->mem[0].kstart+0xa0);
        while (device_operation_status(info,1));
    }
    return 0;
}


// Initialising of the module with output about the irq, I/O region and memory region.
static int device_init(struct pci_dev *dev, const struct pci_device_id *id)
{
	// printk("hi");
//   struct crypto_dev_info *info;
    info = kzalloc(sizeof(struct crypto_dev_info), GFP_KERNEL);
    sem = kzalloc(sizeof(struct semaphore), GFP_KERNEL);
    ksem = kzalloc(sizeof(struct semaphore), GFP_KERNEL);
    sema_init(sem,0);
    sema_init(ksem,1);
    if (!info)
        return -ENOMEM;

    if (pci_enable_device(dev))
        goto out_free;

    // pr_alert("enabled device\n");

    /* BAR 0 has MMIO */
    info->mem[0].name = "mmio";
    info->mem[0].start = pci_resource_start(dev, 0);
    info->mem[0].size = pci_resource_len(dev, 0);
    info->mem[0].kstart=ioremap(info->mem[0].start,info->mem[0].size);
    info->mem[0].dma_start = dma_alloc_coherent(&dev->dev,CDEV_SIZE,&(info->mem[0].paddr),GFP_DMA);

    if (!info->mem[0].start)
        goto out_unrequest;

    // pr_alert("remaped addr for kernel uses\n");
    if (dev->irq){
        /* get device irq number */
        // pr_alert("pci_template: IRQ %d.\n", dev->irq);
        if (pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &info->irq))
        goto out_iounmap;

    /* request irq */
        if (devm_request_irq(&dev->dev, info->irq, hello_crypto_handler, IRQF_SHARED, hello_crypto.name, (void *) info))
        goto out_iounmap;

    }
    // pr_alert("identification=%x address = %lx length = %lx\n", readl(info->mem[0].kstart), (unsigned long)info->mem[0].start, (unsigned long)info->mem[0].size);
    //printk("ayush %lx %lx\n",info->mem[0].start,info->mem[0].size);

    // writel(0x0, info->mem[0].kstart + 4);
    
    // pr_alert("ayush %x\n",readl(info->mem[0].kstart+4));

    // dma
    
    //dma end

    // pr_alert("status : %d\n",device_operation_status(info,0));
    // while (device_operation_status(info,0));
    // device_set_keys(info,0x01,0x32);
    // while (device_operation_status(info,0));
    // pr_alert("keys %x %x\n",ioread8(info->mem[0].kstart+0x0a),ioread8(info->mem[0].kstart+0x0b));
    // writel(0x67,addr);
    // writel(0x67,info->mem[0].kstart + 0xa8);
    // pr_alert("%x %x\n",virt_to_phys(addr),paddr);
    // writel(0x100,info->mem[0].kstart+0x60);
    // while (device_operation_status(info,0));
    // pr_alert("%d\n",device_set_data_address(info,0xa8,0));
    // device_data_encrpyt_dma(info,paddr,1,1);
    // while (device_operation_status(info,1));
    // writel(0x001,info->mem[0].kstart+0x60);
    // pr_alert("%x\n",device_data_decrpyt_dma(info,paddr,sizeof(char),0));
    // while (device_operation_status(info,1));
    // pr_alert("*a1 : %x\n",readl(addr));
    pci_set_drvdata(dev, info);
    
    // pr_alert("End of the method \n");
    return 0;

out_iounmap:
    // pr_alert("crypto:probe_out:iounmap");
    iounmap(info->mem[0].kstart);
out_unrequest:
    // pr_alert("crypto:probe_out_unrequest\n");
    pci_release_regions(dev);
out_disable:
    // pr_alert("crypto:probe_out_disable\n");
    pci_disable_device(dev);
out_free:
    // pr_alert("crypto:probe_out_free\n");
    kfree(info);
    kfree(sem);
    return -ENODEV;
}


static void
device_deinit( struct pci_dev *dev )
{
	/* we get back the driver data we store in
     * the pci_dev struct */
    info = pci_get_drvdata(dev);

    /* let's clean a little */
    // pci_release_regions(dev);
    pci_disable_device(dev);
    if(dev->irq) free_irq(dev->irq,dev);
    iounmap(info->mem[0].kstart);

    kfree(info);
    kfree(sem);
    kfree(ksem);

}

// char devices operations
static int demo_open(struct inode *inode, struct file *file)
{
        atomic_inc(&device_opened);
        try_module_get(THIS_MODULE);
        // file->private_data = kzalloc(10*sizeof(u8), GFP_KERNEL);
        // printk(KERN_INFO "driver: Device opened successfully\n");
        return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
        atomic_dec(&device_opened);
        module_put(THIS_MODULE);
        // kfree(file->private_data);
        // printk(KERN_INFO "driver: Device closed successfully\n");
        return 0;
}

static ssize_t demo_read (struct file * filp, char *buff, size_t count, loff_t * ppos)
{
	// pr_alert("_template_read: Reached.");
   if(*buff == 0x0){
       if(filp->private_data==NULL) filp->private_data=(u8 *)kzalloc(4*sizeof(u8),GFP_KERNEL);
       if(*(buff+1) == 0x0){
        //    pr_alert("yes mmio\n");
           mmio_or_dma=0;
       }
       else if(*(buff+1) == 0x1){
        //    pr_alert("yes dma\n");
           mmio_or_dma=1;
       }
       else if(*(buff+1) == 0x2){
        //    pr_alert("off interrupt\n");
           interrupt_stat=0;
       }
       else if(*(buff+1) == 0x3){
        //    pr_alert("on interrupt\n");
           interrupt_stat=1;
       }
       writeb(mmio_or_dma,filp->private_data);
       writeb(interrupt_stat,filp->private_data+1);
       return 0;
   }
   else if(*buff==1){
    //    pr_alert("hello ji\n");
       if(filp->private_data==NULL) filp->private_data=(u8 *)kzalloc(4*sizeof(u8),GFP_KERNEL);
    //    pr_alert("h %p",filp->private_data);
       strncpy(filp->private_data+2,buff+1,2);
       k1=*(u8 *)buff+1;
       k2=*(u8 *)buff+2;
       return 0;
   }
   else if(*buff==2){
    //    u64 len = *(u64 *)(buff+1);
        if(filp->private_data==NULL){
            filp->private_data=(u8 *)kzalloc(4*sizeof(u8),GFP_KERNEL);
            down(ksem);
            writeb(mmio_or_dma,filp->private_data);
            writeb(interrupt_stat,filp->private_data+1);
            writeb(k1,filp->private_data+2);
            writeb(k2,filp->private_data+3);
            up(ksem);
        }
        u8 isMapped = *(u8 *)(buff+1);
        u64 len = count;
        // char *buff2;
        // pr_alert("len : %ld\n",len);
       
       u32 cnt = 0;
        if(*(u8 *)(filp->private_data)==0){
            // pr_alert("mmio");
            if(isMapped){
                down(ksem);
                if(device_data_encrpyt_mmio(info,0xa8,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                    up(ksem);
                    return -1;
                }       
                up(ksem);
            }
            else{
                // buff2 = kzalloc(len*sizeof(char), GFP_KERNEL);;
                // strncpy(buff2,buff+2,len);
                while (len>CDEV_SIZE){
                    down(ksem);
                    memcpy_toio(info->mem[0].kstart+0xa8,buff+2+cnt*(CDEV_SIZE),CDEV_SIZE);
                    if(device_data_encrpyt_mmio(info,0xa8,CDEV_SIZE,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                        up(ksem);
                        return -1;
                    }
                    memcpy_fromio(buff+cnt*(CDEV_SIZE),info->mem[0].kstart+0xa8,CDEV_SIZE);
                    up(ksem);
                    len-=CDEV_SIZE;
                    cnt+=1;
                }
                // strncpy(buff2,+cnt*(CDEV_SIZE),len);
                // pr_alert("r1");
                down(ksem);
                memcpy_toio(info->mem[0].kstart+0xa8,buff+2+cnt*(CDEV_SIZE),len);
                // pr_alert("r2");
                if(device_data_encrpyt_mmio(info,0xa8,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                    up(ksem);
                    return -1;
                }// pr_alert("r3");
                memcpy_fromio(buff+cnt*(CDEV_SIZE),info->mem[0].kstart+0xa8,len);
                up(ksem);// pr_alert("r4");

                    // pr_alert("encypted data : %x\n",buff);
                // stac();
                // strncpy(buff,buff2,count);
                // clac();   
                // kfree(buff2);
         
                
            }
            // pr_alert("mmio-done");
        }
        else{
            // pr_alert("dma\n");
            if(isMapped) return -1;
            while (len>CDEV_SIZE){
                down(ksem);
                strncpy(info->mem[0].dma_start,buff+2+cnt*CDEV_SIZE,CDEV_SIZE);
                // pr_alert("d1");
                if(device_data_encrpyt_dma(info,info->mem[0].paddr,CDEV_SIZE,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                    up(ksem);
                    return -1;
                }
                // stac();
                strncpy(buff+cnt*CDEV_SIZE,info->mem[0].dma_start,CDEV_SIZE);
                // clac();
                up(ksem);    
                len-=CDEV_SIZE;
                cnt+=1;
            }
            // pr_alert("d1\n");
            down(ksem);
            strncpy(info->mem[0].dma_start,buff+2+cnt*CDEV_SIZE,len);
            // pr_alert("d1\n");
            if(device_data_encrpyt_dma(info,info->mem[0].paddr,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))){
                up(ksem);
                return -1;    
            }
            // pr_alert("d2\n");
            // stac();
            strncpy(buff+cnt*CDEV_SIZE,info->mem[0].dma_start,len);
            // clac();
            up(ksem);
            // pr_alert("dma-done\n");
        }
        return 0;
   }
   else if(*buff==3){
       if(filp->private_data==NULL){
            filp->private_data=(u8 *)kzalloc(4*sizeof(u8),GFP_KERNEL);
            down(ksem);
            writeb(mmio_or_dma,filp->private_data);
            writeb(interrupt_stat,filp->private_data+1);
            writeb(k1,filp->private_data+2);
            writeb(k2,filp->private_data+3);
            up(ksem);
        }
       u8 isMapped = *(buff+1);
        u64 len = count;
        // char *buff2;
        // pr_alert("len : %ld\n",len);
       u32 cnt = 0;
        if(*(u8 *)(filp->private_data)==0){
            // pr_alert("mmio");
            if(isMapped){
                if(device_data_decrpyt_mmio(info,0xa8,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0) return -1;       
            }
            else{
                // buff2 = kzalloc(len*sizeof(char), GFP_KERNEL);;
                // strncpy(buff2,buff+2,len);
                while (len>CDEV_SIZE){
                    down(ksem);
                    memcpy_toio(info->mem[0].kstart+0xa8,buff+2+cnt*(CDEV_SIZE),CDEV_SIZE);
                    if(device_data_decrpyt_mmio(info,0xa8,CDEV_SIZE,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                        up(ksem);
                        return -1;
                    }
                    memcpy_fromio(buff+cnt*(CDEV_SIZE),info->mem[0].kstart+0xa8,CDEV_SIZE);
                    up(ksem);
                    len-=CDEV_SIZE;
                    cnt+=1;
                }
                // strncpy(buff2,+cnt*(CDEV_SIZE),len);
                // pr_alert("r1");
                down(ksem);
                memcpy_toio(info->mem[0].kstart+0xa8,buff+2+cnt*(CDEV_SIZE),len);
                // pr_alert("r2");
                if(device_data_decrpyt_mmio(info,0xa8,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                    up(ksem);
                    return -1;
                }
                // pr_alert("r3");
                memcpy_fromio(buff+cnt*(CDEV_SIZE),info->mem[0].kstart+0xa8,len);
                // pr_alert("r4");
                up(ksem);
                    // pr_alert("encypted data : %x\n",buff);
                // stac();
                // strncpy(buff,buff2,count);
                // clac();    
                // kfree(buff2);
            }
            // pr_alert("mmio-done");
        }
        else{
            // pr_alert("dma");
            if(isMapped) return -1;
            while (len>CDEV_SIZE){
                down(ksem);
                strncpy(info->mem[0].dma_start,buff+2+cnt*CDEV_SIZE,CDEV_SIZE);
                if(device_data_decrpyt_dma(info,info->mem[0].paddr,CDEV_SIZE,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))<0){
                    up(ksem);
                    return -1;
                }
                stac();
                strncpy(buff+cnt*CDEV_SIZE,info->mem[0].dma_start,CDEV_SIZE);
                clac();    
                up(ksem);
                len-=CDEV_SIZE;
                cnt+=1;
            }
            down(ksem);
            strncpy(info->mem[0].dma_start,buff+2+cnt*CDEV_SIZE,len);
            if(device_data_decrpyt_dma(info,info->mem[0].paddr,len,*(u8 *)(filp->private_data+1),*(u8 *)(filp->private_data+2),*(u8 *)(filp->private_data+3))){
                up(ksem);
                return -1;    
            }
            stac();
            strncpy(buff+cnt*CDEV_SIZE,info->mem[0].dma_start,len);
            clac();
            up(ksem); // pr_alert("dma-done");
        }
        return 0;
   }
   else return -1;
}



static ssize_t demo_write (struct file * filp,  const char *buff, size_t count, loff_t * ppos)
{
//    pr_alert("_template_write: Reached.");
   return 0;
}

static int demo_mmap (struct file *filp, struct vm_area_struct *vm){
    u64 len = vm->vm_end-vm->vm_start;
    if(remap_pfn_range(vm, vm->vm_start, __pa(info->mem[0].kstart+0xa8), len, vm->vm_page_prot)<0) return -1;
    else return 0;
}

// char driver init

static
int __init pci_drv_init(void)
{
        int err;
	// printk(KERN_INFO "Hello kernel\n");
            
        major = register_chrdev(0, DEVNAME, &fops);
        err = major;
        if (err < 0) {      
            //  printk(KERN_ALERT "Registering char device failed with %d\n", major);   
             goto error_regdev;
        }                 
        
        demo_class = class_create(THIS_MODULE, DEVNAME);
        err = PTR_ERR(demo_class);
        if (IS_ERR(demo_class))
                goto error_class;

        demo_class->devnode = demo_devnode;

        demo_device = device_create(demo_class, NULL,
                                        MKDEV(major, 0),
                                        NULL, DEVNAME);
        err = PTR_ERR(demo_device);
        if (IS_ERR(demo_device))
                goto error_device;
 
        // printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);                                                              
        atomic_set(&device_opened, 0);
        pci_register_driver(&hello_crypto);
       

	return 0;

error_device:
         class_destroy(demo_class);
error_class:
        unregister_chrdev(major, DEVNAME);
error_regdev:
        return  err;
}

static
void __exit pci_drv_exit(void)
{
        device_destroy(demo_class, MKDEV(major, 0));
        class_destroy(demo_class);
        unregister_chrdev(major, DEVNAME);
        pci_unregister_driver(&hello_crypto);
	// printk(KERN_INFO "Goodbye kernel\n");
}

//module_pci_driver(hello_crypto);
