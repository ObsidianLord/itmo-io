#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("P3401: Margarita Fedotova, Gleb Lebedenko");
MODULE_DESCRIPTION("IO Systems 2021 - lab 3, variant 4");
MODULE_VERSION("1.0");

static char* link = "enp0s3";
module_param(link, charp, 0);

static char* ifname = "vni%d";
static unsigned char data[1500];
static unsigned char filter[1500];

static struct net_device_stats stats;

static struct net_device *child = NULL;
struct priv {
    struct net_device *parent;
};

static char* history;
static size_t size;
static size_t max_size;
static struct proc_dir_entry* proc;

static char check_frame(struct sk_buff *skb, unsigned char data_shift) {
	  unsigned char *user_data_ptr = NULL;
    struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
    struct udphdr *udp = NULL;
    int data_len = 0;
    char *command_name = "filter";

	   if (IPPROTO_UDP == ip->protocol) {
        udp = (struct udphdr*)((unsigned char*)ip + (ip->ihl * 4));
        data_len = ntohs(udp->len) - sizeof(struct udphdr);
        user_data_ptr = (unsigned char *)(skb->data + sizeof(struct iphdr)  + sizeof(struct udphdr)) + data_shift;
        memcpy(data, user_data_ptr, data_len);
        data[data_len] = '\0';

        size_t command_name_len = strlen(command_name);
        char command[command_name_len];

        strncpy(command, data, command_name_len);

        // debug
        // printk(KERN_INFO "lab3: filter = %s, data = %s", filter, data);

        if (strncmp(command, command_name, command_name_len) == 0) {
            memcpy(filter, &data[command_name_len + 1], data_len - command_name_len - 1);
            filter[data_len - command_name_len - 1] = '\0';
            printk(KERN_INFO "lab3: New filter: %s", filter);
        } else if (strncmp(filter, data, data_len) == 0) {

            size_t written_count = sprintf(history + size, "Captured UDP datagram, saddr: %d.%d.%d.%d\ndaddr: %d.%d.%d.%d\nData length: %d. Data:\n%s\n",
              ntohl(ip->saddr) >> 24, (ntohl(ip->saddr) >> 16) & 0x00FF,
              (ntohl(ip->saddr) >> 8) & 0x0000FF, (ntohl(ip->saddr)) & 0x000000FF,
              ntohl(ip->daddr) >> 24, (ntohl(ip->daddr) >> 16) & 0x00FF,
              (ntohl(ip->daddr) >> 8) & 0x0000FF, (ntohl(ip->daddr)) & 0x000000FF,
              data_len, data);
            size += written_count;

            if (size + size > max_size) {
                max_size += max_size;
                history = krealloc(history, sizeof(char) * max_size, GFP_KERNEL);
            }

            printk(KERN_INFO "lab3: Captured filtered UDP datagram. Data: %s", data);
        }
        return 1;
    }
    return 0;
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
	struct sk_buff *skb = *pskb;
  	struct priv *priv = netdev_priv(child);
   // if (child) {

   	if (check_frame(*pskb, 0)) {
   	    stats.rx_packets++;
      	stats.rx_bytes += (*pskb)->len;
    }
        // (*pskb)->dev = child;
        // return RX_HANDLER_ANOTHER;
    //}
    return RX_HANDLER_PASS;
}

static int open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO "%s: device opened", dev->name);
    return 0;
}

static int stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: device closed", dev->name);
    return 0;
}

static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
    struct priv *priv = netdev_priv(dev);

    // if (check_frame(skb, 14)) {
        // stats.tx_packets++;
        // stats.tx_bytes += skb->len;
    // }

    if (priv->parent) {
        skb->dev = priv->parent;
        skb->priority = 1;
        dev_queue_xmit(skb);
        return 0;
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
    return &stats;
}

static struct net_device_ops crypto_net_device_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_get_stats = get_stats,
    .ndo_start_xmit = start_xmit
};

static void setup(struct net_device *dev) {
    int i;
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &crypto_net_device_ops;

    //fill in the MAC address with a phoney
    for (i = 0; i < ETH_ALEN; i++)
        dev->dev_addr[i] = (char)i;
}

static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos) {
  size_t len = size * sizeof(char);
  if (*ppos > 0 || count < len) {
    return 0;
  }
  if (copy_to_user(ubuf, history, len) != 0) {
    return -EFAULT;
  }
  *ppos = len;

  printk(KERN_INFO "/proc/var4: read()\n");
  return len;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read = proc_read
};

int __init vni_init(void) {
    int err = 0;
    struct priv *priv;
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
    if (child == NULL) {
        printk(KERN_ERR "%s: allocate error", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        printk(KERN_ERR "%s: no such net: %s", THIS_MODULE->name, link);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        printk(KERN_ERR "%s: illegal net type", THIS_MODULE->name);
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
    if ((err = dev_alloc_name(child, child->name))) {
        printk(KERN_ERR "%s: allocate name, error %i", THIS_MODULE->name, err);
        free_netdev(child);
        return -EIO;
    }
    if ((proc = proc_create("var4", 0444, NULL, &proc_fops)) == NULL) {
        printk(KERN_ERR "%s: failed to create a proc entry: proc=%s\n", THIS_MODULE->name, "var4");
        free_netdev(child);
        return -EIO;
    }
    size = 0;
    max_size = 200;
    history = (char*) kmalloc(sizeof(char) * max_size, GFP_KERNEL);

    register_netdev(child);
    rtnl_lock();
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
    printk(KERN_INFO "Module %s loaded", THIS_MODULE->name);
    printk(KERN_INFO "%s: create link %s", THIS_MODULE->name, child->name);
    printk(KERN_INFO "%s: registered rx handler for %s", THIS_MODULE->name, priv->parent->name);
    return 0;
}

void __exit vni_exit(void) {
    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        printk(KERN_INFO "%s: unregister rx handler for %s", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);
    proc_remove(proc);
    kfree(history);
    printk(KERN_INFO "Module %s unloaded", THIS_MODULE->name);
}

module_init(vni_init);
module_exit(vni_exit);
