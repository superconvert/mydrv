/*
 * 演示对纯链路层数据进行过滤
 *
 * 通常的 netfilter 都是对网络层进行过滤，对于纯链路层的过滤的例子非常少，代码也是比较少见
 * 其实这层的技术应用就是我们用到的 ebtables 模块，为了大家快速的了解其中的运作原理，本例子
 * 演示了，怎么利用 netfilter 的 hook 对纯链路层数据的抓取
 * 
 * 更多内容，可以关注我的 github :
 * https://github.com/superconvert
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_bridge.h>

#define MAC_ARG(p) p[0],p[1],p[2],p[3],p[4],p[5]
#define MAC_FMT(s) "The "#s" Mac address %02x:%02x:%02x:%02x:%02x:%02x"

static unsigned int hook_pre_routing(
    const struct nf_hook_ops *ops,
    struct sk_buff *skb,
    const struct net_device *in,
    const struct net_device *out,
#ifndef __GENKSYMS__
    const struct nf_hook_state *state
#else
    int (*okfn)(struct sk_buff *)
#endif
    )
{
    struct sk_buff *sb = skb;
    const struct ethhdr *eth = eth_hdr(sb);
    int protocol = ntohs(eth->h_proto);
    // 过滤 GOOSE 协议
    if ( protocol == 0x88b8 ) {
        printk(KERN_INFO "NFBR : "MAC_FMT(Src)", "MAC_FMT(Dst)", The Proto is %x\n", 
            MAC_ARG(eth->h_source), MAC_ARG(eth->h_dest), protocol);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops nf_test_ops[] __read_mostly = {
  {
    .hook = hook_pre_routing,
    .owner = THIS_MODULE,
    .pf = NFPROTO_BRIDGE,
    .hooknum = NF_BR_PRE_ROUTING,
    .priority = NF_BR_PRI_BRNF,
  },
};


static int __init nf_br_init(void)
{
    int ret = 0;
    printk(KERN_INFO "NFBR Loading BRIDGING Module\n");
    if ((ret = nf_register_hooks(nf_test_ops, ARRAY_SIZE(nf_test_ops))) < 0) {
        printk(KERN_INFO "NFBR Can't regiter the hook (ERR=%d)\n",ret);
        return ret;
    }

    return ret;
}

static void __exit nf_br_exit(void)
{
    printk(KERN_INFO "NFBR UnLoading Module\n");
    nf_unregister_hook(nf_test_ops);
}

module_init(nf_br_init);
module_exit(nf_br_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("2.4.1.7");
MODULE_AUTHOR("SuperConvert");
MODULE_DESCRIPTION("Linux Bridge netfilter");
