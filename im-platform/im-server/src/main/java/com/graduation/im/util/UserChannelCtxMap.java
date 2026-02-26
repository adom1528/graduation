package com.graduation.im.util;

import io.netty.channel.Channel;

import java.util.concurrent.ConcurrentHashMap;

/**
 * 户籍室：管理 用户ID <--> Netty通道 的映射关系
 */
public class UserChannelCtxMap {

    // 存储结构：Key = userId, Value = Channel
    // 使用 ConcurrentHashMap 保证多线程安全
    private static final ConcurrentHashMap<Long, Channel> userChannelMap = new ConcurrentHashMap<>();

    public static void addChannel(Long uid, Channel channel) {
        userChannelMap.put(uid, channel);
    }

    public static void removeChannel(Channel channel) {
        // 这是一个耗时操作(遍历)，但在连接断开时才做，频率较低
        // 实际生产中通常会维护双向映射，这里为了简单先这样做
        userChannelMap.values().removeIf(ch -> ch.id().equals(channel.id()));
    }

    public static Channel getChannel(Long uid) {
        return userChannelMap.get(uid);
    }

    // 后面可能会用到：根据通道反查用户ID
    public static Long getUserId(Channel channel) {
        for (Long userId : userChannelMap.keySet()) {
            if (userChannelMap.get(userId) == channel) {
                return userId;
            }
        }
        return null;
    }
}