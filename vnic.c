static struct rte_flow *
vnic_flow_vxlan_rss(
                  const struct rte_flow_action actions[],
                  struct rte_flow_error *err)
{
        struct rte_flow_attr attr  = {
                .ingress = 1,
        };

        struct rte_flow_item_eth vnic_eth_dst = {
                .type = RTE_BE16(0x8000),
        };
        static const struct rte_flow_item_eth eth_dst_mask = {
              .type = RTE_BE16(0xffff),
        };

        static const struct rte_flow_item_ipv4 rte_flow_item_ipv4_spec = {
        .hdr = {
                .src_addr = RTE_BE32(0x0e000001), //14.0.0.1
        },
        };
        static const struct rte_flow_item_ipv4 rte_flow_item_ipv4_mask = {
        .hdr = {
                .src_addr = RTE_BE32(0xffffffff),
        },
        };

        struct rte_flow_item pattern[] = {
                {
                        .type = RTE_FLOW_ITEM_TYPE_IPV4,
                        .spec = &rte_flow_item_ipv4_spec,
                        .mask = &rte_flow_item_ipv4_mask,
                },

                { .type = RTE_FLOW_ITEM_TYPE_END },
        };

        return rte_flow_create(NIC_PORT, &attr, pattern, actions, err);
}

static struct rte_flow *
vnic_flow_ethertype(
                  const struct rte_flow_action actions[],
                  struct rte_flow_error *err)
{
        struct rte_flow_attr attr  = {
                .ingress = 1,
        };
        struct rte_flow_item_eth vnic_eth_dst = {
                .type = RTE_BE16(0x8100),
        };
        static const struct rte_flow_item_eth eth_dst_mask = {
              .type = RTE_BE16(0xffff),
        };
        struct rte_flow_item pattern[] = {
                {
                        .type = RTE_FLOW_ITEM_TYPE_ETH,
                        .spec = &vnic_eth_dst,
                        .mask = &eth_dst_mask,
                },
                { .type = RTE_FLOW_ITEM_TYPE_END },
        };

        return rte_flow_create(NIC_PORT, &attr, pattern, actions, err);
}

static struct rte_flow *
vnic_flow_vlan_range(
                  const struct rte_flow_action actions[],
                  struct rte_flow_error *err)
{
        struct rte_flow_attr attr  = {
                .ingress = 1,
        };
        static const struct rte_flow_item_eth eth_src_mask = {
              .src.addr_bytes = "\xff\xff\xff\xff\xff\xff",
        };
        struct rte_flow_item_eth vnic_eth_src = {
                .src = "\x00\x15\x5d\xe2\xe1\x0a",
        };
        struct rte_flow_item_vlan vlan_spec = {
                .tci = RTE_BE16(0xa),
        };
        struct rte_flow_item_vlan vlan_last = {
              .tci = RTE_BE16(0xf),
        };
        struct rte_flow_item_vlan vlan_mask = {
              .tci = RTE_BE16(0xffff),
        };
        struct rte_flow_item pattern[] = {
                {
                        .type = RTE_FLOW_ITEM_TYPE_ETH,
                        .spec = &vnic_eth_src,
                        .mask = &eth_src_mask,
                },
                {
                        .type = RTE_FLOW_ITEM_TYPE_VLAN,
                        .spec = &vlan_spec,
                        .last = &vlan_last,
                        .mask = &vlan_mask,
                },
                { .type = RTE_FLOW_ITEM_TYPE_END },
        };

        return rte_flow_create(NIC_PORT, &attr, pattern, actions, err);
}

/* Convert VNIC request to rte_flows */
static int
vnic_flow_create(void)
{
        struct rte_flow_action_rss vnic_rss;
        struct rte_flow* flow;
        struct rte_flow_action actions[] = {
                { .type = RTE_FLOW_ACTION_TYPE_VOID },  /* QUEUE or RSS */
                { .type = RTE_FLOW_ACTION_TYPE_END  },
        };
        struct rte_flow_error err;
        int r;

        /* Set action to be either assign to queue or RSS */
              vnic_rss = (struct rte_flow_action_rss) {
                        .types = VNIC_RSS_HASH_TYPES,
                        .queue_num = vi->num_queue,
                        .queue = vi->queue_id,
                };

                actions[0] = (struct rte_flow_action) {
                        .type = RTE_FLOW_ACTION_TYPE_RSS,
                        .conf = &vnic_rss
                };


        //flow = vnic_flow_ethertype(vi, actions, &err);
        //flow = vnic_flow_vlan_range(vi,actions, &err);
        flow = vnic_flow_vxlan_rss(vi, &actions, &err);
        if (flow == NULL) {
                APP_LOG(ERR, "flow create (dst mac) failed: %s error %d %s\n",
                        rte_strerror(rte_errno),
                        err.type, err.message);
                goto fail_dst_mac;
        }
        else
            APP_LOG(DEBUG, "vlan flow created\n");

        return 0;
}
