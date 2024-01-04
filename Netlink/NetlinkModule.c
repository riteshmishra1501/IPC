
/* Demo Greetings Linux Kernel Module written for
kernel 5.15.0-89-generic*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/string.h>
#include <net/sock.h>

#define NETLINK_TEST_PROTOCOL                                                  \
  25 /*we can define our own protocol number for our own module number should  \
        be b/w 17 to 31, 0 to 16 are already used*/
/*Global variables of this LKM*/

static struct sock *nl_sk = NULL; /*Kernel space Netlink socket ptr*/

/* Reciever function for Data received over netlink
 * socket from user space
 * skb - socket buffer, DS used in kernel for packet flow and processing*/

/* When the Data/msg comes over netlink socket from userspace, kernel
 * packages the data in sk_buff data structures and invokes the below
 * function with pointer to that skb*/

static void netlink_recv_function(struct sk_buff *skb_in) {

  struct nlmsghdr *nlh_recv, *nlh_reply;
  char *user_space_data;
  int user_space_data_len;
  struct sk_buff *skb_out;
  char kernel_reply[256];
  int user_space_process_port_id;
  int res;

  printk(KERN_INFO "%s() invoked", __FUNCTION__);

  /*skb carries Netlink Msg which starts with Netlink Header*/
  nlh_recv = (struct nlmsghdr *)(skb_in->data);

  // using process port id we identify which User space process is seding the
  // msg
  user_space_process_port_id = nlh_recv->nlmsg_pid;

  printk(KERN_INFO "%s(%d) : port id of the sending user space process = %u\n",
         __FUNCTION__, __LINE__, user_space_process_port_id);

  user_space_data = (char *)nlmsg_data(nlh_recv);
  user_space_data_len = skb_in->len;

  printk(KERN_INFO "%s(%d) : msg recvd from user space= %s, skb_in->len = %d, "
                   "nlh->nlmsg_len = %d\n",
         __FUNCTION__, __LINE__, user_space_data, user_space_data_len,
         nlh_recv->nlmsg_len);

  if (nlh_recv->nlmsg_flags & NLM_F_ACK) {

    /*Sending reply back to user space process*/
    memset(kernel_reply, 0, sizeof(kernel_reply));

    // this is the message we are sending to User space application
    snprintf(kernel_reply, sizeof(kernel_reply),
             "Msg from Process %d has been processed by kernel",
             nlh_recv->nlmsg_pid);

    // create a new netlink message it will return us a pointer to skb DS
    skb_out = nlmsg_new(sizeof(kernel_reply), 0);

    // Add the netlink header so that USA can decode understand the message
    // coming from kernel
    nlh_reply = nlmsg_put(skb_out, 0, /*Sender is kernel, hence, port-id = 0*/
                          nlh_recv->nlmsg_seq,  /*reply with same Sequence no*/
                          NLMSG_DONE,           /*Metlink Msg type*/
                          sizeof(kernel_reply), /*Payload size*/
                          0);                   /*Flags*/

    // copy the paylod now (from kernel_reply to nlh_reply).

    strncpy(nlmsg_data(nlh_reply), kernel_reply, sizeof(nlmsg_data(nlh_reply)));

    /*Finaly Send the  msg to user space space process*/
    res = nlmsg_unicast(nl_sk, skb_out, user_space_process_port_id);

    if (res < 0) {
      printk(KERN_INFO "Error while sending the data back to user-space\n");
      kfree_skb(skb_out); /*free the internal skb_data also*/
    }
  }
}

static struct netlink_kernel_cfg cfg = {
    .input = netlink_recv_function, /*This function  will recieve msgs from
                                    userspace for Netlink protocol no 25*/
};

/*Init function of this kernel Module*/
static int __init MyNetlink_init(void) {

  printk(KERN_INFO "Hello Kernel, I am kernel Module NetlinkModule\n");

  // create a Netlink socket
  nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST_PROTOCOL, &cfg);

  if (!nl_sk) {
    printk(KERN_INFO "Kernel Netlink Socket for Netlink protocol %u failed.\n",
           NETLINK_TEST_PROTOCOL);
    return -ENOMEM;
  }

  printk(KERN_INFO "Netlink Socket Created Successfully");
  /*This fn must return 0 for module to successfully make its way into kernel*/
  return 0;
}

/*Exit function of this kernel Module*/
static void __exit MyNetlink_exit(void) {

  printk(KERN_INFO "Bye Bye. Exiting kernel Module NetlinkModule\n");
  /*Release any kernel resources held by this module in this fn*/
  netlink_kernel_release(nl_sk);
  nl_sk = NULL;
}

module_init(MyNetlink_init);
module_exit(MyNetlink_exit);

MODULE_LICENSE("GPL");
